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

#define BOOST_TEST_MODULE MSW_Tracker

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/MSW/MswTracker.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <string>
#include <vector>

namespace {

Opm::KeywordLocation location(const std::string& keyword, const int lineno)
{
    return { keyword, "MSW.DATA", lineno };
}

/// Fully specified multi-segmented well 'P' with connections in cells
/// 10, 11, 12 and segments 1..3.
Opm::MswTracker consistentWell()
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("P");
    tracker.connections("P", { 10, 11, 12 }, location("COMPDAT", 10));
    tracker.segments("P", { 1, 2, 3 });
    tracker.linkConnections("P", { { 10, 2 }, { 11, 2 }, { 12, 3 } },
                            location("COMPSEGS", 30));

    return tracker;
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Consistent_Well)
{
    auto tracker = consistentWell();

    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Fully specified multi-segmented well must "
                        "not have any consistency problems");
}

BOOST_AUTO_TEST_CASE(Unknown_Well_Ignored)
{
    auto tracker = Opm::MswTracker{};

    tracker.connections("X", { 1 }, location("COMPDAT", 10));
    tracker.segments("X", { 1 });

    BOOST_CHECK_MESSAGE(! tracker.isKnown("X"), "Well 'X' must not be known");
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Unknown well must not have any consistency problems");
}

BOOST_AUTO_TEST_CASE(Standard_Well_Skipped)
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("I");
    tracker.connections("I", { 1, 2, 3 }, location("COMPDAT", 10));

    BOOST_CHECK_MESSAGE(tracker.isKnown("I"), "Well 'I' must be known");
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Standard well must not have any consistency problems");
}

BOOST_AUTO_TEST_CASE(Later_Connection_Unlinked)
{
    auto tracker = consistentWell();
    BOOST_REQUIRE(tracker.checkAndClearDirty().empty());

    // New connections in a later report step, without COMPSEGS.  Existing
    // cell 11 must not be reported.
    tracker.connections("P", { 11, 42, 17 }, location("COMPDAT", 100));

    const auto problems = tracker.checkAndClearDirty();
    BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{2});

    using Kind = Opm::MswTracker::ProblemKind;

    BOOST_CHECK_EQUAL(problems[0].well, "P");
    BOOST_CHECK_MESSAGE(problems[0].kind == Kind::UnlinkedConnection,
                        "First problem must be an unlinked connection");
    BOOST_CHECK_EQUAL(problems[0].cell, std::size_t{17});
    BOOST_CHECK_EQUAL(problems[0].location.lineno, 100);
    BOOST_CHECK_MESSAGE(problems[0].segments.empty(),
                        "Unlinked connection must not have segments");

    BOOST_CHECK_MESSAGE(problems[1].kind == Kind::UnlinkedConnection,
                        "Second problem must be an unlinked connection");
    BOOST_CHECK_EQUAL(problems[1].cell, std::size_t{42});

    // Problems are reported only once, unless the well changes again.
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Unchanged well must not be rechecked");

    // Linking the new connections resolves the problem.
    tracker.linkConnections("P", { { 17, 3 }, { 42, 3 } }, location("COMPSEGS", 110));
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Linked connections must resolve problem");
}

BOOST_AUTO_TEST_CASE(Connections_Before_Welsegs)
{
    // COMPDAT before WELSEGS in a single report step is fine provided all
    // connections are linked before the check.
    auto tracker = Opm::MswTracker{};

    tracker.defined("P");
    tracker.connections("P", { 1, 2 }, location("COMPDAT", 10));

    tracker.segments("P", { 1, 2 });
    tracker.linkConnections("P", { { 1, 2 }, { 2, 2 } }, location("COMPSEGS", 30));

    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Fully linked well must not have problems");
}

