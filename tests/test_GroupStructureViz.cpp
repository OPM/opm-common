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

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/OutputStream.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/AggregateConnectionData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

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

// These tests pin the exact dot-language output of writeWellGroupGraph().
// The generated .gv files are the user-visible product of this component and
// nothing else in the suite exercises them, so the comparisons below are
// whole-file and exact: a refactoring that quietly renames a graph, reorders a
// relation or changes the leaf-group styling should fail here rather than
// surface as a surprising diff in someone's rendered PDF.
//
// What is compared is the character content of the files, not the bytes on
// disk: the writer opens its stream in text mode and readFile() below reads it
// back the same way.  That is the right level for this component -- dot input
// is text, and pinning raw bytes would only buy a spurious failure on
// platforms that translate newlines, over a difference no consumer of a .gv
// file can observe.
//
// When the format is changed on purpose, regenerate the expected blocks from
// the new output and check that the difference is the intended one.

namespace {

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

    // Deck shared by both Schedule_Based_Graphs and RstState_Based_Graphs.
    // Hierarchy, chosen to exercise every branch of the writer:
    //
    //   FIELD
    //     |-- PLAT-A  (node group)
    //     |     |-- M5S  (leaf group, wells: PROD1 PROD2)   -> orange
    //     |     '-- M5N  (leaf group, well: INJ1)           -> orange
    //     |
    //     '-- C1  (node group)
    //           |-- C1S  (leaf group, no wells)             -> NOT orange
    //           '-- C1N  (leaf group, well: PROD3)          -> orange
    //
    // A group may not hold both wells and subgroups, so the two halves of the
    // "leaf groups are drawn orange" predicate have to be separated across C1S
    // and C1N.  M5S holds two wells, which is what produces the invisible
    // ordering edge in the well-group clusters.
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

} // Anonymous namespace

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Schedule_Based_Graphs)

namespace {

    Opm::Schedule makeSchedule()
    {
        const auto deck = Opm::Parser{}.parseString(deckString());
        const auto es = Opm::EclipseState{ deck };

        return Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    }

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

BOOST_AUTO_TEST_SUITE_END() // Schedule_Based_Graphs

// =============================================================================

BOOST_AUTO_TEST_SUITE(RstState_Based_Graphs)

namespace {

    struct SimulationCase
    {
        explicit SimulationCase(const Opm::Deck& deck)
            : es    { deck }
            , grid  { deck }
            , sched { deck, es, std::make_shared<Opm::Python>() }
        {}

        // Order requirement: 'es' must be declared/initialised before 'sched'.
        Opm::EclipseState es;
        Opm::EclipseGrid  grid;
        Opm::Schedule     sched;
        Opm::Parser       parser;
    };

    void writeRestartFile(const SimulationCase& simCase,
                          const std::string&    baseName,
                          const std::size_t     rptStep)
    {
        const auto sim_step     = rptStep - 1;
        const auto report_step  = static_cast<int>(rptStep);

        const auto sumState     = Opm::SummaryState { Opm::TimeService::now(), 0.0 };
        const auto action_state = Opm::Action::State{};
        const auto wtest_state  = Opm::WellTestState{};

        const auto ih = Opm::RestartIO::Helpers::
            createInteHead(simCase.es, simCase.grid, simCase.sched,
                           0, sim_step, report_step, sim_step);
        const auto lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);
        const auto dh = Opm::RestartIO::Helpers::
            createDoubHead(simCase.es, simCase.sched,
                           sim_step, sim_step + 1, 0, 0);

        auto wellData = Opm::RestartIO::Helpers::AggregateWellData(ih);
        wellData.captureDeclaredWellData(simCase.sched, simCase.es.tracer(),
                                         sim_step, action_state,
                                         wtest_state, sumState, ih);
        wellData.captureDynamicWellData(simCase.sched, simCase.es.tracer(),
                                        sim_step, {}, sumState);

        auto connectionData = Opm::RestartIO::Helpers::AggregateConnectionData(ih);
        connectionData.captureDeclaredConnData(simCase.sched, simCase.grid,
                                               {}, sumState, sim_step);

        auto groupData = Opm::RestartIO::Helpers::AggregateGroupData(ih);
        groupData.captureDeclaredGroupData(simCase.sched,
                                           simCase.es.tracer(),
                                           sim_step, sumState, ih);

        Opm::EclIO::OutputStream::Restart rstFile {
            Opm::EclIO::OutputStream::ResultSet { "./", baseName },
            static_cast<int>(rptStep),
            Opm::EclIO::OutputStream::Formatted { false },
            Opm::EclIO::OutputStream::Unified   { true }
        };

        rstFile.write("INTEHEAD", ih);
        rstFile.write("DOUBHEAD", dh);
        rstFile.write("LOGIHEAD", lh);

        rstFile.write("IGRP", groupData.getIGroup());
        rstFile.write("SGRP", groupData.getSGroup());
        rstFile.write("XGRP", groupData.getXGroup());
        rstFile.write("ZGRP", groupData.getZGroup());

        rstFile.write("IWEL", wellData.getIWell());
        rstFile.write("SWEL", wellData.getSWell());
        rstFile.write("XWEL", wellData.getXWell());
        rstFile.write("ZWEL", wellData.getZWell());

        rstFile.write("ICON", connectionData.getIConn());
        rstFile.write("SCON", connectionData.getSConn());
        rstFile.write("XCON", connectionData.getXConn());
    }

