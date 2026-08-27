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

#include <array>
#include <cstddef>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
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

    const auto& inj = sched.getWell("INJ", 0);
    const auto& connections = inj.getConnections();

    /* Comparison values (CFs and intersected cells) are from ResInsight through importing a deviation file with contents
          WELLNAME: 'INJ1'
          # X   Y    TVDMSL   MDMSL
          500   500  -100.0   0.0
          500   500   8325.0  8325.0
          2500  2500  8425.0  8450.0
       and adjusting the completion data in agreement with the COMPTRAJ data in the input file
     */
    const std::array<double, 4> connection_factor{311.783, 7.79428, 38.9674, 62.3465};
    const std::array<int, 4> global_index{0, 100, 111, 211};
    BOOST_CHECK_EQUAL(connections.size(), 4);
    for (std::size_t i = 0 ; i < connections.size();  ++i ) {
         BOOST_CHECK_CLOSE(connections[i].CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, connection_factor[i]), 2e-2);
         BOOST_CHECK_EQUAL(connections[i].global_index(), global_index[i]);
    }
}

BOOST_AUTO_TEST_CASE(loadCOMPTRAJTESTSPE1_2) {
    Opm::Parser parser;

    const auto deck = parser.parseFile("SPE1CASE1_WELTRAJ_2.DATA");
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState state(deck);
    Opm::Schedule sched(deck, state, python);
    const auto& units = deck.getActiveUnitSystem();

    const auto& inj = sched.getWell("INJ", 0);
    const auto& connections = inj.getConnections();

    /* Comparison values (CFs and intersected cells) are from ResInsight through importing a deviation file with contents
          WELLNAME: 'INJ1'
          # X   Y    TVDMSL   MDMSL
          2500   3500  -100.0   0.0
          2500   3500   8325.0  8325.0
          2750   3750   8375.0  8375.0
          3500   4500   8400.0  8400.0
          4500   5500   8425.0  8425.0
          -999
       and adjusting the completion data in agreement with the COMPTRAJ data in the input file
     */
    const std::array<double, 5> connection_factor{78.5921, 11.7884, 77.9007, 311.585, 155.784};
    const std::array<int, 5> global_index{0, 100, 200, 211, 222};
    BOOST_CHECK_EQUAL(connections.size(), 5);
    for (std::size_t i = 0 ; i < connections.size();  ++i ) {
         BOOST_CHECK_CLOSE(connections[i].CF(), units.to_si(Opm::UnitSystem::measure::transmissibility, connection_factor[i]), 2e-2);
         BOOST_CHECK_EQUAL(connections[i].global_index(), global_index[i]);
    }
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
    /// cells whose top layer starts at 2000 m.
    Opm::Deck comptrajDeck(const std::string& schedule)
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
    BOOST_CHECK_EQUAL(top.comptrajBranches().size(), std::size_t{1});
    BOOST_CHECK_EQUAL(bottom.comptrajBranches().size(), std::size_t{1});
    BOOST_CHECK(top.comptrajBranchCTFs().empty());
    BOOST_CHECK(bottom.comptrajBranchCTFs().empty());
}

BOOST_AUTO_TEST_CASE(Comptraj_Same_Branch_Overlapping_Interval_Last_Wins)
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

    // Re-specifying the only contributing branch replaces its contribution
    // outright rather than adding to it.
    BOOST_REQUIRE_EQUAL(conn.comptrajBranches().size(), std::size_t{1});
    BOOST_CHECK(conn.comptrajBranchCTFs().empty());
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
        BOOST_CHECK_EQUAL(conn.comptrajBranches().size(), std::size_t{1});
        BOOST_CHECK(conn.comptrajBranchCTFs().empty());
    }

    BOOST_CHECK_EQUAL(connectionAt(connections, 2, 2, 0).comptrajBranches().front(), 1);
    BOOST_CHECK_EQUAL(connectionAt(connections, 3, 2, 1).comptrajBranches().front(), 2);
    BOOST_CHECK_EQUAL(connectionAt(connections, 4, 2, 1).comptrajBranches().front(), 2);
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

    BOOST_REQUIRE_EQUAL(shared.comptrajBranches().size(), std::size_t{2});
    BOOST_CHECK_EQUAL(shared.comptrajBranches()[0], 1);
    BOOST_CHECK_EQUAL(shared.comptrajBranches()[1], 2);
    BOOST_REQUIRE_EQUAL(shared.comptrajBranchCTFs().size(), std::size_t{2});

    // CF and Kh are additive.
    BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 10.0 + 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 100.0 + 300.0), 1.0e-8);

    // Wellbore radius and skin are CF-weighted averages.
    BOOST_CHECK_CLOSE(shared.rw(), (10.0*0.10 + 30.0*0.20) / 40.0, 1.0e-8);
    BOOST_CHECK_CLOSE(shared.skinFactor(), (10.0*1.0 + 30.0*5.0) / 40.0, 1.0e-8);

    // The cell that only branch 2 reaches keeps that branch's own values.
    const auto& lateral_only = connectionAt(connections, 3, 2, 1);
    BOOST_REQUIRE_EQUAL(lateral_only.comptrajBranches().size(), std::size_t{1});
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

    BOOST_REQUIRE_EQUAL(shared.comptrajBranches().size(), std::size_t{2});
    BOOST_CHECK_CLOSE(shared.CF(), siCF(deck, 50.0 + 30.0), 1.0e-8);
    BOOST_CHECK_CLOSE(shared.Kh(), siKh(deck, 700.0 + 300.0), 1.0e-8);
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

    BOOST_CHECK(a.comptrajBranches() == b.comptrajBranches());
    BOOST_CHECK_CLOSE(a.CF(), b.CF(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.Kh(), b.Kh(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.rw(), b.rw(), 1.0e-8);
    BOOST_CHECK_CLOSE(a.skinFactor(), b.skinFactor(), 1.0e-8);
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
    BOOST_CHECK_EQUAL(shared.comptrajBranches().size(), std::size_t{2});
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
