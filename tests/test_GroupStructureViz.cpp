/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#define BOOST_TEST_MODULE Group_Structure_Viz
#include <boost/test/unit_test.hpp>

#include <opm/utility/GroupStructureViz.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

#include <tests/WorkArea.hpp>

/*
  These tests pin the exact dot-language output of writeWellGroupGraph().
  The generated .gv files are the user-visible product of this component and
  nothing else in the suite exercises them, so the comparisons below are
  whole-file and exact: a refactoring that quietly renames a graph, reorders a
  relation or changes the leaf-group styling should fail here rather than
  surface as a surprising diff in someone's rendered PDF.

  What is compared is the character content of the files, not the bytes on
  disk: the writer opens its stream in text mode and readFile() below reads it
  back the same way.  That is the right level for this component -- dot input
  is text, and pinning raw bytes would only buy a spurious failure on
  platforms that translate newlines, over a difference no consumer of a .gv
  file can observe.

  When the format is changed on purpose, regenerate the expected blocks from
  the new output and check that the difference is the intended one.
*/

namespace {

    /*
      Hierarchy chosen to exercise every branch of the writer:

          FIELD
            |-- PLAT-A            node group
            |     |-- M5S         leaf group with wells    -> orange
            |     '-- M5N         leaf group with wells    -> orange
            '-- C1                node group
                  |-- C1S         leaf group without wells -> NOT orange
                  '-- C1N         leaf group with wells    -> orange

      A group may not hold both wells and subgroups, so the two halves of the
      "leaf groups are drawn orange" predicate have to be separated across
      C1S and C1N.  PROD* are producers (red), INJ1 is an injector (blue).
      M5S holds two wells, which is what produces the invisible ordering edge
      in the well-group clusters.
    */
    std::string deckString()
    {
        return R"~(RUNSPEC
TITLE
  Group structure visualisation

DIMENS
  5 5 2 /

OIL
WATER
GAS
DISGAS

METRIC

START
  1 'JAN' 2020 /

WELLDIMS
  10 10 10 10 /

GRID

DXV
  5*100 /
DYV
  5*100 /
DZV
  2*10 /
TOPS
  25*2000 /

PERMX
  50*100 /

COPY
  'PERMX' 'PERMY' /
  'PERMX' 'PERMZ' /
/

PORO
  50*0.3 /

PROPS

SOLUTION

SCHEDULE

GRUPTREE
  'PLAT-A' 'FIELD'  /
  'M5S'    'PLAT-A' /
  'M5N'    'PLAT-A' /
  'C1'     'FIELD'  /
  'C1S'    'C1'     /
  'C1N'    'C1'     /
/

WELSPECS
  'PROD1' 'M5S' 1 1 2000 'OIL'   /
  'PROD2' 'M5S' 2 1 2000 'OIL'   /
  'INJ1'  'M5N' 3 1 2000 'WATER' /
  'PROD3' 'C1N' 4 1 2000 'OIL'   /
/

COMPDAT
  'PROD1' 2* 1 1 'OPEN' 1* 1* 0.5 /
  'PROD2' 2* 1 1 'OPEN' 1* 1* 0.5 /
  'INJ1'  2* 1 1 'OPEN' 1* 1* 0.5 /
  'PROD3' 2* 1 1 'OPEN' 1* 1* 0.5 /
/

WCONPROD
  'PROD*' 'OPEN' 'ORAT' 1000 4* 50 /
/

WCONINJE
  'INJ1' 'WATER' 'OPEN' 'RATE' 1000 1* 500 /
/

TSTEP
  10 /

END
)~";
    }

    Opm::Schedule makeSchedule()
    {
        const auto deck = Opm::Parser{}.parseString(deckString());
        const auto es = Opm::EclipseState{ deck };

        return Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    }

    std::string readFile(const std::string& fname)
    {
        std::ifstream is { fname };
        BOOST_REQUIRE_MESSAGE(is, "Could not open generated file " << fname);

        return { std::istreambuf_iterator<char>(is),
                 std::istreambuf_iterator<char>() };
    }

    std::vector<std::string> splitLines(const std::string& s)
    {
        auto lines = std::vector<std::string>{};
        auto is = std::istringstream { s };

        for (auto line = std::string{}; std::getline(is, line); ) {
            lines.push_back(line);
        }

        return lines;
    }

    // Compare line by line and name the first line that differs.  A plain
    // BOOST_CHECK_EQUAL on outputs this size prints both blocks joined by
    // " != " and leaves the reader to find the difference by eye, which for a
    // twenty line graph is not a usable failure report.
    void checkOutputMatches(const std::string& actual, const std::string& expect)
    {
        const auto a = splitLines(actual);
        const auto e = splitLines(expect);

        const auto n = std::min(a.size(), e.size());
        for (auto i = std::size_t{0}; i < n; ++i) {
            BOOST_CHECK_MESSAGE(a[i] == e[i],
                                "Output differs on line " << (i + 1) << "\n"
                                << "  expected: " << e[i] << "\n"
                                << "  actual:   " << a[i]);

            // One reported difference is enough; everything after it is
            // usually the same difference repeated.
            if (a[i] != e[i]) {
                return;
            }
        }

        BOOST_CHECK_MESSAGE(a.size() == e.size(),
                            "Output has " << a.size() << " lines, expected "
                            << e.size());

        // Backstop: the line-wise comparison cannot see a difference that
        // std::getline swallows, such as a missing final newline.
        BOOST_CHECK(actual == expect);
    }

    // writeWellGroupGraph() narrates its progress on std::cout.  That is
    // useful in the command line tool, but only noise here, so point the
    // stream at a sink we throw away.  Note the sink is a real stream buffer
    // rather than nullptr: passing nullptr makes the following clear() raise
    // badbit, which leaves std::cout in a failed state for the duration.
    class SuppressCout
    {
    public:
        SuppressCout() : saved_ { std::cout.rdbuf(this->sink_.rdbuf()) } {}
        ~SuppressCout() { std::cout.rdbuf(this->saved_); }

        SuppressCout(const SuppressCout&) = delete;
        SuppressCout& operator=(const SuppressCout&) = delete;

    private:
        std::ostringstream sink_{};
        std::streambuf*    saved_;
    };

    // Keep the muting strictly around the call under test.  Boost.Test writes
    // its failure reports to std::cout, so an assertion evaluated while the
    // stream is redirected would lose its diagnostic and a broken test would
    // report nothing useful.
    void writeGraph(const Opm::Schedule& sched,
                    const bool           separateWellGroups,
                    const std::string&   casename = "CASE")
    {
        const auto suppress = SuppressCout{};

        Opm::writeWellGroupGraph(sched, casename, separateWellGroups);
    }

    std::string generate(const bool separateWellGroups, const std::string& file)
    {
        writeGraph(makeSchedule(), separateWellGroups);

        return readFile(file);
    }

} // Anonymous namespace

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Combined_Graph)
{
    // separateWellGroups = false: one "<casename>.gv" holding both the
    // group -> group and the group -> well relations.
    WorkArea work;

    const auto expect = std::string {
R"~(// This file was written using utility function 'writeGroupStructure' from OPM.
// Find the source code at github.com/OPM.
// Convert output to PDF with 'dot -Tpdf CASE.gv -o CASE.pdf'
strict digraph "CASE_groups"
{
    "FIELD" -> { "PLAT-A" "C1" }
    "PLAT-A" -> {
    "M5S" [style=filled, fillcolor=orange]; "M5S"
    "M5N" [style=filled, fillcolor=orange]; "M5N" }
    "C1" -> { "C1S"
    "C1N" [style=filled, fillcolor=orange]; "C1N" }
    node [shape=box]
    "M5S" -> { "PROD1" "PROD2" }
    "M5N" -> { "INJ1" }
    "C1N" -> { "PROD3" }
    "PROD1" [color=red]
    "PROD2" [color=red]
    "INJ1" [color=blue]
    "PROD3" [color=red]
}
)~" };

    checkOutputMatches(generate(false, "CASE.gv"), expect);
}

