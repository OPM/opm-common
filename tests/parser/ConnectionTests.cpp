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

#define BOOST_TEST_MODULE CompletionTests
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <opm/common/utility/ActiveGridCells.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <numeric>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <vector>

#include <fmt/format.h>

namespace {
    double cp_rm3_per_db()
    {
        return Opm::prefix::centi*Opm::unit::Poise * Opm::unit::cubic(Opm::unit::meter)
            / (Opm::unit::day * Opm::unit::barsa);
    }

    Opm::WellConnections
    loadCOMPDAT(const std::string& compdat_keyword)
    {
        Opm::WellConnections connections {
            Opm::Connection::Order::TRACK, 10, 10
        };

        const auto deck = Opm::Parser{}.parseString(compdat_keyword);
        const auto wdfac = Opm::WDFAC{};
        const auto loc = Opm::KeywordLocation{};

        Opm::EclipseGrid grid { 10, 10, 10 };
        const Opm::FieldPropsManager field_props {
            deck, Opm::Phases{true, true, true}, grid, Opm::TableManager{}
        };

        const auto ctx = Opm::ParseContext{};
        auto errors = Opm::ErrorGuard{};

        // Must be mutable.
        Opm::CompletedCells completed_cells(grid);
        const auto sg = Opm::ScheduleGrid { grid, field_props, completed_cells };

        std::vector<int> requested_open_complnums;
        std::vector<int> requested_shut_complnums;
        for (const auto& rec : deck["COMPDAT"][0]) {
            connections.loadCOMPDAT(rec, "WELL", wdfac, sg, loc, ctx, errors,
                                    requested_open_complnums,
                                    requested_shut_complnums);
        }

        return connections;
    }
}

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
    Opm::WellConnections completionSet(Opm::Connection::Order::TRACK, 1,1);
    BOOST_CHECK_MESSAGE( completionSet.empty(), "Default-constructed completion set must be empty" );
    BOOST_CHECK_EQUAL( 0U , completionSet.size() );
    BOOST_CHECK(!completionSet.allConnectionsShut());
}



BOOST_AUTO_TEST_CASE(AddCompletionSizeCorrect)
{
    const auto dir = Opm::Connection::Direction::Z;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    const auto depth = 0.0;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 99.88;
    ctf_props.Kh = 355.113;
    ctf_props.rw = 0.25;

    const auto completion1 = Opm::Connection { 10,10,10, 100, 1, Opm::Connection::State::OPEN, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion2 = Opm::Connection { 10,10,11, 102, 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };

    Opm::WellConnections completionSet(Opm::Connection::Order::TRACK, 1,1);
    completionSet.add( completion1 );
    BOOST_CHECK_EQUAL( 1U , completionSet.size() );
    BOOST_CHECK_MESSAGE( !completionSet.empty(), "Non-empty completion set must not be empty" );

    completionSet.add( completion2 );
    BOOST_CHECK_EQUAL( 2U , completionSet.size() );

    BOOST_CHECK_EQUAL( completion1 , completionSet.get(0) );
}


BOOST_AUTO_TEST_CASE(WellConnectionsGetOutOfRangeThrows)
{
    const auto dir = Opm::Connection::Direction::Z;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    const auto depth = 0.0;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 99.88;
    ctf_props.Kh = 355.113;
    ctf_props.rw = 0.25;

    const auto completion1 = Opm::Connection { 10,10,10, 100, 1, Opm::Connection::State::OPEN, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion2 = Opm::Connection { 10,10,11, 102, 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };

    Opm::WellConnections completionSet(Opm::Connection::Order::TRACK, 1,1);
    completionSet.add( completion1 );
    BOOST_CHECK_EQUAL( 1U , completionSet.size() );

    completionSet.add( completion2 );
    BOOST_CHECK_EQUAL( 2U , completionSet.size() );

    BOOST_CHECK_THROW( completionSet.get(10) , std::out_of_range );
}


BOOST_AUTO_TEST_CASE(Compdat_Direction) {
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("X") == Opm::Connection::Direction::X,
                        R"(Direction "X" must be Direction::X)");
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("x") == Opm::Connection::Direction::X,
                        R"(Direction "x" must be Direction::X)");
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("Y") == Opm::Connection::Direction::Y,
                        R"(Direction "Y" must be Direction::Y)");
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("y") == Opm::Connection::Direction::Y,
                        R"(Direction "y" must be Direction::Y)");
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("Z") == Opm::Connection::Direction::Z,
                        R"(Direction "Z" must be Direction::Z)");
    BOOST_CHECK_MESSAGE(Opm::Connection::DirectionFromString("z") == Opm::Connection::Direction::Z,
                        R"(Direction "z" must be Direction::Z)");

    BOOST_CHECK_THROW(Opm::Connection::DirectionFromString(""), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::Connection::DirectionFromString("XX"), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::Connection::DirectionFromString("X-"), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::Connection::DirectionFromString("HeLlo"), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(AddCompletionCopy)
{
    const auto dir = Opm::Connection::Direction::Z;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    const auto depth = 0.0;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 99.88;
    ctf_props.Kh = 355.113;
    ctf_props.rw = 0.25;

    const auto completion1 = Opm::Connection { 10,10,10, 100, 1, Opm::Connection::State::OPEN, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion2 = Opm::Connection { 10,10,11, 101, 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion3 = Opm::Connection { 10,10,12, 102, 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };

    Opm::WellConnections completionSet(Opm::Connection::Order::TRACK, 10,10);
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


BOOST_AUTO_TEST_CASE(ActiveCompletions)
{
    const auto dir = Opm::Connection::Direction::Z;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    const auto depth = 0.0;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 99.88;
    ctf_props.Kh = 355.113;
    ctf_props.rw = 0.25;

    Opm::EclipseGrid grid { 10, 20, 20 };

    const auto completion1 = Opm::Connection { 0,0,0, grid.getGlobalIndex(0,0,0), 1, Opm::Connection::State::OPEN, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion2 = Opm::Connection { 0,0,1, grid.getGlobalIndex(0,0,1), 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };
    const auto completion3 = Opm::Connection { 0,0,2, grid.getGlobalIndex(0,0,2), 1, Opm::Connection::State::SHUT, dir, kind, 0, depth, ctf_props, 0, true };

    Opm::WellConnections completions(Opm::Connection::Order::TRACK, 10,10);
    completions.add( completion1 );
    completions.add( completion2 );
    completions.add( completion3 );

    std::vector<int> actnum(grid.getCartesianSize(), 1);
    actnum[0] = 0;
    grid.resetACTNUM(actnum);

    const Opm::WellConnections active_completions(completions, grid);
    BOOST_CHECK_EQUAL( active_completions.size() , 2U);
    BOOST_CHECK_EQUAL( completion2, active_completions.get(0));
    BOOST_CHECK_EQUAL( completion3, active_completions.get(1));
}

BOOST_AUTO_TEST_CASE(loadCOMPDATTEST)
{
    const Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC); // Unit system used in deck FIRST_SIM.DATA.

    {
        const std::string deck = R"(GRID

PERMX
  1000*0.10 /

COPY
  'PERMX' 'PERMZ' /
  'PERMX' 'PERMY' /
/

PORO
  1000*0.3 /

SCHEDULE

COMPDAT
--                                    CF      Diam    Kh      Skin   Df
    'WELL'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*     1*  'Z'  21.925 /
/)";

        const Opm::WellConnections connections = loadCOMPDAT(deck);
        const auto& conn0 = connections[0];
        BOOST_CHECK_EQUAL(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 1.168));
        BOOST_CHECK_EQUAL(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 107.872));
        BOOST_CHECK_MESSAGE(conn0.ctfAssignedFromInput(), "CTF Must be Assigned From Input");
    }

    {
        const std::string deck = R"(GRID

PERMX
  1000*0.10 /

COPY
  'PERMX' 'PERMZ' /
  'PERMX' 'PERMY' /
/

PORO
  1000*0.3 /

SCHEDULE

COMPDAT
--                                CF      Diam    Kh      Skin   Df
'WELL'  1  1   1   1 'OPEN' 1*    1.168   0.311   0       1*     1*  'Z'  21.925 /
/)";

        const Opm::WellConnections connections = loadCOMPDAT(deck);
        const auto& conn0 = connections[0];
        BOOST_CHECK_EQUAL(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 1.168));
        BOOST_CHECK_EQUAL(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 0.10 * 1.0));
    }
}


