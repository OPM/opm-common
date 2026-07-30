/*
  Copyright 2026 Equinor ASA

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

#define BOOST_TEST_MODULE Simple_MultiSegment

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/MSW/makeMSWellFromStandardWell.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace {

    // nominal 6" tubing; only feeds the (unused) friction term
    constexpr double TUBING_D = 0.1524;

    // A connection at (0, 0, k), sitting at the given depth, whose sort_value
    // records the position it had in the input deck.
    Opm::Connection makeConnection(const int k,
                                   const double depth,
                                   const std::size_t input_index)
    {
        auto ctf = Opm::Connection::CTFProperties{};
        ctf.CF = 1.0;
        ctf.Kh = 1.0;
        ctf.Ke = 1.0;
        ctf.rw = 0.1;
        ctf.r0 = 1.0;

        return Opm::Connection {
            /*i*/ 0, /*j*/ 0, /*k*/ k,
            /*global_index*/ static_cast<std::size_t>(k),
            /*complnum*/ static_cast<int>(input_index) + 1,
            Opm::Connection::State::OPEN,
            Opm::Connection::Direction::Z,
            Opm::Connection::CTFKind::DeckValue,
            /*satTableId*/ 0,
            depth,
            ctf,
            /*sort_value*/ input_index,
            /*defaultSatTabId*/ true
        };
    }

    Opm::Well makeStandardWell(const Opm::Connection::Order ordering,
                               const std::vector<Opm::Connection>& conns)
    {
        auto w = Opm::Well {
            "W1", "G", 0, 0, 0, 0, 100.0,
            Opm::WellType { true, Opm::Phase::OIL },
            Opm::Well::ProducerCMode::ORAT,
            ordering,
            Opm::UnitSystem::newMETRIC(),
            -3.0e+20,
            0.0, true, true, 0,
            Opm::Well::GasInflowEquation::STD
        };

        w.updateConnections(std::make_shared<Opm::WellConnections>
                            (ordering, 0, 0, conns), /*force*/ true);
        return w;
    }

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Per_Connection_Conversion)

BOOST_AUTO_TEST_CASE(Segment_Per_Connection_In_Container_Order)
{
    const auto conns = std::vector {
        makeConnection(0, 1000.0, 0),
        makeConnection(1, 1010.0, 1),
        makeConnection(2, 1020.0, 2),
    };

    auto w = makeStandardWell(Opm::Connection::Order::INPUT, conns);
    BOOST_REQUIRE(! w.isMultiSegment());

    w = Opm::makeMultiSegmentWellPerConnection(w, Opm::UnitSystem::newMETRIC(), TUBING_D);

    BOOST_CHECK(w.isMultiSegment());

    // Top segment plus one per connection.
    BOOST_CHECK_EQUAL(w.getSegments().size(), conns.size() + 1);

    // Connection at container position c is wired to segment c + 2, and each
    // segment carries exactly one connection.
    const auto& out = w.getConnections();
    BOOST_REQUIRE_EQUAL(out.size(), conns.size());
    for (auto c = 0*out.size(); c < out.size(); ++c) {
        BOOST_CHECK(out[c].attachedToSegment());
        BOOST_CHECK_EQUAL(out[c].segment(), static_cast<int>(c) + 2);
    }
}