BOOST_AUTO_TEST_CASE(Separate_Group_Structure)
{
    // separateWellGroups = true: group -> group relations only; the well
    // membership moves to its own file.
    WorkArea work;

    const auto expect = std::string {
R"~(// This file was written using utility function 'writeGroupStructure' from OPM.
// Find the source code at github.com/OPM.
// Convert output to PDF with 'dot -Tpdf CASE_group_structure.gv -o CASE_group_structure.pdf'
strict digraph "CASE_groups"
{
    "FIELD" -> { "PLAT-A" "C1" }
    "PLAT-A" -> {
    "M5S" [style=filled, fillcolor=orange]; "M5S"
    "M5N" [style=filled, fillcolor=orange]; "M5N" }
    "C1" -> { "C1S"
    "C1N" [style=filled, fillcolor=orange]; "C1N" }
}
)~" };

    checkOutputMatches(generate(true, "CASE_group_structure.gv"), expect);
}

BOOST_AUTO_TEST_CASE(Separate_Well_Groups)
{
    // The companion file: one cluster per group that owns wells.  M5S has two
    // wells and therefore also carries the invisible ordering edge.
    WorkArea work;

    const auto expect = std::string {
R"~(// This file was written using utility function 'writeWellGroupRelations' from OPM.
// Find the source code at github.com/OPM.
// Convert output to PDF with 'dot -Tpdf CASE_well_groups.gv -o CASE_well_groups.pdf'
strict digraph "CASE_well_groups"
{
    node [shape=box, style=normal, fillcolor=white];
    subgraph "cluster_wells_M5S" {
        label = < <b>Group: M5S</b> >;
        color = lightgrey;
        "PROD1" [color=red, fillcolor=white, style=filled];
        "PROD2" [color=red, fillcolor=white, style=filled];
        "PROD1" -> "PROD2" [style=invis];
    }
    subgraph "cluster_wells_M5N" {
        label = < <b>Group: M5N</b> >;
        color = lightgrey;
        "INJ1" [color=blue, fillcolor=white, style=filled];
    }
    subgraph "cluster_wells_C1N" {
        label = < <b>Group: C1N</b> >;
        color = lightgrey;
        "PROD3" [color=red, fillcolor=white, style=filled];
    }
}
)~" };

    checkOutputMatches(generate(true, "CASE_well_groups.gv"), expect);
}