BOOST_AUTO_TEST_CASE(loadCOMPDATTESTSPE1) {
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE1CASE1.DATA");
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState state(deck);
    Opm::Schedule sched(deck, state, python);
    const auto& units = deck.getActiveUnitSystem();

    const auto& prod = sched.getWell("PROD", 0);
    const auto& connections = prod.getConnections();
    const auto& conn0 = connections[0];
    /* Expected values come from Eclipse simulation. */
    BOOST_CHECK_CLOSE(conn0.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, 10.609), 2e-2);
    BOOST_CHECK_CLOSE(conn0.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, 10000), 1e-6);
    BOOST_CHECK_MESSAGE(!conn0.ctfAssignedFromInput(), "Calculated CTF must NOT be assigned from input");
}


struct exp_conn {
    std::string well;
    int ci;
    double CF;
    double Kh;
};

BOOST_AUTO_TEST_CASE(loadCOMPDATTESTSPE9) {
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE9_CP_PACKED.DATA");
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState state(deck);
    Opm::Schedule sched(deck, state, python);
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
     const auto& well = sched.getWell(ec.well, 0);
       const auto& connections = well.getConnections();
       const auto& conn = connections[ec.ci - 1];

       BOOST_CHECK_CLOSE( conn.CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, ec.CF), 2e-1);
       BOOST_CHECK_CLOSE( conn.Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, ec.Kh), 1e-1);
       BOOST_CHECK_MESSAGE( !conn.ctfAssignedFromInput(), "Calculated SPE9 CTF values must NOT be assigned from input");
   }
}

