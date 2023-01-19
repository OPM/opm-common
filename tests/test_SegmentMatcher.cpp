/*
  Copyright 2022 Equinor ASA

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

#define BOOST_TEST_MODULE Segment_Matcher

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>

#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleTypes.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

BOOST_AUTO_TEST_SUITE(Set_Descriptor)

BOOST_AUTO_TEST_CASE(Default)
{
    const auto request = Opm::SegmentMatcher::SetDescriptor{};

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Defaulted SetDescriptor must NOT "
                        "have a specific segment number");

    BOOST_CHECK_MESSAGE(! request.wellNames().has_value(),
                        "Defaulted SetDescriptor must NOT "
                        "have a specific well name pattern");
}

BOOST_AUTO_TEST_SUITE(Segment_Number)

BOOST_AUTO_TEST_SUITE(Integer_Overload)

BOOST_AUTO_TEST_CASE(Specific)
{
    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(123);

    BOOST_REQUIRE_MESSAGE(request.segmentNumber().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific segment number");

    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 123);

    request.segmentNumber(1729);
    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 1729);
}

BOOST_AUTO_TEST_CASE(NonPositive)
{
    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(0);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Zero segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");

    request.segmentNumber(-1);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Negative segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Positve_To_Negative)
{
    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(11);

    BOOST_REQUIRE_MESSAGE(request.segmentNumber().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific segment number");

    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 11);

    request.segmentNumber(-1);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Negative segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_SUITE_END() // Integer_Overload

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(StringView_Overload)

BOOST_AUTO_TEST_CASE(Specific)
{
    using namespace std::literals;

    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("123"sv);

    BOOST_REQUIRE_MESSAGE(request.segmentNumber().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific segment number");

    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 123);

    request.segmentNumber("'1729'"sv);
    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 1729);
}

BOOST_AUTO_TEST_CASE(NonPositive)
{
    using namespace std::literals;

    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("0"sv);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Zero segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");

    request.segmentNumber("'-1'");

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Negative segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Asterisk)
{
    using namespace std::literals;

    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("*"sv);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Defaulted segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Positve_To_Negative)
{
    using namespace std::literals;

    auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("'11'"sv);

    BOOST_REQUIRE_MESSAGE(request.segmentNumber().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific segment number");

    BOOST_CHECK_EQUAL(request.segmentNumber().value(), 11);

    request.segmentNumber("-1"sv);

    BOOST_CHECK_MESSAGE(! request.segmentNumber().has_value(),
                        "Negative segment number must NOT "
                        "have a specifc segment number "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Invalid)
{
    using namespace std::literals;

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("'1*'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("'123;'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("x"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("-123-"sv), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Leading_And_Trailing_Blanks)
{
    using namespace std::literals;

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(" 123 "sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("' 1729'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber("'27 '"sv), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END() // StringView_Overload

BOOST_AUTO_TEST_SUITE_END() // Segment_Number

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Well_Name_Pattern)

BOOST_AUTO_TEST_CASE(Single_Well)
{
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("OP01");

    BOOST_REQUIRE_MESSAGE(request.wellNames().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific well name");

    BOOST_CHECK_EQUAL(request.wellNames().value(), "OP01");
}

BOOST_AUTO_TEST_CASE(Well_Pattern)
{
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("OP01*");

    BOOST_REQUIRE_MESSAGE(request.wellNames().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific well name");

    BOOST_CHECK_EQUAL(request.wellNames().value(), "OP01*");
}

BOOST_AUTO_TEST_CASE(Asterisk)
{
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("*");

    BOOST_CHECK_MESSAGE(request.wellNames().has_value(),
                        "Assigned SetDescriptor must "
                        "have a specific well name");

    BOOST_CHECK_EQUAL(request.wellNames().value(), "*");
}

BOOST_AUTO_TEST_CASE(Invalid_Characters_Unchecked)
{
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("Ab+C;E^F/");

    BOOST_CHECK_MESSAGE(request.wellNames().has_value(),
                        "Assigned SetDescriptor must "
                        "have a specific well name");

    BOOST_CHECK_EQUAL(request.wellNames().value(), "Ab+C;E^F/");
}

BOOST_AUTO_TEST_SUITE_END() // Well_Name_Pattern

BOOST_AUTO_TEST_SUITE_END() // Set_Descriptor

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Matcher)

namespace {
    Opm::Segment makeSegment(const int segmentNumber)
    {
        return { segmentNumber, 1, 1, 1.0, 0.0, 0.5, 0.01, 0.25, 1.23, true, 0.0, 0.0 };
    }

    std::shared_ptr<Opm::WellSegments> makeSegments(const int numSegments)
    {
        auto segments = std::vector<Opm::Segment>{};
        segments.reserve(numSegments);

        for (auto segment = 0; segment < numSegments; ++segment) {
            segments.push_back(makeSegment(segment + 1));
        }

        return std::make_shared<Opm::WellSegments>
            (Opm::WellSegments::CompPressureDrop::HFA, segments);
    }

    Opm::Well makeProducerWell(const std::string& wname,
                               const std::size_t  insert,
                               const int          numSegments)
    {
        auto w = Opm::Well {
            wname, "G", 0, insert, 1, 2, {},
            Opm::WellType { true, Opm::Phase::OIL }, // Oil producer
            Opm::Well::ProducerCMode::ORAT,
            Opm::Connection::Order::INPUT,
            Opm::UnitSystem::newMETRIC(),
            -3.0e+20,           // UDQ undefined
            0.0, true, true, 0,
            Opm::Well::GasInflowEquation::STD
        };

        if (numSegments > 0) {
            w.updateSegments(makeSegments(numSegments));
        }

        return w;
    }

    Opm::Well makeInjectionWell(const std::string& wname,
                                const std::size_t  insert,
                                const int          numSegments)
    {
        auto w = Opm::Well {
            wname, "G", 0, insert, 1, 2, {},
            Opm::WellType { false, Opm::Phase::GAS }, // Gas injector
            Opm::Well::ProducerCMode::ORAT,
            Opm::Connection::Order::INPUT,
            Opm::UnitSystem::newMETRIC(),
            -3.0e+20,           // UDQ undefined
            0.0, true, true, 0,
            Opm::Well::GasInflowEquation::STD
        };

        if (numSegments > 0) {
            w.updateSegments(makeSegments(numSegments));
        }

        return w;
    }

    // Collection of wells
    //   OP-01: Producer, MSW, 20 segments (1 .. 20)
    //   OP-02: Producer, MSW,  5 segments (1 ..  5)
    //   OP-06: Producer, Standard well
    //   OPROD: Producer, MSW, 2 segments (1 .. 2)
    //
    //   GI-01: Injector, MSW, 10 segments (1 .. 10)
    //   GI-08: Injector, Standard well
    //   I-45: Injector, MSW, 1 segment (1)
    Opm::ScheduleState dynamicInputData()
    {
        auto block = Opm::ScheduleState { Opm::TimeService::now() };

        block.wells.update(makeProducerWell("OP-01", 0, 20));
        block.wells.update(makeProducerWell("OP-02", 1, 5));
        block.wells.update(makeProducerWell("OP-06", 2, 0));
        block.wells.update(makeProducerWell("OPROD", 3, 2));
        block.wells.update(makeInjectionWell("GI-01", 4, 10));
        block.wells.update(makeInjectionWell("GI-08", 5,  0));
        block.wells.update(makeInjectionWell("I-45", 6, 1));

        block.well_order.update(Opm::NameOrder {
                "OP-01", "OP-02", "OP-06", "OPROD", "GI-01", "GI-08", "I-45",
            });

        return block;
    }
}

BOOST_AUTO_TEST_SUITE(Indexed_Lookup)

BOOST_AUTO_TEST_CASE(Single_Well_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-01").segmentNumber(17);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

        BOOST_CHECK_EQUAL(segSet.numWells(), std::size_t{1});

        const auto expectWells = std::vector<std::string> { "OP-01" };
        const auto expectSeg = std::vector<int> { 17 };
        const auto segments = segSet.segments(0);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-02").segmentNumber("5");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

        BOOST_CHECK_EQUAL(segSet.numWells(), std::size_t{1});

        const auto expectWells = std::vector<std::string> { "OP-02" };
        const auto expectSeg = std::vector<int> { 5 };
        const auto segments = segSet.segments(0);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(Single_Well_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-01");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        BOOST_CHECK_EQUAL(segSet.numWells(), std::size_t{1});

        const auto expectWells = std::vector<std::string> { "OP-01" };
        const auto expectSeg = std::vector<int> {
             1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
            11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        };
        const auto segments = segSet.segments(0);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-02").segmentNumber("*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        BOOST_CHECK_EQUAL(segSet.numWells(), std::size_t{1});

        const auto expectWells = std::vector<std::string> { "OP-02" };
        const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
        const auto segments = segSet.segments(0);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("GI-01").segmentNumber("'-1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must be scalar");

        const auto expectWells = std::vector<std::string> { "GI-01" };
        const auto expectSeg = std::vector<int> {
             1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        };
        const auto segments = segSet.segments(0);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(Single_Well_Missing_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-01").segmentNumber(42);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-02").segmentNumber("'6'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), filtered down to MS wells
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .segmentNumber(1);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, filtered down to MS wells
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*").segmentNumber("'1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Single_Segment_Scalar)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(14);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
    BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

    const auto expectWells = std::vector<std::string> {
        "OP-01",
    };

    BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

    for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
        const auto expectSeg = std::vector<int> { 14 };
        const auto segments = segSet.segments(well);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Partially_Missing_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), filtered down to MS wells whose segment
    // set contains segment 2.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .segmentNumber(2);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01"
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 2 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, filtered down to MS wells whose
    // segment set contains segment 7.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*").segmentNumber("'7'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "GI-01",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 7 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), unset segmentNumber(), filtered down to
    // all segments in all MS wells.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{};

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments(0);
            BOOST_CHECK_EQUAL(segments.well(), "OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments(1);
            BOOST_CHECK_EQUAL(segments.well(), "OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments(2);
            BOOST_CHECK_EQUAL(segments.well(), "OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // GI-01
        {
            const auto expectSeg = std::vector<int> {
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            };
            const auto segments = segSet.segments(3);
            BOOST_CHECK_EQUAL(segments.well(), "GI-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // I-45
        {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(4);
            BOOST_CHECK_EQUAL(segments.well(), "I-45");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, unset segmentNumber(), filtered
    // down to all segments in all MS wells..
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments(0);
            BOOST_CHECK_EQUAL(segments.well(), "OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments(1);
            BOOST_CHECK_EQUAL(segments.well(), "OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments(2);
            BOOST_CHECK_EQUAL(segments.well(), "OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // GI-01
        {
            const auto expectSeg = std::vector<int> {
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            };
            const auto segments = segSet.segments(3);
            BOOST_CHECK_EQUAL(segments.well(), "GI-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // I-45
        {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(4);
            BOOST_CHECK_EQUAL(segments.well(), "I-45");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Selected wells, specific segmentNumber(), filtered down to
    // those MS wells which match the pattern and which have that segment.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-*").segmentNumber(3);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 3 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Selected wells, specific segmentNumber(), filtered down to
    // those MS wells which match the pattern and which have that segment.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("I*").segmentNumber(1);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "I-45",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);

            BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_Partially_Missing_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Selected wells, specific segmentNumber(), filtered down to those MS
    // wells which match the pattern and which have that segment.
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("OP*").segmentNumber(3);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
    BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

    const auto expectWells = std::vector<std::string> {
        "OP-01", "OP-02",
    };

    BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

    for (auto well = 0*segSet.numWells(); well < segSet.numWells(); ++well) {
        const auto expectSeg = std::vector<int> { 3 };
        const auto segments = segSet.segments(well);

        BOOST_CHECK_EQUAL(segments.well(), expectWells[well]);

        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Producer wells, unset segmentNumber(), filtered down to all segments
    // in those MS producer wells which match the well name pattern.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments(0);
            BOOST_CHECK_EQUAL(segments.well(), "OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments(1);
            BOOST_CHECK_EQUAL(segments.well(), "OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments(2);
            BOOST_CHECK_EQUAL(segments.well(), "OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Selected producer wells, defaulted segmentNumber(), filtered down to
    // all segments in those MS producer wells which match the well name
    // pattern.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OPR*").segmentNumber("'-1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OPROD",
        };

        BOOST_CHECK_EQUAL(segSet.numWells(), expectWells.size());

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments(0);
            BOOST_CHECK_EQUAL(segments.well(), "OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_Missing_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("OP*").segmentNumber(42);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
}

BOOST_AUTO_TEST_SUITE_END() // Indexed_Lookup

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(WellName_Lookup)

BOOST_AUTO_TEST_CASE(Single_Well_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-01").segmentNumber(17);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

        const auto expectWells = std::vector<std::string> { "OP-01" };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        const auto expectSeg = std::vector<int> { 17 };
        const auto segments = segSet.segments(wells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-02").segmentNumber("5");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

        const auto expectWells = std::vector<std::string> { "OP-02" };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        const auto expectSeg = std::vector<int> { 5 };
        const auto segments = segSet.segments(wells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(Single_Well_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-01");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> { "OP-01" };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        const auto expectSeg = std::vector<int> {
             1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
            11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        };
        const auto segments = segSet.segments(wells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-02").segmentNumber("*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> { "OP-02" };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
        const auto segments = segSet.segments(wells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("GI-01").segmentNumber("'-1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must be scalar");

        const auto expectWells = std::vector<std::string> { "GI-01" };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        const auto expectSeg = std::vector<int> {
             1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        };
        const auto segments = segSet.segments(wells[0]);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), filtered down to MS wells
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .segmentNumber(1);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, filtered down to MS wells
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*").segmentNumber("'1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Single_Segment_Scalar)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(14);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
    BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must be scalar");

    const auto expectWells = std::vector<std::string> {
        "OP-01",
    };
    const auto wells = segSet.wells();
    BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                  expectWells.begin(), expectWells.end());

    for (const auto& well : wells) {
        const auto expectSeg = std::vector<int> { 14 };
        const auto segments = segSet.segments(well);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Partially_Missing_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), filtered down to MS wells whose segment
    // set contains segment 2.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .segmentNumber(2);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01"
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 2 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, filtered down to MS wells whose
    // segment set contains segment 7.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*").segmentNumber("'7'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "GI-01",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 7 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Unset wellNames(), unset segmentNumber(), filtered down to
    // all segments in all MS wells.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{};

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments("OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments("OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments("OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // GI-01
        {
            const auto expectSeg = std::vector<int> {
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            };
            const auto segments = segSet.segments("GI-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // I-45
        {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments("I-45");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Pattern matching all wells, unset segmentNumber(), filtered
    // down to all segments in all MS wells..
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD", "GI-01", "I-45",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments("OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments("OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments("OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // GI-01
        {
            const auto expectSeg = std::vector<int> {
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            };
            const auto segments = segSet.segments("GI-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // I-45
        {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments("I-45");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(All_Wells_Missing_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .segmentNumber(42);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
}

BOOST_AUTO_TEST_CASE(Select_Wells_Single_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Case 1: Selected wells, specific segmentNumber(), filtered down to
    // those MS wells which match the pattern and which have that segment.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP-*").segmentNumber(3);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 3 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Case 2: Selected wells, specific segmentNumber(), filtered down to
    // those MS wells which match the pattern and which have that segment.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("I*").segmentNumber(1);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(  segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "I-45",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        for (const auto& well : wells) {
            const auto expectSeg = std::vector<int> { 1 };
            const auto segments = segSet.segments(well);
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_Partially_Missing_Segment)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Selected wells, specific segmentNumber(), filtered down to those MS
    // wells which match the pattern and which have that segment.
    const auto request = Opm::SegmentMatcher::SetDescriptor{}
        .wellNames("OP*").segmentNumber(3);

    const auto segSet = matcher.findSegments(request);
    BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
    BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

    const auto expectWells = std::vector<std::string> {
        "OP-01", "OP-02",
    };
    const auto wells = segSet.wells();
    BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                  expectWells.begin(), expectWells.end());

    for (const auto& well : wells) {
        const auto expectSeg = std::vector<int> { 3 };
        const auto segments = segSet.segments(well);
        BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                      expectSeg.begin(), expectSeg.end());
    }
}

BOOST_AUTO_TEST_CASE(Select_Wells_All_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    // Producer wells, unset segmentNumber(), filtered down to all segments
    // in those MS producer wells which match the well name pattern.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OP*");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OP-01", "OP-02", "OPROD",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        // OP-01
        {
            const auto expectSeg = std::vector<int> {
                 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            };
            const auto segments = segSet.segments("OP-01");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OP-02
        {
            const auto expectSeg = std::vector<int> { 1, 2, 3, 4, 5, };
            const auto segments = segSet.segments("OP-02");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments("OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }

    // Selected producer wells, defaulted segmentNumber(), filtered down to
    // all segments in those MS producer wells which match the well name
    // pattern.
    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OPR*").segmentNumber("'-1'");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(! segSet.empty(), "Resulting segment set must not be empty");
        BOOST_CHECK_MESSAGE(! segSet.isScalar(), "Resulting segment set must not be scalar");

        const auto expectWells = std::vector<std::string> {
            "OPROD",
        };
        const auto wells = segSet.wells();
        BOOST_CHECK_EQUAL_COLLECTIONS(wells      .begin(), wells      .end(),
                                      expectWells.begin(), expectWells.end());

        // OPROD
        {
            const auto expectSeg = std::vector<int> { 1, 2, };
            const auto segments = segSet.segments("OPROD");
            BOOST_CHECK_EQUAL_COLLECTIONS(segments .begin(), segments .end(),
                                          expectSeg.begin(), expectSeg.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Missing_Wells_Specific_Segments)
{
    // Note: Lifetime of input data must exceed that of matcher object
    const auto mswInputData = dynamicInputData();
    const auto matcher = Opm::SegmentMatcher { mswInputData };

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("Hello").segmentNumber(2);

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
    }

    {
        const auto request = Opm::SegmentMatcher::SetDescriptor{}
            .wellNames("OIL*").segmentNumber("11");

        const auto segSet = matcher.findSegments(request);
        BOOST_CHECK_MESSAGE(segSet.empty(), "Resulting segment set must be empty");
    }
}

BOOST_AUTO_TEST_SUITE_END() // WellName_Lookup

BOOST_AUTO_TEST_SUITE_END() // Matcher
