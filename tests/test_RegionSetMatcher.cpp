/*
  Copyright 2024 Equinor ASA

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

#define BOOST_TEST_MODULE Region_Set_Matcher

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/RegionSetMatcher.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstddef>

BOOST_AUTO_TEST_SUITE(Set_Descriptor)

BOOST_AUTO_TEST_CASE(Default)
{
    const auto request = Opm::RegionSetMatcher::SetDescriptor{};

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Defaulted SetDescriptor must NOT "
                        "have a specific region ID");

    BOOST_CHECK_MESSAGE(! request.regionSet().has_value(),
                        "Defaulted SetDescriptor must NOT "
                        "have a specific region set name");
}

BOOST_AUTO_TEST_SUITE(Region_ID)

BOOST_AUTO_TEST_SUITE(Integer_Overload)

BOOST_AUTO_TEST_CASE(Specific)
{
    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID(123);

    BOOST_REQUIRE_MESSAGE(request.regionID().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region ID");

    BOOST_CHECK_EQUAL(request.regionID().value(), 123);

    request.regionID(1729);
    BOOST_CHECK_EQUAL(request.regionID().value(), 1729);
}

BOOST_AUTO_TEST_CASE(NonPositive)
{
    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID(0);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Zero region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");

    request.regionID(-1);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Negative region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Positve_To_Negative)
{
    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID(11);

    BOOST_REQUIRE_MESSAGE(request.regionID().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region ID");

    BOOST_CHECK_EQUAL(request.regionID().value(), 11);

    request.regionID(-1);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Negative region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_SUITE_END() // Integer_Overload

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(StringView_Overload)

BOOST_AUTO_TEST_CASE(Specific)
{
    using namespace std::literals;

    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("123"sv);

    BOOST_REQUIRE_MESSAGE(request.regionID().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region ID");

    BOOST_CHECK_EQUAL(request.regionID().value(), 123);

    request.regionID("'1729'"sv);
    BOOST_CHECK_EQUAL(request.regionID().value(), 1729);
}

BOOST_AUTO_TEST_CASE(NonPositive)
{
    using namespace std::literals;

    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("0"sv);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Zero region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");

    request.regionID("'-1'");

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Negative region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Asterisk)
{
    using namespace std::literals;

    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("*"sv);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Defaulted region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Positve_To_Negative)
{
    using namespace std::literals;

    auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("'11'"sv);

    BOOST_REQUIRE_MESSAGE(request.regionID().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region iD");

    BOOST_CHECK_EQUAL(request.regionID().value(), 11);

    request.regionID("-1"sv);

    BOOST_CHECK_MESSAGE(! request.regionID().has_value(),
                        "Negative region ID must NOT "
                        "have a specifc region ID "
                        "in the final descriptor");
}

BOOST_AUTO_TEST_CASE(Invalid)
{
    using namespace std::literals;

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("'1*'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("'123;'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("x"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("-123-"sv), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Leading_And_Trailing_Blanks)
{
    using namespace std::literals;

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID(" 123 "sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("' 1729'"sv), std::invalid_argument);

    BOOST_CHECK_THROW(const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .regionID("'27 '"sv), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END() // StringView_Overload

BOOST_AUTO_TEST_SUITE_END() // Region_ID

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(RegionSet_Name)

BOOST_AUTO_TEST_CASE(Single_Region_Set)
{
    const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .vectorName("ROPR_NUM");

    BOOST_REQUIRE_MESSAGE(request.regionSet().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region set name");

    BOOST_CHECK_EQUAL(request.regionSet().value(), "NUM");
}

BOOST_AUTO_TEST_CASE(Single_Region_Set_UDQ)
{
    const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .vectorName("RUGBYZYX");

    BOOST_REQUIRE_MESSAGE(request.regionSet().has_value(),
                          "Assigned SetDescriptor must "
                          "have a specific region set name");

    BOOST_CHECK_EQUAL(request.regionSet().value(), "ZYX");
}

BOOST_AUTO_TEST_CASE(All_Region_Sets)
{
    const auto request = Opm::RegionSetMatcher::SetDescriptor{}
        .vectorName("ROPR");

    BOOST_REQUIRE_MESSAGE(! request.regionSet().has_value(),
                          "SetDescriptor machting ALL sets must "
                          "NOT have a specific region set name");
}

BOOST_AUTO_TEST_SUITE_END() // RegionSet_Name

BOOST_AUTO_TEST_SUITE_END() // Set_Descriptor

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Matcher)

namespace {
    Opm::FIPRegionStatistics
    makeFIPStats(const std::size_t declaredMaxRegID, const std::string& input)
    {
        const auto fp = Opm::EclipseState {
            Opm::Parser{}.parseString(input)
        }.fieldProps();

        return Opm::FIPRegionStatistics {
            declaredMaxRegID, fp, [](std::vector<int>&) {}
        };
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Single_Region_Set)

BOOST_AUTO_TEST_CASE(Single_Region)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 4 4 5 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR_NUM").regionID(3);

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "single region in a single region set");

    {
        const auto expect = std::vector<std::string> { "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{1});

    {
        const auto regIxRange = matchingRegions.regions(0);
        const auto expect = std::vector { 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions("NUM");
        const auto expect = std::vector { 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Regions_DeclaredMax)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 3 2 2 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR_NUM");

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(! matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "multiple regions in a single region set");

    {
        const auto expect = std::vector<std::string> { "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{1});

    {
        const auto regIxRange = matchingRegions.regions(0);
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions("NUM");
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Regions_DefinedMax)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 3 5 5 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR_NUM");

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(! matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "multiple regions in a single region set");

    {
        const auto expect = std::vector<std::string> { "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{1});

    {
        const auto regIxRange = matchingRegions.regions(0);
        const auto expect = std::vector { 1, 2, 3, 4, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions("NUM");
        const auto expect = std::vector { 1, 2, 3, 4, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END() // Single_Region_Set

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multiple_Region_Sets)

BOOST_AUTO_TEST_SUITE_END() // Multiple_Region_Sets

BOOST_AUTO_TEST_CASE(Single_Region)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 4 4 5 /
FIPABC
 1 1 1 2 2
 1 1 1 2 2 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR").regionID(3);

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(! matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "single region in multiple region sets");

    {
        const auto expect = std::vector<std::string> { "ABC", "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{2});

    for (auto i = 0*matchingRegions.numRegionSets();
         i < matchingRegions.numRegionSets(); ++i)
    {
        const auto regIxRange = matchingRegions.regions(i);
        const auto expect = std::vector { 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    for (const auto* regSet : { "ABC", "NUM" }) {
        const auto regIxRange = matchingRegions.regions(regSet);
        const auto expect = std::vector { 3 };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Regions_DeclaredMax)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 3 3 3 /
FIPABC
 1 1 1 2 2
 1 1 1 2 2 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR");

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(! matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "single region in multiple region sets");

    {
        const auto expect = std::vector<std::string> { "ABC", "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{2});

    for (auto i = 0*matchingRegions.numRegionSets();
         i < matchingRegions.numRegionSets(); ++i)
    {
        const auto regIxRange = matchingRegions.regions(i);
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    for (const auto* regSet : { "ABC", "NUM" }) {
        const auto regIxRange = matchingRegions.regions(regSet);
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_CASE(All_Regions_DefinedMax)
{
    // FIP region statistics object lifetime must exceed RegionSetMatcher
    // object lifetime.
    const auto fipStats = makeFIPStats(4, R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
TOPS
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
 1 1 1 2 2
 3 3 5 5 4 /
FIPABC
 1 1 1 2 2
 1 1 1 2 2 /
)");

    const auto matcher = Opm::RegionSetMatcher { fipStats };

    const auto descr = Opm::RegionSetMatcher::SetDescriptor {}
        .vectorName("ROPR");

    const auto matchingRegions = matcher.findRegions(descr);

    BOOST_CHECK_MESSAGE(! matchingRegions.empty(), "Matching range must be non-empty");
    BOOST_CHECK_MESSAGE(! matchingRegions.isScalar(),
                        "Matching range must constitute a "
                        "single region in multiple region sets");

    {
        const auto expect = std::vector<std::string> { "ABC", "NUM" };
        const auto regSets = matchingRegions.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets.begin(), regSets.end(),
                                      expect .begin(), expect .end());
    }

    BOOST_CHECK_EQUAL(matchingRegions.numRegionSets(), std::size_t{2});


    {
        const auto regIxRange = matchingRegions.regions(0); // ABC
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions(1); // NUM
        const auto expect = std::vector { 1, 2, 3, 4, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions("ABC");
        const auto expect = std::vector { 1, 2, 3, 4, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }

    {
        const auto regIxRange = matchingRegions.regions("NUM");
        const auto expect = std::vector { 1, 2, 3, 4, 5, };
        BOOST_CHECK_EQUAL_COLLECTIONS(regIxRange.begin(), regIxRange.end(),
                                      expect.begin(), expect.end());
    }
}

BOOST_AUTO_TEST_SUITE_END() // Matcher