BOOST_AUTO_TEST_CASE(ApplyWellPI) {
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
10 10 3 /

START
  5 OCT 2020 /

GRID
DXV
  10*100 /
DYV
  10*100 /
DZV
  3*10 /
DEPTHZ
  121*2000 /

ACTNUM
  100*1
  99*1 0
  100*1
/

PERMX
  300*100 /
PERMY
  300*100 /
PERMZ
  300*100 /
PORO
  300*0.3 /

SCHEDULE
WELSPECS
  'P' 'G' 10 10 2005 'LIQ' /
/

COMPDAT
  'P' 0 0 1 3 OPEN 1 100 /
/

TSTEP
  10
/

END
)");

    const auto es    = Opm::EclipseState{ deck };
    const auto sched = Opm::Schedule{ deck, es };

    const auto expectCF = 100.0*cp_rm3_per_db();

    auto connP = sched.getWell("P", 0).getConnections();
    for (const auto& conn : connP) {
        BOOST_CHECK_CLOSE(conn.CF(), expectCF, 1.0e-10);
    }

    {
        std::vector<bool> scalingApplicable;

        connP.applyWellPIScaling(2.0, scalingApplicable);  // No "prepare" -> no change.
        for (const auto& conn : connP) {
            BOOST_CHECK_CLOSE(conn.CF(), expectCF, 1.0e-10);
        }
    }

    // All CFs scaled by factor 2.
    BOOST_CHECK_MESSAGE( connP.prepareWellPIScaling(), "First call to prepareWellPIScaling must be a state change");
    BOOST_CHECK_MESSAGE(!connP.prepareWellPIScaling(), "Second call to prepareWellPIScaling must NOT be a state change");

    {
        std::vector<bool> scalingApplicable;

        connP.applyWellPIScaling(2.0, scalingApplicable);  // No "prepare" -> no change.
        for (const auto& conn : connP) {
            BOOST_CHECK_CLOSE(conn.CF(), 2.0*expectCF, 1.0e-10);
        }
    }

    // Reset CF -- simulating COMPDAT record (inactive cell)
    auto ctf_props = Opm::Connection::CTFProperties{};
    ctf_props.CF = 50.0*cp_rm3_per_db();
    ctf_props.Kh = 0.123;
    ctf_props.rw = 0.234;
    ctf_props.r0 = 0.157;

    connP.addConnection(8, 9, 0, // 9, 10, 1
                        199,
                        Opm::Connection::State::OPEN,
                        2015.0, ctf_props, 1);

    BOOST_REQUIRE_EQUAL(connP.size(), std::size_t{3});

    BOOST_CHECK_CLOSE(connP[0].CF(),  2.0*expectCF       , 1.0e-10);
    BOOST_CHECK_CLOSE(connP[1].CF(),  2.0*expectCF       , 1.0e-10);
    BOOST_CHECK_CLOSE(connP[2].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);

    // Should not apply to connection whose CF was manually specified
    {
        std::vector<bool> scalingApplicable;
        connP.applyWellPIScaling(2.0, scalingApplicable);

        BOOST_CHECK_CLOSE(connP[0].CF(),  4.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[1].CF(),  4.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[2].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
    }

    // Prepare new scaling.  Simulating new WELPI record.
    // New scaling applies to all connections.
    BOOST_CHECK_MESSAGE(connP.prepareWellPIScaling(), "Third call to prepareWellPIScaling must be a state change");

    {
        std::vector<bool> scalingApplicable;
        connP.applyWellPIScaling(2.0, scalingApplicable);

        BOOST_CHECK_CLOSE(connP[0].CF(),   8.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[1].CF(),   8.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[2].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
    }

    // Reset CF -- simulating COMPDAT record (active cell)
    connP.addConnection(8, 9, 1, // 9, 10, 2
                        198,
                        Opm::Connection::State::OPEN,
                        2015.0, ctf_props, 1);

    BOOST_REQUIRE_EQUAL(connP.size(), std::size_t{4});

    {
        std::vector<bool> scalingApplicable;
        connP.applyWellPIScaling(2.0, scalingApplicable);

        BOOST_CHECK_CLOSE(connP[0].CF(),  16.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[1].CF(),  16.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[2].CF(), 200.0*cp_rm3_per_db(), 1.0e-10);
        BOOST_CHECK_CLOSE(connP[3].CF(),  50.0*cp_rm3_per_db(), 1.0e-10);
    }

    {
        std::vector<bool> scalingApplicable;

        connP.applyWellPIScaling(2.0, scalingApplicable);
        BOOST_CHECK_CLOSE(connP[0].CF(), 32.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[1].CF(), 32.0*expectCF       , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[2].CF(), 400.0*cp_rm3_per_db(), 1.0e-10);
        BOOST_CHECK_CLOSE(connP[3].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(Completion_From_Global_Connection_Index) {
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
7 OCT 2020 /

DIMENS
  10 10 3 /

GRID
DXV
  10*100.0 /
DYV
  10*100.0 /
DZV
  3*10.0 /

DEPTHZ
  121*2000.0 /

PERMX
  300*100.0 /
PERMY
  300*100.0 /
PERMZ
  300*10.0 /
PORO
  300*0.3 /

SCHEDULE
WELSPECS
  'P' 'G' 10 10 2005 'LIQ' /
/
COMPDAT
  'P' 0 0 1 1 OPEN 1 100 /
/

TSTEP
  10
/

COMPDAT
  'P' 0 0 2 2 OPEN 1 50 /
/

TSTEP
  10
/

END
)");

    const auto es    = Opm::EclipseState{ deck };
    const auto sched = Opm::Schedule{ deck, es };

    {
        const auto connP = sched.getWell("P", 0).getConnections();

        const auto complnum_100 =
            getCompletionNumberFromGlobalConnectionIndex(connP, 100 - 1);
        const auto complnum_200 =
            getCompletionNumberFromGlobalConnectionIndex(connP, 200 - 1);

        BOOST_CHECK_MESSAGE(  complnum_100.has_value(), "Completion number must be defined at time 0 for connection in cell (10,10,1)");
        BOOST_CHECK_MESSAGE(! complnum_200.has_value(), "Completion number must NOT be defined at time 0 for connection in cell (10,10,2)");

        BOOST_CHECK_EQUAL(complnum_100.value(), 1);
    }

    {
        const auto connP = sched.getWell("P", 1).getConnections();

        const auto complnum_100 =
            getCompletionNumberFromGlobalConnectionIndex(connP, 100 - 1);
        const auto complnum_200 =
            getCompletionNumberFromGlobalConnectionIndex(connP, 200 - 1);

        BOOST_CHECK_MESSAGE(complnum_100.has_value(), "Completion number must be defined at time 0 for connection in cell (10,10,1)");
        BOOST_CHECK_MESSAGE(complnum_200.has_value(), "Completion number must be defined at time 0 for connection in cell (10,10,2)");

        BOOST_CHECK_EQUAL(complnum_100.value(), 1);
        BOOST_CHECK_EQUAL(complnum_200.value(), 2);
    }
}

BOOST_AUTO_TEST_CASE(testReAndConnectionLength) {
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE1CASE1.DATA");
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState state(deck);
    Opm::Schedule sched(deck, state, python);

    const auto& prod = sched.getWell("PROD", 0);
    const auto& connections = prod.getConnections();
    const auto& conn0 = connections[0];
    BOOST_CHECK_CLOSE(conn0.re(), 171.96498506535622 , 2e-2);
    BOOST_CHECK_CLOSE(conn0.connectionLength(),15.239999999999782, 1e-6);
}

BOOST_AUTO_TEST_CASE(loadCOMPTRAJTESTSPE1) {
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE1CASE1_WELTRAJ.DATA");
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState state(deck);
    Opm::Schedule sched(deck, state, python);
    const auto& units = deck.getActiveUnitSystem();

    const auto check_connections = [&units](const Opm::WellConnections& connections,
                                            const std::vector<double>& connection_factor,
                                            const std::vector<double>& kh,
                                            const std::vector<int>& global_index)
    {
        BOOST_REQUIRE_EQUAL(connections.size(), connection_factor.size());
        for (std::size_t i = 0; i < connections.size(); ++i) {
            BOOST_CHECK_CLOSE(connections[i].CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, connection_factor[i]), 0.1);
            BOOST_CHECK_CLOSE(connections[i].Kh(), units.to_si(Opm::UnitSystem::measure::effective_Kh, kh[i]), 0.1);
            BOOST_CHECK_EQUAL(connections[i].global_index(), global_index[i]);
        }
    };

    /* Comparison values (CFs, Kh and intersected cells) are from the PRT file of a reference run with the COMPDAT
       keyword exported by ResInsight after importing the deviation files
          WELLNAME: 'INJ'
          # X     Y      TVDMSL  MDMSL
          10400   11600  8325.0  8325.0
          10400   11600  8425.0  8425.0
          -999
          WELLNAME: 'PROD'
          # X       Y       TVDMSL  MDMSL
          3400.00   4500.00   8325.00   8325.00
          3858.19   4688.67   8374.60   8822.99
          5326.76   6005.95   8403.97   10796.0
          6500.00   7300.00   8410.00   12542.7
          -999
       and adjusting the completion data in agreement with the COMPTRAJ data in the input file.
       The coordinates are given in the map coordinate system defined by MAPAXES.

       CF and Kh are given with four significant digits and checked with a 0.1% tolerance.  This
       also covers the small difference between the Darcy constant used by ResInsight and OPM.
     */

    const auto& inj = sched.getWell("INJ", 0);
    check_connections(inj.getConnections(),
                      {10.45, 1.568, 10.45},
                      {1.0e4, 1.5e3, 1.0e4},
                      {88, 188, 288});

    const auto& prod = sched.getWell("PROD", 0);
    BOOST_CHECK_EQUAL(prod.getHeadI(), 1);
    BOOST_CHECK_EQUAL(prod.getHeadJ(), 1);
    check_connections(prod.getConnections(),
                      {110.5, 17.76, 36.05, 60.75, 235.2, 94.74, 222.7, 74.78, 89.20},
                      {1.004e5, 1.613e4, 3.273e4, 5.516e4, 2.135e5, 8.602e4, 2.022e5, 6.789e4, 8.099e4},
                      {11, 111, 211, 212, 222, 223, 233, 234, 244});
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Dflt_Action)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX 0  2 2  1 1  2 2 /
 PERMY 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext{};
    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto& well_p = sched.back().wells("P");

    BOOST_REQUIRE_MESSAGE(well_p.hasConnections(),
                          R"(Well "P" must have connections at end of simulation)");

    BOOST_REQUIRE_EQUAL(well_p.getConnections().size(), std::size_t{2});

    {
        const auto& c0 = well_p.getConnections()[0];

        BOOST_CHECK_EQUAL(c0.getI(), 1);
        BOOST_CHECK_EQUAL(c0.getJ(), 0);
        BOOST_CHECK_EQUAL(c0.getK(), 0);
    }

    {
        const auto& c1 = well_p.getConnections()[1];

        BOOST_CHECK_EQUAL(c1.getI(), 1);
        BOOST_CHECK_EQUAL(c1.getJ(), 0);
        BOOST_CHECK_EQUAL(c1.getK(), 2);
    }
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Throw)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX 0  2 2  1 1  2 2 /
 PERMY 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::THROW_EXCEPTION },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };

    BOOST_CHECK_THROW(Opm::Schedule(deck, es, ctx, errors, std::make_shared<Opm::Python>()),
                      Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Diagnostic_Text)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX 0  2 2  1 1  2 2 /
 PERMY 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 44
Connection (2,1,2) (direction 'Z') for well P ignored because
   PERMX=0.000e+00 mD and PERMY=0.000e+00 mD.)");
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Diagnostic_Text_Kx10)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX 10  2 2  1 1  2 2 /
 PERMY  0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 44
Connection (2,1,2) (direction 'Z') for well P ignored because
   PERMX=1.000e+01 mD and PERMY=0.000e+00 mD.)");
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Diagnostic_Text_Ky10)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX  0  2 2  1 1  2 2 /
 PERMY 10 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 44
Connection (2,1,2) (direction 'Z') for well P ignored because
   PERMX=0.000e+00 mD and PERMY=1.000e+01 mD.)");
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Diagnostic_Text_DirX)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Ky(2,1,2) = Kz(2,1,2) = 0.
EQUALS
 PERMY 0  2 2  1 1  2 2 /
 PERMZ 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  1  OPEN  1  1*  0.3048  /
  'P'  2  1  2  2  OPEN  1  1*  0.3048 1* 1* 1* 'X' /
  'P'  2  1  3  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 44
Connection (2,1,2) (direction 'X') for well P ignored because
   PERMY=0.000e+00 mD and PERMZ=0.000e+00 mD.)");
}