    Opm::RestartIO::RstState
    loadRstState(const SimulationCase& simCase,
                 const std::string&    baseName,
                 const std::size_t     rptStep)
    {
        auto rstFile = std::make_shared<Opm::EclIO::ERst>(baseName + ".UNRST");
        auto rstView = std::make_shared<Opm::EclIO::RestartFileView>
            (std::move(rstFile), rptStep);

        return Opm::RestartIO::RstState::
            load(std::move(rstView), simCase.parser, 1);
    }

    Opm::RestartIO::RstState makeRstState(const SimulationCase& simCase)
    {
        writeRestartFile(simCase, "CASE", 1);
        return loadRstState(simCase, "CASE", 1);
    }

    void writeGraph(const Opm::RestartIO::RstState& rst,
                    const bool                       separateWellGroups,
                    const std::string&               casename = "CASE")
    {
        const auto suppress = SuppressCout{};
        Opm::writeWellGroupGraph(rst, casename, separateWellGroups);
    }

    std::string generate(const Opm::RestartIO::RstState& rst,
                         const bool                       separateWellGroups,
                         const std::string&               file)
    {
        writeGraph(rst, separateWellGroups);
        return readFile(file);
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Combined_Graph)
{
    // The group order in the RstState-based hierarchy differs from the
    // Schedule-based one: non-FIELD groups appear before FIELD because
    // the restart file stores them in insert_index order and appends FIELD
    // at the end, whereas Schedule::groupNames() returns FIELD first.
    WorkArea work;

    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };
    const auto rst = makeRstState(simCase);

    const auto expect = std::string {
R"~(// This file was written using utility function 'writeGroupStructure' from OPM.
// Find the source code at github.com/OPM.
// Convert output to PDF with 'dot -Tpdf CASE.gv -o CASE.pdf'
strict digraph "CASE_groups"
{
    "PLAT-A" -> {
    "M5S" [style=filled, fillcolor=orange]; "M5S"
    "M5N" [style=filled, fillcolor=orange]; "M5N" }
    "C1" -> { "C1S"
    "C1N" [style=filled, fillcolor=orange]; "C1N" }
    "FIELD" -> { "PLAT-A" "C1" }
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

    checkOutputMatches(generate(rst, false, "CASE.gv"), expect);
}

BOOST_AUTO_TEST_CASE(Separate_Group_Structure)
{
    WorkArea work;

    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };
    const auto rst = makeRstState(simCase);

    const auto expect = std::string {
R"~(// This file was written using utility function 'writeGroupStructure' from OPM.
// Find the source code at github.com/OPM.
// Convert output to PDF with 'dot -Tpdf CASE_group_structure.gv -o CASE_group_structure.pdf'
strict digraph "CASE_groups"
{
    "PLAT-A" -> {
    "M5S" [style=filled, fillcolor=orange]; "M5S"
    "M5N" [style=filled, fillcolor=orange]; "M5N" }
    "C1" -> { "C1S"
    "C1N" [style=filled, fillcolor=orange]; "C1N" }
    "FIELD" -> { "PLAT-A" "C1" }
}
)~" };

    checkOutputMatches(generate(rst, true, "CASE_group_structure.gv"), expect);
}

BOOST_AUTO_TEST_CASE(Separate_Well_Groups)
{
    // The well-group clusters are identical to the Schedule-based output
    // because both iterate well groups in index order (M5S, M5N, C1N) and
    // the well ordering within each cluster follows seqIndex.
    WorkArea work;

    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };
    const auto rst = makeRstState(simCase);

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

    checkOutputMatches(generate(rst, true, "CASE_well_groups.gv"), expect);
}

BOOST_AUTO_TEST_CASE(File_Set_Depends_On_Separation)
{
    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };

    {
        WorkArea work;
        const auto rst = makeRstState(simCase);

        writeGraph(rst, false);

        BOOST_CHECK(   std::ifstream{"CASE.gv"}.is_open());
        BOOST_CHECK(! std::ifstream{"CASE_group_structure.gv"}.is_open());
        BOOST_CHECK(! std::ifstream{"CASE_well_groups.gv"}.is_open());
    }

    {
        WorkArea work;
        const auto rst = makeRstState(simCase);

        writeGraph(rst, true);

        BOOST_CHECK(! std::ifstream{"CASE.gv"}.is_open());
        BOOST_CHECK(   std::ifstream{"CASE_group_structure.gv"}.is_open());
        BOOST_CHECK(   std::ifstream{"CASE_well_groups.gv"}.is_open());
    }
}

BOOST_AUTO_TEST_CASE(Default_Is_Combined)
{
    WorkArea work;

    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };
    const auto rst = makeRstState(simCase);

    {
        const auto suppress = SuppressCout{};
        Opm::writeWellGroupGraph(rst, "CASE");
    }

    BOOST_CHECK(   std::ifstream{"CASE.gv"}.is_open());
    BOOST_CHECK(! std::ifstream{"CASE_group_structure.gv"}.is_open());
    BOOST_CHECK(! std::ifstream{"CASE_well_groups.gv"}.is_open());
}

BOOST_AUTO_TEST_CASE(Reports_Unwritable_Output)
{
    WorkArea work;

    const auto simCase = SimulationCase { Opm::Parser{}.parseString(deckString()) };
    const auto rst = makeRstState(simCase);

    BOOST_CHECK_THROW(writeGraph(rst, false, "no_such_dir/CASE"),
                      std::runtime_error);

    BOOST_CHECK_THROW(writeGraph(rst, true, "no_such_dir/CASE"),
                      std::runtime_error);

    BOOST_CHECK(! std::ifstream{"CASE.gv"}.is_open());
}

BOOST_AUTO_TEST_SUITE_END() // RstState_Based_Graphs
