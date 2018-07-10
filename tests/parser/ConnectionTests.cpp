/*
  Copyright 2013 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE CompletionTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

namespace Opm {

inline std::ostream& operator<<( std::ostream& stream, const Connection& c ) {
    return stream << "(" << c.getI() << "," << c.getJ() << "," << c.getK() << ")";
}

inline std::ostream& operator<<( std::ostream& stream, const WellConnections& cs ) {
    stream << "{ ";
    for( const auto& c : cs ) stream << c << " ";
    return stream << "}";
}

}





BOOST_AUTO_TEST_CASE(CreateWellConnectionsOK) {
    Opm::WellConnections completionSet;
    BOOST_CHECK_EQUAL( 0U , completionSet.size() );
}



BOOST_AUTO_TEST_CASE(AddCompletionSizeCorrect) {
    Opm::WellCompletion::DirectionEnum dir = Opm::WellCompletion::DirectionEnum::Z;
    Opm::WellConnections completionSet;
    Opm::Connection completion1( 10,10,10, 1, 0.0, Opm::WellCompletion::OPEN , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion2( 10,10,11, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);
    completionSet.add( completion1 );
    BOOST_CHECK_EQUAL( 1U , completionSet.size() );

    completionSet.add( completion2 );
    BOOST_CHECK_EQUAL( 2U , completionSet.size() );

    BOOST_CHECK_EQUAL( completion1 , completionSet.get(0) );
}


BOOST_AUTO_TEST_CASE(WellConnectionsGetOutOfRangeThrows) {
    Opm::WellCompletion::DirectionEnum dir = Opm::WellCompletion::DirectionEnum::Z;
    Opm::Connection completion1( 10,10,10, 1, 0.0, Opm::WellCompletion::OPEN , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion2( 10,10,11, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::WellConnections completionSet;
    completionSet.add( completion1 );
    BOOST_CHECK_EQUAL( 1U , completionSet.size() );

    completionSet.add( completion2 );
    BOOST_CHECK_EQUAL( 2U , completionSet.size() );

    BOOST_CHECK_THROW( completionSet.get(10) , std::out_of_range );
}





BOOST_AUTO_TEST_CASE(AddCompletionCopy) {
    Opm::WellConnections completionSet;
    Opm::WellCompletion::DirectionEnum dir = Opm::WellCompletion::DirectionEnum::Z;

    Opm::Connection completion1( 10,10,10, 1, 0.0, Opm::WellCompletion::OPEN , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion2( 10,10,11, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion3( 10,10,12, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);

    completionSet.add( completion1 );
    completionSet.add( completion2 );
    completionSet.add( completion3 );
    BOOST_CHECK_EQUAL( 3U , completionSet.size() );

    auto copy = completionSet;
    BOOST_CHECK_EQUAL( 3U , copy.size() );

    BOOST_CHECK_EQUAL( completion1 , copy.get(0));
    BOOST_CHECK_EQUAL( completion2 , copy.get(1));
    BOOST_CHECK_EQUAL( completion3 , copy.get(2));
}


BOOST_AUTO_TEST_CASE(ActiveCompletions) {
    Opm::EclipseGrid grid(10,20,20);
    Opm::WellCompletion::DirectionEnum dir = Opm::WellCompletion::DirectionEnum::Z;
    Opm::WellConnections completions;
    Opm::Connection completion1( 0,0,0, 1, 0.0, Opm::WellCompletion::OPEN , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion2( 0,0,1, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);
    Opm::Connection completion3( 0,0,2, 1, 0.0, Opm::WellCompletion::SHUT , 99.88, 355.113, 0.25, 0, dir,0);

    completions.add( completion1 );
    completions.add( completion2 );
    completions.add( completion3 );

    std::vector<int> actnum(grid.getCartesianSize(), 1);
    actnum[0] = 0;
    grid.resetACTNUM( actnum.data() );

    Opm::WellConnections active_completions(completions, grid);
    BOOST_CHECK_EQUAL( active_completions.size() , 2);
    BOOST_CHECK_EQUAL( completion2, active_completions.get(0));
    BOOST_CHECK_EQUAL( completion3, active_completions.get(1));
}

Opm::WellConnections loadCOMPDAT(const std::string& compdat_keyword) {
    Opm::EclipseGrid grid(10,10,10);
    Opm::TableManager tables;
    Opm::Parser parser;
    const auto deck = parser.parseString(compdat_keyword, Opm::ParseContext());
    Opm::Eclipse3DProperties props(deck, tables, grid );
    const auto& keyword = deck.getKeyword("COMPDAT", 0);
    Opm::WellConnections connections;
    for (const auto& rec : keyword)
        connections.loadCOMPDAT(rec, grid, props);

    return connections;
}

BOOST_AUTO_TEST_CASE(loadCOMPDATTEST) {
    Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC); // Unit system used in deck FIRST_SIM.DATA.
    {
        const std::string deck = R"(COMPDAT
--                                    CF      Diam    Kh      Skin   Df
    'WELL'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*     1*  'Z'  21.925 /
/)";
        Opm::WellConnections connections = loadCOMPDAT(deck);
        const auto& conn0 = connections[0];
        BOOST_CHECK_EQUAL(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 1.168));
        BOOST_CHECK_EQUAL(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 107.872));
    }

    {
        const std::string deck = R"(GRID

PERMX
  1000*0.10 /

COPY
  'PERMX' 'PERMZ' /
  'PERMX' 'PERMY' /
/

SCHEDULE

COMPDAT
--                                CF      Diam    Kh      Skin   Df
'WELL'  1  1   1   1 'OPEN' 1*    1.168   0.311   0       1*     1*  'Z'  21.925 /
/)";
        Opm::WellConnections connections = loadCOMPDAT(deck);
        const auto& conn0 = connections[0];
        BOOST_CHECK_EQUAL(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 1.168));
        BOOST_CHECK_EQUAL(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 0.10 * 1.0));
    }
}


BOOST_AUTO_TEST_CASE(loadCOMPDATTESTSPE1) {
    Opm::ParseContext parseContext;
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE1CASE1.DATA", parseContext);
    Opm::EclipseState state(deck, parseContext);
    Opm::Schedule sched(deck, state, parseContext);
    const auto& units = deck.getActiveUnitSystem();

    const auto& prod = sched.getWell("PROD");
    const auto& connections = prod->getConnections(0);
    const auto& conn0 = connections[0];
    /* Expected values come from Eclipse simulation. */
    BOOST_CHECK_CLOSE(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 10.609), 2e-2);
    BOOST_CHECK_CLOSE(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 10000), 1e-6);
}