BOOST_AUTO_TEST_CASE(Compdat_Zero_Perm_Diagnostic_Text_DirY)
{
    const auto deck = Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Kz(2,1,2) = 0.
EQUALS
 PERMX 0  2 2  1 1  2 2 /
 PERMZ 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 2 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  1  OPEN  1  1*  0.3048  /
  'P'  2  1  2  2  OPEN  1  1*  0.3048 1* 1* 1* 'Y' /
  'P'  2  1  3  3  OPEN  1  1*  0.3048  /
/
TSTEP
  5*10 /
END
)");

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 44
Connection (2,1,2) (direction 'Y') for well P ignored because
   PERMZ=0.000e+00 mD and PERMX=0.000e+00 mD.)");
}

// ---------------------------------------------------------------------------
// Multiple COMPTRAJ records and branches per well
// ---------------------------------------------------------------------------

namespace {

    /// Build a deck around \p schedule with a 5x5x3 grid of 100x100x10 m
    /// cells whose top layer starts at 2000 m.  Keywords in \p extra_grid
    /// are added to the end of the GRID section.
    Opm::Deck comptrajDeck(const std::string& schedule,
                           const std::string& extra_grid = {})
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
  5 5 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
  2 20 1 2 /
WSEGDIMS
  2 20 3 /
GRID
DXV
  5*100 /
DYV
  5*100 /
DZV
  3*10 /
DEPTHZ
  36*2000 /
EQUALS
  PERMX 100 /
  PERMY 100 /
  PERMZ  10 /
  PORO    0.3 /
/
)" + extra_grid + R"(
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
  2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
)" + schedule + R"(
TSTEP
  5*10 /
END
)");
    }

    /// Convert a connection transmissibility factor from the deck's units.
    double siCF(const Opm::Deck& deck, const double cf)
    {
        return deck.getActiveUnitSystem()
            .to_si(Opm::UnitSystem::measure::transmissibility, cf);
    }

    /// Convert a Kh product from the deck's units.
    double siKh(const Opm::Deck& deck, const double kh)
    {
        return deck.getActiveUnitSystem()
            .to_si(Opm::UnitSystem::measure::effective_Kh, kh);
    }

    Opm::Schedule comptrajSchedule(const Opm::Deck& deck)
    {
        const auto es = Opm::EclipseState { deck };

        return Opm::Schedule {
            deck, es, Opm::ParseContext{}, *std::make_unique<Opm::ErrorGuard>(),
            std::make_shared<Opm::Python>()
        };
    }

    const Opm::Connection&
    connectionAt(const Opm::WellConnections& connections, const int i, const int j, const int k)
    {
        for (const auto& conn : connections) {
            if (conn.sameCoordinate(i, j, k)) {
                return conn;
            }
        }

        BOOST_FAIL(fmt::format("No connection at ({},{},{})", i, j, k));
        return connections[0];
    }

    // A vertical main branch through column (3,3) and a lateral running in
    // the X direction through the middle layer, so that cell (3,3,2) is
    // perforated by both.  Measured depths are chosen so that MD == TVD along
    // the vertical part.
    const std::string weltraj_two_branches = R"(WELSPECS
  'W1' 'G' 3 3 2000.0 OIL /
/
WELTRAJ
-- WELL BRANCH   X      Y      TVD     MD
  'W1'    1     250    250    2000    2000 /
  'W1'    1     250    250    2030    2030 /
  'W1'    2     250    250    2015    2015 /
  'W1'    2     450    250    2015    2215 /
/
)";

    // A main branch going down from the top layer into the middle one and
    // back up again, entering cell (3,3,1) twice: from MD 2002 to 2010 and
    // from MD 2030 to 2035.  An interval from MD 2002 to 2012 only covers
    // the first visit, one from MD 2028 to 2035 only the second.
    const std::string weltraj_u_shape = R"(WELSPECS
  'W1' 'G' 3 3 2000.0 OIL /
/
WELTRAJ
-- WELL BRANCH   X      Y      TVD     MD
  'W1'    1     250    250    2000    2000 /
  'W1'    1     250    250    2015    2015 /
  'W1'    1     260    250    2015    2025 /
  'W1'    1     260    250    2005    2035 /
/
)";

    /// Connection in cell (3,3,1), which weltraj_u_shape enters twice.
    const Opm::Connection& reenteredConnection(const Opm::Schedule& sched)
    {
        return connectionAt(sched.getWell("W1", 0).getConnections(), 2, 2, 0);
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Comptraj_Same_Branch_Disjoint_Intervals)
{
    // Two COMPTRAJ records for the same branch, covering the top and the
    // bottom layer but not the middle one.
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
COMPTRAJ
  'W1'    1    2020   2030   2*     1*    1*   30.0  0.25  300  0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    BOOST_REQUIRE_EQUAL(connections.size(), std::size_t{2});

    const auto& top = connectionAt(connections, 2, 2, 0);
    const auto& bottom = connectionAt(connections, 2, 2, 2);

    BOOST_CHECK(top.fromTrajectory());
    BOOST_CHECK(bottom.fromTrajectory());

    // Each cell has a single contributor, so its CTF is that record's.
    BOOST_CHECK_EQUAL(top.intervalContributions().size(), std::size_t{1});
    BOOST_CHECK_EQUAL(bottom.intervalContributions().size(), std::size_t{1});
}

BOOST_AUTO_TEST_CASE(Comptraj_Same_Branch_Respecified_Interval_Last_Wins)
{
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
COMPTRAJ
  'W1'    1    2000   2010   2*     1*    1*   30.0  0.25  300  0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    BOOST_REQUIRE_EQUAL(connections.size(), std::size_t{1});

    const auto& conn = connectionAt(connections, 2, 2, 0);

    // Re-specifying the same interval replaces its contribution outright
    // rather than adding to it.
    BOOST_CHECK_EQUAL(conn.intervalContributions().size(), std::size_t{1});
    BOOST_CHECK_CLOSE(conn.CF(), siCF(deck, 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(conn.Kh(), siKh(deck, 300.0), 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Comptraj_Two_Branches_Without_Shared_Cell)
{
    // Branch 1 perforates the top layer only, branch 2 the two cells the
    // lateral reaches beyond the shared column.
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
  'W1'    2    2115   2215   2*     1*    1*   30.0  0.25  300  0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    BOOST_REQUIRE_EQUAL(connections.size(), std::size_t{3});

    for (const auto& conn : connections) {
        BOOST_REQUIRE_EQUAL(conn.intervalContributions().size(), std::size_t{1});
    }

    const auto branchAt = [&connections](const int i, const int j, const int k)
    { return connectionAt(connections, i, j, k).intervalContributions().front().interval.branch; };

    BOOST_CHECK_EQUAL(branchAt(2, 2, 0), 1);
    BOOST_CHECK_EQUAL(branchAt(3, 2, 1), 2);
    BOOST_CHECK_EQUAL(branchAt(4, 2, 1), 2);
}

BOOST_AUTO_TEST_CASE(Comptraj_Two_Branches_Sharing_A_Cell)
{
    // Both branches perforate cell (3,3,2); their transmissibilities combine.
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2010   2020   2*     1*    1*   10.0  0.20  100  1.0 /
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.40  300  5.0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    const auto& shared = connectionAt(connections, 2, 2, 1);

    BOOST_REQUIRE_EQUAL(shared.intervalContributions().size(), std::size_t{2});
    BOOST_CHECK_EQUAL(shared.intervalContributions()[0].interval.branch, 1);
    BOOST_CHECK_EQUAL(shared.intervalContributions()[1].interval.branch, 2);

    // CF and Kh are additive.
    BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 10.0 + 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 100.0 + 300.0), 1.0e-8);
    BOOST_CHECK(shared.dir() == Opm::Connection::Direction::X);
    BOOST_CHECK_EQUAL(shared.Ke(), shared.intervalContributions()[1].ctf.Ke);

    // Wellbore radius and skin are CF-weighted averages.
    BOOST_CHECK_CLOSE(shared.rw(), (10.0*0.10 + 30.0*0.20) / 40.0, 1.0e-8);
    BOOST_CHECK_CLOSE(shared.skinFactor(), (10.0*1.0 + 30.0*5.0) / 40.0, 1.0e-8);

    // The cell that only branch 2 reaches keeps that branch's own values.
    const auto& lateral_only = connectionAt(connections, 3, 2, 1);
    BOOST_REQUIRE_EQUAL(lateral_only.intervalContributions().size(), std::size_t{1});
    BOOST_CHECK_CLOSE(lateral_only.CF(), siCF(deck, 30.0), 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Comptraj_Respecifying_One_Branch_Of_A_Shared_Cell)
{
    // The third record re-specifies branch 1 on the shared cell; branch 2's
    // contribution must survive unchanged.
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2010   2020   2*     1*    1*   10.0  0.20  100  1.0 /
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.40  300  5.0 /
/
COMPTRAJ
  'W1'    1    2010   2020   2*     1*    1*   50.0  0.20  700  1.0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    const auto& shared = connectionAt(connections, 2, 2, 1);

    BOOST_REQUIRE_EQUAL(shared.intervalContributions().size(), std::size_t{2});
    BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 50.0 + 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 700.0 + 300.0), 1.0e-8);
    BOOST_CHECK(shared.dir() == Opm::Connection::Direction::Z);
    BOOST_CHECK_EQUAL(shared.Ke(), shared.intervalContributions()[0].ctf.Ke);
}

