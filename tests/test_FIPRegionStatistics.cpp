/*
  Copyright (c) 2024 Equinor ASA

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

#define BOOST_TEST_MODULE FIP_Region_Statistics

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace {
    Opm::FieldPropsManager fieldProps(const std::string& input)
    {
        return Opm::EclipseState { Opm::Parser{}.parseString(input) }.fieldProps();
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Maximum_Region_ID)

BOOST_AUTO_TEST_SUITE(Sequential)

BOOST_AUTO_TEST_CASE(Nothing)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
1 1 1 /
GRID
DX
100 /
DY
100 /
DZ
5 /
TOPS
2000 /
PORO
 0.3 / -- Needed to derive #active cells => auto-generate FIPNUM
)"), [](std::vector<int>&) {}
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector { std::string{ "NUM" } };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 1);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 1);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), -1); // No such region
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), -1); // No such region
}

BOOST_AUTO_TEST_CASE(Builtin_FIPNUM)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
DEPTHZ
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
1 1 2 2 3
3 3 4 2 1 /
)"), [](std::vector<int>&) {}
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector { std::string{ "NUM" } };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 4);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 4);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), -1); // No such region
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), -1); // No such region
}

BOOST_AUTO_TEST_CASE(UserDefined_RegionSets)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
DEPTHZ
12*2000 /
PORO
10*0.3 /
REGIONS
FIPABC
1 1 2 2 3
3 3 4 2 1 /
FIPRE2
1 1 1 1 1
2 2 2 2 2 /
)"), [](std::vector<int>&) {}
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector {
            std::string{ "ABC" }, std::string{ "NUM" }, std::string{ "RE2" },
        };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), 4);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), 4);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 1);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 1);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("RE2"), 2);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPRE2"), 2);
}

BOOST_AUTO_TEST_SUITE_END() // Sequential

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Parallel_Synthetic)

BOOST_AUTO_TEST_CASE(Nothing)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
1 1 1 /
GRID
DX
100 /
DY
100 /
DZ
5 /
TOPS
2000 /
PORO
 0.3 / -- Needed to derive #active cells => auto-generate FIPNUM
)"), [](std::vector<int>& maxID)
     {
         std::fill(maxID.begin(), maxID.end(), 42);
     }
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector { std::string{ "NUM" } };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 42);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 42);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), -1); // No such region
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), -1); // No such region
}

BOOST_AUTO_TEST_CASE(Builtin_FIPNUM)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
DEPTHZ
12*2000 /
PORO
10*0.3 /
REGIONS
FIPNUM
1 1 2 2 3
3 3 4 2 1 /
)"), [](std::vector<int>& maxID)
     {
         maxID.front() = 6;
     }
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector { std::string{ "NUM" } };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 6);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 6);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), -1); // No such region
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), -1); // No such region
}

BOOST_AUTO_TEST_CASE(UserDefined_RegionSets)
{
    const auto fipStats = Opm::FIPRegionStatistics {
        3, fieldProps(R"(RUNSPEC
DIMENS
5 1 2 /
GRID
DXV
5*100 /
DYV
100 /
DZV
2*5 /
DEPTHZ
12*2000 /
PORO
10*0.3 /
REGIONS
FIPABC
1 1 2 2 3
3 3 4 2 1 /
FIPRE2
1 1 1 1 1
2 2 2 2 2 /
)"), [](std::vector<int>& maxID)
     {
         maxID[0] = 6;          // FIPABC
         // maxID[1] (FIPNUM) left untouched
         maxID[2] = 1;          // FIPRE2.  This is a LIE!
     }
    };

    BOOST_CHECK_EQUAL(fipStats.declaredMaximumRegionID(), 3);

    {
        const auto expextRegSets = std::vector {
            std::string{ "ABC" }, std::string{ "NUM" }, std::string{ "RE2" },
        };
        const auto& regSets = fipStats.regionSets();
        BOOST_CHECK_EQUAL_COLLECTIONS(regSets      .begin(), regSets      .end(),
                                      expextRegSets.begin(), expextRegSets.end());
    }

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("ABC"), 6);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPABC"), 6);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("NUM"), 1);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPNUM"), 1);

    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("RE2"), 1);
    BOOST_CHECK_EQUAL(fipStats.maximumRegionID("FIPRE2"), 1);
}

BOOST_AUTO_TEST_SUITE_END() // Parallel_Synthetic

BOOST_AUTO_TEST_SUITE_END() // Maximum_Region_ID