BOOST_AUTO_TEST_CASE(Order_Survives_A_Reorder_When_Container_Is_Not_Input_Order)
{
    // The case that matters for the sort_value question: a well whose
    // container order differs from its input order, as COMPORD DEPTH/TRACK
    // produces.  Here the deepest connection was entered first, so input
    // index 0 sits last in the container.
    const auto conns = std::vector {
        makeConnection(0, 1000.0, /*input_index*/ 2),
        makeConnection(1, 1010.0, /*input_index*/ 0),
        makeConnection(2, 1020.0, /*input_index*/ 1),
    };

    auto w = makeStandardWell(Opm::Connection::Order::DEPTH, conns);
    w = Opm::makeMultiSegmentWellPerConnection(w, Opm::UnitSystem::newMETRIC(), TUBING_D);

    // Well::updateConnections() calls WellConnections::order(), which -- now
    // that every connection is attached to a segment -- takes the orderMSW()
    // path and sorts by sort_value().  For a well that is now multisegment,
    // COMPORD is ignored and the connection order must be *input* order
    // (Connection.hpp: for MSW, input == simulation == restart).  So the
    // conversion must leave sort_value at the input index rather than
    // overwriting it with the container position.
    //
    // complnum was set to input_index + 1, so input order is complnum order.
    const auto& out = w.getConnections();
    BOOST_REQUIRE_EQUAL(out.size(), conns.size());
    for (auto c = 0*out.size(); c < out.size(); ++c) {
        BOOST_CHECK_EQUAL(out[c].complnum(), static_cast<int>(c) + 1);
    }

    // Whatever the order, every connection must still own a distinct segment,
    // at its own depth.
    auto segments = std::vector<int>{};
    for (const auto& c : out) {
        BOOST_CHECK(c.attachedToSegment());
        segments.push_back(c.segment());
        BOOST_CHECK_CLOSE(w.getSegments().getFromSegmentNumber(c.segment()).depth(),
                          c.depth(), 1.0e-8);
    }
    std::sort(segments.begin(), segments.end());
    BOOST_CHECK(std::adjacent_find(segments.begin(), segments.end()) == segments.end());
}

BOOST_AUTO_TEST_CASE(No_Op_When_Already_MultiSegment)
{
    const auto conns = std::vector { makeConnection(0, 1000.0, 0) };
    auto w = makeStandardWell(Opm::Connection::Order::INPUT, conns);

    w = Opm::makeMultiSegmentWellPerConnection(w, Opm::UnitSystem::newMETRIC(), TUBING_D);
    const auto nseg = w.getSegments().size();

    // Converting again must not add segments.
    w = Opm::makeMultiSegmentWellPerConnection(w, Opm::UnitSystem::newMETRIC(), TUBING_D);
    BOOST_CHECK_EQUAL(w.getSegments().size(), nseg);
}

BOOST_AUTO_TEST_CASE(Input_Well_Is_Left_Untouched)
{
    const auto conns = std::vector {
        makeConnection(0, 1000.0, 0),
        makeConnection(1, 1010.0, 1),
    };

    const auto original = makeStandardWell(Opm::Connection::Order::INPUT, conns);

    const auto converted =
        Opm::makeMultiSegmentWellPerConnection(original, Opm::UnitSystem::newMETRIC(), TUBING_D);

    // The conversion returns a new well; the caller's own well must not change.
    BOOST_CHECK(! original.isMultiSegment());
    BOOST_CHECK(converted.isMultiSegment());

    for (const auto& c : original.getConnections()) {
        BOOST_CHECK(! c.attachedToSegment());
    }
    for (const auto& c : converted.getConnections()) {
        BOOST_CHECK(c.attachedToSegment());
    }
}

BOOST_AUTO_TEST_CASE(Segment_Depths_Follow_Connection_Depths)
{
    // The hydrostatic head is driven by segment depth differences, so each
    // segment must sit at its connection's centre depth.
    const auto conns = std::vector {
        makeConnection(0, 1000.0, 0),
        makeConnection(1, 1050.0, 1),
        makeConnection(2, 1100.0, 2),
    };

    auto w = makeStandardWell(Opm::Connection::Order::INPUT, conns);
    w = Opm::makeMultiSegmentWellPerConnection(w, Opm::UnitSystem::newMETRIC(), TUBING_D);

    const auto& segs = w.getSegments();
    for (auto c = 0*conns.size(); c < conns.size(); ++c) {
        const auto& seg = segs.getFromSegmentNumber(static_cast<int>(c) + 2);
        BOOST_CHECK_CLOSE(seg.depth(), conns[c].depth(), 1.0e-8);
    }

    // Measured depth must be strictly increasing down the tubing.
    for (auto s = 1*1; s + 1 < static_cast<int>(segs.size()); ++s) {
        BOOST_CHECK_LT(segs.getFromSegmentNumber(s + 1).totalLength(),
                       segs.getFromSegmentNumber(s + 2).totalLength());
    }
}

BOOST_AUTO_TEST_SUITE_END() // Per_Connection_Conversion