BOOST_AUTO_TEST_CASE(Comptraj_Branch_Order_Does_Not_Matter)
{
    const auto branch_first = R"(COMPTRAJ
  'W1'    1    2010   2020   2*     1*    1*   10.0  0.20  100  1.0 /
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.40  300  5.0 /
/
)";

    const auto lateral_first = R"(COMPTRAJ
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.40  300  5.0 /
  'W1'    1    2010   2020   2*     1*    1*   10.0  0.20  100  1.0 /
/
)";

    const auto deck_a = comptrajDeck(weltraj_two_branches + branch_first);
    const auto deck_b = comptrajDeck(weltraj_two_branches + lateral_first);

    const auto sched_a = comptrajSchedule(deck_a);
    const auto sched_b = comptrajSchedule(deck_b);

    const auto& a = connectionAt(sched_a.getWell("W1", 0).getConnections(), 2, 2, 1);
    const auto& b = connectionAt(sched_b.getWell("W1", 0).getConnections(), 2, 2, 1);

    BOOST_CHECK(a.intervalContributions() == b.intervalContributions());
    BOOST_CHECK_CLOSE(a.CF(), b.CF(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.Kh(), b.Kh(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.rw(), b.rw(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.skinFactor(), b.skinFactor(), 1.0e-8);
    BOOST_CHECK(a.dir() == Opm::Connection::Direction::X);
    BOOST_CHECK(a.dir() == b.dir());
    BOOST_CHECK_EQUAL(a.Ke(), b.Ke());
}

BOOST_AUTO_TEST_CASE(Comptraj_Direction_Combines_Individual_Crossings)
{
    using Direction = Opm::Connection::Direction;
    using Contribution = Opm::Connection::IntervalContribution;
    const auto weaker = Contribution{
        .interval = {.branch = 1, .top = 0, .bottom = 10},
        .ctf = {.CF = 16, .Ke = 2},
        .direction = Direction::X,
        .strongest_crossing_cf = 8,
    };
    const auto stronger = Contribution{
        .interval = {.branch = 2, .top = 0, .bottom = 10},
        .ctf = {.CF = 10, .Ke = 3},
        .direction = Direction::Y,
        .strongest_crossing_cf = 10,
    };
    for (const bool reversed : {false, true}) {
        auto conn = Opm::Connection{};
        conn.addIntervalContribution(reversed ? stronger : weaker);
        conn.addIntervalContribution(reversed ? weaker : stronger);
        BOOST_CHECK(conn.dir() == Direction::Y);
        BOOST_CHECK_EQUAL(conn.Ke(), 3);
        BOOST_CHECK_EQUAL(conn.CF(), 26);
        conn.removeIntervalContribution(stronger.interval);
        BOOST_CHECK(conn.dir() == Direction::X);
        BOOST_CHECK_EQUAL(conn.Ke(), 2);

        auto tied = stronger;
        tied.strongest_crossing_cf = weaker.strongest_crossing_cf;
        conn.addIntervalContribution(tied);
        BOOST_CHECK(conn.dir() == Direction::X);
        BOOST_CHECK_EQUAL(conn.Ke(), 2);
        auto reverse_tie = Opm::Connection{};
        reverse_tie.addIntervalContribution(tied);
        reverse_tie.addIntervalContribution(weaker);
        BOOST_CHECK(reverse_tie.dir() == Direction::X);
        BOOST_CHECK_EQUAL(reverse_tie.Ke(), 2);

        auto changed = weaker;
        changed.direction = Direction::Z;
        BOOST_CHECK(!(changed == weaker));
        changed = weaker;
        changed.strongest_crossing_cf += 1;
        BOOST_CHECK(!(changed == weaker));
    }
}

BOOST_AUTO_TEST_CASE(Comptraj_Compdat_Mixing_Is_Rejected)
{
    // COMPDAT after COMPTRAJ.
    {
        const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
COMPDAT
  'W1'  3  3  1  1  OPEN  1  1*  0.25 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }

    // COMPTRAJ after COMPDAT.
    {
        const auto deck = comptrajDeck(R"(WELSPECS
  'W1' 'G' 3 3 2000.0 OIL /
/
COMPDAT
  'W1'  3  3  1  1  OPEN  1  1*  0.25 /
/
WELTRAJ
  'W1'    1     250    250    2000    2000 /
  'W1'    1     250    250    2030    2030 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }
}

BOOST_AUTO_TEST_CASE(Comptraj_Repeated_Records_Are_Accepted)
{
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
COMPTRAJ
  'W1'    1    2020   2030   2*     1*    1*   30.0  0.25  300  0 /
/
COMPTRAJ
  'W1'    2    2115   2215   2*     1*    1*   30.0  0.25  300  0 /
/
)");

    BOOST_CHECK_NO_THROW(comptrajSchedule(deck));
}

BOOST_AUTO_TEST_CASE(Comptraj_Branch_Number_Must_Be_Positive)
{
    // WELTRAJ.
    {
        const auto deck = comptrajDeck(R"(WELSPECS
  'W1' 'G' 3 3 2000.0 OIL /
/
WELTRAJ
  'W1'    0     250    250    2000    2000 /
  'W1'    0     250    250    2030    2030 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }

    // COMPTRAJ.
    {
        const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'   -1    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }
}

BOOST_AUTO_TEST_CASE(Comptraj_Unknown_Branch_Is_An_Input_Error)
{
    // Branch 3 is never given a trajectory.  This used to escape as a raw
    // std::out_of_range from the trajectory lookup.
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    3    2000   2010   2*     1*    1*   10.0  0.25  100  0 /
/
)");

    BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(Comptraj_Shutting_A_Shared_Cell_Shuts_It_For_All_Branches)
{
    const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2020   2*     OPEN  1*   10.0  0.20  100  1.0 /
  'W1'    2    2015   2115   2*     SHUT  1*   30.0  0.40  300  5.0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    const auto& shared = connectionAt(connections, 2, 2, 1);

    // The state of the last record to touch the cell wins, for every branch.
    BOOST_CHECK_EQUAL(shared.intervalContributions().size(), std::size_t{2});
    BOOST_CHECK(shared.state() == Opm::Connection::State::SHUT);

    // Cells that only branch 1 reaches stay open.
    BOOST_CHECK(connectionAt(connections, 2, 2, 0).state() == Opm::Connection::State::OPEN);
}

BOOST_AUTO_TEST_CASE(Comptraj_Perforation_Interval_Must_Lie_On_The_Branch)
{
    // Past the end of the branch's trajectory.
    {
        const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2000   2100   2*     1*    1*   10.0  0.25  100  0 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }

    // Before its start.
    {
        const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    2    2000   2100   2*     1*    1*   10.0  0.25  100  0 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }

    // Reversed.
    {
        const auto deck = comptrajDeck(weltraj_two_branches + R"(COMPTRAJ
  'W1'    1    2020   2010   2*     1*    1*   10.0  0.25  100  0 /
/
)");

        BOOST_CHECK_THROW(comptrajSchedule(deck), Opm::OpmInputError);
    }
}

BOOST_AUTO_TEST_CASE(Comptraj_Three_Branches_Sharing_A_Cell)
{
    // A main branch with two laterals kicking off inside the same cell, so
    // that cell (3,3,2) has three contributors.
    const auto deck = comptrajDeck(R"(WELSPECS
  'W1' 'G' 3 3 2000.0 OIL /
/
WELTRAJ
  'W1'    1     250    250    2000    2000 /
  'W1'    1     250    250    2030    2030 /
  'W1'    2     250    250    2015    2015 /
  'W1'    2     450    250    2015    2215 /
  'W1'    3     250    250    2015    2015 /
  'W1'    3     250    450    2015    2215 /
/
COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2010   2020   2*     1*    1*   10.0  0.20  100  1.0 /
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.40  300  5.0 /
  'W1'    3    2015   2115   2*     1*    1*   60.0  0.60  600  9.0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& connections = sched.getWell("W1", 0).getConnections();

    const auto& shared = connectionAt(connections, 2, 2, 1);

    BOOST_REQUIRE_EQUAL(shared.intervalContributions().size(), std::size_t{3});
    BOOST_CHECK_EQUAL(shared.intervalContributions()[0].interval.branch, 1);
    BOOST_CHECK_EQUAL(shared.intervalContributions()[1].interval.branch, 2);
    BOOST_CHECK_EQUAL(shared.intervalContributions()[2].interval.branch, 3);

    BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 10.0 + 30.0 + 60.0), 1.0e-8);
    BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 100.0 + 300.0 + 600.0), 1.0e-8);

    BOOST_CHECK_CLOSE(shared.rw(),
                      (10.0*0.10 + 30.0*0.20 + 60.0*0.30) / 100.0, 1.0e-8);
    BOOST_CHECK_CLOSE(shared.skinFactor(),
                      (10.0*1.0 + 30.0*5.0 + 60.0*9.0) / 100.0, 1.0e-8);

}

BOOST_AUTO_TEST_CASE(Comptraj_Reentering_A_Cell_Sums_Calculated_CTF)
{
    // CF and Kh calculated from the cell properties.
    const auto first_visit = std::string { R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2002   2012   2*     1*    1*    1*  0.25   1*    0 /
/
)" };

    const auto second_visit = std::string { R"(COMPTRAJ
  'W1'    1    2028   2035   2*     1*    1*    1*  0.25   1*    0 /
/
)" };

    const auto both_visits = std::string { R"(COMPTRAJ
  'W1'    1    2002   2035   2*     1*    1*    1*  0.25   1*    0 /
/
)" };

    const auto sched_first = comptrajSchedule(comptrajDeck(weltraj_u_shape + first_visit));
    const auto sched_second = comptrajSchedule(comptrajDeck(weltraj_u_shape + second_visit));

    const auto& first = reenteredConnection(sched_first);
    const auto& second = reenteredConnection(sched_second);

    BOOST_CHECK_CLOSE(first.connectionLength(), 8.0, 1.0e-8);
    BOOST_CHECK_CLOSE(second.connectionLength(), 5.0, 1.0e-8);
    BOOST_CHECK_CLOSE(first.CF() / second.CF(), 8.0 / 5.0, 1.0e-8);

    // Both visits contribute, whether they are covered by a single interval
    // or by separate ones.
    const auto sched_single = comptrajSchedule(comptrajDeck(weltraj_u_shape + both_visits));
    const auto sched_separate =
        comptrajSchedule(comptrajDeck(weltraj_u_shape + first_visit + second_visit));

    for (const auto* sched : { &sched_single, &sched_separate }) {
        const auto& conn = reenteredConnection(*sched);

        BOOST_CHECK_CLOSE(conn.CF(), first.CF() + second.CF(), 1.0e-8);
        BOOST_CHECK_CLOSE(conn.Kh(), first.Kh() + second.Kh(), 1.0e-8);
        BOOST_CHECK_CLOSE(conn.connectionLength(), 8.0 + 5.0, 1.0e-8);
        BOOST_CHECK(conn.kind() == Opm::Connection::CTFKind::Defaulted);
    }

    BOOST_CHECK_EQUAL(reenteredConnection(sched_single).intervalContributions().size(),
                      std::size_t{1});
    BOOST_CHECK_EQUAL(reenteredConnection(sched_separate).intervalContributions().size(),
                      std::size_t{2});
}

BOOST_AUTO_TEST_CASE(Comptraj_Reentry_Direction_Uses_Strongest_Crossing)
{
    const auto trajectory = std::string{R"(WELSPECS
  'W1' 'G' 3 3 2000 OIL /
/
WELTRAJ
  'W1' 1 220 250 2005 0 /
  'W1' 1 350 250 2005 130 /
  'W1' 1 350 350 2005 230 /
  'W1' 1 250 350 2005 330 /
  'W1' 1 250 205 2005 475 /
/
)"};
    const auto record = [](const int top, const int bottom)
    { return fmt::format("COMPTRAJ\n 'W1' 1 {} {} 2* 1* 1* 1* 0.25 1* 0 /\n/\n", top, bottom); };
    const auto first = comptrajSchedule(comptrajDeck(trajectory + record(0, 130)));
    const auto last = comptrajSchedule(comptrajDeck(trajectory + record(330, 475)));
    const auto& x = connectionAt(first.getWell("W1", 0).getConnections(), 2, 2, 0);
    const auto& y = connectionAt(last.getWell("W1", 0).getConnections(), 2, 2, 0);
    BOOST_CHECK(x.dir() == Opm::Connection::Direction::X);
    BOOST_CHECK(y.dir() == Opm::Connection::Direction::Y);
    BOOST_REQUIRE(y.CF() > x.CF());
    for (const auto& records : {record(0, 475),
                               record(0, 130) + record(330, 475),
                               record(330, 475) + record(0, 130)}) {
        const auto sched = comptrajSchedule(comptrajDeck(trajectory + records));
        const auto& conn = connectionAt(sched.getWell("W1", 0).getConnections(), 2, 2, 0);
        BOOST_CHECK(conn.dir() == Opm::Connection::Direction::Y);
        BOOST_CHECK_CLOSE(conn.CF(), x.CF() + y.CF(), 1.0e-8);
        BOOST_CHECK_CLOSE(conn.Kh(), x.Kh() + y.Kh(), 1.0e-8);
        BOOST_CHECK_EQUAL(conn.intervalContributions().back().strongest_crossing_cf, y.CF());
    }
    // An explicit CF applies once per cell. All its crossings have equal
    // ranking, so the first crossing supplies the direction.
    const auto explicit_cf = comptrajSchedule(comptrajDeck(trajectory + R"(COMPTRAJ
  'W1' 1 0 475 2* 1* 1* 10 0.25 100 0 /
/
)"));
    BOOST_CHECK(connectionAt(explicit_cf.getWell("W1", 0).getConnections(), 2, 2, 0).dir()
                == Opm::Connection::Direction::X);
}

BOOST_AUTO_TEST_CASE(Comptraj_Reentering_A_Cell_Counts_Input_CTF_Once)
{
    // A single interval entering cell (3,3,1) twice.
    const auto deck = comptrajDeck(weltraj_u_shape + R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2002   2035   2*     1*    1*   10.0  0.25  100    0 /
/
)");

    const auto sched = comptrajSchedule(deck);
    const auto& conn = reenteredConnection(sched);

    BOOST_CHECK_EQUAL(conn.intervalContributions().size(), std::size_t{1});
    BOOST_CHECK_CLOSE(conn.CF(), siCF(deck, 10.0), 1.0e-8);
    BOOST_CHECK_CLOSE(conn.Kh(), siKh(deck, 100.0), 1.0e-8);
    BOOST_CHECK(conn.kind() == Opm::Connection::CTFKind::DeckValue);
}

BOOST_AUTO_TEST_CASE(Comptraj_Same_Branch_Intervals_Sharing_A_Cell)
{
    // Two intervals of the same branch, each entering cell (3,3,1) once,
    // combine like intervals of different branches do, in either order.
    const auto first_visit = std::string { R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2002   2012   2*     1*    1*   10.0  0.20  100  1.0 /
/
)" };

    const auto second_visit = std::string { R"(COMPTRAJ
  'W1'    1    2028   2035   2*     1*    1*   30.0  0.40  300  5.0 /
/
)" };

    const auto deck = comptrajDeck(weltraj_u_shape + first_visit + second_visit);
    const auto sched = comptrajSchedule(deck);
    const auto& conn = reenteredConnection(sched);

    BOOST_REQUIRE_EQUAL(conn.intervalContributions().size(), std::size_t{2});
    BOOST_CHECK_EQUAL(conn.intervalContributions()[0].interval.branch, 1);
    BOOST_CHECK_EQUAL(conn.intervalContributions()[1].interval.branch, 1);

    BOOST_CHECK_CLOSE(conn.CF(), siCF(deck, 10.0 + 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(conn.Kh(), siKh(deck, 100.0 + 300.0), 1.0e-8);
    BOOST_CHECK_CLOSE(conn.rw(), (10.0*0.10 + 30.0*0.20) / 40.0, 1.0e-8);
    BOOST_CHECK_CLOSE(conn.skinFactor(), (10.0*1.0 + 30.0*5.0) / 40.0, 1.0e-8);

    const auto sched_reversed =
        comptrajSchedule(comptrajDeck(weltraj_u_shape + second_visit + first_visit));
    const auto& reversed = reenteredConnection(sched_reversed);

    BOOST_CHECK(reversed.intervalContributions() == conn.intervalContributions());
    BOOST_CHECK(reversed.ctfProperties() == conn.ctfProperties());
}

BOOST_AUTO_TEST_CASE(Comptraj_Input_And_Calculated_CTF_Sharing_A_Cell)
{
    const auto input = std::string { R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2002   2012   2*     1*    1*   10.0  0.25  100    0 /
/
)" };

    const auto calculated = std::string { R"(COMPTRAJ
  'W1'    1    2028   2035   2*     1*    1*    1*  0.25   1*    0 /
/
)" };

    const auto sched_calculated = comptrajSchedule(comptrajDeck(weltraj_u_shape + calculated));
    const auto& calculated_only = reenteredConnection(sched_calculated);

    for (const auto& schedule : { input + calculated, calculated + input }) {
        const auto deck = comptrajDeck(weltraj_u_shape + schedule);
        const auto sched = comptrajSchedule(deck);
        const auto& conn = reenteredConnection(sched);

        // The values add up, and the result counts as given in the input.
        BOOST_CHECK_CLOSE(conn.CF(), siCF(deck, 10.0) + calculated_only.CF(), 1.0e-8);
        BOOST_CHECK_CLOSE(conn.Kh(), siKh(deck, 100.0) + calculated_only.Kh(), 1.0e-8);
        BOOST_CHECK(conn.kind() == Opm::Connection::CTFKind::DeckValue);
    }
}

BOOST_AUTO_TEST_CASE(Comptraj_Respecified_Interval_Releases_Cells_It_No_Longer_Perforates)
{
    // Cell (3,3,2) has no lateral permeability, so a connection factor
    // calculated from the cell properties is not representable.  An
    // interval only perforates the cell with a factor given in the input.
    const auto zero_perm = std::string { R"(EQUALS
  PERMX 0 3 3 3 3 2 2 /
  PERMY 0 3 3 3 3 2 2 /
/
)" };

    const auto input_ctf = std::string { R"(COMPTRAJ
-- WELL BRANCH  TOP    BOT   REF NO STATE SAT   CF   DIAM   KH  SKIN
  'W1'    1    2000   2030   2*     1*    1*   10.0  0.25  100  0 /
/
)" };

    const auto calculated_ctf = std::string { R"(COMPTRAJ
  'W1'    1    2000   2030   2*     1*    1*    1*  0.25   1*  0 /
/
)" };

    // A cell perforated by the re-specified interval only is no longer
    // connected.  Connections made afterwards are still numbered uniquely.
    {
        const auto lateral = std::string { R"(COMPTRAJ
  'W1'    2    2115   2215   2*     1*    1*   30.0  0.25  300  0 /
/
)" };

        const auto deck = comptrajDeck(weltraj_two_branches + input_ctf
                                       + calculated_ctf + lateral, zero_perm);
        const auto sched = comptrajSchedule(deck);
        const auto& connections = sched.getWell("W1", 0).getConnections();

        BOOST_REQUIRE_EQUAL(connections.size(), std::size_t{4});

        auto complnums = std::set<int>{};
        auto sort_values = std::set<std::size_t>{};
        for (const auto& conn : connections) {
            BOOST_CHECK_MESSAGE(! conn.sameCoordinate(2, 2, 1),
                                "Cell (3,3,2) must no longer be connected");

            complnums.insert(conn.complnum());
            sort_values.insert(conn.sort_value());
        }

        BOOST_CHECK_EQUAL(complnums.size(), connections.size());
        BOOST_CHECK_EQUAL(sort_values.size(), connections.size());

        BOOST_CHECK(connectionAt(connections, 2, 2, 0).kind() ==
                    Opm::Connection::CTFKind::Defaulted);
        BOOST_CHECK(connectionAt(connections, 2, 2, 2).kind() ==
                    Opm::Connection::CTFKind::Defaulted);
    }

    // A cell also perforated by another interval keeps that one's
    // contribution only.
    {
        const auto lateral = std::string { R"(COMPTRAJ
  'W1'    2    2015   2115   2*     1*    1*   30.0  0.25  300  0 /
/
)" };

        const auto deck = comptrajDeck(weltraj_two_branches + input_ctf
                                       + lateral + calculated_ctf, zero_perm);
        const auto sched = comptrajSchedule(deck);
        const auto& connections = sched.getWell("W1", 0).getConnections();

        BOOST_REQUIRE_EQUAL(connections.size(), std::size_t{4});

        const auto& shared = connectionAt(connections, 2, 2, 1);
        BOOST_REQUIRE_EQUAL(shared.intervalContributions().size(), std::size_t{1});
        BOOST_CHECK_EQUAL(shared.intervalContributions().front().interval.branch, 2);
        BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 30.0), 1.0e-8);
        BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 300.0), 1.0e-8);
    }
}

