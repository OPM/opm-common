/*
  Copyright 2016 Statoil ASA.

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

#define BOOST_TEST_MODULE RegionCache
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/RegionCache.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

Opm::Deck summaryDeck()
{
    return Opm::Parser{}.parseFile("summary_deck.DATA");
}

bool cmp_list(const std::vector<std::string>& l1,
              const std::vector<std::string>& l2)
{
    std::unordered_set<std::string> s1(l1.begin(), l1.end());
    std::unordered_set<std::string> s2(l2.begin(), l2.end());
    return s1 == s2;
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(create)
{
    const auto  deck     = summaryDeck();
    const auto  es       = Opm::EclipseState { deck };
    const auto  schedule = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    const auto& grid     = es.getInputGrid();

    const auto regCache = Opm::out::RegionCache {
        {"FIPNUM"}, es.fieldProps(), grid, schedule
    };

    {
        const auto& empty = regCache.connections( "FIPNUM", 4 );
        BOOST_CHECK_EQUAL( empty.size() , 0U );
    }

    {
        const auto& top_layer = regCache.connections(  "FIPNUM", 1 );
        BOOST_CHECK_EQUAL( top_layer.size() , 4U );
        {
            auto pair = top_layer[0];
            BOOST_CHECK_EQUAL( pair.first , "W_1");
            BOOST_CHECK_EQUAL( pair.second , grid.getGlobalIndex(0, 0, 0));
        }
    }

    BOOST_CHECK( regCache.wells("FIPXYZ", 100).empty() );
    BOOST_CHECK( regCache.wells("FIPXYZ", 1).empty() );
    BOOST_CHECK( regCache.wells("FIPNUM", 100).empty() );
    BOOST_CHECK( cmp_list(regCache.wells("FIPNUM", 1),  {"W_1", "W_2", "W_3", "W_4"}));
    BOOST_CHECK( cmp_list(regCache.wells("FIPNUM", 11), {"W_6"}));
}

BOOST_AUTO_TEST_CASE(InactiveLayers)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
5 5 6 /
START
22 'SEP' 2023 /
GRID
DXV
5*100 /
DYV
5*100 /
DZV
6*10 /
DEPTHZ
36*2000 /
ACTNUM
25*1       -- K=1
25*1       -- K=2
25*0       -- K=3
25*1       -- K=4
25*1       -- K=5
10*1
 1 1 0 1 1 -- ACTNUM(3, 3, 6) == 0
10*1 /     -- K=6
PERMX
150*100 /
PERMY
150*100 /
PERMZ
150*10 /
PORO
150*0.3 /
REGIONS
FIPNUM
25*6 25*1 25*2 25*3 25*4 25*5 /
FIPTEST1
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6   -- K=1
--
7 2 9 6 1
7 2 9 6 1
7 2 9 6 1
7 2 9 6 1
7 2 9 6 1   -- K=2
--
2 9 6 1 7
2 9 6 1 7
2 9 6 1 7
2 9 6 1 7
2 9 6 1 7   -- K=3
--
9 6 1 7 2
9 6 1 7 2
9 6 1 7 2
9 6 1 7 2
9 6 1 7 2   -- K=4
--
6 1 7 2 9
6 1 7 2 9
6 1 7 2 9
6 1 7 2 9
6 1 7 2 9   -- K=5
--
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6
1 7 2 9 6 / -- K=6
SCHEDULE
WELSPECS
 'P' 'G' 3 3 2005.0 'OIL' /
/
COMPDAT
 'P' 2* 1 6 /
/
TSTEP
10 /
END
)");

    const auto  es       = Opm::EclipseState { deck };
    const auto  schedule = Opm::Schedule { deck, es, std::make_shared<Opm::Python>() };
    const auto& grid     = es.getInputGrid();
    const auto  regCache = Opm::out::RegionCache {
        std::set<std::string>{"FIPNUM", "FIPTEST1"},
        es.fieldProps(), grid, schedule
    };

    const auto expect_conn = std::vector {
        std::size_t { 12}, // (2, 2, 0), FIP: {NUM = 6, TEST1 = 2 }
        std::size_t { 37}, // (2, 2, 1), FIP: {NUM = 1, TEST1 = 9 }
        std::size_t { 87}, // (2, 2, 3), FIP: {NUM = 3, TEST1 = 1 }
        std::size_t {112}, // (2, 2, 4), FIP: {NUM = 4, TEST1 = 7 }
    };

    {
        const auto& wconn = schedule.back().wells("P").getConnections();
        BOOST_CHECK_EQUAL(wconn.size(), expect_conn.size());

        for (auto i = 0*wconn.size(); i < wconn.size(); ++i) {
            BOOST_CHECK_EQUAL(wconn[i].global_index(), expect_conn[i]);
        }
    }

    // FIPNUM
    {
        // FIPNUM = 1
        {
            const auto& conns = regCache.connections("FIPNUM", 1);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[1]);
        }

        // FIPNUM = 2
        {
            const auto& conns = regCache.connections("FIPNUM", 2);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPNUM=2");
        }

        // FIPNUM = 3
        {
            const auto& conns = regCache.connections("FIPNUM", 3);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[2]);
        }

        // FIPNUM = 4
        {
            const auto& conns = regCache.connections("FIPNUM", 4);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[3]);
        }

        // FIPNUM = 5
        {
            const auto& conns = regCache.connections("FIPNUM", 5);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPNUM=5");
        }

        // FIPNUM = 6
        {
            const auto& conns = regCache.connections("FIPNUM", 6);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[0]);
        }
    }

    // FIPTEST1
    {
        // FIPTEST1 = 1
        {
            const auto& conns = regCache.connections("FIPTEST1", 1);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[2]);
        }

        // FIPTEST1 = 2
        {
            const auto& conns = regCache.connections("FIPTEST1", 2);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[0]);
        }

        // FIPTEST1 = 3
        {
            const auto& conns = regCache.connections("FIPTEST1", 3);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPTEST1=3");
        }

        // FIPTEST1 = 4
        {
            const auto& conns = regCache.connections("FIPTEST1", 4);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPTEST1=4");
        }

        // FIPTEST1 = 5
        {
            const auto& conns = regCache.connections("FIPTEST1", 5);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPTEST1=5");
        }

        // FIPTEST1 = 6
        {
            const auto& conns = regCache.connections("FIPTEST1", 6);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPTEST1=6");
        }

        // FIPTEST1 = 7
        {
            const auto& conns = regCache.connections("FIPTEST1", 7);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[3]);
        }

        // FIPTEST1 = 8
        {
            const auto& conns = regCache.connections("FIPTEST1", 8);
            BOOST_CHECK_MESSAGE(conns.empty(), "There must be no connections for FIPTEST1=8");
        }

        // FIPTEST1 = 9
        {
            const auto& conns = regCache.connections("FIPTEST1", 9);
            BOOST_CHECK_EQUAL(conns.size(), std::size_t{1});

            const auto& [well, cellIx] = conns.front();
            BOOST_CHECK_EQUAL(well, "P");
            BOOST_CHECK_EQUAL(cellIx, expect_conn[1]);
        }
    }
}