struct exp_conn {
    std::string well;
    int ci;
    double CF;
    double Kh;
};

BOOST_AUTO_TEST_CASE(loadCOMPDATTESTSPE9) {
    Opm::ParseContext parseContext;
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE9_CP_PACKED.DATA", parseContext);
    Opm::EclipseState state(deck, parseContext);
    Opm::Schedule sched(deck, state, parseContext);
    const auto& units = deck.getActiveUnitSystem();
/*
  The list of the expected values come from the PRT file in an ECLIPSE simulation.
*/
    std::vector<exp_conn> expected = {
  {"INJE1"   ,1 ,     0.166,    111.9},
  {"INJE1"   ,2 ,     0.597,    402.6},
  {"INJE1"   ,3 ,     1.866,   1259.2},
  {"INJE1"   ,4 ,    12.442,   8394.2},
  {"INJE1"   ,5 ,     6.974,   4705.3},

  {"PRODU2"  ,1 ,     0.893,    602.8},
  {"PRODU2"  ,2 ,     3.828,   2582.8},
  {"PRODU2"  ,3 ,     0.563,    380.0},

  {"PRODU3"  ,1 ,     1.322,    892.1},
  {"PRODU3"  ,2 ,     3.416,   2304.4},

  {"PRODU4"  ,1 ,     4.137,   2791.2},
  {"PRODU4"  ,2 ,    66.455,  44834.7},

  {"PRODU5"  ,1 ,     0.391,    264.0},
  {"PRODU5"  ,2 ,     7.282,   4912.6},
  {"PRODU5"  ,3 ,     1.374,    927.3},

  {"PRODU6"  ,1 ,     1.463,    987.3},
  {"PRODU6"  ,2 ,     1.891,   1275.8},

  {"PRODU7"  ,1 ,     1.061,    716.1},
  {"PRODU7"  ,2 ,     5.902,   3982.0},
  {"PRODU7"  ,3 ,     0.593,    400.1},

  {"PRODU8"  ,1 ,     0.993,    670.1},
  {"PRODU8"  ,2 ,    17.759,  11981.5},

  {"PRODU9"  ,1 ,     0.996,    671.9},
  {"PRODU9"  ,2 ,     2.548,   1719.0},

  {"PRODU10" ,1 ,    11.641,   7853.9},
  {"PRODU10" ,2 ,     7.358,   4964.1},
  {"PRODU10" ,3 ,     0.390,    262.8},

  {"PRODU11" ,2 ,     3.536,   2385.6},

  {"PRODU12" ,1 ,     3.028,   2043.1},
  {"PRODU12" ,2 ,     0.301,    202.7},
  {"PRODU12" ,3 ,     0.279,    188.3},

  {"PRODU13" ,2 ,     5.837,   3938.1},

  {"PRODU14" ,1 ,   180.976, 122098.1},
  {"PRODU14" ,2 ,    25.134,  16957.0},
  {"PRODU14" ,3 ,     0.532,    358.7},

  {"PRODU15" ,1 ,     4.125,   2783.1},
  {"PRODU15" ,2 ,     6.431,   4338.7},

  {"PRODU16" ,2 ,     5.892,   3975.0},

  {"PRODU17" ,1 ,    80.655,  54414.9},
  {"PRODU17" ,2 ,     9.098,   6138.3},

  {"PRODU18" ,1 ,     1.267,    855.1},
  {"PRODU18" ,2 ,    18.556,  12518.9},

  {"PRODU19" ,1 ,    15.589,  10517.2},
  {"PRODU19" ,3 ,     1.273,    859.1},

  {"PRODU20" ,1 ,     3.410,   2300.5},
  {"PRODU20" ,2 ,     0.191,    128.8},
  {"PRODU20" ,3 ,     0.249,    168.1},

  {"PRODU21" ,1 ,     0.596,    402.0},
  {"PRODU21" ,2 ,     0.163,    109.9},

  {"PRODU22" ,1 ,     4.021,   2712.8},
  {"PRODU22" ,2 ,     0.663,    447.1},

  {"PRODU23" ,1 ,     1.542,   1040.2},

  {"PRODU24" ,1 ,    78.939,  53257.0},
  {"PRODU24" ,3 ,    17.517,  11817.8},

  {"PRODU25" ,1 ,     3.038,   2049.5},
  {"PRODU25" ,2 ,     0.926,    624.9},
  {"PRODU25" ,3 ,     0.891,    601.3},

  {"PRODU26" ,1 ,     0.770,    519.6},
  {"PRODU26" ,3 ,     0.176,    118.6}};

   for (const auto& ec : expected) {
       const auto& well = sched.getWell(ec.well);
       const auto& connections = well->getConnections(0);
       const auto& conn = connections[ec.ci - 1];

       BOOST_CHECK_CLOSE( conn.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, ec.CF), 2e-1);
       BOOST_CHECK_CLOSE( conn.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, ec.Kh), 1e-1);
   }
}