namespace {

    /// WELSPECS of well W1 with head location \p head_ij, followed by a main
    /// branch that enters the top layer in cell (2,2,1) from above, before
    /// running in the X direction through cells (3,2,1), (4,2,1) and
    /// (5,2,1).  A lateral leaves it in cell (3,2,1) and runs in the Y
    /// direction through cells (3,3,1), (3,4,1) and (3,5,1).  Both branches
    /// lie in the middle of the top layer.
    std::string weltrajDeviated(const std::string& head_ij)
    {
        return R"(WELSPECS
  'W1' 'G' )" + head_ij + R"( 2000.0 OIL /
/
WELTRAJ
-- WELL BRANCH   X      Y      TVD     MD
  'W1'    1     150    150    1990    1990 /
  'W1'    1     150    150    2005    2005 /
  'W1'    1     450    150    2005    2305 /
  'W1'    2     250    150    2005    2105 /
  'W1'    2     250    450    2005    2405 /
/
)";
    }

} // Anonymous namespace

namespace {

    /// Cells of \p connections, in order, as one-based "(I,J,K)" strings.
    std::vector<std::string> connectedCells(const Opm::WellConnections& connections)
    {
        auto cells = std::vector<std::string>{};
        for (const auto& conn : connections) {
            cells.push_back(fmt::format("({},{},{})", conn.getI() + 1, conn.getJ() + 1, conn.getK() + 1));
        }

        return cells;
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Comptraj_Track_Order_Follows_The_Branches)
{
    // Perforates cells (4,2,1) and (5,2,1).
    const auto main_branch = std::string {
        "  'W1'    1    2180   2280   2*     1*    1*    1*  0.25   1*  0 /\n"
    };

    // Perforates cells (3,3,1) and (3,4,1), which lie closer to the head.
    const auto lateral = std::string {
        "  'W1'    2    2180   2280   2*     1*    1*    1*  0.25   1*  0 /\n"
    };

    for (const auto& records : { main_branch + lateral, lateral + main_branch }) {
        const auto sched = comptrajSchedule(comptrajDeck(weltrajDeviated("2 2") +
                                                         "COMPTRAJ\n" + records + "/\n"));
        const auto& connections = sched.getWell("W1", 0).getConnections();

        BOOST_REQUIRE(connections.ordering() == Opm::Connection::Order::TRACK);

        // COMPTRAJ uses the regular TRACK-ordering heuristic, regardless of
        // the order in which the records created the connections.
        const auto expected = std::vector<std::string> {
            "(3,3,1)", "(3,4,1)", "(4,2,1)", "(5,2,1)"
        };

        const auto cells = connectedCells(connections);
        BOOST_CHECK_EQUAL_COLLECTIONS(cells.begin(), cells.end(),
                                      expected.begin(), expected.end());
    }
}

namespace {