BOOST_AUTO_TEST_CASE(File_Set_Depends_On_Separation)
{
    // Which files exist at all is part of the contract: the well-group file
    // is produced only when the caller asks for the structures to be split,
    // and the combined file is named after the case with no suffix.
    {
        WorkArea work;

        writeGraph(makeSchedule(), false);

        BOOST_CHECK(   std::ifstream{"CASE.gv"}.is_open());
        BOOST_CHECK(! std::ifstream{"CASE_group_structure.gv"}.is_open());
        BOOST_CHECK(! std::ifstream{"CASE_well_groups.gv"}.is_open());
    }

    {
        WorkArea work;

        writeGraph(makeSchedule(), true);

        BOOST_CHECK(! std::ifstream{"CASE.gv"}.is_open());
        BOOST_CHECK(   std::ifstream{"CASE_group_structure.gv"}.is_open());
        BOOST_CHECK(   std::ifstream{"CASE_well_groups.gv"}.is_open());
    }
}

BOOST_AUTO_TEST_CASE(Default_Is_Combined)
{
    // separateWellGroups defaults to false in the declaration; make sure the
    // default keeps producing the single combined file.
    WorkArea work;

    {
        // Defaulted third argument.
        const auto suppress = SuppressCout{};
        Opm::writeWellGroupGraph(makeSchedule(), "CASE");
    }

    BOOST_CHECK(   std::ifstream{"CASE.gv"}.is_open());
    BOOST_CHECK(! std::ifstream{"CASE_group_structure.gv"}.is_open());
    BOOST_CHECK(! std::ifstream{"CASE_well_groups.gv"}.is_open());
}

BOOST_AUTO_TEST_CASE(Reports_Unwritable_Output)
{
    // Both writers turn a stream that will not open into a runtime_error.
    // Directing the output at a directory that does not exist is the portable
    // way to provoke that, and it covers both files: the combined graph and,
    // with separateWellGroups, the group-structure file written first.
    //
    // Note the writers announce themselves on std::cout before testing the
    // stream, so the call has to stay muted; SuppressCout is released while
    // the exception unwinds, which leaves std::cout intact for Boost's own
    // reporting.
    WorkArea work;

    const auto sched = makeSchedule();

    BOOST_CHECK_THROW(writeGraph(sched, false, "no_such_dir/CASE"),
                      std::runtime_error);

    BOOST_CHECK_THROW(writeGraph(sched, true, "no_such_dir/CASE"),
                      std::runtime_error);

    // Nothing should have been left behind in the working directory either.
    BOOST_CHECK(! std::ifstream{"CASE.gv"}.is_open());
}