BOOST_AUTO_TEST_CASE(Unknown_Segment)
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("P");
    tracker.connections("P", { 10, 11 }, location("COMPDAT", 10));
    tracker.segments("P", { 1, 2 });
    tracker.linkConnections("P", { { 10, 2 }, { 11, 5 } }, location("COMPSEGS", 30));

    const auto problems = tracker.checkAndClearDirty();
    BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{1});

    BOOST_CHECK_MESSAGE(problems[0].kind == Opm::MswTracker::ProblemKind::UnknownSegment,
                        "Problem must be an unknown segment");
    BOOST_CHECK_EQUAL(problems[0].cell, std::size_t{11});
    BOOST_REQUIRE_EQUAL(problems[0].segments.size(), std::size_t{1});
    BOOST_CHECK_EQUAL(problems[0].segments.front(), 5);
    BOOST_CHECK_EQUAL(problems[0].location.keyword, "COMPSEGS");

    // Defining the segment later resolves the problem.
    tracker.segments("P", { 3, 4, 5 });
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Defining missing segment must resolve problem");
}

BOOST_AUTO_TEST_CASE(Multiply_Linked_Within_Keyword)
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("P");
    tracker.connections("P", { 10, 11 }, location("COMPDAT", 10));
    tracker.segments("P", { 1, 2, 3 });
    tracker.linkConnections("P", { { 10, 2 }, { 11, 2 }, { 10, 3 }, { 10, 2 } },
                            location("COMPSEGS", 30));

    const auto problems = tracker.checkAndClearDirty();
    BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{1});

    BOOST_CHECK_MESSAGE(problems[0].kind == Opm::MswTracker::ProblemKind::MultiplyLinked,
                        "Problem must be a multiply linked connection");
    BOOST_CHECK_EQUAL(problems[0].cell, std::size_t{10});

    const auto expect = std::vector { 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS(problems[0].segments.begin(), problems[0].segments.end(),
                                  expect.begin(), expect.end());

    // Reported once only.
    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Conflict must be reported once");
}

BOOST_AUTO_TEST_CASE(Same_Segment_Twice_Within_Keyword)
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("P");
    tracker.connections("P", { 10 }, location("COMPDAT", 10));
    tracker.segments("P", { 1, 2 });
    tracker.linkConnections("P", { { 10, 2 }, { 10, 2 } }, location("COMPSEGS", 30));

    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Repeated identical link must not be a problem");
}

BOOST_AUTO_TEST_CASE(Relink_In_Later_Keyword)
{
    auto tracker = consistentWell();
    BOOST_REQUIRE(tracker.checkAndClearDirty().empty());

    tracker.linkConnections("P", { { 10, 3 } }, location("COMPSEGS", 200));

    BOOST_CHECK_MESSAGE(tracker.checkAndClearDirty().empty(),
                        "Relinking in later keyword must not be a problem");
}

BOOST_AUTO_TEST_CASE(Only_Dirty_Wells_Checked)
{
    auto tracker = Opm::MswTracker{};

    tracker.defined("P1");
    tracker.defined("P2");

    // P1: unknown segment, reported in first check only.
    tracker.connections("P1", { 1 }, location("COMPDAT", 10));
    tracker.segments("P1", { 1 });
    tracker.linkConnections("P1", { { 1, 7 } }, location("COMPSEGS", 20));

    {
        const auto problems = tracker.checkAndClearDirty();
        BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{1});
        BOOST_CHECK_EQUAL(problems[0].well, "P1");
    }

    // P2: unlinked connection.  P1 is unchanged and not rechecked.
    tracker.connections("P2", { 5 }, location("COMPDAT", 50));
    tracker.segments("P2", { 1 });

    {
        const auto problems = tracker.checkAndClearDirty();
        BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{1});
        BOOST_CHECK_EQUAL(problems[0].well, "P2");
    }
}

BOOST_AUTO_TEST_CASE(Redefine_Keeps_Data)
{
    auto tracker = consistentWell();
    BOOST_REQUIRE(tracker.checkAndClearDirty().empty());

    tracker.defined("P");
    tracker.connections("P", { 13 }, location("COMPDAT", 300));

    const auto problems = tracker.checkAndClearDirty();
    BOOST_REQUIRE_EQUAL(problems.size(), std::size_t{1});
    BOOST_CHECK_EQUAL(problems[0].cell, std::size_t{13});
}