    Opm::Deck zeroPermMSWDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
START
  18 MAR 2026 /
OIL
WATER
DIMENS
3 1 3 /
TABDIMS
/
EQLDIMS
/
WELLDIMS
 1 3 1 1 /
WSEGDIMS
 1 5 1 /
GRID
DXV
 3*100 /
DYV
 100 /
DZV
 3*10 /
DEPTHZ
 8*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/
-- Kx(2,1,2) = Ky(2,1,2) = 0.
EQUALS
 PERMX 0  2 2  1 1  2 2 /
 PERMY 0 /
/
PROPS
DENSITY
  800 1000 1 /
SOLUTION
EQUIL
2010 200 2010 1.23 1995 0.0 1* 1* -5 /
SCHEDULE
WELSPECS
  'P' 'G' 2 1 2005.0 LIQ /
/
COMPDAT
  'P'  2  1  1  3  OPEN  1  1*  0.3048  /
/
WELSEGS
  'P' 2000.0 0.0 1* 'INC' 'HFA' /
  2  4  1  1  10.0  10.0  0.1  1.0e-3 /
/
COMPSEGS
  'P' /
  2  1  1  1   0.0  10.0 /
  2  1  2  1  10.0  20.0 /
  2  1  3  1  20.0  30.0 /
/
TSTEP
  5*10 /
END
)");
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Compsegs_Zero_Perm_Dflt_Action)
{
    const auto deck = zeroPermMSWDeck();

    const auto ctx = Opm::ParseContext{};
    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto& well_p = sched.back().wells("P");

    BOOST_REQUIRE_MESSAGE(well_p.isMultiSegment(),
                          R"(Well "P" must be a multi-segmented well)");

    const auto& conns = well_p.getConnections();
    BOOST_REQUIRE_EQUAL(conns.size(), std::size_t{2});

    BOOST_CHECK_EQUAL(conns[0].getK(), 0);
    BOOST_CHECK_MESSAGE(conns[0].attachedToSegment(),
                        "Connection (2,1,1) must be attached to a segment");

    BOOST_CHECK_EQUAL(conns[1].getK(), 2);
    BOOST_CHECK_MESSAGE(conns[1].attachedToSegment(),
                        "Connection (2,1,3) must be attached to a segment");
}

BOOST_AUTO_TEST_CASE(Compsegs_Zero_Perm_Diagnostic_Text)
{
    const auto deck = zeroPermMSWDeck();

    const auto ctx = Opm::ParseContext {
        std::vector {
            std::pair { Opm::ParseContext::SCHEDULE_COMPDAT_ZERO_PERM,
                        Opm::InputErrorAction::DELAYED_EXIT1 },
        }
    };

    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto diagnostic = errors.formattedErrors();
    errors.clear();

    // Note: Leading newline ("R(\n)) added by ErrorGuard::formattedErrors().
    BOOST_CHECK_EQUAL(diagnostic, R"(
Problem with keyword COMPDAT
In <memory string> line 46
Connection (2,1,2) (direction 'Z') for well P ignored because
   PERMX=0.000e+00 mD and PERMY=0.000e+00 mD.
Problem with keyword COMPSEGS
In <memory string> line 53
Connection (2,1,2) for well P not attached to a segment because the
connection does not exist, likely having been ignored due to zero permeability
   PERMX=0.000e+00 mD, PERMY=0.000e+00 mD, and PERMZ=1.000e+01 mD.)");
}

BOOST_AUTO_TEST_CASE(Predicate_Same_Except_For_Ordering)
{
    const auto deck = zeroPermMSWDeck();

    const auto ctx = Opm::ParseContext{};
    auto errors = Opm::ErrorGuard{};

    const auto es = Opm::EclipseState { deck };
    const auto sched = Opm::Schedule {
        deck, es, ctx, errors,
        std::make_shared<Opm::Python>()
    };

    const auto& well_p = sched.back().wells("P");
    const auto& conns = well_p.getConnections();

    auto permuted = Opm::WellConnections {
        conns.ordering(),
        conns.getHeadI(),
        conns.getHeadJ(),
    };

    {
        auto ix = std::vector<std::size_t>(conns.size());
        std::iota(ix.begin(), ix.end(), 0);
        std::ranges::reverse(ix);

        for (const auto& idx : ix) {
            permuted.add(conns[idx]);
        }
    }

    BOOST_CHECK_MESSAGE(conns != permuted, "Permuted connections should not be equal to the original ordering");
    BOOST_CHECK_MESSAGE(permuted.isSameExceptForOrdering(conns),
                        "Permuted connections should be the same as the original except for ordering");
}
