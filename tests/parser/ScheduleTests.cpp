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

#define BOOST_TEST_MODULE ScheduleTests

#include <boost/test/unit_test.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 71
#include <boost/test/floating_point_comparison.hpp>
#else
#include <boost/test/tools/floating_point_comparison.hpp>
#endif

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/common/utility/ActiveGridCells.hpp>
#include <opm/common/utility/TimeService.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/BCConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>
#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/Group/GTNode.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRate.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFoamProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFractureSeeds.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>
#include <opm/input/eclipse/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include "tests/WorkArea.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

using namespace Opm;

namespace {
    double liquid_PI_unit()
    {
        return UnitSystem::newMETRIC().to_si(UnitSystem::measure::liquid_productivity_index, 1.0);
    }

    double sm3_per_day()
    {
        return UnitSystem::newMETRIC().to_si(UnitSystem::measure::liquid_surface_rate, 1.0);
    }

    double cp_rm3_per_db()
    {
        return UnitSystem::newMETRIC().to_si(UnitSystem::measure::transmissibility, 1.0);
    }

    Schedule make_schedule(const std::string& deck_string)
    {
        const auto deck = Parser{}.parseString(deck_string);

        EclipseGrid grid(10, 10, 10);
        const TableManager table (deck);
        const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
        const Runspec runspec (deck);

        return { deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>() };
    }

    std::string createDeck()
    {
        return { R"(
START
8 MAR 1998 /

SCHEDULE

)" };
    }

    std::string createDeckWithWells()
    {
        return { R"(
START             -- 0
10 MAI 2007 /
SCHEDULE
WELSPECS
     'W_1'        'OP'   30   37  3.33       'OIL'  7* /
/
DATES             -- 1
 10  'JUN'  2007 /
/
DATES             -- 2,3
  10  JLY 2007 /
  10  AUG 2007 /
/
WELSPECS
     'WX2'        'OP'   30   37  3.33       'OIL'  7* /
     'W_3'        'OP'   20   51  3.92       'OIL'  7* /
/;
)" };
    }

    std::string createDeckWTEST()
    {
        return { R"(
START             -- 0
10 MAI 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WELSPECS
     'DEFAULT'    'OP'   30   37  3.33       'OIL'  7*/
     'ALLOW'      'OP'   30   37  3.33       'OIL'  3*  YES /
     'BAN'        'OP'   20   51  3.92       'OIL'  3*  NO /
     'W1'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'W2'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'W3'         'OP'   20   51  3.92       'OIL'  3*  NO /
/

COMPDAT
 'BAN'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
/

WCONHIST
     'BAN'      'OPEN'      'RESV'      0.000      0.000      0.000  5* /
/

SUMTHIN
  1 /

WTEST
   'ALLOW'   1   'PE' /
/

DATES             -- 1
 10  JUN 2007 /
/

WTEST
   'ALLOW'  1  '' /
   'BAN'    1  'DGC' /
/

WCONHIST
     'BAN'      'OPEN'      'RESV'      1.000      0.000      0.000  5* /
/

DATES             -- 2
 10  JUL 2007 /
/

SUMTHIN
  10 /


WELSPECS
     'I1'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'I2'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'I3'         'OP'   20   51  3.92       'OIL'  3*  NO /
/

WLIST
  '*ILIST'  'NEW'  I1 /
  '*ILIST'  'ADD'  I2 /
  '*EMPTY'  'NEW' /
/

WCONPROD
     'BAN'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/


DATES             -- 3
 10  AUG 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      0 /
/

DATES             -- 4
 10  SEP 2007 /
/

WELOPEN
 'BAN' OPEN /
/

DATES             -- 5
 10  NOV 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      1.0 /
/
)" };
    }

    std::string createDeckForTestingCrossFlow()
    {
        return { R"(
START             -- 0
10 MAI 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WELSPECS
     'DEFAULT'    'OP'   30   37  3.33       'OIL'  7*/
     'ALLOW'      'OP'   30   37  3.33       'OIL'  3*  YES /
     'BAN'        'OP'   20   51  3.92       'OIL'  3*  NO /
/

COMPDAT
 'BAN'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
/

WCONHIST
     'BAN'      'OPEN'      'RESV'      0.000      0.000      0.000  5* /
/

DATES             -- 1
 10  JUN 2007 /
/

WCONHIST
     'BAN'      'OPEN'      'RESV'      1.000      0.000      0.000  5* /
/

DATES             -- 2
 10  JUL 2007 /
/

WELSPECS
     'BAN'        'OP'   20   51  3.92       'OIL'  2*  STOP YES /
/


WCONPROD
     'BAN'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/


DATES             -- 3
 10  AUG 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      0 /
/

DATES             -- 4
 10  SEP 2007 /
/

WELOPEN
 'BAN' OPEN /
/

DATES             -- 4
 10  NOV 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      1.0 /
/
)" };
    }

    std::string createDeckWithWellsOrdered()
    {
        return { R"(
START             -- 0
10 MAI 2007 /
WELLDIMS
   *  *   3 /
SCHEDULE
WELSPECS
     'CW_1'        'CG'   3   3  3.33       'OIL'  7* /
     'BW_2'        'BG'   3   3  3.33       'OIL'  7* /
     'AW_3'        'AG'   2   5  3.92       'OIL'  7* /
/
)" };
    }

    std::string createDeckWithWellsOrderedGRUPTREE()
    {
        return { R"(
START             -- 0
10 MAI 2007 /
SCHEDULE
GRUPTREE
  PG1 PLATFORM /
  PG2 PLATFORM /
  CG1  PG1 /
  CG2  PG2 /
/
WELSPECS
     'DW_0'        'CG1'   3   3  3.33       'OIL'  7* /
     'CW_1'        'CG1'   3   3  3.33       'OIL'  7* /
     'BW_2'        'CG2'   3   3  3.33       'OIL'  7* /
     'AW_3'        'CG2'   2   5  3.92       'OIL'  7* /
/
)" };
    }

    std::string createDeckWithWellsAndCompletionData()
    {
        return { R"(
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 /
 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 /
 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 /
 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 /
 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 /
/
DATES             -- 2,3
 10  JUL 2007 /
 10  AUG 2007 /
/
COMPDAT // with defaulted I and J
 'OP_1'  0  *   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
)" };
    }

    bool has_name(const std::vector<std::string>& names,
                  const std::string& name)
    {
        return std::any_of(names.begin(), names.end(),
                           [&name](const std::string& search)
                           {
                               return search == name;
                           });
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingReturnsDefaults) {
    Deck deck;
    Parser parser;
    deck.addKeyword( DeckKeyword( parser.getKeyword("SCHEDULE" )));
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>());
    BOOST_CHECK_EQUAL( schedule.getStartTime() , asTimeT( TimeStampUTC(1983, 1, 1)));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrdered) {
    const auto& schedule = make_schedule( createDeckWithWellsOrdered() );
    auto well_names = schedule.wellNames();

    BOOST_CHECK( has_name(well_names, "CW_1"));
    BOOST_CHECK( has_name(well_names, "BW_2"));
    BOOST_CHECK( has_name(well_names, "AW_3"));

    auto group_names = schedule.groupNames();
    BOOST_CHECK_EQUAL( "FIELD", group_names[0]);
    BOOST_CHECK_EQUAL( "CG", group_names[1]);
    BOOST_CHECK_EQUAL( "BG", group_names[2]);
    BOOST_CHECK_EQUAL( "AG", group_names[3]);

    auto restart_groups = schedule.restart_groups(0);
    BOOST_REQUIRE_EQUAL(restart_groups.size(), 4U);
    for (std::size_t group_index = 0; group_index < restart_groups.size() - 1; group_index++) {
        const auto& group_ptr = restart_groups[group_index];
        BOOST_CHECK_EQUAL(group_ptr->insert_index(), group_index + 1);
    }
    const auto& field_ptr = restart_groups.back();
    BOOST_CHECK_EQUAL(field_ptr->insert_index(), 0U);
    BOOST_CHECK_EQUAL(field_ptr->name(), "FIELD");
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrderedGRUPTREE)
{
    const auto schedule = make_schedule(createDeckWithWellsOrderedGRUPTREE());
    const auto group_names = schedule.groupNames("P*", 0);

    BOOST_CHECK( std::find(group_names.begin(), group_names.end(), "PG1") != group_names.end() );
    BOOST_CHECK( std::find(group_names.begin(), group_names.end(), "PG2") != group_names.end() );
    BOOST_CHECK( std::find(group_names.begin(), group_names.end(), "PLATFORM") != group_names.end() );
}


BOOST_AUTO_TEST_CASE(GroupTree2TEST) {
    const auto& schedule = make_schedule( createDeckWithWellsOrderedGRUPTREE() );

    BOOST_CHECK_THROW( schedule.groupTree("NO_SUCH_GROUP", 0), std::exception);
    auto cg1 = schedule.getGroup("CG1", 0);
    BOOST_CHECK( cg1.hasWell("DW_0"));
    BOOST_CHECK( cg1.hasWell("CW_1"));

    auto cg1_tree = schedule.groupTree("CG1", 0);
    BOOST_CHECK_EQUAL(cg1_tree.wells().size(), 2U);

    auto gt = schedule.groupTree(0);
    BOOST_CHECK_EQUAL(gt.wells().size(), 0U);
    BOOST_CHECK_EQUAL(gt.group().name(), "FIELD");
    BOOST_CHECK_THROW(gt.parent_name(), std::invalid_argument);

    auto cg = gt.groups();
    auto pg = cg[0];
    BOOST_CHECK_EQUAL(cg.size(), 1U);
    BOOST_CHECK_EQUAL(pg.group().name(), "PLATFORM");
    BOOST_CHECK_EQUAL(pg.parent_name(), "FIELD");
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithStart) {
    const auto& schedule = make_schedule( createDeck() );
    BOOST_CHECK_EQUAL( schedule.getStartTime() , asTimeT(TimeStampUTC(1998, 3  , 8 )));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithSCHEDULENoThrow) {
    BOOST_CHECK_NO_THROW( make_schedule( "SCHEDULE" ));
}

BOOST_AUTO_TEST_CASE(EmptyScheduleHasNoWells) {
    const auto& schedule = make_schedule( createDeck() );
    BOOST_CHECK_EQUAL( 0U , schedule.numWells() );
    BOOST_CHECK_EQUAL( false , schedule.hasWell("WELL1") );
    BOOST_CHECK_THROW( schedule.getWell("WELL2", 0) , std::exception);
}



BOOST_AUTO_TEST_CASE(EmptyScheduleHasFIELDGroup) {
    const auto& schedule = make_schedule( createDeck() );

    BOOST_CHECK_EQUAL( 1U , schedule.back().groups.size() );
    BOOST_CHECK_EQUAL( true , schedule.back().groups.has("FIELD") );
    BOOST_CHECK_EQUAL( false , schedule.back().groups.has("GROUP") );
    BOOST_CHECK_THROW( schedule[0].groups.get("GROUP") , std::exception);
}

BOOST_AUTO_TEST_CASE(HasGroup_At_Time) {
    const auto input = std::string { R"(
SCHEDULE
WELSPECS
-- Group 'P' exists from the first report step
  'P1' 'P' 1 1  2502.5  'OIL' /
/
WCONPROD
  'P1' 'OPEN' 'ORAT'  123.4  4*  50.0 /
/
TSTEP
  10 20 30 40 /
WELSPECS
-- Group 'I' does not exist before now (report step 4, zero-based = 3)
  'I1' 'I' 5 5 2522.5 'WATER' /
/
WCONINJE
  'I1' 'WATER'  'OPEN'  'RATE'  200  1*  450.0 /
/
TSTEP
  50 50 /
END
)"
    };

    const auto sched = make_schedule(input);

    BOOST_CHECK_MESSAGE(sched.back().groups.has("P"), R"(Group "P" Must Exist)");
    BOOST_CHECK_MESSAGE(sched.back().groups.has("I"), R"(Group "I" Must Exist)");

    BOOST_CHECK_MESSAGE(  sched[3].groups.has("P"), R"(Group "P" Must Exist at Report Step 3)");
    BOOST_CHECK_MESSAGE(! sched[3].groups.has("I"), R"(Group "I" Must NOT Exist at Report Step 3)");
    BOOST_CHECK_MESSAGE(  sched[4].groups.has("I"), R"(Group "I" Must Exist at Report Step 4)");

    BOOST_CHECK_MESSAGE(sched[6].groups.has("P"), R"(Group "P" Must Exist At Last Report Step)");
    BOOST_CHECK_MESSAGE(sched[6].groups.has("I"), R"(Group "I" Must Exist At Last Report Step)");

    BOOST_CHECK_THROW(sched[3].groups.get("I"), std::exception);
}

BOOST_AUTO_TEST_CASE(Change_Injector_Type) {
    const auto input = std::string { R"(
SCHEDULE
WELSPECS
-- Group 'I' does not exist before now (report step 4, zero-based = 3)
  'I1' 'I' 5 5 2522.5 'WATER' /
/
WCONINJE
  'I1' 'WATER'  'OPEN'  'RATE'  200  1*  450.0 /
/
TSTEP
  50 50 /
WCONINJE
  'I1' 'GAS'  'OPEN'  'RATE'  200  1*  450.0 /
/
TSTEP
  50 50 /
END
)"
    };

    const auto sched = make_schedule(input);
    BOOST_CHECK(  sched[0].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_UPDATE) );
    BOOST_CHECK(  !sched[1].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_UPDATE) );
    BOOST_CHECK(  sched[2].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_UPDATE) );
    BOOST_CHECK(  !sched[3].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_UPDATE) );
    BOOST_CHECK(  !sched[0].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_TYPE_CHANGED) );
    BOOST_CHECK(  !sched[1].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_TYPE_CHANGED) );
    BOOST_CHECK(  sched[2].wellgroup_events().hasEvent("I1", ScheduleEvents::Events::INJECTION_TYPE_CHANGED) );
    BOOST_CHECK(  !sched[3].wellgroup_events().hasEvent("I1",ScheduleEvents::Events::INJECTION_TYPE_CHANGED) );
}

BOOST_AUTO_TEST_CASE(WellsIterator_Empty_EmptyVectorReturned) {
    const auto& schedule = make_schedule( createDeck() );

    const auto wells_alltimesteps = schedule.getWellsatEnd();
    BOOST_CHECK_EQUAL(0U, wells_alltimesteps.size());

    const auto wells_t0 = schedule.getWells(0);
    BOOST_CHECK_EQUAL(0U, wells_t0.size());

    // The time argument is beyond the length of the vector
    BOOST_CHECK_THROW(schedule.getWells(1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(WellsIterator_HasWells_WellsReturned) {
    const auto& schedule = make_schedule( createDeckWithWells() );
    size_t timeStep = 0;

    const auto wells_alltimesteps = schedule.getWellsatEnd();
    BOOST_CHECK_EQUAL(3U, wells_alltimesteps.size());
    const auto wells_t0 = schedule.getWells(timeStep);
    BOOST_CHECK_EQUAL(1U, wells_t0.size());
    const auto wells_t3 = schedule.getWells(3);
    BOOST_CHECK_EQUAL(3U, wells_t3.size());

    const auto& unique = schedule.unique<NameOrder>();
    BOOST_CHECK_EQUAL( unique.size(), 2 );
    BOOST_CHECK_EQUAL( unique[0].first, 0 );
    BOOST_CHECK_EQUAL( unique[1].first, 3 );

    BOOST_CHECK( unique[0].second == schedule[0].well_order());
    BOOST_CHECK( unique[1].second == schedule[3].well_order());
}



BOOST_AUTO_TEST_CASE(ReturnNumWellsTimestep) {
    const auto& schedule = make_schedule( createDeckWithWells() );

    BOOST_CHECK_EQUAL(schedule.numWells(0), 1U);
    BOOST_CHECK_EQUAL(schedule.numWells(1), 1U);
    BOOST_CHECK_EQUAL(schedule.numWells(2), 1U);
    BOOST_CHECK_EQUAL(schedule.numWells(3), 3U);
}

BOOST_AUTO_TEST_CASE(TestCrossFlowHandling) {
    const auto& schedule = make_schedule( createDeckForTestingCrossFlow() );

    BOOST_CHECK_EQUAL(schedule.getWell("BAN", 0).getAllowCrossFlow(), false);
    BOOST_CHECK_EQUAL(schedule.getWell("ALLOW", 0).getAllowCrossFlow(), true);
    BOOST_CHECK_EQUAL(schedule.getWell("DEFAULT", 0).getAllowCrossFlow(), true);
    // we do not SHUT wells due to crossflow flag in the parser
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 0).getStatus());
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 1).getStatus());
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 2).getStatus());
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 3).getStatus());
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 4).getStatus());
    BOOST_CHECK(Well::Status::OPEN == schedule.getWell("BAN", 5).getStatus());

    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 0).getAllowCrossFlow() );
    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 1).getAllowCrossFlow() );
    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 2).getAllowCrossFlow() );
    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 3).getAllowCrossFlow() );
    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 4).getAllowCrossFlow() );
    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 5).getAllowCrossFlow() );

    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 0).getAutomaticShutIn() );
    BOOST_CHECK_EQUAL(true, schedule.getWell("BAN", 1).getAutomaticShutIn() );
    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 2).getAutomaticShutIn() );
    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 3).getAutomaticShutIn() );
    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 4).getAutomaticShutIn() );
    BOOST_CHECK_EQUAL(false, schedule.getWell("BAN", 5).getAutomaticShutIn() );
}

namespace {

    std::string createDeckWithWellsAndSkinFactorChanges()
    {
        return { R"(RUNSPEC
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
-- Well  I  J  K1  K2 Status SATNUM  CTF      Diam   Kh       Skin  D   Dir  PER (r0)
 'OP_1'  9  9   1   1 'OPEN' 1*      32.948   0.311  3047.839 1*    1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*      46.825   0.311  4332.346 1*    1*  'X'  22.123 /
 'OP_2'  8  8   1   3 'OPEN' 1*       1.168   0.311   107.872 1*    1*  'Y'  21.925 /
 'OP_2'  8  7   3   3 'OPEN' 1*      15.071   0.311  1391.859 1*    1*  'Y'  21.920 /
 'OP_2'  8  7   3   6 'OPEN' 1*       6.242   0.311   576.458 1*    1*  'Y'  21.915 /
 'OP_3'  7  7   1   1 'OPEN' 1*      27.412   0.311  2445.337 1*    1*  'Y'  18.521 /
 'OP_3'  7  7   2   2 'OPEN' 1*      55.195   0.311  4923.842 1*    1*  'Y'  18.524 /
/
DATES             -- 2
 10  JUL 2007 /
/

CSKIN
'OP_1'  9  9  1  1    1.5 /
'OP_2'  4*           -1.0 /
'OP_3'  2*    1  2   10.0 /
'OP_3'  7  7  1  1  -1.15 /
/

)" };
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsAndSkinFactorChanges)
{
    auto metricCF = [units = Opm::UnitSystem::newMETRIC()]
        (const double ctf)
    {
        return units.from_si(Opm::UnitSystem::measure::transmissibility, ctf);
    };

    const auto schedule = make_schedule(createDeckWithWellsAndSkinFactorChanges());

    // OP_1
    {
        const auto& cs = schedule.getWell("OP_1", 2).getConnections();
        BOOST_CHECK_CLOSE(cs.getFromIJK(8, 8, 0).skinFactor(), 1.5, 1e-10);

        // denom = 2*pi*Kh / CTF = 4.95609889
        //
        // New CTF = CTF * denom / (denom + S) = 32.948 * 4.95609889 / (4.95609889 + 1.5)
        const double expectCF = 25.292912792;
        BOOST_CHECK_CLOSE(metricCF(cs.getFromIJK(8, 8, 0).CF()), expectCF, 1.0e-5);
    }

    // OP_2
    {
        const auto& well = schedule.getWell("OP_2", 2);
        const auto& cs = well.getConnections();
        for (size_t i = 0; i < cs.size(); i++) {
            BOOST_CHECK_CLOSE(cs.get(i).skinFactor(), -1.0, 1e-10);
        }

        // denom = 2*pi*Kh / CTF = 4.947899898
        //
        // New CTF = CTF * denom / (denom + S) = 6.242 * 4.947899898 / (4.947899898 - 1.0)
        const double expectCF = 7.82309378689;
        BOOST_CHECK_CLOSE(metricCF(cs.getFromIJK(7, 6, 2).CF()), expectCF, 1.0e-5);
    }

    // OP_3
    {
        const auto& well = schedule.getWell("OP_3", 2);
        const auto& cs = well.getConnections();
        BOOST_CHECK_CLOSE(cs.getFromIJK(6, 6, 0).skinFactor(), - 1.15, 1e-10);
        BOOST_CHECK_CLOSE(cs.getFromIJK(6, 6, 1).skinFactor(),  10.0, 1e-10);

        // denom = 2*pi*Kh / CTF = 4.7794177751
        //
        // New CTF = CTF * denom / (denom + S) = 27.412 * 4.7794177751 / (4.7794177751 - 1.15)
        const double expectCF1 = 36.09763553531;
        BOOST_CHECK_CLOSE(metricCF(cs.getFromIJK(6, 6, 0).CF()), expectCF1, 1.0e-5);

        // denom = 2*pi*Kh / CTF = 4.7794879307
        //
        // New CTF = CTF * denom / (denom + S) = 55.195 * 4.7794879307 / (4.7794879307 + 10)
        const double expectCF2 = 17.84932181501;
        BOOST_CHECK_CLOSE(metricCF(cs.getFromIJK(6, 6, 1).CF()), expectCF2, 1.0e-5);
    }
}

namespace {

    std::string createDeckWithWPIMULTandWELPIandCSKIN()
    {
        return { R"(
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

DATES             -- 2
 10  JUL 2007 /
/
CSKIN
'OP_1'  9  9  1  1  1.5  /
/

DATES             -- 3
 10  AUG 2007 /
/
WPIMULT
OP_1  1.30 /
/
WPIMULT
OP_1  1.30 /
/

DATES             -- 4
 10  SEP 2007 /
/
CSKIN
'OP_1'  9  9  1  1  0.5  /
/

DATES             -- 5
 10  OCT 2007 /
/
WPIMULT
OP_1  1.30 /
/

DATES             -- 6
 10  NOV 2007 /
/
WELPI
OP_1 50 /
/

DATES             -- 7
 10  DEC 2007 /
/
CSKIN
'OP_1'  9  9  1  1  5.0  /
/

DATES             -- 8
 10  JAN 2008 /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

DATES             -- 9
 10  FEB 2008 /
/
CSKIN
'OP_1'  9  9  1  1  -1.0  /
/

)" };
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWPIMULTandWELPIandCSKIN)
{
    auto metricCF = [units = Opm::UnitSystem::newMETRIC()](const double ctf)
    {
        return units.from_si(Opm::UnitSystem::measure::transmissibility, ctf);
    };

    // Note: Schedule must be mutable for WELPI scaling.
    auto schedule = make_schedule(createDeckWithWPIMULTandWELPIandCSKIN());

    // Report step 2
    {
        const auto& cs = schedule.getWell("OP_1", 2).getConnections();
        const auto& conn = cs.getFromIJK(8, 8, 0);

        BOOST_CHECK_CLOSE(conn.skinFactor(), 1.5, 1e-10);

        // denom = 2*pi*Kh / CTF = 4.95609889
        //
        // New CTF = CTF * denom / (denom + S) = 32.948 * 4.95609889 / (4.95609889 + 1.5)
        const double expectCF = 25.292912792376;
        BOOST_CHECK_CLOSE(metricCF(conn.CF()), expectCF, 1.0e-5);
    }

    // Report step 3
    {
        const auto& cs_prev = schedule.getWell("OP_1", 2).getConnections();
        const auto& cs_curr = schedule.getWell("OP_1", 3).getConnections();
        BOOST_CHECK_CLOSE(cs_curr.getFromIJK(8, 8, 0).CF() / cs_prev.getFromIJK(8, 8, 0).CF(), 1.3, 1e-5);
    }

    // Report step 4
    {
        const auto& cs = schedule.getWell("OP_1", 4).getConnections();
        const auto& conn = cs.getFromIJK(8, 8, 0);

        BOOST_CHECK_CLOSE(conn.skinFactor(), 0.5, 1e-10);

        // CF from CSKIN multiplied by 1.3 from WPIMULT
        // denom = 2*pi*Kh / CTF = 4.95609889
        // mult = 1.3
        //
        // New CTF = mult * CTF * denom / (denom + S) = 1.3 * 32.948 * 4.95609889 / (4.95609889 + 0.5)
        const double expectCF = 38.90721454349;
        BOOST_CHECK_CLOSE(metricCF(conn.CF()), expectCF, 1e-5);
    }

    // Report step 5
    {
        const auto& cs_prev = schedule.getWell("OP_1", 4).getConnections();
        const auto& cs_curr = schedule.getWell("OP_1", 5).getConnections();
        BOOST_CHECK_CLOSE(cs_curr.getFromIJK(8, 8, 0).CF() / cs_prev.getFromIJK(8, 8, 0).CF(), 1.3, 1e-5);
    }

    // Report step 6
    {
        auto cvrtPI = [units = Opm::UnitSystem::newMETRIC()](const double pi)
        {
            return units.to_si(Opm::UnitSystem::measure::liquid_productivity_index, pi);
        };

        const auto init_pi = cvrtPI(100.0);
        schedule.applyWellProdIndexScaling("OP_1", 6, init_pi);

        const auto target_pi = schedule[6].target_wellpi.at("OP_1");
        BOOST_CHECK_CLOSE(target_pi, 50.0, 1.0e-5);
    }

    // Report step 7
    {
        const auto& cs = schedule.getWell("OP_1", 7).getConnections();
        const auto& conn = cs.getFromIJK(8, 8, 0);

        BOOST_CHECK_CLOSE(conn.skinFactor(), 5.0, 1e-10);

        // denom = 2*pi*Kh / CTF = 4.95609889
        // mult = 1.3 * 1.3 * (50 / 100) = 0.845
        //
        // New CTF = mult * CTF * denom / (denom + S) = 0.845 * 32.948 * 4.95609889 / (4.95609889 + 5)

        const auto expectCF = 13.8591478493;
        BOOST_CHECK_CLOSE(metricCF(conn.CF()), expectCF, 1.0e-5);
    }

    // Report step 8
    {
        const auto& cs = schedule.getWell("OP_1", 8).getConnections();
        const auto& conn = cs.getFromIJK(8, 8, 0);

        const auto expectCF = 32.948;
        BOOST_CHECK_CLOSE(metricCF(conn.CF()), expectCF, 1.0e-5);
    }

    // Report step 9
    {
        const auto& cs = schedule.getWell("OP_1", 9).getConnections();
        const auto& conn = cs.getFromIJK(8, 8, 0);

        BOOST_CHECK_CLOSE(conn.skinFactor(), -1.0, 1e-10);

        // CF from CSKIN with WPIMULT and WELLPI multiplier reset to 1.0
        //
        // denom = 2*pi*Kh / CTF = 4.95609889
        //
        // New CTF = CTF * denom / (denom + S) = 32.948 * 4.95609889 / (4.95609889 - 1)
        const auto expectCF = 41.276406579873;
        BOOST_CHECK_CLOSE(metricCF(conn.CF()), expectCF, 1.0e-5);
    }
}

namespace {

    std::string createDeckWithWellsAndConnectionDataWithWELOPEN()
    {
        return { R"(
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 /
 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 /
 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 /
 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 /
 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 /
/
DATES             -- 2,3
 10  JUL 2007 /
 10  AUG 2007 /
/
COMPDAT
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WELOPEN
 'OP_1' SHUT /
 '*'    OPEN 0 0 3 /
 'OP_2' SHUT 0 0 0 4 6 /
 'OP_3' SHUT 0 0 0 /
/
DATES             -- 4
 10  JUL 2008 /
/
WELOPEN
 'OP_1' OPEN /
 'OP_2' OPEN 0 0 0 4 6 /
 'OP_3' OPEN 0 0 0 /
/
DATES             -- 5
 10  OKT 2008 /
/
WELOPEN
 'OP_1' SHUT 0 0 0 0 0 /
/
)" };
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsAndConnectionDataWithWELOPEN) {
    const auto& schedule = make_schedule(createDeckWithWellsAndConnectionDataWithWELOPEN());
    {
        constexpr auto well_shut = Well::Status::SHUT;
        constexpr auto well_open = Well::Status::OPEN;

        BOOST_CHECK(well_shut == schedule.getWell("OP_1", 3).getStatus(  ));
        BOOST_CHECK(well_open == schedule.getWell("OP_1", 4).getStatus(  ));
        BOOST_CHECK(well_shut == schedule.getWell("OP_1", 5).getStatus(  ));
    }
    {
        constexpr auto comp_shut = Connection::State::SHUT;
        constexpr auto comp_open = Connection::State::OPEN;
        {
            const auto& well = schedule.getWell("OP_2", 3);
            const auto& cs = well.getConnections( );

            BOOST_CHECK_EQUAL( 7U, cs.size() );
            BOOST_CHECK_EQUAL( 4U, cs.num_open() );
            BOOST_CHECK(comp_shut == cs.getFromIJK( 7, 6, 2 ).state());
            BOOST_CHECK(comp_shut == cs.getFromIJK( 7, 6, 3 ).state());
            BOOST_CHECK(comp_shut == cs.getFromIJK( 7, 6, 4 ).state());
            BOOST_CHECK(comp_open == cs.getFromIJK( 7, 7, 2 ).state());
        }
        {
            const auto& well = schedule.getWell("OP_2", 4);
            const auto& cs2 = well.getConnections( );
            BOOST_CHECK(comp_open == cs2.getFromIJK( 7, 6, 2 ).state());
            BOOST_CHECK(comp_open == cs2.getFromIJK( 7, 6, 3 ).state());
            BOOST_CHECK(comp_open == cs2.getFromIJK( 7, 6, 4 ).state());
            BOOST_CHECK(comp_open == cs2.getFromIJK( 7, 7, 2 ).state());
        }
        {
            const auto& well = schedule.getWell("OP_3", 3);
            const auto& cs3 = well.getConnections(  );
            BOOST_CHECK(comp_shut == cs3.get( 0 ).state());
        }
        {
            const auto& well = schedule.getWell("OP_3", 4);
            const auto& cs4 = well.getConnections(  );
            BOOST_CHECK(comp_open == cs4.get( 0 ).state());
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWELOPEN_TryToOpenWellWithShutCompletionsDoNotOpenWell) {
    Opm::Parser parser;
    std::string input = R"(
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 2
 10  JUL 2008 /
/
WELOPEN
 'OP_1' OPEN /
/
DATES             -- 3
 10  OKT 2008 /
/
WELOPEN
 'OP_1' SHUT 0 0 0 0 0 /
/
DATES             -- 4
 10  NOV 2008 /
/
WELOPEN
 'OP_1' OPEN /
/
)";
    const auto& schedule = make_schedule(input);
    const auto& well2_3 = schedule.getWell("OP_1",3);
    const auto& well2_4 = schedule.getWell("OP_1",4);
    BOOST_CHECK(Well::Status::SHUT == well2_3.getStatus());
    BOOST_CHECK(Well::Status::SHUT == well2_4.getStatus());
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWELOPEN_CombineShutCompletionsAndAddNewCompletionsDoNotShutWell) {
  Opm::Parser parser;
  std::string input = R"(
START             -- 0
1 NOV 1979 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 1 DES 1979/
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 2
 10  JUL 2008 /
/
WELOPEN
 'OP_1' OPEN /
/
DATES             -- 3
 10  OKT 2008 /
/
WELOPEN
 'OP_1' SHUT 0 0 0 0 0 /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 4
 10  NOV 2008 /
/
WELOPEN
 'OP_1' SHUT 0 0 0 0 0 /
/
DATES             -- 5
 11  NOV 2008 /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 6
 12  NOV 2008 /
/
)";

  const auto& schedule = make_schedule(input);
  const auto& well_3 = schedule.getWell("OP_1", 3);
  const auto& well_4 = schedule.getWell("OP_1", 4);
  const auto& well_5 = schedule.getWell("OP_1", 5);
  // timestep 3. Close all completions with WELOPEN and immediately open new completions with COMPDAT.
  BOOST_CHECK(Well::Status::OPEN == well_3.getStatus());
  BOOST_CHECK( !schedule[3].wellgroup_events().hasEvent("OP_1", ScheduleEvents::WELL_STATUS_CHANGE));
  // timestep 4. Close all completions with WELOPEN. The well will be shut since no completions
  // are open.
  BOOST_CHECK(Well::Status::SHUT == well_4.getStatus());
  BOOST_CHECK( schedule[4].wellgroup_events().hasEvent("OP_1", ScheduleEvents::WELL_STATUS_CHANGE));
  // timestep 5. Open new completions. But keep the well shut,
  BOOST_CHECK(Well::Status::SHUT == well_5.getStatus());
}




BOOST_AUTO_TEST_CASE(createDeckWithWeltArg) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I1' 'I' 5 5 2522.5 'WATER' /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3   9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'I1'  8 8   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'I1'  8 8   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I1'  8 8   3   9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

WCONPROD
 'OP_1'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/

WCONINJE
  'I1' 'WATER'  'OPEN'  'RATE'  200  1*  450.0 /
/
DATES             -- 2
 20  JAN 2010 /
/
WELTARG
 OP_1     ORAT        1300 /
 OP_1     WRAT        1400 /
 OP_1     GRAT        1500.52 /
 OP_1     LRAT        1600.58 /
 OP_1     RESV        1801.05 /
 OP_1     BHP         1900 /
 OP_1     THP         2000 /
 OP_1     GUID        2300.14 /
 OP_1     LIFT        1234 /
/

DATES
 1 FEB 2010 /
/

WELTARG
I1  THP 100.0 /
/

WTMULT
OP_1 ORAT 2 /
OP_1 GRAT 3 /
OP_1 WRAT 4 /
I1 WRAT 2 /
I1 BHP 3 /
I1 THP 4 /
/

)";

    const auto& schedule = make_schedule(input);
    Opm::UnitSystem unitSystem = UnitSystem( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();
    double siFactorG = unitSystem.parse("GasSurfaceVolume/Time").getSIScaling();
    double siFactorP = unitSystem.parse("Pressure").getSIScaling();
    SummaryState st(TimeService::now(), 0.0);

    const auto& well_1 = schedule.getWell("OP_1", 1);
    const auto wpp_1 = well_1.getProductionProperties();
    BOOST_CHECK_EQUAL(wpp_1.WaterRate.get<double>(), 0);
    BOOST_CHECK (wpp_1.hasProductionControl( Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK (!wpp_1.hasProductionControl( Opm::Well::ProducerCMode::RESV) );



    const auto& well_2 = schedule.getWell("OP_1", 2);
    const auto wpp_2 = well_2.getProductionProperties();
    const auto prod_controls = wpp_2.controls(st, 0);

    BOOST_CHECK_CLOSE(prod_controls.oil_rate, 1300 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.water_rate, 1400 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.gas_rate, 1500.52 * siFactorG, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.liquid_rate, 1600.58 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.resv_rate, 1801.05 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.bhp_limit, 1900 * siFactorP, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls.thp_limit, 2000 * siFactorP, 1e-13);
    BOOST_CHECK_CLOSE(wpp_2.ALQValue.get<double>(), 1234, 1e-13);

    BOOST_CHECK (wpp_2.hasProductionControl( Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK (wpp_2.hasProductionControl( Opm::Well::ProducerCMode::RESV) );


    const auto& well_3 = schedule.getWell("OP_1", 3);
    const auto wpp_3 = well_3.getProductionProperties();
    const auto prod_controls3 = wpp_3.controls(st, 0);

    BOOST_CHECK_CLOSE(prod_controls3.oil_rate, 2 * 1300 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls3.water_rate, 4 * 1400 * siFactorL, 1e-13);
    BOOST_CHECK_CLOSE(prod_controls3.gas_rate, 3 * 1500.52 * siFactorG, 1e-13);


    const auto& inj_controls2 = schedule.getWell("I1", 2).getInjectionProperties().controls(unitSystem, st, 0);
    const auto& inj_controls3 = schedule.getWell("I1", 3).getInjectionProperties().controls(unitSystem, st, 0);

    BOOST_CHECK_EQUAL(inj_controls2.surface_rate * 2, inj_controls3.surface_rate);
    BOOST_CHECK_EQUAL(inj_controls2.bhp_limit * 3, inj_controls3.bhp_limit);
    BOOST_CHECK_EQUAL(inj_controls3.thp_limit, 4 * 100 * siFactorP);
}


BOOST_AUTO_TEST_CASE(createDeckWithWeltArg_UDA) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/

UDQ
   ASSIGN WUORAT 10 /
   ASSIGN WUWRAT 20 /
/


WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
 'OP_1'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/
DATES             -- 2
 20  JAN 2010 /
/
WELTARG
 OP_1     ORAT        WUORAT /
 OP_1     WRAT        WUWRAT /
/
)";

    const auto& schedule = make_schedule(input);
    SummaryState st(TimeService::now(), 0.0);
    Opm::UnitSystem unitSystem = UnitSystem( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();

    st.update_well_var("OP_1", "WUORAT", 10);
    st.update_well_var("OP_1", "WUWRAT", 20);

    const auto& well_1 = schedule.getWell("OP_1", 1);
    const auto wpp_1 = well_1.getProductionProperties();
    BOOST_CHECK_EQUAL(wpp_1.OilRate.get<double>(), 0);
    BOOST_CHECK_EQUAL(wpp_1.WaterRate.get<double>(), 0);
    BOOST_CHECK (wpp_1.hasProductionControl( Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK (!wpp_1.hasProductionControl( Opm::Well::ProducerCMode::RESV) );



    const auto& well_2 = schedule.getWell("OP_1", 2);
    const auto wpp_2 = well_2.getProductionProperties();
    BOOST_CHECK( wpp_2.OilRate.is<std::string>() );
    BOOST_CHECK_EQUAL( wpp_2.OilRate.get<std::string>(), "WUORAT" );
    BOOST_CHECK_EQUAL( wpp_2.WaterRate.get<std::string>(), "WUWRAT" );
    const auto prod_controls = wpp_2.controls(st, 0);

    BOOST_CHECK_EQUAL(prod_controls.oil_rate, 10 * siFactorL);
    BOOST_CHECK_EQUAL(prod_controls.water_rate, 20 * siFactorL);

    BOOST_CHECK (wpp_2.hasProductionControl( Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK (wpp_2.hasProductionControl( Opm::Well::ProducerCMode::WRAT) );
}

BOOST_AUTO_TEST_CASE(createDeckWithWeltArg_UDA_Exception) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/



WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
 'OP_1'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/
DATES             -- 2
 20  JAN 2010 /
/
WELTARG
 OP_1     ORAT        WUORAT /
 OP_1     WRAT        WUWRAT /
/
)";

    BOOST_CHECK_THROW(make_schedule(input), std::exception);
}

BOOST_AUTO_TEST_CASE(Weltarg_Empty_WList)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
 6 5 7 /

OIL
WATER
GAS

METRIC

START
 3 'JUL' 2025 /

--
WELLDIMS
--max.well  max.con/well  max.grup  max.w/grup  WLISTDYN
  10        10            30        30    6*      2      /

--
TABDIMS
--ntsfun     ntpvt  max.nssfun  max.nppvt  max.ntfip  max.nrpvt
  1          1      50          60         72         60 /

GRID

DXV
 6*123.4 /

DYV
 5*123.4 /

DZV
 7*12.34 /

DEPTHZ
 42*2000.0 /

EQUALS
  PORO 0.3 /
  PERMX 100.0 /
  PERMY 100.0 /
  PERMZ  10.0 /
  NTG  0.82 /
/

PROPS

SWOF
  0 0 1 0
  1 1 0 0 /

SGOF
  0 0 1 0
  1 1 0 0 /

PVTW
  1 2 3 4 5 /

PVDG
   1 1     0.001
 250 0.001 0.001 /

PVDO
   1 1     0.25
 250 0.99  0.25 /

SOLUTION

SWAT
  210*0.25 /

SGAS
  210*0.6 /

PRESSURE
  210*100 /

SCHEDULE

WELSPECS
  'P-1'   'TEST'  1  1  1*  'OIL'  2*  'STOP' /
/

COMPDAT
-- WELL    I   J  K1   K2            Sat.   CF   DIAM
   'P-1'   1   1   1	4    'OPEN'  1*     1*   0.25 /
/

WCONPROD
  'P-1' 'OPEN'  'ORAT'  123.4 /
/

WLIST
 '*EMPTY' NEW /
/

WELTARG
-- Resetting a target on an empty WLIST is a no-op.
 '*EMPTY' GRAT 13500 /
/

DATES
 10 JUL 2025 /
/
END
)");

    const auto es = EclipseState { deck };

    // This is the real test here.  We're supposed to be able to create a
    // Schedule object even when there is a WELTARG applied to an '*EMPTY'
    // WLIST.  The rest of the statements are just to ensure that there is
    // an actual assertion in this unit test.
    const auto schedule = Schedule { deck, es };

    const auto udq_default = 0.0;
    const auto st = SummaryState { TimeService::now(), udq_default };

    const auto controls = schedule.back().wells("P-1")
        .getProductionProperties()
        .controls(st, udq_default);

    BOOST_CHECK_CLOSE(controls.oil_rate, 123.4*sm3_per_day(), 1.0e-8);
}

BOOST_AUTO_TEST_CASE(createDeckWithWeltArgException) {
    std::string input = R"(
SCHEDULE
WELTARG
 OP_1     GRAT        1500.52 /
 OP_1     LRAT        /
 OP_1     RESV        1801.05 /
/;
)";

    BOOST_CHECK_THROW(make_schedule(input), Opm::OpmInputError);
}


BOOST_AUTO_TEST_CASE(createDeckWithWeltArgException2) {
    std::string input = R"(
SCHEDULE
WELTARG
 OP_1     LRAT        /
 OP_1     RESV        1801.05 /
/
)";
    BOOST_CHECK_THROW(make_schedule(input), Opm::OpmInputError);
}


BOOST_AUTO_TEST_CASE(createDeckWithWPIMULT) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
DATES             -- 2
 20  JAN 2010 /
/
WPIMULT
OP_1  1.30 /
/
DATES             -- 3
 20  JAN 2011 /
/
WPIMULT
OP_1  1.30 /
/
DATES             -- 4
 20  JAN 2012 /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
)";

    const auto& schedule = make_schedule(input);
    const auto& cs1 = schedule.getWell("OP_1", 1).getConnections();
    const auto& cs2 = schedule.getWell("OP_1", 2).getConnections();
    const auto& cs3 = schedule.getWell("OP_1", 3).getConnections();
    const auto& cs4 = schedule.getWell("OP_1", 4).getConnections();
    for(size_t i = 0; i < cs2.size(); i++)
        BOOST_CHECK_CLOSE(cs2.get( i ).CF() / cs1.get(i).CF(), 1.3, 1e-13);

    for(size_t i = 0; i < cs3.size(); i++ )
        BOOST_CHECK_CLOSE(cs3.get( i ).CF() / cs1.get(i).CF(), (1.3*1.3), 1e-13);

    for(size_t i = 0; i < cs4.size(); i++ )
        BOOST_CHECK_CLOSE(cs4.get( i ).CF(), cs1.get(i).CF(), 1e-13);

    auto sim_time1 = TimeStampUTC{ schedule.simTime(1) };
    BOOST_CHECK_EQUAL(sim_time1.day(), 10);
    BOOST_CHECK_EQUAL(sim_time1.month(), 10);
    BOOST_CHECK_EQUAL(sim_time1.year(), 2008);

    sim_time1 = schedule.simTime(3);
    BOOST_CHECK_EQUAL(sim_time1.day(), 20);
    BOOST_CHECK_EQUAL(sim_time1.month(), 1);
    BOOST_CHECK_EQUAL(sim_time1.year(), 2011);
}

BOOST_AUTO_TEST_CASE(createDeckWithMultipleWPIMULT) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
WELSPECS
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/
COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_2'     8   8   1   1 'OPEN'       1*       50         2*            2*        'X'  22.100 /
 'OP_2'     8   8   2   2 'OPEN'       1*       50        2*            2*        'X'  22.100 /
 'OP_2'     8   8   3   3 'OPEN'       1*       50        2*            2*        'X'  22.100 /
/
DATES             -- 0
 20  JAN 2009 /
/
WPIMULT
 'OP_1'  2.0  /
 'OP_2'  3.0 /
 'OP_1'  0.8   -1 -1 -1 /  -- all connections
 'OP_2'  7.0 /
/
DATES             -- 1
 20  JAN 2010 /
/
WPIMULT
 'OP_1'  0.5  /
/
DATES             -- 2
 20  JAN 2011 /
/

COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/

WPIMULT
 'OP_1'  2.0  /
 'OP_1'  0.8   0 0 0 /  -- all connections but not defaulted
/

DATES             -- 3
 20  JAN 2012 /
/

COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/

WPIMULT
 'OP_1'  2.0  /
 'OP_1'  0.8 /  -- all connections
/

DATES             -- 4
 20  JAN 2013 /
/

COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/

WPIMULT
 'OP_1'  2.0  /
 'OP_1'  0.8 /  -- all connections
 'OP_1'  0.50  2* 4 /
 'OP_1'  0.10  2* 4 /
/
DATES             -- 5
 20  JAN 2014 /
/
COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/

WPIMULT
 'OP_1'  2.0  /
 'OP_1'  0.10  2* 4 /
/
WPIMULT
  'OP_1'  0.8 /  -- all connections
  'OP_1'  0.50  2* 4 /
/
DATES             -- 6
 20  FEB 2014 /
/
COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_1'     9   9   1   1 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   2   2 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   3   3 'OPEN'       1*       100        2*            2*        'X'  22.100 /
 'OP_1'     9   9   4   4 'OPEN'       1*       100        2*            2*        'X'  22.100 /
/
COMPDAT
-- WELL     I   J  K1   K2            Sat.      CF        DIAM    KH    SKIN ND    DIR   Ro
 'OP_2'     8   8   1   1 'OPEN'       1*       50         2*            2*        'X'  22.100 /
 'OP_2'     8   8   2   2 'OPEN'       1*       50        2*            2*        'X'  22.100 /
 'OP_2'     8   8   3   3 'OPEN'       1*       50        2*            2*        'X'  22.100 /
/
WPIMULT
 'OP_1'  2.0  /
 'OP_2'  3.0 /
/
WPIMULT
 'OP_1'  0.8   -1 -1 -1 /  -- all connections
 'OP_2'  7.0 /
/
DATES             -- 7
 20  FEB 2014 /
/
END
)";

    const auto& schedule = make_schedule(input);
    const auto& cs0 = schedule.getWell("OP_1", 0).getConnections();
    const auto& cs1 = schedule.getWell("OP_1", 1).getConnections();
    const auto& cs2 = schedule.getWell("OP_1", 2).getConnections();
    const auto& cs3 = schedule.getWell("OP_1", 3).getConnections();
    const auto& cs4 = schedule.getWell("OP_1", 4).getConnections();
    const auto& cs5 = schedule.getWell("OP_1", 5).getConnections();
    const auto& cs6 = schedule.getWell("OP_1", 6).getConnections();
    const auto& cs7 = schedule.getWell("OP_1", 7).getConnections();
    const auto& cs0_2 = schedule.getWell("OP_2", 0).getConnections();
    const auto& cs1_2 = schedule.getWell("OP_2", 1).getConnections();
    const auto& cs2_2 = schedule.getWell("OP_2", 2).getConnections();
    const auto& cs7_2 = schedule.getWell("OP_2", 7).getConnections();

    for (size_t i = 0; i < cs1_2.size(); ++i ) {
        BOOST_CHECK_CLOSE(cs1_2.get(i).CF() / cs0_2.get(i).CF(), 7.0, 1.e-13);
        BOOST_CHECK_CLOSE(cs2_2.get(i).CF() / cs1_2.get(i).CF(), 1.0, 1.e-13);
        BOOST_CHECK_CLOSE(cs7_2.get(i).CF() / cs0_2.get(i).CF(), 7.0, 1.e-13);
    }
    for (size_t i = 0; i < cs1.size(); ++i ) {
        BOOST_CHECK_CLOSE(cs1.get(i).CF() / cs0.get(i).CF(), 0.8, 1.e-13);
        BOOST_CHECK_CLOSE(cs2.get(i).CF() / cs1.get(i).CF(), 0.5, 1.e-13);
        BOOST_CHECK_CLOSE(cs3.get(i).CF() / cs0.get(i).CF(), 1.6, 1.e-13);
        BOOST_CHECK_CLOSE(cs4.get(i).CF() / cs0.get(i).CF(), 0.8, 1.e-13);
        BOOST_CHECK_CLOSE(cs7.get(i).CF() / cs0.get(i).CF(), 0.8, 1.e-13);
    }

    for (size_t i = 0; i < 3; ++i) {
        BOOST_CHECK_CLOSE(cs5.get(i).CF() / cs0.get(i).CF(), 0.8, 1.e-13);
        BOOST_CHECK_CLOSE(cs6.get(i).CF() / cs0.get(i).CF(), 0.8, 1.e-13);
    }
    BOOST_CHECK_CLOSE(cs5.get(3).CF() / cs0.get(3).CF(), 0.04, 1.e-13);
    BOOST_CHECK_CLOSE(cs6.get(3).CF() / cs0.get(3).CF(), 0.04, 1.e-13);
}


BOOST_AUTO_TEST_CASE(WELSPECS_WGNAME_SPACE) {
        Opm::Parser parser;
        const std::string input = R"(
        START  -- 0
         10 'JAN' 2000 /
        RUNSPEC
        DIMENS
          10 10 10 /
        GRID
        DX
        1000*0.25 /
        DY
        1000*0.25 /
        DZ
        1000*0.25 /
        TOPS
        100*0.25 /
        SCHEDULE
        DATES             -- 1
         10  OKT 2008 /
        /
        WELSPECS
            ' PROD1' 'G1'  1 1 10 'OIL' /
            'PROD2' 'G2'  2 2 10 'OIL' /
            'PROD3' 'H1'  3 3 10 'OIL' /
        /
        GCONPROD
        'G1' 'ORAT' 1000 /
        /
        DATES             -- 2
         10  NOV 2008 /
        /
        GCONPROD
        'G*' 'ORAT' 2000 /
        /
        )";

        auto deck = parser.parseString(input);
        auto python = std::make_shared<Python>();
        EclipseGrid grid( deck );
        TableManager table ( deck );
        FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
        Runspec runspec (deck);
        ParseContext parseContext;
        ErrorGuard errors;

        parseContext.update(ParseContext::PARSE_WGNAME_SPACE, InputErrorAction::THROW_EXCEPTION);
        BOOST_CHECK_THROW( Opm::Schedule(deck, grid, fp, NumericalAquifers{}, runspec, parseContext, errors, python), Opm::OpmInputError);

        parseContext.update(ParseContext::PARSE_WGNAME_SPACE, InputErrorAction::IGNORE);
        BOOST_CHECK_NO_THROW( Opm::Schedule(deck, grid, fp, NumericalAquifers{}, runspec, parseContext, errors, python));
}

BOOST_AUTO_TEST_CASE(createDeckModifyMultipleGCONPROD) {
        const std::string input = R"(
        START  -- 0
         10 'JAN' 2000 /
        RUNSPEC
        DIMENS
          10 10 10 /
        GRID
        DX
        1000*0.25 /
        DY
        1000*0.25 /
        DZ
        1000*0.25 /
        TOPS
        100*0.25 /
        SCHEDULE
        DATES             -- 1
         10  OKT 2008 /
        /
        WELSPECS
            'PROD1' 'G1'  1 1 10 'OIL' /
            'PROD2' 'G2'  2 2 10 'OIL' /
            'PROD3' 'H1'  3 3 10 'OIL' /
        /
        GCONPROD
        'G1' 'ORAT' 1000 /
        /
        DATES             -- 2
         10  NOV 2008 /
        /
        GCONPROD
        'G*' 'ORAT' 2000 0 0 0 'NONE' 'YES' 148 'OIL'/
        /
        DATES             -- 3
         10  DEC 2008 /
        /
        GCONPROD
        'G*' 'ORAT' 2000 1000 0 0 'NONE' 'YES' 148 'OIL'/
        /
        DATES             -- 4
         10  JAN 2009 /
        /
        GCONPROD
        'G*' 'ORAT' 2000 1000 0 0 'RATE' 'YES' 148 'OIL'/
        /
        )";

        const auto& schedule = make_schedule(input);
        Opm::SummaryState st(TimeService::now(), 0.0);

        Opm::UnitSystem unitSystem = UnitSystem(UnitSystem::UnitType::UNIT_TYPE_METRIC);
        double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();

        {
            auto g = schedule.getGroup("G1", 1);
            BOOST_CHECK_CLOSE(g.productionControls(st).oil_target, 1000 * siFactorL, 1e-13);
            BOOST_CHECK(g.has_control(Group::ProductionCMode::ORAT));
            BOOST_CHECK(!g.has_control(Group::ProductionCMode::WRAT));
            BOOST_CHECK_EQUAL(g.productionControls(st).guide_rate, 0);
        }
        {
            auto g = schedule.getGroup("G1", 2);
            BOOST_CHECK_CLOSE(g.productionControls(st).oil_target, 2000 * siFactorL, 1e-13);
            BOOST_CHECK_EQUAL(g.productionControls(st).guide_rate, 148);
            BOOST_CHECK_EQUAL(true, g.productionControls(st).guide_rate_def == Group::GuideRateProdTarget::OIL);
        }
        {
            auto g = schedule.getGroup("G1", 3);
            BOOST_CHECK_CLOSE(g.productionControls(st).oil_target, 2000 * siFactorL, 1e-13);
            BOOST_CHECK(g.has_control(Group::ProductionCMode::ORAT));
            BOOST_CHECK(!g.has_control(Group::ProductionCMode::WRAT));
        }
        {
            auto g = schedule.getGroup("G1", 4);
            BOOST_CHECK_CLOSE(g.productionControls(st).oil_target, 2000 * siFactorL, 1e-13);
            BOOST_CHECK(g.has_control(Group::ProductionCMode::ORAT));
            BOOST_CHECK_CLOSE(g.productionControls(st).water_target, 1000 * siFactorL, 1e-13);
            BOOST_CHECK(g.has_control(Group::ProductionCMode::WRAT));
        }

        auto g2 = schedule.getGroup("G2", 2);
        BOOST_CHECK_CLOSE(g2.productionControls(st).oil_target, 2000 * siFactorL, 1e-13);

        auto gh = schedule.getGroup("H1", 1);


        BOOST_CHECK(  !schedule[1].wellgroup_events().hasEvent( "G2", ScheduleEvents::GROUP_PRODUCTION_UPDATE));
        BOOST_CHECK(   schedule[2].wellgroup_events().hasEvent( "G2", ScheduleEvents::GROUP_PRODUCTION_UPDATE));

}

BOOST_AUTO_TEST_CASE(createDeckWithDRSDT) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
DRSDT
0.0003
/
)";

    const auto& schedule = make_schedule(input);
    size_t currentStep = 1;
    const auto& ovap = schedule[currentStep].oilvap();

    BOOST_CHECK_EQUAL(true,   ovap.getOption(0));
    BOOST_CHECK(ovap.getType() == OilVaporizationProperties::OilVaporization::DRDT);

    BOOST_CHECK_EQUAL(true,   ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());
}

BOOST_AUTO_TEST_CASE(createDeckWithDRSDTCON) {
        std::string input = R"(
START             -- 0
19 JUN 2007 /
TABDIMS
 1* 2 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
DRSDTCON
/
/
DATES             -- 1
 15  OKT 2008 /
/
DRSDTCON
0.01 0.3 1e-7 /
/
)";
    const auto& schedule = make_schedule(input);
    size_t currentStep = 1;
    const auto& ovap = schedule[currentStep].oilvap();

    BOOST_CHECK_EQUAL(true,   ovap.getOption(0));
    BOOST_CHECK(ovap.getType() == OilVaporizationProperties::OilVaporization::DRSDTCON);

    BOOST_CHECK_EQUAL(true,   ovap.drsdtActive(0));
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive(0));
    BOOST_CHECK_EQUAL(true,   ovap.drsdtConvective(0));
    BOOST_CHECK_CLOSE(ovap.getMaxDRSDT(0), 0.04, 1e-9);
    BOOST_CHECK_CLOSE(ovap.getOmega(0), 3e-9, 1e-9);
    BOOST_CHECK_CLOSE(ovap.getPsi(0), 0.34, 1e-9);
    BOOST_CHECK_CLOSE(ovap.getMaxDRSDT(1), 0.04, 1e-9);
    BOOST_CHECK_CLOSE(ovap.getOmega(1), 3e-9, 1e-9);
    BOOST_CHECK_CLOSE(ovap.getPsi(1), 0.34, 1e-9);
    const auto& ovap2 = schedule[2].oilvap();
    BOOST_CHECK_CLOSE(ovap2.getMaxDRSDT(0), 0.01, 1e-9);
    BOOST_CHECK_CLOSE(ovap2.getOmega(0), 1e-7, 1e-9);
    BOOST_CHECK_CLOSE(ovap2.getPsi(0), 0.3, 1e-9);
    BOOST_CHECK_CLOSE(ovap2.getMaxDRSDT(1), 0.04, 1e-9);
    BOOST_CHECK_CLOSE(ovap2.getOmega(1), 3e-9, 1e-9);
    BOOST_CHECK_CLOSE(ovap2.getPsi(1), 0.34, 1e-9);

}

BOOST_AUTO_TEST_CASE(createDeckWithDRSDTR) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
TABDIMS
 1* 3 /

SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
DRSDTR
0 /
1 /
2 /
)";

    const auto& schedule = make_schedule(input);
    size_t currentStep = 1;
    const auto& ovap = schedule[currentStep].oilvap();
    auto unitSystem =  UnitSystem::newMETRIC();
    for (int i = 0; i < 3; ++i) {
        double value = unitSystem.to_si( UnitSystem::measure::gas_surface_rate, i );
        BOOST_CHECK_EQUAL(value, ovap.getMaxDRSDT(i));
        BOOST_CHECK_EQUAL(true,   ovap.getOption(i));
        BOOST_CHECK_EQUAL(true,   ovap.drsdtActive(i));
        BOOST_CHECK_EQUAL(false,   ovap.drvdtActive(i));
    }

    BOOST_CHECK_EQUAL(true,   ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());

    BOOST_CHECK(ovap.getType() == OilVaporizationProperties::OilVaporization::DRDT);

}


BOOST_AUTO_TEST_CASE(createDeckWithDRSDTthenDRVDT) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
DRSDT
0.0003
/
DATES             -- 2
 10  OKT 2009 /
/
DRVDT
0.100
/
DATES             -- 3
 10  OKT 2010 /
/
VAPPARS
2 0.100
/
)";

    const auto& schedule = make_schedule(input);

    const OilVaporizationProperties& ovap1 = schedule[1].oilvap();
    BOOST_CHECK(ovap1.getType() == OilVaporizationProperties::OilVaporization::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap1.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap1.drvdtActive());

    const OilVaporizationProperties& ovap2 = schedule[2].oilvap();
    //double value =  ovap2.getMaxDRVDT(0);
    //BOOST_CHECK_EQUAL(1.1574074074074074e-06, value);
    BOOST_CHECK(ovap2.getType() == OilVaporizationProperties::OilVaporization::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap2.drvdtActive());
    BOOST_CHECK_EQUAL(true,   ovap2.drsdtActive());

    const OilVaporizationProperties& ovap3 = schedule[3].oilvap();
    BOOST_CHECK(ovap3.getType() == OilVaporizationProperties::OilVaporization::VAPPARS);
    BOOST_CHECK_EQUAL(false,   ovap3.drvdtActive());
    BOOST_CHECK_EQUAL(false,   ovap3.drsdtActive());


}

BOOST_AUTO_TEST_CASE(createDeckWithVAPPARS) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
VAPPARS
2 0.100
/
)";

    const auto& schedule = make_schedule(input);
    const OilVaporizationProperties& ovap0 = schedule[0].oilvap();
    BOOST_CHECK(ovap0.getType() == OilVaporizationProperties::OilVaporization::UNDEF);
    size_t currentStep = 1;
    const OilVaporizationProperties& ovap = schedule[currentStep].oilvap();
    BOOST_CHECK(ovap.getType() == OilVaporizationProperties::OilVaporization::VAPPARS);
    double vap1 =  ovap.vap1();
    BOOST_CHECK_EQUAL(2, vap1);
    double vap2 =  ovap.vap2();
    BOOST_CHECK_EQUAL(0.100, vap2);
    BOOST_CHECK_EQUAL(false, ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false, ovap.drvdtActive());

}

BOOST_AUTO_TEST_CASE(createDeckWithVAPPARSInSOLUTION) {
    std::string input = R"(
SOLUTION
VAPPARS
2 0.100
/

START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
)";

    const auto& schedule = make_schedule(input);
    for (int i = 0; i < 2; ++i) {
        const OilVaporizationProperties& ovap = schedule[i].oilvap();
        BOOST_CHECK(ovap.getType() == OilVaporizationProperties::OilVaporization::VAPPARS);
        double vap1 =  ovap.vap1();
        BOOST_CHECK_EQUAL(2, vap1);
        double vap2 =  ovap.vap2();
        BOOST_CHECK_EQUAL(0.100, vap2);
        BOOST_CHECK_EQUAL(false,   ovap.drsdtActive());
        BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());
    }
}

BOOST_AUTO_TEST_CASE(changeBhpLimitInHistoryModeWithWeltarg) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P' 'OPEN' 'RESV' 6*  500 /
/
WCONINJH
 'I' 'WATER' 1* 100 250 /
/
WELTARG
   'P' 'BHP' 50 /
   'I' 'BHP' 600 /
/
DATES             -- 2
 15  OKT 2008 /
/
WCONHIST
   'P' 'OPEN' 'RESV' 6*  500/
/
WCONINJH
 'I' 'WATER' 1* 100 250 /
/
DATES             -- 3
 18  OKT 2008 /
/
WCONHIST
   'I' 'OPEN' 'RESV' 6*  /
/
DATES             -- 4
 20  OKT 2008 /
/
WCONINJH
 'I' 'WATER' 1* 100 250 /
/
)";

    const auto& sched = make_schedule(input);
    const auto st = ::Opm::SummaryState{ TimeService::now(), 0.0 };
    UnitSystem unit_system(UnitSystem::UnitType::UNIT_TYPE_METRIC);

    // The BHP limit should not be effected by WCONHIST
    {
        const auto& c1 = sched.getWell("P",1).getProductionProperties().controls(st, 0);
        const auto& c2 = sched.getWell("P",2).getProductionProperties().controls(st, 0);
        BOOST_CHECK_EQUAL(c1.bhp_limit, 50 * 1e5); // 1
        BOOST_CHECK_EQUAL(c2.bhp_limit, 50 * 1e5); // 2
    }
    {
        const auto& c1 = sched.getWell("I",1).getInjectionProperties().controls(unit_system, st, 0);
        const auto& c2 = sched.getWell("I",2).getInjectionProperties().controls(unit_system, st, 0);
        BOOST_CHECK_EQUAL(c1.bhp_limit, 600 * 1e5); // 1
        BOOST_CHECK_EQUAL(c2.bhp_limit, 600 * 1e5); // 2
    }
    BOOST_CHECK_EQUAL(sched.getWell("I", 2).getInjectionProperties().hasInjectionControl(Opm::Well::InjectorCMode::BHP), true);

    // The well is producer for timestep 3 and the injection properties BHPTarget should be set to zero.
    BOOST_CHECK(sched.getWell("I", 3).isProducer());
    BOOST_CHECK_EQUAL(sched.getWell("I", 3).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::BHP), true );
    BOOST_CHECK_EQUAL(sched.getWell("I", 4).getInjectionProperties().hasInjectionControl(Opm::Well::InjectorCMode::BHP), true );
    {
        const auto& c3 = sched.getWell("I",3).getInjectionProperties().controls(unit_system, st, 0);
        const auto& c4 = sched.getWell("I",4).getInjectionProperties().controls(unit_system, st, 0);
        BOOST_CHECK_EQUAL(c3.bhp_limit, 0); // 1
        BOOST_CHECK_EQUAL(c4.bhp_limit, 6891.2 * 1e5); // 2
    }
}

BOOST_AUTO_TEST_CASE(changeModeWithWHISTCTL) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 2
 15  OKT 2008 /
/
WHISTCTL
 RESV /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 3
 18  OKT 2008 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 4
 20  OKT 2008 /
/
WHISTCTL
 LRAT /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 5
 25  OKT 2008 /
/
WHISTCTL
 NONE /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
)";

    const auto& schedule = make_schedule(input);

    //Start
    BOOST_CHECK_THROW(schedule.getWell("P1", 0), std::exception);
    BOOST_CHECK_THROW(schedule.getWell("P2", 0), std::exception);

    //10  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);
    BOOST_CHECK(schedule.getWell("P2", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);

    //15  OKT 2008
    {
        const auto& props1 = schedule.getWell("P1", 2).getProductionProperties();
        const auto& props2 = schedule.getWell("P2", 2).getProductionProperties();

        BOOST_CHECK(props1.controlMode == Opm::Well::ProducerCMode::RESV);
        BOOST_CHECK(props2.controlMode == Opm::Well::ProducerCMode::RESV);
        // under history mode, a producing well should have only one rate target/limit or have no rate target/limit.
        // the rate target/limit from previous report step should not be kept.
        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    }

    //18  OKT 2008
    {
        const auto& props1 = schedule.getWell("P1", 3).getProductionProperties();
        const auto& props2 = schedule.getWell("P2", 3).getProductionProperties();

        BOOST_CHECK(props1.controlMode == Opm::Well::ProducerCMode::RESV);
        BOOST_CHECK(props2.controlMode == Opm::Well::ProducerCMode::RESV);

        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    }

    // 20 OKT 2008
    {
        const auto& props1 = schedule.getWell("P1", 4).getProductionProperties();
        const auto& props2 = schedule.getWell("P2", 4).getProductionProperties();

        BOOST_CHECK(props1.controlMode == Opm::Well::ProducerCMode::LRAT);
        BOOST_CHECK(props2.controlMode == Opm::Well::ProducerCMode::LRAT);

        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::RESV) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::RESV) );
    }

    // 25 OKT 2008
    {
        const auto& props1 = schedule.getWell("P1", 5).getProductionProperties();
        const auto& props2 = schedule.getWell("P2", 5).getProductionProperties();

        BOOST_CHECK(props1.controlMode == Opm::Well::ProducerCMode::ORAT);
        BOOST_CHECK(props2.controlMode == Opm::Well::ProducerCMode::ORAT);

        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::LRAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::LRAT) );
        BOOST_CHECK( !props1.hasProductionControl(Opm::Well::ProducerCMode::RESV) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::Well::ProducerCMode::RESV) );
    }

    BOOST_CHECK_THROW(schedule.getWell(10,0), std::exception);
    std::vector<std::string> well_names = {"P1", "P2", "I"};
    BOOST_CHECK_EQUAL( well_names.size(), schedule[1].well_order().size());

    for (std::size_t well_index = 0; well_index < well_names.size(); well_index++) {
        const auto& well = schedule.getWell(well_index, 1);
        BOOST_CHECK_EQUAL( well.name(), well_names[well_index]);
    }
}

BOOST_AUTO_TEST_CASE(fromWCONHISTtoWCONPROD) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 2
 15  OKT 2008 /
/
WCONPROD
 'P1' 'OPEN' 'GRAT' 1*    200.0 300.0 /
 'P2' 'OPEN' 'WRAT' 1*    100.0 300.0 /
/
DATES             -- 3
 18  OKT 2008 /
/
)";

    const auto& schedule = make_schedule(input);
    //Start
    BOOST_CHECK_THROW(schedule.getWell("P1", 0), std::exception);
    BOOST_CHECK_THROW(schedule.getWell("P2", 0), std::exception);

    //10  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);
    BOOST_CHECK(schedule.getWell("P2", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);

    //15  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 2).getProductionProperties().controlMode == Opm::Well::ProducerCMode::GRAT);
    BOOST_CHECK(schedule.getWell("P1", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::WRAT) );
    BOOST_CHECK(schedule.getWell("P2", 2).getProductionProperties().controlMode == Opm::Well::ProducerCMode::WRAT);
    BOOST_CHECK(schedule.getWell("P2", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::GRAT) );
    // the previous control limits/targets should not stay
    BOOST_CHECK( !schedule.getWell("P1", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK( !schedule.getWell("P2", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
}

BOOST_AUTO_TEST_CASE(WHISTCTL_NEW_WELL) {
    Opm::Parser parser;
    std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WHISTCTL
 GRAT/
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 2
 15  OKT 2008 /
/
WHISTCTL
 RESV /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 3
 18  OKT 2008 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 4
 20  OKT 2008 /
/
WHISTCTL
 LRAT /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 5
 25  OKT 2008 /
/
WHISTCTL
 NONE /
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
)";

    const auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    //10  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::GRAT);
    BOOST_CHECK(schedule.getWell("P2", 1).getProductionProperties().controlMode == Opm::Well::ProducerCMode::GRAT);

    //15  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 2).getProductionProperties().controlMode == Opm::Well::ProducerCMode::RESV);
    BOOST_CHECK(schedule.getWell("P2", 2).getProductionProperties().controlMode == Opm::Well::ProducerCMode::RESV);
    // under history mode, a producing well should have only one rate target/limit or have no rate target/limit.
    // the rate target/limit from previous report step should not be kept.
    BOOST_CHECK( !schedule.getWell("P1", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK( !schedule.getWell("P2", 2).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );

    //18  OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 3).getProductionProperties().controlMode == Opm::Well::ProducerCMode::RESV);
    BOOST_CHECK(schedule.getWell("P2", 3).getProductionProperties().controlMode == Opm::Well::ProducerCMode::RESV);
    BOOST_CHECK( !schedule.getWell("P1", 3).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK( !schedule.getWell("P2", 3).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );

    // 20 OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 4).getProductionProperties().controlMode == Opm::Well::ProducerCMode::LRAT);
    BOOST_CHECK(schedule.getWell("P2", 4).getProductionProperties().controlMode == Opm::Well::ProducerCMode::LRAT);
    BOOST_CHECK( !schedule.getWell("P1", 4).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK( !schedule.getWell("P2", 4).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::ORAT) );
    BOOST_CHECK( !schedule.getWell("P1", 4).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::RESV) );
    BOOST_CHECK( !schedule.getWell("P2", 4).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::RESV) );

    // 25 OKT 2008
    BOOST_CHECK(schedule.getWell("P1", 5).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);
    BOOST_CHECK(schedule.getWell("P2", 5).getProductionProperties().controlMode == Opm::Well::ProducerCMode::ORAT);
    BOOST_CHECK( !schedule.getWell("P1", 5).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::RESV) );
    BOOST_CHECK( !schedule.getWell("P2", 5).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::RESV) );
    BOOST_CHECK( !schedule.getWell("P1", 5).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::LRAT) );
    BOOST_CHECK( !schedule.getWell("P2", 5).getProductionProperties().hasProductionControl(Opm::Well::ProducerCMode::LRAT) );
}



BOOST_AUTO_TEST_CASE(unsupportedOptionWHISTCTL) {
    const std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P1' 'OPEN' 'ORAT' 5*/
 'P2' 'OPEN' 'ORAT' 5*/
/
DATES             -- 2
 15  OKT 2008 /
/
WHISTCTL
 * YES /
)";

    const auto deck = Parser{}.parseString(input);
    auto python = std::make_shared<Python>();
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    BOOST_CHECK_THROW(Schedule(deck, grid, fp, NumericalAquifers{}, runspec, python), OpmInputError);
}

BOOST_AUTO_TEST_CASE(move_HEAD_I_location) {
    const std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
            DATES             -- 2
                15  OKT 2008 /
            /

            WELSPECS
                'W1' 'G1'  4 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
    )";

    const auto deck = Parser{}.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK_EQUAL(2, schedule.getWell("W1", 1).getHeadI());
    BOOST_CHECK_EQUAL(3, schedule.getWell("W1", 2).getHeadI());
}

BOOST_AUTO_TEST_CASE(change_ref_depth) {
    const std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
            DATES             -- 2
                15  OKT 2008 /
            /

            WELSPECS
                'W1' 'G1'  3 3 12.0 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
    )";

    const auto deck = Parser{}.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK_CLOSE(2873.94, schedule.getWell("W1", 1).getRefDepth(), 1e-5);
    BOOST_CHECK_EQUAL(12.0, schedule.getWell("W1", 2).getRefDepth());
}


BOOST_AUTO_TEST_CASE(WTEMP_well_template) {
    const std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'OIL' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W3' 'G2'  6 6 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            WTEMP
                'W*' 40.0 /
            /

            DATES             -- 2
                15  OKT 2008 /
            /

            WCONINJE
            'W2' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            'W3' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            /

    )";

    const auto deck = Parser{}.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK_THROW(schedule.getWell("W1", 1).inj_temperature(), std::logic_error);
    BOOST_CHECK_THROW(schedule.getWell("W1", 2).inj_temperature(), std::logic_error);

    BOOST_CHECK_THROW(schedule.getWell("W2", 1).inj_temperature(), std::logic_error);
    BOOST_CHECK_CLOSE(313.15, schedule.getWell("W2", 2).inj_temperature(), 1e-5);

    BOOST_CHECK_THROW(schedule.getWell("W3", 1).inj_temperature(), std::logic_error);
    BOOST_CHECK_CLOSE(313.15, schedule.getWell("W3", 2).inj_temperature(), 1e-5);
}


BOOST_AUTO_TEST_CASE(WTEMPINJ_well_template) {
    const std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'OIL' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W3' 'G2'  6 6 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            WCONINJE
            'W2' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            'W3' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            /

            DATES             -- 2
                15  OKT 2008 /
            /

            WINJTEMP
                'W*' 1* 40.0 1* /
            /
    )";

        const auto deck = Parser().parseString(input);
        EclipseGrid grid(10,10,10);
        const TableManager table ( deck );
        const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
        const Runspec runspec (deck);
        const Schedule schedule {
            deck, grid, fp, NumericalAquifers{},
            runspec, std::make_shared<Python>()
        };

        BOOST_CHECK_THROW(schedule.getWell("W1", 1).inj_temperature(), std::logic_error);
        BOOST_CHECK_THROW(schedule.getWell("W2", 1).inj_temperature(), std::logic_error);
        BOOST_CHECK_THROW(schedule.getWell("W3", 1).inj_temperature(), std::logic_error);

        BOOST_CHECK(schedule.getWell("W1", 2).hasInjTemperature());
        BOOST_CHECK_THROW(schedule.getWell("W1", 2).inj_temperature(), std::logic_error);
        BOOST_CHECK(schedule.getWell("W2", 2).hasInjTemperature());
        BOOST_CHECK_CLOSE(313.15, schedule.getWell("W2", 2).inj_temperature(), 1e-5);
        BOOST_CHECK(schedule.getWell("W3", 2).hasInjTemperature());
        BOOST_CHECK_CLOSE(313.15, schedule.getWell("W3", 2).inj_temperature(), 1e-5);
}

BOOST_AUTO_TEST_CASE( COMPDAT_sets_automatic_complnum ) {
    const auto deck = Parser{}.parseString(R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
  1000*0.3 /
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
SCHEDULE
DATES             -- 1
    10  OKT 2008 /
/
WELSPECS
    'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
    'W1' 0 0 1 1 'SHUT' 1*    / -- regular completion (1)
    'W1' 0 0 2 2 'SHUT' 1*    / -- regular completion (2)
    'W1' 0 0 3 4 'SHUT' 1*    / -- two completions in one record (3, 4)
/

DATES             -- 2
    11  OKT 2008 /
/

COMPDAT
    'W1' 0 0 1 1 'SHUT' 1*    / -- respecify, essentially ignore (1)
/
END
)");

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    const auto& cs1 = schedule.getWell( "W1", 1 ).getConnections(  );
    BOOST_CHECK_EQUAL( 1, cs1.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs1.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs1.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs1.get( 3 ).complnum() );

    const auto& cs2 = schedule.getWell( "W1", 2 ).getConnections(  );
    BOOST_CHECK_EQUAL( 1, cs2.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs2.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs2.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs2.get( 3 ).complnum() );
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_wells ) {
    const auto deck = Parser{}.parseString(R"(
START             -- 0
19 JUN 2007 /
GRID
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE
DATES             -- 1
    10  OKT 2008 /
/
WELSPECS
    'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
    'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
    'W1' 0 0 1 1 'SHUT' 1*    / -- regular completion (1)
    'W1' 0 0 2 2 'SHUT' 1*    / -- regular completion (2)
    'W1' 0 0 3 4 'SHUT' 1*    / -- two completions in one record (3, 4)
    'W2' 0 0 3 3 'SHUT' 1*    / -- regular completion (1)
    'W2' 0 0 1 3 'SHUT' 1*    / -- two completions (one exist already) (2, 3)
    'W*' 0 0 3 5 'SHUT' 1*    / -- two completions, two wells (includes existing
                                -- and adding for both wells)
/
)");

    EclipseGrid grid(10, 10, 10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    {
        const auto& w1cs = schedule.getWell( "W1", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w1cs.get( 0 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w1cs.get( 1 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w1cs.get( 2 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w1cs.get( 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w1cs.get( 4 ).complnum() );

        const auto& w2cs = schedule.getWell( "W2", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w2cs.getFromIJK( 4, 4, 2 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w2cs.getFromIJK( 4, 4, 0 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w2cs.getFromIJK( 4, 4, 1 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w2cs.getFromIJK( 4, 4, 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w2cs.getFromIJK( 4, 4, 4 ).complnum() );
    }

    {
        const auto& w1cs = schedule.getWell( "W1", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w1cs.get( 0 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w1cs.get( 1 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w1cs.get( 2 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w1cs.get( 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w1cs.get( 4 ).complnum() );

        const auto& w2cs = schedule.getWell( "W2", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w2cs.getFromIJK( 4, 4, 2 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w2cs.getFromIJK( 4, 4, 0 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w2cs.getFromIJK( 4, 4, 1 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w2cs.getFromIJK( 4, 4, 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w2cs.getFromIJK( 4, 4, 4 ).complnum() );

        BOOST_CHECK_THROW( w2cs.get( 5 ).complnum(), std::out_of_range );
    }
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_records_same_completion ) {
    const auto deck = Parser{}.parseString(R"(
START             -- 0
19 JUN 2007 /
GRID
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /
SCHEDULE
DATES             -- 1
    10  OKT 2008 /
/
WELSPECS
    'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
    'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
    'W1' 0 0 1 2 'SHUT' 1*    / -- multiple completion (1, 2)
    'W1' 0 0 2 2 'SHUT' 1*    / -- updated completion (2)
    'W1' 0 0 3 3 'SHUT' 1*    / -- regular completion (3)
/
)");

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    const auto& cs = schedule.getWell( "W1", 1 ).getConnections();
    BOOST_CHECK_EQUAL( 3U, cs.size() );
    BOOST_CHECK_EQUAL( 1, cs.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs.get( 2 ).complnum() );
}


BOOST_AUTO_TEST_CASE( complump_less_than_1 ) {
    const std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            GRID
            PERMX
              1000*0.10/
            COPY
              PERMX PERMY /
              PERMX PERMZ /
            /
            SCHEDULE

            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            COMPDAT
                'W1' 0 0 1 2 'SHUT' 1*    /
            /

            COMPLUMP
                'W1' 0 0 0 0 0 /
            /
    )";

    const auto deck = Parser().parseString( input);
    auto python = std::make_shared<Python>();
    EclipseGrid grid( 10, 10, 10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);

    BOOST_CHECK_THROW( Schedule(deck, grid, fp, NumericalAquifers{}, runspec, python), std::exception );
}

BOOST_AUTO_TEST_CASE( complump ) {
    const auto deck = Parser{}.parseString(R"(
START             -- 0
19 JUN 2007 /
GRID
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE

WELSPECS
    'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
    'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
    'W1' 0 0 1 2 'SHUT' 1*    /    Global Index = 23, 123, 223, 323, 423, 523
    'W1' 0 0 2 3 'SHUT' 1*    /
    'W1' 0 0 4 6 'SHUT' 1*    /
    'W2' 0 0 3 4 'SHUT' 1*    /
    'W2' 0 0 1 4 'SHUT' 1*    /
/

COMPLUMP
    -- name I J K1 K2 C
    -- where C is the completion number of this lump
    'W1' 0 0 1 3 1 /
/

DATES             -- 1
 10  OKT 2008 /
/

WELOPEN
    'W1' 'OPEN' 0 0 0 1 1 /
/
)");

    constexpr auto open = Connection::State::OPEN;
    constexpr auto shut = Connection::State::SHUT;

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    const auto& sc0 = schedule.getWell("W1", 0).getConnections();
    /* complnum should be modified by COMPLNUM */
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 0 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 1 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 2 ).complnum() );

    BOOST_CHECK( shut == sc0.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK( shut == sc0.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK( shut == sc0.getFromIJK( 2, 2, 2 ).state() );

    const auto& sc1  = schedule.getWell("W1", 1).getConnections();
    BOOST_CHECK( open == sc1.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK( open == sc1.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK( open == sc1.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK( shut == sc1.getFromIJK( 2, 2, 3 ).state() );

    const auto& completions = schedule.getWell("W1", 1).getCompletions();
    BOOST_CHECK_EQUAL(completions.size(), 4U);

    const auto& c1 = completions.at(1);
    BOOST_CHECK_EQUAL(c1.size(), 3U);

    for (const auto& pair : completions) {
        if (pair.first == 1)
            BOOST_CHECK(pair.second.size() > 1);
        else
            BOOST_CHECK_EQUAL(pair.second.size(), 1U);
    }

    const auto& w0 = schedule.getWell("W1", 0);
    BOOST_CHECK(w0.hasCompletion(1));
    BOOST_CHECK(!w0.hasCompletion(2));

    const auto& conn0 = w0.getConnections(100);
    BOOST_CHECK(conn0.empty());

    const auto& conn_all = w0.getConnections();
    const auto& conn1 = w0.getConnections(1);
    BOOST_CHECK_EQUAL( conn1.size(), 3);
    for (const auto& conn : conn_all) {
        if (conn.complnum() == 1) {
            auto conn_iter = std::find_if(conn1.begin(), conn1.end(), [&conn](const Connection * cptr)
                                                                      {
                                                                          return *cptr == conn;
                                                                      });
            BOOST_CHECK( conn_iter != conn1.end() );
        }
    }

    const auto& all_connections = w0.getConnections();
    auto global_index = grid.getGlobalIndex(2,2,0);
    BOOST_CHECK( all_connections.hasGlobalIndex(global_index));
    const auto& conn_g = all_connections.getFromGlobalIndex(global_index);
    const auto& conn_ijk = all_connections.getFromIJK(2,2,0);
    BOOST_CHECK(conn_g == conn_ijk);

    BOOST_CHECK_THROW( all_connections.getFromGlobalIndex(100000), std::exception );
}

BOOST_AUTO_TEST_CASE( COMPLUMP_specific_coordinates ) {
    const auto deck = Parser{}.parseString(R"(
START             -- 0
19 JUN 2007 /
GRID
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE

WELSPECS
    'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT                         -- completion number
    'W1' 1 1 1 1 'SHUT' 1*    / -- 1
    'W1' 1 1 2 2 'SHUT' 1*    / -- 2
    'W1' 0 0 1 2 'SHUT' 1*    / -- 3, 4
    'W1' 0 0 2 3 'SHUT' 1*    / -- 5
    'W1' 2 2 1 1 'SHUT' 1*    / -- 6
    'W1' 2 2 4 6 'SHUT' 1*    / -- 7, 8, 9
/

DATES             -- 1
    10  OKT 2008 /
/


DATES             -- 2
    15  OKT 2008 /
/

COMPLUMP
    -- name I J K1 K2 C
    -- where C is the completion number of this lump
    'W1' 0 0 2 3 2 / -- all with k = [2 <= k <= 3] -> {2, 4, 5}
    'W1' 2 2 1 5 7 / -- fix'd i,j, k = [1 <= k <= 5] -> {6, 7, 8}
/

WELOPEN
    'W1' OPEN 0 0 0 2 2 / -- open the new 2 {2, 4, 5}
    'W1' OPEN 0 0 0 5 7 / -- open 5..7 {5, 6, 7, 8}
/
)");

    constexpr auto open = Connection::State::OPEN;
    constexpr auto shut = Connection::State::SHUT;

    EclipseGrid grid(10, 10, 10);
    const TableManager table (deck );
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    const auto& cs1 = schedule.getWell("W1", 1).getConnections();
    const auto& cs2 = schedule.getWell("W1", 2).getConnections();
    BOOST_CHECK_EQUAL( 9U, cs1.size() );
    BOOST_CHECK( shut == cs1.getFromIJK( 0, 0, 1 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 1, 1, 0 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 1, 1, 3 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 1, 1, 4 ).state() );
    BOOST_CHECK( shut == cs1.getFromIJK( 1, 1, 5 ).state() );

    BOOST_CHECK( open == cs2.getFromIJK( 0, 0, 1 ).state() );
    BOOST_CHECK( shut == cs2.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK( open == cs2.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK( open == cs2.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK( open == cs2.getFromIJK( 1, 1, 0 ).state() );
    BOOST_CHECK( open == cs2.getFromIJK( 1, 1, 3 ).state() );
    BOOST_CHECK( open == cs2.getFromIJK( 1, 1, 4 ).state() );
    BOOST_CHECK( shut == cs2.getFromIJK( 1, 1, 5 ).state() );
}

BOOST_AUTO_TEST_CASE(TestCompletionStateEnum2String) {
    BOOST_CHECK( "AUTO" == Connection::State2String(Connection::State::AUTO));
    BOOST_CHECK( "OPEN" == Connection::State2String(Connection::State::OPEN));
    BOOST_CHECK( "SHUT" == Connection::State2String(Connection::State::SHUT));
}


BOOST_AUTO_TEST_CASE(TestCompletionStateEnumFromString) {
    BOOST_CHECK_THROW( Connection::StateFromString("XXX") , std::invalid_argument );
    BOOST_CHECK( Connection::State::AUTO == Connection::StateFromString("AUTO"));
    BOOST_CHECK( Connection::State::SHUT == Connection::StateFromString("SHUT"));
    BOOST_CHECK( Connection::State::SHUT == Connection::StateFromString("STOP"));
    BOOST_CHECK( Connection::State::OPEN == Connection::StateFromString("OPEN"));
}



BOOST_AUTO_TEST_CASE(TestCompletionStateEnumLoop) {
    BOOST_CHECK( Connection::State::AUTO == Connection::StateFromString( Connection::State2String( Connection::State::AUTO ) ));
    BOOST_CHECK( Connection::State::SHUT == Connection::StateFromString( Connection::State2String( Connection::State::SHUT ) ));
    BOOST_CHECK( Connection::State::OPEN == Connection::StateFromString( Connection::State2String( Connection::State::OPEN ) ));

    BOOST_CHECK( "AUTO" == Connection::State2String(Connection::StateFromString(  "AUTO" ) ));
    BOOST_CHECK( "OPEN" == Connection::State2String(Connection::StateFromString(  "OPEN" ) ));
    BOOST_CHECK( "SHUT" == Connection::State2String(Connection::StateFromString(  "SHUT" ) ));
}


/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestCompletionDirectionEnum2String)
{
    BOOST_CHECK("X" == Connection::Direction2String(Connection::Direction::X));
    BOOST_CHECK("Y" == Connection::Direction2String(Connection::Direction::Y));
    BOOST_CHECK("Z" == Connection::Direction2String(Connection::Direction::Z));
}

BOOST_AUTO_TEST_CASE(TestCompletionDirectionEnumFromString)
{
    BOOST_CHECK_THROW(Connection::DirectionFromString("XXX"), std::invalid_argument);

    BOOST_CHECK(Connection::Direction::X == Connection::DirectionFromString("X"));
    BOOST_CHECK(Connection::Direction::Y == Connection::DirectionFromString("Y"));
    BOOST_CHECK(Connection::Direction::Z == Connection::DirectionFromString("Z"));
}

BOOST_AUTO_TEST_CASE(TestCompletionConnectionDirectionLoop)
{
    BOOST_CHECK(Connection::Direction::X == Connection::DirectionFromString(Connection::Direction2String(Connection::Direction::X)));
    BOOST_CHECK(Connection::Direction::Y == Connection::DirectionFromString(Connection::Direction2String(Connection::Direction::Y)));
    BOOST_CHECK(Connection::Direction::Z == Connection::DirectionFromString(Connection::Direction2String(Connection::Direction::Z)));

    BOOST_CHECK("X" == Connection::Direction2String(Connection::DirectionFromString("X")));
    BOOST_CHECK("Y" == Connection::Direction2String(Connection::DirectionFromString("Y")));
    BOOST_CHECK("Z" == Connection::Direction2String(Connection::DirectionFromString("Z")));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE" , Group::InjectionCMode2String(Group::InjectionCMode::NONE));
    BOOST_CHECK_EQUAL( "RATE" , Group::InjectionCMode2String(Group::InjectionCMode::RATE));
    BOOST_CHECK_EQUAL( "RESV" , Group::InjectionCMode2String(Group::InjectionCMode::RESV));
    BOOST_CHECK_EQUAL( "REIN" , Group::InjectionCMode2String(Group::InjectionCMode::REIN));
    BOOST_CHECK_EQUAL( "VREP" , Group::InjectionCMode2String(Group::InjectionCMode::VREP));
    BOOST_CHECK_EQUAL( "FLD"  , Group::InjectionCMode2String(Group::InjectionCMode::FLD));
}


BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnumFromString) {
    BOOST_CHECK_THROW( Group::InjectionCModeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK( Group::InjectionCMode::NONE == Group::InjectionCModeFromString("NONE"));
    BOOST_CHECK( Group::InjectionCMode::RATE == Group::InjectionCModeFromString("RATE"));
    BOOST_CHECK( Group::InjectionCMode::RESV == Group::InjectionCModeFromString("RESV"));
    BOOST_CHECK( Group::InjectionCMode::REIN == Group::InjectionCModeFromString("REIN"));
    BOOST_CHECK( Group::InjectionCMode::VREP == Group::InjectionCModeFromString("VREP"));
    BOOST_CHECK( Group::InjectionCMode::FLD  == Group::InjectionCModeFromString("FLD"));
}



BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnumLoop) {
    BOOST_CHECK( Group::InjectionCMode::NONE == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::NONE ) ));
    BOOST_CHECK( Group::InjectionCMode::RATE == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::RATE ) ));
    BOOST_CHECK( Group::InjectionCMode::RESV == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::RESV ) ));
    BOOST_CHECK( Group::InjectionCMode::REIN == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::REIN ) ));
    BOOST_CHECK( Group::InjectionCMode::VREP == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::VREP ) ));
    BOOST_CHECK( Group::InjectionCMode::FLD  == Group::InjectionCModeFromString( Group::InjectionCMode2String( Group::InjectionCMode::FLD ) ));

    BOOST_CHECK_EQUAL( "NONE" , Group::InjectionCMode2String(Group::InjectionCModeFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL( "RATE" , Group::InjectionCMode2String(Group::InjectionCModeFromString( "RATE" ) ));
    BOOST_CHECK_EQUAL( "RESV" , Group::InjectionCMode2String(Group::InjectionCModeFromString( "RESV" ) ));
    BOOST_CHECK_EQUAL( "REIN" , Group::InjectionCMode2String(Group::InjectionCModeFromString( "REIN" ) ));
    BOOST_CHECK_EQUAL( "VREP" , Group::InjectionCMode2String(Group::InjectionCModeFromString( "VREP" ) ));
    BOOST_CHECK_EQUAL( "FLD"  , Group::InjectionCMode2String(Group::InjectionCModeFromString( "FLD"  ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE" , Group::ProductionCMode2String(Group::ProductionCMode::NONE));
    BOOST_CHECK_EQUAL( "ORAT" , Group::ProductionCMode2String(Group::ProductionCMode::ORAT));
    BOOST_CHECK_EQUAL( "WRAT" , Group::ProductionCMode2String(Group::ProductionCMode::WRAT));
    BOOST_CHECK_EQUAL( "GRAT" , Group::ProductionCMode2String(Group::ProductionCMode::GRAT));
    BOOST_CHECK_EQUAL( "LRAT" , Group::ProductionCMode2String(Group::ProductionCMode::LRAT));
    BOOST_CHECK_EQUAL( "CRAT" , Group::ProductionCMode2String(Group::ProductionCMode::CRAT));
    BOOST_CHECK_EQUAL( "RESV" , Group::ProductionCMode2String(Group::ProductionCMode::RESV));
    BOOST_CHECK_EQUAL( "PRBL" , Group::ProductionCMode2String(Group::ProductionCMode::PRBL));
}


BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnumFromString) {
    BOOST_CHECK_THROW(Group::ProductionCModeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK(Group::ProductionCMode::NONE  == Group::ProductionCModeFromString("NONE"));
    BOOST_CHECK(Group::ProductionCMode::ORAT  == Group::ProductionCModeFromString("ORAT"));
    BOOST_CHECK(Group::ProductionCMode::WRAT  == Group::ProductionCModeFromString("WRAT"));
    BOOST_CHECK(Group::ProductionCMode::GRAT  == Group::ProductionCModeFromString("GRAT"));
    BOOST_CHECK(Group::ProductionCMode::LRAT  == Group::ProductionCModeFromString("LRAT"));
    BOOST_CHECK(Group::ProductionCMode::CRAT  == Group::ProductionCModeFromString("CRAT"));
    BOOST_CHECK(Group::ProductionCMode::RESV  == Group::ProductionCModeFromString("RESV"));
    BOOST_CHECK(Group::ProductionCMode::PRBL  == Group::ProductionCModeFromString("PRBL"));
}



BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnumLoop) {
    BOOST_CHECK( Group::ProductionCMode::NONE == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::NONE ) ));
    BOOST_CHECK( Group::ProductionCMode::ORAT == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::ORAT ) ));
    BOOST_CHECK( Group::ProductionCMode::WRAT == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::WRAT ) ));
    BOOST_CHECK( Group::ProductionCMode::GRAT == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::GRAT ) ));
    BOOST_CHECK( Group::ProductionCMode::LRAT == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::LRAT ) ));
    BOOST_CHECK( Group::ProductionCMode::CRAT == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::CRAT ) ));
    BOOST_CHECK( Group::ProductionCMode::RESV == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::RESV ) ));
    BOOST_CHECK( Group::ProductionCMode::PRBL == Group::ProductionCModeFromString( Group::ProductionCMode2String( Group::ProductionCMode::PRBL ) ));

    BOOST_CHECK_EQUAL( "NONE" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL( "ORAT" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "ORAT" ) ));
    BOOST_CHECK_EQUAL( "WRAT" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "WRAT" ) ));
    BOOST_CHECK_EQUAL( "GRAT" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "GRAT" ) ));
    BOOST_CHECK_EQUAL( "LRAT" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "LRAT" ) ));
    BOOST_CHECK_EQUAL( "CRAT" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "CRAT" ) ));
    BOOST_CHECK_EQUAL( "RESV" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "RESV" ) ));
    BOOST_CHECK_EQUAL( "PRBL" , Group::ProductionCMode2String(Group::ProductionCModeFromString( "PRBL" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE"     , Group::ExceedAction2String(Group::ExceedAction::NONE));
    BOOST_CHECK_EQUAL( "CON"      , Group::ExceedAction2String(Group::ExceedAction::CON));
    BOOST_CHECK_EQUAL( "+CON"     , Group::ExceedAction2String(Group::ExceedAction::CON_PLUS));
    BOOST_CHECK_EQUAL( "WELL"     , Group::ExceedAction2String(Group::ExceedAction::WELL));
    BOOST_CHECK_EQUAL( "PLUG"     , Group::ExceedAction2String(Group::ExceedAction::PLUG));
    BOOST_CHECK_EQUAL( "RATE"     , Group::ExceedAction2String(Group::ExceedAction::RATE));
}


BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitActionEnumFromString) {
    BOOST_CHECK_THROW( Group::ExceedActionFromString("XXX") , std::invalid_argument );

    BOOST_CHECK(Group::ExceedAction::NONE     == Group::ExceedActionFromString("NONE"));
    BOOST_CHECK(Group::ExceedAction::CON      == Group::ExceedActionFromString("CON" ));
    BOOST_CHECK(Group::ExceedAction::CON_PLUS == Group::ExceedActionFromString("+CON"));
    BOOST_CHECK(Group::ExceedAction::WELL     == Group::ExceedActionFromString("WELL"));
    BOOST_CHECK(Group::ExceedAction::PLUG     == Group::ExceedActionFromString("PLUG"));
    BOOST_CHECK(Group::ExceedAction::RATE     == Group::ExceedActionFromString("RATE"));
}



BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitActionEnumLoop) {
    BOOST_CHECK( Group::ExceedAction::NONE     == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::NONE     ) ));
    BOOST_CHECK( Group::ExceedAction::CON      == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::CON      ) ));
    BOOST_CHECK( Group::ExceedAction::CON_PLUS == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::CON_PLUS ) ));
    BOOST_CHECK( Group::ExceedAction::WELL     == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::WELL     ) ));
    BOOST_CHECK( Group::ExceedAction::PLUG     == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::PLUG     ) ));
    BOOST_CHECK( Group::ExceedAction::RATE     == Group::ExceedActionFromString( Group::ExceedAction2String( Group::ExceedAction::RATE     ) ));

    BOOST_CHECK_EQUAL("NONE" , Group::ExceedAction2String(Group::ExceedActionFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL("CON"  , Group::ExceedAction2String(Group::ExceedActionFromString( "CON"  ) ));
    BOOST_CHECK_EQUAL("+CON" , Group::ExceedAction2String(Group::ExceedActionFromString( "+CON" ) ));
    BOOST_CHECK_EQUAL("WELL" , Group::ExceedAction2String(Group::ExceedActionFromString( "WELL" ) ));
    BOOST_CHECK_EQUAL("PLUG" , Group::ExceedAction2String(Group::ExceedActionFromString( "PLUG" ) ));
    BOOST_CHECK_EQUAL("RATE" , Group::ExceedAction2String(Group::ExceedActionFromString( "RATE" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestInjectorEnum2String) {
    BOOST_CHECK_EQUAL( "OIL"  ,  InjectorType2String(InjectorType::OIL));
    BOOST_CHECK_EQUAL( "GAS"  ,  InjectorType2String(InjectorType::GAS));
    BOOST_CHECK_EQUAL( "WATER" , InjectorType2String(InjectorType::WATER));
    BOOST_CHECK_EQUAL( "MULTI" , InjectorType2String(InjectorType::MULTI));
}


BOOST_AUTO_TEST_CASE(TestInjectorEnumFromString) {
    BOOST_CHECK_THROW( InjectorTypeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK( InjectorType::OIL   == InjectorTypeFromString("OIL"));
    BOOST_CHECK( InjectorType::WATER == InjectorTypeFromString("WATER"));
    BOOST_CHECK( InjectorType::WATER == InjectorTypeFromString("WAT"));
    BOOST_CHECK( InjectorType::GAS   == InjectorTypeFromString("GAS"));
    BOOST_CHECK( InjectorType::MULTI == InjectorTypeFromString("MULTI"));
}



BOOST_AUTO_TEST_CASE(TestInjectorEnumLoop) {
    BOOST_CHECK( InjectorType::OIL   == InjectorTypeFromString( InjectorType2String( InjectorType::OIL ) ));
    BOOST_CHECK( InjectorType::WATER == InjectorTypeFromString( InjectorType2String( InjectorType::WATER ) ));
    BOOST_CHECK( InjectorType::GAS   == InjectorTypeFromString( InjectorType2String( InjectorType::GAS ) ));
    BOOST_CHECK( InjectorType::MULTI == InjectorTypeFromString( InjectorType2String( InjectorType::MULTI ) ));

    BOOST_CHECK_EQUAL( "MULTI"    , InjectorType2String(InjectorTypeFromString(  "MULTI" ) ));
    BOOST_CHECK_EQUAL( "OIL"      , InjectorType2String(InjectorTypeFromString(  "OIL" ) ));
    BOOST_CHECK_EQUAL( "GAS"      , InjectorType2String(InjectorTypeFromString(  "GAS" ) ));
    BOOST_CHECK_EQUAL( "WATER"    , InjectorType2String(InjectorTypeFromString(  "WATER" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(InjectorCOntrolMopdeEnum2String) {
    BOOST_CHECK_EQUAL( "RATE"  , Opm::WellInjectorCMode2String(Well::InjectorCMode::RATE));
    BOOST_CHECK_EQUAL( "RESV"  , Opm::WellInjectorCMode2String(Well::InjectorCMode::RESV));
    BOOST_CHECK_EQUAL( "BHP"   , Opm::WellInjectorCMode2String(Well::InjectorCMode::BHP));
    BOOST_CHECK_EQUAL( "THP"   , Opm::WellInjectorCMode2String(Well::InjectorCMode::THP));
    BOOST_CHECK_EQUAL( "GRUP"  , Opm::WellInjectorCMode2String(Well::InjectorCMode::GRUP));
}


BOOST_AUTO_TEST_CASE(InjectorControlModeEnumFromString) {
    BOOST_CHECK_THROW( Opm::WellInjectorCModeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK( Well::InjectorCMode::RATE == Opm::WellInjectorCModeFromString("RATE"));
    BOOST_CHECK( Well::InjectorCMode::BHP  == Opm::WellInjectorCModeFromString("BHP"));
    BOOST_CHECK( Well::InjectorCMode::RESV == Opm::WellInjectorCModeFromString("RESV"));
    BOOST_CHECK( Well::InjectorCMode::THP  == Opm::WellInjectorCModeFromString("THP"));
    BOOST_CHECK( Well::InjectorCMode::GRUP == Opm::WellInjectorCModeFromString("GRUP"));
}



BOOST_AUTO_TEST_CASE(InjectorControlModeEnumLoop) {
    BOOST_CHECK( Well::InjectorCMode::RATE == Opm::WellInjectorCModeFromString( Opm::WellInjectorCMode2String( Well::InjectorCMode::RATE ) ));
    BOOST_CHECK( Well::InjectorCMode::BHP  == Opm::WellInjectorCModeFromString( Opm::WellInjectorCMode2String( Well::InjectorCMode::BHP ) ));
    BOOST_CHECK( Well::InjectorCMode::RESV == Opm::WellInjectorCModeFromString( Opm::WellInjectorCMode2String( Well::InjectorCMode::RESV ) ));
    BOOST_CHECK( Well::InjectorCMode::THP  == Opm::WellInjectorCModeFromString( Opm::WellInjectorCMode2String( Well::InjectorCMode::THP ) ));
    BOOST_CHECK( Well::InjectorCMode::GRUP == Opm::WellInjectorCModeFromString( Opm::WellInjectorCMode2String( Well::InjectorCMode::GRUP ) ));

    BOOST_CHECK_EQUAL( "THP"  , Opm::WellInjectorCMode2String(Opm::WellInjectorCModeFromString(  "THP" ) ));
    BOOST_CHECK_EQUAL( "RATE" , Opm::WellInjectorCMode2String(Opm::WellInjectorCModeFromString(  "RATE" ) ));
    BOOST_CHECK_EQUAL( "RESV" , Opm::WellInjectorCMode2String(Opm::WellInjectorCModeFromString(  "RESV" ) ));
    BOOST_CHECK_EQUAL( "BHP"  , Opm::WellInjectorCMode2String(Opm::WellInjectorCModeFromString(  "BHP" ) ));
    BOOST_CHECK_EQUAL( "GRUP" , Opm::WellInjectorCMode2String(Opm::WellInjectorCModeFromString(  "GRUP" ) ));
}



/*****************************************************************/

BOOST_AUTO_TEST_CASE(InjectorStatusEnum2String) {
    BOOST_CHECK_EQUAL( "OPEN",  Opm::WellStatus2String(Well::Status::OPEN));
    BOOST_CHECK_EQUAL( "SHUT",  Opm::WellStatus2String(Well::Status::SHUT));
    BOOST_CHECK_EQUAL( "AUTO",  Opm::WellStatus2String(Well::Status::AUTO));
    BOOST_CHECK_EQUAL( "STOP",  Opm::WellStatus2String(Well::Status::STOP));
}


BOOST_AUTO_TEST_CASE(InjectorStatusEnumFromString) {
    BOOST_CHECK_THROW( Opm::WellStatusFromString("XXX") , std::invalid_argument );
    BOOST_CHECK( Well::Status::OPEN == Opm::WellStatusFromString("OPEN"));
    BOOST_CHECK( Well::Status::AUTO == Opm::WellStatusFromString("AUTO"));
    BOOST_CHECK( Well::Status::SHUT == Opm::WellStatusFromString("SHUT"));
    BOOST_CHECK( Well::Status::STOP == Opm::WellStatusFromString("STOP"));
}



BOOST_AUTO_TEST_CASE(InjectorStatusEnumLoop) {
    BOOST_CHECK( Well::Status::OPEN == Opm::WellStatusFromString( Opm::WellStatus2String( Well::Status::OPEN ) ));
    BOOST_CHECK( Well::Status::AUTO == Opm::WellStatusFromString( Opm::WellStatus2String( Well::Status::AUTO ) ));
    BOOST_CHECK( Well::Status::SHUT == Opm::WellStatusFromString( Opm::WellStatus2String( Well::Status::SHUT ) ));
    BOOST_CHECK( Well::Status::STOP == Opm::WellStatusFromString( Opm::WellStatus2String( Well::Status::STOP ) ));

    BOOST_CHECK_EQUAL( "STOP", Opm::WellStatus2String(Opm::WellStatusFromString(  "STOP" ) ));
    BOOST_CHECK_EQUAL( "OPEN", Opm::WellStatus2String(Opm::WellStatusFromString(  "OPEN" ) ));
    BOOST_CHECK_EQUAL( "SHUT", Opm::WellStatus2String(Opm::WellStatusFromString(  "SHUT" ) ));
    BOOST_CHECK_EQUAL( "AUTO", Opm::WellStatus2String(Opm::WellStatusFromString(  "AUTO" ) ));
}



/*****************************************************************/

BOOST_AUTO_TEST_CASE(ProducerCOntrolMopdeEnum2String) {
    BOOST_CHECK_EQUAL( "ORAT"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::ORAT));
    BOOST_CHECK_EQUAL( "WRAT"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::WRAT));
    BOOST_CHECK_EQUAL( "GRAT"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::GRAT));
    BOOST_CHECK_EQUAL( "LRAT"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::LRAT));
    BOOST_CHECK_EQUAL( "CRAT"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::CRAT));
    BOOST_CHECK_EQUAL( "RESV"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::RESV));
    BOOST_CHECK_EQUAL( "BHP"   ,  Opm::WellProducerCMode2String(Well::ProducerCMode::BHP));
    BOOST_CHECK_EQUAL( "THP"   ,  Opm::WellProducerCMode2String(Well::ProducerCMode::THP));
    BOOST_CHECK_EQUAL( "GRUP"  ,  Opm::WellProducerCMode2String(Well::ProducerCMode::GRUP));
}


BOOST_AUTO_TEST_CASE(ProducerControlModeEnumFromString) {
    BOOST_CHECK_THROW( Opm::WellProducerCModeFromString("XRAT") , std::invalid_argument );
    BOOST_CHECK( Well::ProducerCMode::ORAT   == Opm::WellProducerCModeFromString("ORAT"));
    BOOST_CHECK( Well::ProducerCMode::WRAT   == Opm::WellProducerCModeFromString("WRAT"));
    BOOST_CHECK( Well::ProducerCMode::GRAT   == Opm::WellProducerCModeFromString("GRAT"));
    BOOST_CHECK( Well::ProducerCMode::LRAT   == Opm::WellProducerCModeFromString("LRAT"));
    BOOST_CHECK( Well::ProducerCMode::CRAT   == Opm::WellProducerCModeFromString("CRAT"));
    BOOST_CHECK( Well::ProducerCMode::RESV   == Opm::WellProducerCModeFromString("RESV"));
    BOOST_CHECK( Well::ProducerCMode::BHP    == Opm::WellProducerCModeFromString("BHP" ));
    BOOST_CHECK( Well::ProducerCMode::THP    == Opm::WellProducerCModeFromString("THP" ));
    BOOST_CHECK( Well::ProducerCMode::GRUP   == Opm::WellProducerCModeFromString("GRUP"));
}



BOOST_AUTO_TEST_CASE(ProducerControlModeEnumLoop) {
    BOOST_CHECK( Well::ProducerCMode::ORAT == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::ORAT ) ));
    BOOST_CHECK( Well::ProducerCMode::WRAT == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::WRAT ) ));
    BOOST_CHECK( Well::ProducerCMode::GRAT == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::GRAT ) ));
    BOOST_CHECK( Well::ProducerCMode::LRAT == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::LRAT ) ));
    BOOST_CHECK( Well::ProducerCMode::CRAT == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::CRAT ) ));
    BOOST_CHECK( Well::ProducerCMode::RESV == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::RESV ) ));
    BOOST_CHECK( Well::ProducerCMode::BHP  == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::BHP  ) ));
    BOOST_CHECK( Well::ProducerCMode::THP  == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::THP  ) ));
    BOOST_CHECK( Well::ProducerCMode::GRUP == Opm::WellProducerCModeFromString( Opm::WellProducerCMode2String( Well::ProducerCMode::GRUP ) ));

    BOOST_CHECK_EQUAL( "ORAT"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "ORAT"  ) ));
    BOOST_CHECK_EQUAL( "WRAT"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "WRAT"  ) ));
    BOOST_CHECK_EQUAL( "GRAT"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "GRAT"  ) ));
    BOOST_CHECK_EQUAL( "LRAT"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "LRAT"  ) ));
    BOOST_CHECK_EQUAL( "CRAT"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "CRAT"  ) ));
    BOOST_CHECK_EQUAL( "RESV"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "RESV"  ) ));
    BOOST_CHECK_EQUAL( "BHP"       , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "BHP"   ) ));
    BOOST_CHECK_EQUAL( "THP"       , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "THP"   ) ));
    BOOST_CHECK_EQUAL( "GRUP"      , Opm::WellProducerCMode2String(Opm::WellProducerCModeFromString( "GRUP"  ) ));
}

/*******************************************************************/
/*****************************************************************/

BOOST_AUTO_TEST_CASE(GuideRatePhaseEnum2String) {
    BOOST_CHECK_EQUAL( "OIL"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::OIL));
    BOOST_CHECK_EQUAL( "WAT"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::WAT));
    BOOST_CHECK_EQUAL( "GAS"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::GAS));
    BOOST_CHECK_EQUAL( "LIQ"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::LIQ));
    BOOST_CHECK_EQUAL( "COMB" ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::COMB));
    BOOST_CHECK_EQUAL( "WGA"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::WGA));
    BOOST_CHECK_EQUAL( "CVAL" ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::CVAL));
    BOOST_CHECK_EQUAL( "RAT"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::RAT));
    BOOST_CHECK_EQUAL( "RES"  ,        Opm::WellGuideRateTarget2String(Well::GuideRateTarget::RES));
    BOOST_CHECK_EQUAL( "UNDEFINED"  ,  Opm::WellGuideRateTarget2String(Well::GuideRateTarget::UNDEFINED));
}


BOOST_AUTO_TEST_CASE(GuideRatePhaseEnumFromString) {
    BOOST_CHECK_THROW( Opm::WellGuideRateTargetFromString("XRAT") , std::invalid_argument );
    BOOST_CHECK( Well::GuideRateTarget::OIL       == Opm::WellGuideRateTargetFromString("OIL"));
    BOOST_CHECK( Well::GuideRateTarget::WAT       == Opm::WellGuideRateTargetFromString("WAT"));
    BOOST_CHECK( Well::GuideRateTarget::GAS       == Opm::WellGuideRateTargetFromString("GAS"));
    BOOST_CHECK( Well::GuideRateTarget::LIQ       == Opm::WellGuideRateTargetFromString("LIQ"));
    BOOST_CHECK( Well::GuideRateTarget::COMB      == Opm::WellGuideRateTargetFromString("COMB"));
    BOOST_CHECK( Well::GuideRateTarget::WGA       == Opm::WellGuideRateTargetFromString("WGA"));
    BOOST_CHECK( Well::GuideRateTarget::CVAL      == Opm::WellGuideRateTargetFromString("CVAL"));
    BOOST_CHECK( Well::GuideRateTarget::RAT       == Opm::WellGuideRateTargetFromString("RAT"));
    BOOST_CHECK( Well::GuideRateTarget::RES       == Opm::WellGuideRateTargetFromString("RES"));
    BOOST_CHECK( Well::GuideRateTarget::UNDEFINED == Opm::WellGuideRateTargetFromString("UNDEFINED"));
}



BOOST_AUTO_TEST_CASE(GuideRatePhaseEnum2Loop) {
    BOOST_CHECK( Well::GuideRateTarget::OIL        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::OIL ) ));
    BOOST_CHECK( Well::GuideRateTarget::WAT        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::WAT ) ));
    BOOST_CHECK( Well::GuideRateTarget::GAS        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::GAS ) ));
    BOOST_CHECK( Well::GuideRateTarget::LIQ        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::LIQ ) ));
    BOOST_CHECK( Well::GuideRateTarget::COMB       == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::COMB ) ));
    BOOST_CHECK( Well::GuideRateTarget::WGA        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::WGA ) ));
    BOOST_CHECK( Well::GuideRateTarget::CVAL       == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::CVAL ) ));
    BOOST_CHECK( Well::GuideRateTarget::RAT        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::RAT ) ));
    BOOST_CHECK( Well::GuideRateTarget::RES        == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::RES ) ));
    BOOST_CHECK( Well::GuideRateTarget::UNDEFINED  == Opm::WellGuideRateTargetFromString( Opm::WellGuideRateTarget2String( Well::GuideRateTarget::UNDEFINED ) ));

    BOOST_CHECK_EQUAL( "OIL"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "OIL"  ) ));
    BOOST_CHECK_EQUAL( "WAT"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "WAT"  ) ));
    BOOST_CHECK_EQUAL( "GAS"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "GAS"  ) ));
    BOOST_CHECK_EQUAL( "LIQ"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "LIQ"  ) ));
    BOOST_CHECK_EQUAL( "COMB"       , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "COMB"  ) ));
    BOOST_CHECK_EQUAL( "WGA"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "WGA"  ) ));
    BOOST_CHECK_EQUAL( "CVAL"       , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "CVAL"  ) ));
    BOOST_CHECK_EQUAL( "RAT"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "RAT"  ) ));
    BOOST_CHECK_EQUAL( "RES"        , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "RES"  ) ));
    BOOST_CHECK_EQUAL( "UNDEFINED"  , Opm::WellGuideRateTarget2String(Opm::WellGuideRateTargetFromString( "UNDEFINED"  ) ));

}

BOOST_AUTO_TEST_CASE(handleWEFAC) {
    const std::string input = R"(
START             -- 0
19 JUN 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
    'P'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'P'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'P'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
 'P' 'OPEN' 'RESV' 6*  500 /
/
WCONINJH
 'I' 'WATER' 1* 100 250 /
/
WEFAC
   'P' 0.5 /
   'I' 0.9 /
/
DATES             -- 2
 15  OKT 2008 /
/

DATES             -- 3
 18  OKT 2008 /
/
WEFAC
   'P' 1.0 /
/
)";

    const auto deck = Parser{}.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    //1
    BOOST_CHECK_EQUAL(schedule.getWell("P", 1).getEfficiencyFactor(), 0.5);
    BOOST_CHECK_EQUAL(schedule.getWell("I", 1).getEfficiencyFactor(), 0.9);

    //2
    BOOST_CHECK_EQUAL(schedule.getWell("P", 2).getEfficiencyFactor(), 0.5);
    BOOST_CHECK_EQUAL(schedule.getWell("I", 2).getEfficiencyFactor(), 0.9);

    //3
    BOOST_CHECK_EQUAL(schedule.getWell("P", 3).getEfficiencyFactor(), 1.0);
    BOOST_CHECK_EQUAL(schedule.getWell("I", 3).getEfficiencyFactor(), 0.9);
}

BOOST_AUTO_TEST_CASE(historic_BHP_and_THP) {
    const std::string input = R"(
START             -- 0
19 JUN 2007 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
 'P' 'OP' 9 9 1 'OIL' 1* /
 'P1' 'OP' 9 9 1 'OIL' 1* /
 'I' 'OP' 9 9 1 'WATER' 1* /
/
WCONHIST
 P SHUT ORAT 6  500 0 0 0 1.2 1.1 /
/
WCONPROD
 P1 SHUT ORAT 6  500 0 0 0 3.2 /
/
WCONINJH
 I WATER STOP 100 2.1 2.2 /
/
)";

    const auto deck = Parser{}.parseString(input);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    {
        const auto& prod = schedule.getWell("P", 1).getProductionProperties();
        const auto& pro1 = schedule.getWell("P1", 1).getProductionProperties();
        const auto& inje = schedule.getWell("I", 1).getInjectionProperties();

        BOOST_CHECK_CLOSE( 1.1 * 1e5,  prod.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 1.2 * 1e5,  prod.THPH, 1e-5 );
        BOOST_CHECK_CLOSE( 2.1 * 1e5,  inje.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 2.2 * 1e5,  inje.THPH, 1e-5 );
        BOOST_CHECK_CLOSE( 0.0 * 1e5,  pro1.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 0.0 * 1e5,  pro1.THPH, 1e-5 );

        {
            const auto& wtest_config = schedule[0].wtest_config.get();
            BOOST_CHECK(wtest_config.empty());
        }

        {
            const auto& wtest_config = schedule[1].wtest_config.get();
            BOOST_CHECK(wtest_config.empty());
        }
    }
}

BOOST_AUTO_TEST_CASE(FilterCompletions2) {
    const auto& deck = Parser{}.parseString(createDeckWithWellsAndCompletionData());
    EclipseGrid grid1(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid1, table);
    const Runspec runspec (deck);

    /* mutable */ Schedule schedule {
        deck, grid1, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    std::vector<int> actnum = grid1.getACTNUM();

    {
        const auto& c1_1 = schedule.getWell("OP_1", 1).getConnections();
        const auto& c1_3 = schedule.getWell("OP_1", 3).getConnections();
        BOOST_CHECK_EQUAL(2U, c1_1.size());
        BOOST_CHECK_EQUAL(9U, c1_3.size());
    }

    actnum[grid1.getGlobalIndex(8,8,1)] = 0;
    {
        std::vector<int> globalCell(grid1.getNumActive());
        for(std::size_t i = 0; i < grid1.getNumActive(); ++i) {
            if (actnum[grid1.getGlobalIndex(i)]) {
                globalCell[i] = grid1.getGlobalIndex(i);
            }
        }

        ActiveGridCells active(grid1.getNXYZ(), globalCell.data(),
                               grid1.getNumActive());

        schedule.filterConnections(active);

        const auto& c1_1 = schedule.getWell("OP_1", 1).getConnections();
        const auto& c1_3 = schedule.getWell("OP_1", 3).getConnections();
        BOOST_CHECK_EQUAL(1U, c1_1.size());
        BOOST_CHECK_EQUAL(8U, c1_3.size());
    }
}

BOOST_AUTO_TEST_CASE(VFPINJ_TEST) {
    const std::string input = R"(
START
8 MAR 1998 /

GRID
PORO
  1000*0.25 /
PERMX
  1000*0.10/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
SCHEDULE
VFPINJ
-- Table Depth  Rate   TAB  UNITS  BODY
-- ----- ----- ----- ----- ------ -----
       5  32.9   WAT   THP METRIC   BHP /
-- Rate axis
1 3 5 /
-- THP axis
7 11 /
-- Table data with THP# <values 1-num_rates>
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /
TSTEP
10 10/
VFPINJ
-- Table Depth  Rate   TAB  UNITS  BODY
-- ----- ----- ----- ----- ------ -----
       5  100   GAS   THP METRIC   BHP /
-- Rate axis
1 3 5 /
-- THP axis
7 11 /
-- Table data with THP# <values 1-num_rates>
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /
--
VFPINJ
-- Table Depth  Rate   TAB  UNITS  BODY
-- ----- ----- ----- ----- ------ -----
       10 200  WAT   THP METRIC   BHP /
-- Rate axis
1 3 5 /
-- THP axis
7 11 /
-- Table data with THP# <values 1-num_rates>
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /
)";

    const auto schedule = make_schedule(input);

    BOOST_CHECK( schedule[0].events().hasEvent(ScheduleEvents::VFPINJ_UPDATE));
    BOOST_CHECK( !schedule[1].events().hasEvent(ScheduleEvents::VFPINJ_UPDATE));
    BOOST_CHECK( schedule[2].events().hasEvent(ScheduleEvents::VFPINJ_UPDATE));

    // No such table id
    BOOST_CHECK_THROW(schedule[0].vfpinj(77), std::exception);

    // Table not defined at step 0
    BOOST_CHECK_THROW(schedule[0].vfpinj(10), std::exception);

    const Opm::VFPInjTable& vfpinjTable2 = schedule[2].vfpinj(5);
    BOOST_CHECK_EQUAL(vfpinjTable2.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpinjTable2.getDatumDepth(), 100);
    BOOST_CHECK(vfpinjTable2.getFloType() == Opm::VFPInjTable::FLO_TYPE::FLO_GAS);

    const Opm::VFPInjTable& vfpinjTable3 = schedule[2].vfpinj(10);
    BOOST_CHECK_EQUAL(vfpinjTable3.getTableNum(), 10);
    BOOST_CHECK_EQUAL(vfpinjTable3.getDatumDepth(), 200);
    BOOST_CHECK(vfpinjTable3.getFloType() == Opm::VFPInjTable::FLO_TYPE::FLO_WAT);

    const Opm::VFPInjTable& vfpinjTable = schedule[0].vfpinj(5);
    BOOST_CHECK_EQUAL(vfpinjTable.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpinjTable.getDatumDepth(), 32.9);
    BOOST_CHECK(vfpinjTable.getFloType() == Opm::VFPInjTable::FLO_TYPE::FLO_WAT);

    //Flo axis
    {
        const std::vector<double>& flo = vfpinjTable.getFloAxis();
        BOOST_REQUIRE_EQUAL(flo.size(), 3U);

        //Unit of FLO is SM3/day, convert to SM3/second
        double conversion_factor = 1.0 / (60*60*24);
        BOOST_CHECK_EQUAL(flo[0], 1*conversion_factor);
        BOOST_CHECK_EQUAL(flo[1], 3*conversion_factor);
        BOOST_CHECK_EQUAL(flo[2], 5*conversion_factor);
    }

    //THP axis
    {
        const std::vector<double>& thp = vfpinjTable.getTHPAxis();
        BOOST_REQUIRE_EQUAL(thp.size(), 2U);

        //Unit of THP is barsa => convert to pascal
        double conversion_factor = 100000.0;
        BOOST_CHECK_EQUAL(thp[0], 7*conversion_factor);
        BOOST_CHECK_EQUAL(thp[1], 11*conversion_factor);
    }

    //The data itself
    {
        const auto size = vfpinjTable.shape();

        BOOST_CHECK_EQUAL(size[0], 2U);
        BOOST_CHECK_EQUAL(size[1], 3U);

        //Table given as BHP => barsa. Convert to pascal
        double conversion_factor = 100000.0;

        double index = 0.5;
        for (std::size_t t = 0; t < size[0]; ++t) {
            for (std::size_t f = 0; f < size[1]; ++f) {
                index += 1.0;
                BOOST_CHECK_EQUAL(vfpinjTable(t,f), index*conversion_factor);
            }
        }
    }
}

// tests for the polymer injectivity case
BOOST_AUTO_TEST_CASE(POLYINJ_TEST) {
    const std::string deckData = R"(
START
   8 MAR 2018/
GRID
PORO
  1000*0.25 /
PERMX
  1000*0.25 /
COPY
  PERMX  PERMY /
  PERMX  PERMZ /
/
PROPS
SCHEDULE
WELSPECS
'INJE01' 'I'    1  1 1 'WATER'     /
/
WCONINJE
'INJE01' 'WATER' 'OPEN' 'RATE' 800.00  1* 1000 /
/
TSTEP
 1/
WPOLYMER
    'INJE01' 1.0  0.0 /
/
WPMITAB
   'INJE01' 2 /
/
WSKPTAB
    'INJE01' 1  1 /
/
TSTEP
 2*1/
WPMITAB
   'INJE01' 3 /
/
WSKPTAB
    'INJE01' 2  2 /
/
TSTEP
 1 /
)";

    const auto deck = Parser{}.parseString(deckData);
    EclipseGrid grid1(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid1, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid1, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    const auto& poly0 = schedule.getWell("INJE01", 0).getPolymerProperties();
    const auto& poly1 = schedule.getWell("INJE01", 1).getPolymerProperties();
    const auto& poly3 = schedule.getWell("INJE01", 3).getPolymerProperties();

    BOOST_CHECK_EQUAL(poly0.m_plymwinjtable, -1);
    BOOST_CHECK_EQUAL(poly0.m_skprwattable, -1);
    BOOST_CHECK_EQUAL(poly0.m_skprpolytable, -1);

    BOOST_CHECK_EQUAL(poly1.m_plymwinjtable, 2);
    BOOST_CHECK_EQUAL(poly1.m_skprwattable, 1);
    BOOST_CHECK_EQUAL(poly1.m_skprpolytable, 1);

    BOOST_CHECK_EQUAL(poly3.m_plymwinjtable, 3);
    BOOST_CHECK_EQUAL(poly3.m_skprwattable, 2);
    BOOST_CHECK_EQUAL(poly3.m_skprpolytable, 2);
}

// Test for WFOAM
BOOST_AUTO_TEST_CASE(WFOAM_TEST) {
    const std::string input = R"(
START
   8 MAR 2018/
GRID
PERMX
  1000*0.25 /
PORO
  1000*0.25 /
COPY
  PERMX  PERMY /
  PERMX  PERMZ /
/
PROPS
SCHEDULE
WELSPECS
'INJE01' 'I'    1  1 1 'WATER'     /
/
WCONINJE
'INJE01' 'GAS' 'OPEN' 'RATE' 80000.00  1* 1000 /
/
TSTEP
 1/
WFOAM
    'INJE01' 0.2 /
/
TSTEP
 2*1/
WFOAM
    'INJE01' 0.3 /
/
TSTEP
 1 /
)";

    const auto& schedule = make_schedule(input);

    const auto& f0 = schedule.getWell("INJE01", 0).getFoamProperties();
    const auto& f1 = schedule.getWell("INJE01", 1).getFoamProperties();
    const auto& f3 = schedule.getWell("INJE01", 3).getFoamProperties();

    BOOST_CHECK_EQUAL(f0.m_foamConcentration, 0.0);
    BOOST_CHECK_EQUAL(f1.m_foamConcentration, 0.2);
    BOOST_CHECK_EQUAL(f3.m_foamConcentration, 0.3);
}


BOOST_AUTO_TEST_CASE(WTEST_CONFIG) {
    const auto& schedule = make_schedule(createDeckWTEST());

    const auto& wtest_config1 = schedule[0].wtest_config.get();
    BOOST_CHECK(!wtest_config1.empty());
    BOOST_CHECK(wtest_config1.has("ALLOW"));
    BOOST_CHECK(!wtest_config1.has("BAN"));

    const auto& wtest_config2 = schedule[1].wtest_config.get();
    BOOST_CHECK(!wtest_config2.empty());
    BOOST_CHECK(!wtest_config2.has("ALLOW"));
    BOOST_CHECK(wtest_config2.has("BAN"));
    BOOST_CHECK(wtest_config2.has("BAN", WellTestConfig::Reason::GROUP));
    BOOST_CHECK(!wtest_config2.has("BAN", WellTestConfig::Reason::PHYSICAL));
}


namespace {

    bool has(const std::vector<std::string>& l, const std::string& s)
    {
        return std::any_of(l.begin(), l.end(),
                           [&s](const std::string& search)
                           {
                               return search == s;
                           });
    }
}

BOOST_AUTO_TEST_CASE(WELL_STATIC) {
    const auto& deck = Parser{}.parseString(createDeckWithWells());
    EclipseGrid grid1(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid1, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid1, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK_THROW( schedule.getWell("NO_SUCH_WELL", 0), std::exception);
    BOOST_CHECK_THROW( schedule.getWell("W_3", 0)         , std::exception);

    auto ws = schedule.getWell("W_3", 3);
    {
        // Make sure the copy constructor works.
        Well ws_copy(ws);
    }
    BOOST_CHECK_EQUAL(ws.name(), "W_3");

    BOOST_CHECK(!ws.updateHead(19, 50));
    BOOST_CHECK(ws.updateHead(1,50));
    BOOST_CHECK(!ws.updateHead(1,50));
    BOOST_CHECK(ws.updateHead(1,1));
    BOOST_CHECK(!ws.updateHead(1,1));

    BOOST_CHECK(ws.updateRefDepth(1.0));
    BOOST_CHECK(!ws.updateRefDepth(1.0));

    ws.updateStatus(Well::Status::SHUT);

    const auto& connections = ws.getConnections();
    BOOST_CHECK_EQUAL(connections.size(), 0U);
    auto c2 = std::make_shared<WellConnections>(Connection::Order::TRACK, 1,1);
    c2->addConnection(1, 1, 1,
                      grid1.getGlobalIndex(1, 1, 1),
                      Connection::State::OPEN,
                      100.0, Connection::CTFProperties{}, 10);

    BOOST_CHECK(  ws.updateConnections(c2, false) );
    BOOST_CHECK( !ws.updateConnections(c2, false) );
}


BOOST_AUTO_TEST_CASE(WellNames)
{
    const auto& schedule = make_schedule(createDeckWTEST());

    {
        const auto names = schedule.wellNames("NO_SUCH_WELL", 0);
        BOOST_CHECK_EQUAL(names.size(), 0U);
    }

    {
        const auto w1names = schedule.wellNames("W1", 0);
        BOOST_CHECK_EQUAL(w1names.size(), 1U);
        BOOST_CHECK_EQUAL(w1names[0], "W1");
    }

    {
        const auto i1names = schedule.wellNames("11", 0);
        BOOST_CHECK_EQUAL(i1names.size(), 0U);
    }

    {
        const auto listnamese = schedule.wellNames("*NO_LIST", 0);
        BOOST_CHECK_EQUAL( listnamese.size(), 0U);
    }

    {
        const auto listnames0 = schedule.wellNames("*ILIST", 0);
        BOOST_CHECK_EQUAL( listnames0.size(), 0U);
    }

    {
        const auto listnames1 = schedule.wellNames("*ILIST", 2);

        BOOST_CHECK_EQUAL( listnames1.size(), 2U);
        BOOST_CHECK( has(listnames1, "I1"));
        BOOST_CHECK( has(listnames1, "I2"));
    }

    {
        const auto pnames1 = schedule.wellNames("I*", 0);
        BOOST_CHECK_EQUAL(pnames1.size(), 0U);
    }

    {
        const auto pnames2 = schedule.wellNames("W*", 0);

        BOOST_CHECK_EQUAL(pnames2.size(), 3U);
        BOOST_CHECK( has(pnames2, "W1"));
        BOOST_CHECK( has(pnames2, "W2"));
        BOOST_CHECK( has(pnames2, "W3"));
    }

    {
        const auto anames = schedule.wellNames("?", 0, {"W1", "W2"});

        BOOST_CHECK_EQUAL(anames.size(), 2U);
        BOOST_CHECK(has(anames, "W1"));
        BOOST_CHECK(has(anames, "W2"));
    }

    {
        const auto all_names0 = schedule.wellNames("*", 0);

        BOOST_CHECK_EQUAL( all_names0.size(), 6U);
        BOOST_CHECK( has(all_names0, "W1"));
        BOOST_CHECK( has(all_names0, "W2"));
        BOOST_CHECK( has(all_names0, "W3"));
        BOOST_CHECK( has(all_names0, "DEFAULT"));
        BOOST_CHECK( has(all_names0, "ALLOW"));
    }

    {
        const auto all_names = schedule.wellNames("*", 2);

        BOOST_CHECK_EQUAL( all_names.size(), 9U);
        BOOST_CHECK( has(all_names, "I1"));
        BOOST_CHECK( has(all_names, "I2"));
        BOOST_CHECK( has(all_names, "I3"));
        BOOST_CHECK( has(all_names, "W1"));
        BOOST_CHECK( has(all_names, "W2"));
        BOOST_CHECK( has(all_names, "W3"));
        BOOST_CHECK( has(all_names, "DEFAULT"));
        BOOST_CHECK( has(all_names, "ALLOW"));
        BOOST_CHECK( has(all_names, "BAN"));
    }

    {
        auto abs_all = schedule.wellNames();
        BOOST_CHECK_EQUAL(abs_all.size(), 9U);
    }

    {
        WellMatcher wm0{};
        const auto& wml0 = wm0.wells();
        BOOST_CHECK(wml0.empty());
    }

    {
        NameOrder wo({"P3", "P2", "P1"});
        wo.add("W3");
        wo.add("W2");
        wo.add("W1");

        BOOST_CHECK_EQUAL( wo.size(), 6 );
        BOOST_CHECK_THROW( wo[6], std::exception );
        BOOST_CHECK_EQUAL( wo[2], "P1" );

        const WellMatcher wm1{std::move(wo)};
        const auto pwells = std::vector<std::string> {"P3", "P2", "P1"};
        BOOST_CHECK(pwells == wm1.wells("P*"));
    }

    const auto wm2 = schedule.wellMatcher(4);
    {
        const auto& all_wells = wm2.wells();
        BOOST_CHECK_EQUAL(all_wells.size(), 9);

        for (const auto& w : std::vector<std::string> {
                "W1", "W2", "W3", "I1", "I2", "I3",
                "DEFAULT", "ALLOW", "BAN",
            })
        {
            BOOST_CHECK(has(all_wells, w));
        }
    }

    {
        const std::vector<std::string> wwells = {"W1", "W2", "W3"};
        BOOST_CHECK( wm2.wells("W*") == wwells );
        BOOST_CHECK( wm2.wells("XYZ*").empty() );
        BOOST_CHECK( wm2.wells("XYZ").empty() );
    }

    {
        const auto def = wm2.wells("DEFAULT");

        BOOST_CHECK_EQUAL(def.size() , 1);
        BOOST_CHECK_EQUAL(def[0], "DEFAULT");
    }

    {
        const auto l2 = wm2.wells("*ILIST");

        BOOST_CHECK_EQUAL( l2.size(), 2U);
        BOOST_CHECK( has(l2, "I1"));
        BOOST_CHECK( has(l2, "I2"));
    }
}

BOOST_AUTO_TEST_CASE(WellOrderTest)
{
    NameOrder wo{};
    wo.add("W1");
    wo.add("W2");
    wo.add("W3");
    wo.add("W4");

    const std::vector<std::string> sorted_wells = {"W1", "W2", "W3", "W4"};
    const std::vector<std::string> unsorted_wells = {"W4", "W3", "W2", "W1"};

    BOOST_CHECK( wo.sort(unsorted_wells) == sorted_wells );
    BOOST_CHECK( wo.names() == sorted_wells );
    BOOST_CHECK( wo.has("W1"));
    BOOST_CHECK( !wo.has("G1"));
}

BOOST_AUTO_TEST_CASE(GroupOrderTest)
{
    const std::size_t max_groups = 9;
    GroupOrder go(max_groups);

    const std::vector<std::string> groups1 = {"FIELD"};
    const std::vector<std::string> groups2 = {"FIELD", "G1", "G2", "G3"};

    BOOST_CHECK(go.names() == groups1);
    go.add("G1");
    go.add("G2");
    go.add("G3");
    BOOST_CHECK(go.names() == groups2);

    const auto& restart_groups = go.restart_groups();
    BOOST_CHECK_EQUAL(restart_groups.size(), max_groups + 1);
    BOOST_CHECK_EQUAL( *restart_groups[0], "G1");
    BOOST_CHECK_EQUAL( *restart_groups[1], "G2");
    BOOST_CHECK_EQUAL( *restart_groups[2], "G3");
    BOOST_CHECK_EQUAL( *restart_groups[max_groups], "FIELD");

    for (std::size_t g = 3; g < max_groups; ++g) {
        BOOST_CHECK( !restart_groups[g].has_value() );
    }
}

BOOST_AUTO_TEST_CASE(Has_Well)
{
    const auto schedule = make_schedule(createDeckWTEST());

    // Start of simulation
    {
        const auto wm = schedule.wellMatcher(0);

        BOOST_CHECK_MESSAGE(  wm.hasWell("W1"), R"(Well "W1" must exist at time zero)");
        BOOST_CHECK_MESSAGE(! wm.hasWell("W4"), R"(Well "W4" must NOT exist at time zero)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("W?"), R"(Wells matching pattern "W?" must exist at time zero)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("W*"), R"(Wells matching pattern "W*" must exist at time zero)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("DEF*"), R"(Wells matching pattern "DEF*" must exist at time zero)");
        BOOST_CHECK_MESSAGE(! wm.hasWell("*ILIST"), R"(Wells matching pattern "*ILIST" must NOT exist at time zero)");
    }

    // Report step 2--injectors and well lists introduced
    {
        const auto wm = schedule.wellMatcher(2);

        BOOST_CHECK_MESSAGE(  wm.hasWell("W1"), R"(Well "W1" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(! wm.hasWell("W4"), R"(Well "W4" must NOT exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("W?"), R"(Wells matching pattern "W?" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("W*"), R"(Wells matching pattern "W*" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("DEF*"), R"(Wells matching pattern "DEF*" must exist at report step 2)");

        BOOST_CHECK_MESSAGE(  wm.hasWell("I1"), R"(Well "I1" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(! wm.hasWell("I4"), R"(Well "I4" must NOT exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("I?"), R"(Wells matching pattern "I?" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("I*"), R"(Wells matching pattern "I*" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("*ILIST"), R"(Wells matching pattern "*ILIST" must exist at report step 2)");
        BOOST_CHECK_MESSAGE(  wm.hasWell("*IL*"), R"(Wells matching pattern "*IL*" must exist at report step 2)");

        // Well list '*EMPTY' exists, but has no wells => hasWell() returns 'false'.
        BOOST_CHECK_MESSAGE(! wm.hasWell("*EMPTY"), R"(Wells matching pattern "*EMPTY" must NOT exist at report step 2)");
    }
}

BOOST_AUTO_TEST_CASE(Has_Group_Simple)
{
    const auto schedule = make_schedule(createDeckWTEST());

    // Start of simulation
    const auto& go = schedule[0].group_order();

    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("OP"), R"(Group "OP" must exist at time zero)");
    BOOST_CHECK_MESSAGE(! go.anyGroupMatches("OPE"), R"(Group "OPE" must NOT exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("OP*"), R"(Groups matching pattern "OP*" must exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("O*"), R"(Groups matching pattern "O*" must exist at time zero)");
    BOOST_CHECK_MESSAGE(! go.anyGroupMatches("NO*"), R"(Groups matching pattern "NO*" must NOT exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("FI*"), R"(Groups matching pattern "FI*" must exist at time zero)");
}

BOOST_AUTO_TEST_CASE(Has_Group_GroupTree)
{
    const auto schedule = make_schedule(createDeckWithWellsOrderedGRUPTREE());

    // Start of simulation
    const auto& go = schedule[0].group_order();

    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("FIELD"), R"(Group "FIELD" must exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("FI*"), R"(Groups matching pattern "FI*" must exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("PLATFORM"), R"(Group "OP" must exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("PLAT*"), R"(Groups matching the pattern "PLAT*" must exist at time zero)");
    BOOST_CHECK_MESSAGE(! go.anyGroupMatches("PG13"), R"(Group "PG13" must NOT exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("PG*"), R"(Groups matching "PG*" must exist at time zero)");
    BOOST_CHECK_MESSAGE(  go.anyGroupMatches("PG1*"), R"(Groups matching "PG1*" must exist at time zero)");
}

BOOST_AUTO_TEST_CASE(nupcol) {
    std::string input = R"(
RUNSPEC
START             -- 0
19 JUN 2007 /
MINNPCOL
  6 /
NUPCOL
  20 /
SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/
NUPCOL
  1* /
DATES             -- 1
 10  OKT 2009 /
/
NUPCOL
  4 /
DATES             -- 1
 10  OKT 2010 /
/
)";
    const auto& schedule = make_schedule(input);
    {
        // Flow uses 12 as default
        BOOST_CHECK_EQUAL(schedule[0].nupcol(),20);
        BOOST_CHECK_EQUAL(schedule[1].nupcol(),12);
        BOOST_CHECK_EQUAL(schedule[2].nupcol(), 6);
    }
}

BOOST_AUTO_TEST_CASE(TESTGuideRateConfig) {
    const std::string input = R"(
START             -- 0
10 MAI 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WELSPECS
     'W1'    'G1'   1 2  3.33       'OIL'  7*/
     'W2'    'G2'   1 3  3.33       'OIL'  3*  YES /
     'W3'    'G3'   1 4  3.92       'OIL'  3*  NO /
/

COMPDAT
 'W1'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
 'W2'  1  1   2   2 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
 'W3'  1  1   3   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
/

WCONPROD
     'W1'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/

WGRUPCON
    'W1' 'YES'   0.50 'OIL' /
    'W2' 'YES'   0.50 'GAS' /
/

GCONPROD
 'G1' 'ORAT' 1000 /
 'G2' 'ORAT' 1000 5* 0.25 'OIL' /
/


DATES             -- 1
 10  JUN 2007 /
/

WCONHIST
     'W1'      'OPEN'      'ORAT'      1.000      0.000      0.000  5* /
/

WGRUPCON
    'W1' 'YES'   0.75 'WAT' /
    'W2' 'NO' /
/

GCONPROD
 'G2' 'ORAT' 1000 /
 'G1' 'ORAT' 1000 6* 'FORM' /
/

DATES             -- 2
 10  JUL 2007 /
/


WCONPROD
     'W1'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/


DATES             -- 3
 10  AUG 2007 /
/


DATES             -- 4
 10  SEP 2007 /
/


DATES             -- 5
 10  NOV 2007 /
/

WELSPECS
     'W4'    'G1'   1 2  3.33       'OIL'  7*/
/

DATES       -- 6
    10 DEC 2007 /
/

COMPDAT
  'W4'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
/

     )";

    const auto& schedule = make_schedule(input);
    {
        const auto& grc = schedule[0].guide_rate();
        const auto& w1_node = grc.well("W1");
        BOOST_CHECK(w1_node.target == Well::GuideRateTarget::OIL);

        const auto& w2_node = grc.well("W2");
        BOOST_CHECK(w2_node.target == Well::GuideRateTarget::GAS);

        BOOST_CHECK(!grc.has_production_group("G1"));
        BOOST_CHECK(grc.has_production_group("G2"));
    }
    {
        const auto& grc = schedule[2].guide_rate();
        const auto& w1_node = grc.well("W1");
        BOOST_CHECK(w1_node.target == Well::GuideRateTarget::WAT);
        BOOST_CHECK_EQUAL(w1_node.guide_rate, 0.75);

        BOOST_CHECK(grc.has_well("W1"));
        BOOST_CHECK(!grc.has_well("W2"));
        BOOST_CHECK_THROW( grc.well("W2"), std::out_of_range);

        BOOST_CHECK(grc.has_production_group("G1"));
        BOOST_CHECK(!grc.has_production_group("G2"));
    }

    {
        GuideRate gr(schedule);
        double oil_pot = 1;
        double gas_pot = 1;
        double wat_pot = 1;

        gr.compute("XYZ",1, 1.0, oil_pot, gas_pot, wat_pot);
    }
    {
        const auto& changed_wells = schedule.changed_wells(0);
        BOOST_CHECK_EQUAL( changed_wells.size() , 3U);
        for (const auto& wname : {"W1", "W2", "W2"}) {
            auto find_well = std::find(changed_wells.begin(), changed_wells.end(), wname);
            BOOST_CHECK(find_well != changed_wells.end());
        }
    }
    {
        const auto& changed_wells = schedule.changed_wells(2);
        BOOST_CHECK_EQUAL( changed_wells.size(), 0U);
    }
    {
        const auto& changed_wells = schedule.changed_wells(4);
        BOOST_CHECK_EQUAL( changed_wells.size(), 0U);
    }
    {
        const auto& changed_wells = schedule.changed_wells(5);
        BOOST_CHECK_EQUAL( changed_wells.size(), 1U);
        BOOST_CHECK_EQUAL( changed_wells[0], "W4");
    }
    {
        const auto& changed_wells = schedule.changed_wells(6);
        BOOST_CHECK_EQUAL( changed_wells.size(), 1U);
        BOOST_CHECK_EQUAL( changed_wells[0], "W4");
    }
}

BOOST_AUTO_TEST_CASE(Injection_Control_Mode_From_Well) {
    const auto input = R"(RUNSPEC

SCHEDULE
WELSPECS
     'W1'    'G1'   1 2  3.33       'OIL'  7*/
     'W2'    'G2'   1 3  3.33       'OIL'  3*  YES /
     'W3'    'G3'   1 4  3.92       'OIL'  3*  NO /
     'W4'    'G3'   2 2  3.92       'OIL'  3*  NO /
     'W5'    'G3'   2 3  3.92       'OIL'  3*  NO /
     'W6'    'G3'   2 4  3.92       'OIL'  3*  NO /
     'W7'    'G3'   3 2  3.92       'OIL'  3*  NO /
/
VFPINJ
-- Table Depth  Rate   TAB  UNITS  BODY
-- ----- ----- ----- ----- ------ -----
       5  32.9   WAT   THP METRIC   BHP /
-- Rate axis
1 3 5 /
-- THP axis
7 11 /
-- Table data with THP# <values 1-num_rates>
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /

WCONINJE
  'W1' 'WATER'  'OPEN'  'GRUP' /
  'W2' 'GAS'  'OPEN'  'RATE'  200  1*  450.0 /
  'W3' 'OIL'  'OPEN'  'RATE'  200  1*  450.0 /
  'W4' 'WATER'  'OPEN'  'RATE'  200  1*  450.0 /
  'W5' 'WATER'  'OPEN'  'RESV'  200  175  450.0 /
  'W6' 'GAS'  'OPEN'  'BHP'  200  1*  450.0 /
  'W7' 'GAS'  'OPEN'  'THP'  200  1*  450.0 150 5 /
/

TSTEP
  30*30 /

END
)";

    const auto sched = make_schedule(input);
    const auto st = ::Opm::SummaryState{ TimeService::now(), 0.0 };

    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W1", 10), st), -1);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W2", 10), st), 3);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W3", 10), st), 1);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W4", 10), st), 2);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W5", 10), st), 5);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W6", 10), st), 7);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W7", 10), st), 6);
}

BOOST_AUTO_TEST_CASE(Production_Control_Mode_From_Well) {
    const auto input = R"(RUNSPEC

SCHEDULE
VFPPROD
-- table_num, datum_depth, flo, wfr, gfr, pressure, alq, unit, table_vals
42 7.0E+03 LIQ WCT GOR THP ' ' METRIC BHP /
1.0 / flo axis
0.0 1.0 / THP axis
0.0 / WFR axis
0.0 / GFR axis
0.0 / ALQ axis
-- Table itself: thp_idx wfr_idx gfr_idx alq_idx <vals>
1 1 1 1 0.0 /
2 1 1 1 1.0 /

WELSPECS
     'W1'    'G1'   1 2  3.33       'OIL'  7*/
     'W2'    'G2'   1 3  3.33       'OIL'  3*  YES /
     'W3'    'G3'   1 4  3.92       'OIL'  3*  NO /
     'W4'    'G3'   2 2  3.92       'OIL'  3*  NO /
     'W5'    'G3'   2 3  3.92       'OIL'  3*  NO /
     'W6'    'G3'   2 4  3.92       'OIL'  3*  NO /
     'W7'    'G3'   3 2  3.92       'OIL'  3*  NO /
     'W8'    'G3'   3 3  3.92       'OIL'  3*  NO /
/

WCONPROD
  'W1' 'OPEN'  'GRUP' /
  'W2' 'OPEN'  'ORAT' 1000.0 /
  'W3' 'OPEN'  'WRAT' 1000.0 250.0 /
  'W4' 'OPEN'  'GRAT' 1000.0 250.0 30.0e3 /
  'W5' 'OPEN'  'LRAT' 1000.0 250.0 30.0e3 1500.0 /
  'W6' 'OPEN'  'RESV' 1000.0 250.0 30.0e3 1500.0 314.15 /
  'W7' 'OPEN'  'BHP' 1000.0 250.0 30.0e3 1500.0 314.15 27.1828 /
  'W8' 'OPEN'  'THP' 1000.0 250.0 30.0e3 1500.0 314.15 27.1828 31.415 42 /
/

TSTEP
  30*30 /

END
)";

    const auto sched = make_schedule(input);
    const auto st = ::Opm::SummaryState{ TimeService::now(), 0.0 };

    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W1", 10), st), -1);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W2", 10), st), 1);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W3", 10), st), 2);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W4", 10), st), 3);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W5", 10), st), 4);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W6", 10), st), 5);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W7", 10), st), 7);
    BOOST_CHECK_EQUAL(Well::eclipseControlMode(sched.getWell("W8", 10), st), 6);
}

BOOST_AUTO_TEST_CASE(GASLIFT_OPT) {
    GasLiftOpt glo{};
    BOOST_CHECK(!glo.active());
    BOOST_CHECK_THROW(glo.group("NO_SUCH_GROUP"), std::out_of_range);
    BOOST_CHECK_THROW(glo.well("NO_SUCH_WELL"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(GASLIFT_OPT_DECK) {
    const auto input = R"(-- Turns on gas lift optimization
SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
 /

LIFTOPT
 12500 5E-3 0.0 YES /


-- Group lift gas limits for gas lift optimization
GLIFTOPT
 'PLAT-A'  200000 /  --
/

WELSPECS
--WELL     GROUP  IHEEL JHEEL   DREF PHASE   DRAD INFEQ SIINS XFLOW PRTAB  DENS
 'B-1H'  'B1'   11    3      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'B-2H'  'B1'    4    7      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'B-3H'  'B1'   11   12      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'C-1H'  'C1'   13   20      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'C-2H'  'C1'   12   27      1*   OIL     1*   1*   SHUT 1* 1* 1* /
/

-- well savailable for gass lift
-- minimum gas lift rate, enough to keep well flowing
WLIFTOPT
 'B-1H'   YES   150000   1.01   -1.0  /
 'B-2H'   YES   150000   1.01   -1.0  /
 'B-3H'   YES   150000   1.01   -1.0  /
 'C-1H'   YES   150000   1.01   -1.0  1.0 YES/
 'C-2H'   NO    150000   1.01   -1.0  /
/
)";

    Opm::UnitSystem unitSystem = UnitSystem( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    double siFactorG = unitSystem.parse("GasSurfaceVolume/Time").getSIScaling();
    const auto sched = make_schedule(input);
    const auto& glo = sched.glo(0);
    const auto& plat_group = glo.group("PLAT-A");
    BOOST_CHECK_EQUAL( *plat_group.max_lift_gas(), siFactorG * 200000);
    BOOST_CHECK(!plat_group.max_total_gas().has_value());
    BOOST_CHECK(glo.has_group("PLAT-A"));
    BOOST_CHECK(!glo.has_well("NO-GROUP"));


    const auto& w1 = glo.well("B-1H");
    BOOST_CHECK(w1.use_glo());
    BOOST_CHECK_EQUAL(*w1.max_rate(), 150000 * siFactorG);
    BOOST_CHECK_EQUAL(w1.weight_factor(), 1.01);

    const auto& w2 = glo.well("C-2H");
    BOOST_CHECK_EQUAL(w2.weight_factor(), 1.00);
    BOOST_CHECK_EQUAL(w2.min_rate(), 0.00);
    BOOST_CHECK_EQUAL(w2.inc_weight_factor(), 0.00);
    BOOST_CHECK(!w2.alloc_extra_gas());

    const auto& w3 = glo.well("C-1H");
    BOOST_CHECK_EQUAL(w3.min_rate(), -1.00 * siFactorG);
    BOOST_CHECK_EQUAL(w3.inc_weight_factor(), 1.00);
    BOOST_CHECK(w3.alloc_extra_gas());
    BOOST_CHECK(glo.has_well("C-1H"));
    BOOST_CHECK(!glo.has_well("NO-WELL"));
}

BOOST_AUTO_TEST_CASE(WellPI) {
    const auto deck = Parser{}.parseString(R"(RUNSPEC
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
  'P' 0 0 1 3 OPEN 1 100 /
/

TSTEP
  10
/

WELPI
  'P'  200.0 /
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

    const auto es    = EclipseState{ deck };
    const auto sched = Schedule{ deck, es };

    // Apply WELPI before seeing WELPI data
    {
        const auto expectCF = 100.0*cp_rm3_per_db();
        auto wellP = sched.getWell("P", 0);

        std::vector<bool> scalingApplicable;
        wellP.applyWellProdIndexScaling(2.7182818, scalingApplicable);
        for (const auto& conn : wellP.getConnections()) {
            BOOST_CHECK_CLOSE(conn.CF(), expectCF, 1.0e-10);
        }

        for (const bool applicable : scalingApplicable) {
            BOOST_CHECK_MESSAGE(! applicable, "No connection must be eligible for WELPI scaling");
        }
    }

    // Apply WELPI after seeing WELPI data.
    {
        const auto expectCF = (200.0 / 100.0) * 100.0*cp_rm3_per_db();
        auto wellP = sched.getWell("P", 1);

        const auto scalingFactor = wellP.convertDeckPI(200) / (100.0*liquid_PI_unit());
        BOOST_CHECK_CLOSE(scalingFactor, 2.0, 1.0e-10);

        std::vector<bool> scalingApplicable;
        wellP.applyWellProdIndexScaling(scalingFactor, scalingApplicable);
        for (const auto& conn : wellP.getConnections()) {
            BOOST_CHECK_CLOSE(conn.CF(), expectCF, 1.0e-10);
        }

        for (const bool applicable : scalingApplicable) {
            BOOST_CHECK_MESSAGE(applicable, "All connections must be eligible for WELPI scaling");
        }
    }

    // Apply WELPI after new COMPDAT.
    {
        const auto expectCF = (200.0 / 100.0) * 100.0*cp_rm3_per_db();
        auto wellP = sched.getWell("P", 2);

        const auto scalingFactor = wellP.convertDeckPI(200) / (100.0*liquid_PI_unit());
        BOOST_CHECK_CLOSE(scalingFactor, 2.0, 1.0e-10);

        std::vector<bool> scalingApplicable;
        wellP.applyWellProdIndexScaling(scalingFactor, scalingApplicable);
        const auto& connP = wellP.getConnections();
        BOOST_CHECK_CLOSE(connP[0].CF(), expectCF          , 1.0e-10);
        BOOST_CHECK_CLOSE(connP[1].CF(), 50*cp_rm3_per_db(), 1.0e-10);
        BOOST_CHECK_CLOSE(connP[2].CF(), expectCF          , 1.0e-10);

        BOOST_CHECK_MESSAGE(bool(scalingApplicable[0]), "Connection[0] must be eligible for WELPI scaling");
        BOOST_CHECK_MESSAGE(!    scalingApplicable[1] , "Connection[1] must NOT be eligible for WELPI scaling");
        BOOST_CHECK_MESSAGE(bool(scalingApplicable[0]), "Connection[2] must be eligible for WELPI scaling");
    }

    {
        const auto& target_wellpi = sched[1].target_wellpi;
        BOOST_CHECK_EQUAL( target_wellpi.size(), 1);
        BOOST_CHECK_EQUAL( target_wellpi.count("P"), 1);
    }
    {
        const auto& target_wellpi = sched[2].target_wellpi;
        BOOST_CHECK_EQUAL( target_wellpi.size(), 0);
    }
}

BOOST_AUTO_TEST_CASE(Schedule_ApplyWellProdIndexScaling) {
    const auto deck = Parser{}.parseString(R"(RUNSPEC
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
WELSPECS -- 0
  'P' 'G' 10 10 2005 'LIQ' /
/
COMPDAT
  'P' 0 0 1 3 OPEN 1 100 /
/

TSTEP -- 1
  10
/

WELPI -- 1
  'P'  200.0 /
/

TSTEP -- 2
  10
/

COMPDAT -- 2
  'P' 0 0 2 2 OPEN 1 50 /
/

TSTEP -- 3
  10
/

WELPI --3
  'P'  50.0 /
/

TSTEP -- 4
  10
/

COMPDAT -- 4
  'P' 10 9 2 2 OPEN 1 100 1.0 3* 'Y' /
  'P' 10 8 2 2 OPEN 1  75 1.0 3* 'Y' /
  'P' 10 7 2 2 OPEN 1  25 1.0 3* 'Y' /
/

TSTEP -- 5
  10
/

END
)");

    const auto es    = EclipseState{ deck };
    auto       sched = Schedule{ deck, es };

    BOOST_REQUIRE_EQUAL(sched.size(),         std::size_t{6});
    //BOOST_REQUIRE_EQUAL(sched.getTimeMap().numTimesteps(), std::size_t{5});
    //BOOST_REQUIRE_EQUAL(sched.getTimeMap().last(),         std::size_t{5});

    BOOST_REQUIRE_MESSAGE(sched[1].wellgroup_events().hasEvent("P", ScheduleEvents::Events::WELL_PRODUCTIVITY_INDEX),
                          R"(Schedule must have WELL_PRODUCTIVITY_INDEX Event for well "P" at report step 1)");

    BOOST_REQUIRE_MESSAGE(sched[3].wellgroup_events().hasEvent("P", ScheduleEvents::Events::WELL_PRODUCTIVITY_INDEX),
                          R"(Schedule must have WELL_PRODUCTIVITY_INDEX Event for well "P" at report step 3)");

    BOOST_REQUIRE_MESSAGE(sched[1].events().hasEvent(ScheduleEvents::Events::WELL_PRODUCTIVITY_INDEX),
                          "Schedule must have WELL_PRODUCTIVITY_INDEX Event at report step 1");

    BOOST_REQUIRE_MESSAGE(sched[3].events().hasEvent(ScheduleEvents::Events::WELL_PRODUCTIVITY_INDEX),
                          "Schedule must have WELL_PRODUCTIVITY_INDEX Event at report step 3");

    auto getScalingFactor = [&sched](const std::size_t report_step, double targetPI, const double wellPI) -> double
    {
        return sched.getWell("P", report_step).convertDeckPI(targetPI) / wellPI;
    };

    auto applyWellPIScaling = [&sched](const std::size_t report_step, const double newWellPI)
    {
        sched.applyWellProdIndexScaling("P", report_step, newWellPI);
    };

    auto getConnections = [&sched](const std::size_t report_step)
    {
        return sched.getWell("P", report_step).getConnections();
    };

    // Apply WELPI scaling after end of time series => no change to CTFs
    {
        const auto report_step   = std::size_t{1};
        const auto scalingFactor = getScalingFactor(report_step, sched[report_step].target_wellpi.at("P"), 100.0*liquid_PI_unit());

        BOOST_CHECK_CLOSE(scalingFactor, 2.0, 1.0e-10);

        applyWellPIScaling(1729, scalingFactor);

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(0);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(1);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(2);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,             1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,             1.0e-10);
        }

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(3);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,             1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,             1.0e-10);
        }

        {
            const auto& conns = getConnections(4);
            BOOST_REQUIRE_EQUAL(conns.size(), 6);

            BOOST_CHECK_CLOSE(conns[0].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(),  50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[3].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[4].CF(),  75.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[5].CF(),  25.0*cp_rm3_per_db(), 1.0e-10);
        }
    }

    // Apply WELPI scaling after first WELPI specification
    {
        const auto report_step   = std::size_t{1};
        const auto newWellPI     = 100.0*liquid_PI_unit();

        applyWellPIScaling(report_step, newWellPI);

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(0);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(1);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(2);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,             1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,             1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(3);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,             1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,             1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(4);
            BOOST_REQUIRE_EQUAL(conns.size(), 6);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,              1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(),  50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,              1.0e-10);
            BOOST_CHECK_CLOSE(conns[3].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[4].CF(),  75.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[5].CF(),  25.0*cp_rm3_per_db(), 1.0e-10);
        }
    }

    // Apply WELPI scaling after second WELPI specification
    {
        const auto report_step   = std::size_t{3};
        double newWellPI = 200*liquid_PI_unit();

        applyWellPIScaling(report_step, newWellPI);

        {
            const auto expectCF = 100.0*cp_rm3_per_db();

            const auto& conns = getConnections(0);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(1);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF, 1.0e-10);
        }

        {
            const auto expectCF = 200.0*cp_rm3_per_db();

            const auto& conns = getConnections(2);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,             1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 50.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,             1.0e-10);
        }

        {
            const auto expectCF = 50.0*cp_rm3_per_db();

            const auto& conns = getConnections(3);
            BOOST_REQUIRE_EQUAL(conns.size(), 3);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,      1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 0.25*expectCF, 1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,      1.0e-10);
        }

        {
            const auto expectCF = 50.0*cp_rm3_per_db();

            const auto& conns = getConnections(4);
            BOOST_REQUIRE_EQUAL(conns.size(), 6);

            BOOST_CHECK_CLOSE(conns[0].CF(), expectCF,              1.0e-10);
            BOOST_CHECK_CLOSE(conns[1].CF(), 0.25*expectCF,         1.0e-10);
            BOOST_CHECK_CLOSE(conns[2].CF(), expectCF,              1.0e-10);
            BOOST_CHECK_CLOSE(conns[3].CF(), 100.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[4].CF(),  75.0*cp_rm3_per_db(), 1.0e-10);
            BOOST_CHECK_CLOSE(conns[5].CF(),  25.0*cp_rm3_per_db(), 1.0e-10);
        }
    }
}

namespace {

void cmp_vector(const std::vector<double>&v1, const std::vector<double>& v2) {
    BOOST_CHECK_EQUAL(v1.size(), v2.size());
    for (std::size_t i = 0; i < v1.size(); i++)
        BOOST_CHECK_CLOSE(v1[i], v2[i], 1e-4);
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(VFPPROD_SCALING) {
    const auto deck = Parser{}.parseFile("VFP_CASE.DATA");
    const auto es    = EclipseState{ deck };
    const auto sched = Schedule{ deck, es };
    const auto& vfp_table = sched[0].vfpprod(1);
    const std::vector<double> flo = { 0.000578704,  0.001157407,  0.002893519,  0.005787037,  0.008680556,  0.011574074,  0.017361111,  0.023148148,  0.034722222,  0.046296296};
    const std::vector<double> thp = {1300000.000000000, 2500000.000000000, 5000000.000000000, 7500000.000000000, 10000000.000000000};
    const std::vector<double> wfr = { 0.000000000,  0.100000000,  0.200000000,  0.300000000,  0.400000000,  0.500000000,  0.600000000,  0.700000000,  0.800000000,  0.990000000};
    const std::vector<double> gfr = {100.000000000, 200.000000000, 300.000000000, 400.000000000, 500.000000000, 750.000000000, 1000.000000000, 2000.000000000};
    const std::vector<double> alq = { 0.000000000,  50.000000000,  100.000000000,  150.000000000,  200.000000000};

    cmp_vector(flo, vfp_table.getFloAxis());
    cmp_vector(thp, vfp_table.getTHPAxis());
    cmp_vector(wfr, vfp_table.getWFRAxis());
    cmp_vector(gfr, vfp_table.getGFRAxis());
    cmp_vector(alq, vfp_table.getALQAxis());

    for (std::size_t index = 0; index < sched.size(); index++) {
        const auto& state = sched[index];
        BOOST_CHECK_EQUAL(index, state.sim_step());
    }
}


BOOST_AUTO_TEST_CASE(WPAVE) {
    const std::string deck_string = R"(
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

PORO
  300*0.3 /

SCHEDULE
WELSPECS -- 0
  'P1' 'G' 10 10 2005 'LIQ' /
  'P2' 'G' 1 10 2005 'LIQ' /
  'P3' 'G' 2 10 2005 'LIQ' /
  'P4' 'G' 3 10 2005 'LIQ' /
/


TSTEP -- 1
  10
/


WPAVE   -- PAVG1
  0.75 0.25 /


TSTEP -- 2
  10
/

WWPAVE
  P1 0.30 0.60 /   -- PAVG2
  P3 0.40 0.70 /   -- PAVG3
/


TSTEP -- 3
  10
/

WPAVE   -- PAVG4
  0.10 0.10 /


TSTEP -- 4
  10
/

TSTEP -- 5
  10
/

END
)";

    const auto deck = Parser{}.parseString(deck_string);
    const auto es    = EclipseState{ deck };
    auto       sched = Schedule{ deck, es };

    PAvg pavg0;
    PAvg pavg1( deck["WPAVE"][0].getRecord(0) );
    PAvg pavg2( deck["WWPAVE"][0].getRecord(0) );
    PAvg pavg3( deck["WWPAVE"][0].getRecord(1) );
    PAvg pavg4( deck["WPAVE"][1].getRecord(0) );

    {
        const auto& w1 = sched.getWell("P1", 0);
        const auto& w4 = sched.getWell("P4", 0);

        BOOST_CHECK(w1.pavg() == pavg0);
        BOOST_CHECK(w4.pavg() == pavg0);
    }

    {
        const auto& w1 = sched.getWell("P1", 1);
        const auto& w4 = sched.getWell("P4", 1);

        BOOST_CHECK(w1.pavg() == pavg1);
        BOOST_CHECK(w4.pavg() == pavg1);
    }

    {
        const auto& w1 = sched.getWell("P1", 2);
        const auto& w3 = sched.getWell("P3", 2);
        const auto& w4 = sched.getWell("P4", 2);

        BOOST_CHECK(w1.pavg() == pavg2);
        BOOST_CHECK(w3.pavg() == pavg3);
        BOOST_CHECK(w4.pavg() == pavg1);
    }

    {
        const auto& w1 = sched.getWell("P1", 3);
        const auto& w2 = sched.getWell("P2", 3);
        const auto& w3 = sched.getWell("P3", 3);
        const auto& w4 = sched.getWell("P4", 3);

        BOOST_CHECK(w1.pavg() == pavg4);
        BOOST_CHECK(w2.pavg() == pavg4);
        BOOST_CHECK(w3.pavg() == pavg4);
        BOOST_CHECK(w4.pavg() == pavg4);
    }
}


BOOST_AUTO_TEST_CASE(WELL_STATUS) {
    const std::string deck_string = R"(
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

PORO
  300*0.3 /
PERMX
    300*1 /
PERMY
    300*0.1 /
PERMZ
    300*0.01 /

SCHEDULE
WELSPECS -- 0
  'P1' 'G' 10 10 2005 'LIQ' /
/

COMPDAT
  'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

WCONPROD
  'P1' 'OPEN' 'ORAT'  123.4  4*  50.0 /
/


TSTEP -- 1
  10 /

NETBALAN
  1 2 3 4 5 6 7 8 /

WELPI
  'P1'  200.0 /
/

TSTEP -- 2
  10 /

WELOPEN
   'P1' SHUT /
/

TSTEP -- 3,4,5
  10 10 10 /

WELOPEN
   'P1' OPEN /
/

TSTEP -- 6,7,8
  10 10 10/

END

END
)";

    const auto deck = Parser{}.parseString(deck_string);
    const auto es    = EclipseState{ deck };
    auto       sched = Schedule{ deck, es };
    {
        const auto& well = sched.getWell("P1", 0);
        BOOST_CHECK( well.getStatus() ==  Well::Status::OPEN);
    }
    {
        const auto& well = sched.getWell("P1", 1);
        BOOST_CHECK( well.getStatus() ==  Well::Status::OPEN);
    }

    {
        const auto& well = sched.getWell("P1", 2);
        BOOST_CHECK( well.getStatus() ==  Well::Status::SHUT);
    }
    {
        const auto& well = sched.getWell("P1", 5);
        BOOST_CHECK( well.getStatus() ==  Well::Status::OPEN);
    }

    sched.shut_well("P1", 0);


    auto netbalan0 = sched[0].network_balance();
    BOOST_CHECK(netbalan0.mode() == Network::Balance::CalcMode::TimeStepStart);

    auto netbalan1 = sched[1].network_balance();
    BOOST_CHECK(netbalan1.mode() == Network::Balance::CalcMode::TimeInterval);
    BOOST_CHECK_EQUAL( netbalan1.interval(), 86400 );
    BOOST_CHECK_EQUAL( netbalan1.pressure_tolerance(), 200000 );
    BOOST_CHECK_EQUAL( netbalan1.pressure_max_iter(), 3 );
    BOOST_CHECK_EQUAL( netbalan1.thp_tolerance(), 4 );
    BOOST_CHECK_EQUAL( netbalan1.thp_max_iter(), 5 );
}

namespace {

bool compare_dates(const time_point& t, int year, int month, int day) {
    return t == TimeService::from_time_t( asTimeT( TimeStampUTC(year, month, day)));
}

bool compare_dates(const time_point& t, const std::array<int, 3>& ymd)
{
    return compare_dates(t, ymd[0], ymd[1], ymd[2]);
}

std::string dates_msg(const time_point& t, std::array<int,3>& ymd) {
    auto ts = TimeStampUTC( std::chrono::system_clock::to_time_t(t) );
    return fmt::format("Different dates: {}-{}-{} != {}-{}-{}", ts.year(), ts.month(), ts.day(), ymd[0], ymd[1], ymd[2]);
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(ScheduleStateDatesTest) {
    const auto& sched = make_schedule(createDeckWTEST());
    BOOST_CHECK_EQUAL(sched.size(), 6);
    BOOST_CHECK( compare_dates(sched[0].start_time(), 2007, 5, 10 ));
    BOOST_CHECK( compare_dates(sched[0].end_time(), 2007, 6, 10 ));

    BOOST_CHECK( compare_dates(sched[1].start_time(), 2007, 6, 10));
    BOOST_CHECK( compare_dates(sched[1].end_time(), 2007, 7, 10 ));

    BOOST_CHECK( compare_dates(sched[2].start_time(), 2007, 7, 10));
    BOOST_CHECK( compare_dates(sched[2].end_time(), 2007, 8, 10 ));

    BOOST_CHECK( compare_dates(sched[3].start_time(), 2007, 8, 10));
    BOOST_CHECK( compare_dates(sched[3].end_time(), 2007, 9, 10 ));

    BOOST_CHECK( compare_dates(sched[4].start_time(), 2007, 9, 10));
    BOOST_CHECK( compare_dates(sched[4].end_time(), 2007, 11, 10 ));

    BOOST_CHECK( compare_dates(sched[5].start_time(), 2007, 11, 10));
    BOOST_CHECK_THROW( sched[5].end_time(), std::exception );
}


BOOST_AUTO_TEST_CASE(ScheduleStateTest) {
    auto t1 = TimeService::from_time_t( std::chrono::system_clock::to_time_t( TimeService::now() ) );
    auto t2 = t1 + std::chrono::hours(48);

    ScheduleState ts1(t1);
    BOOST_CHECK( t1 == ts1.start_time() );
    BOOST_CHECK_THROW( ts1.end_time(), std::exception );

    ScheduleState ts2(t1, t2);
    BOOST_CHECK( t1 == ts2.start_time() );
    BOOST_CHECK( t2 == ts2.end_time() );
}

BOOST_AUTO_TEST_CASE(ScheduleDeckTest) {
    {
        ScheduleDeck sched_deck;
        BOOST_CHECK_EQUAL( sched_deck.size(), 1 );
        BOOST_CHECK_THROW( sched_deck[1], std::exception );
        const auto& block = sched_deck[0];
        BOOST_CHECK_EQUAL( block.size(), 0 );
    }
    {
        Parser parser;
        auto deck = parser.parseString( createDeckWTEST() );
        Runspec runspec{deck};
        ScheduleDeck sched_deck( TimeService::from_time_t(runspec.start_time()), deck, {} );
        BOOST_CHECK_EQUAL( sched_deck.size(), 6 );

        std::vector<std::string> first_kw = {"WELSPECS", "WTEST", "SUMTHIN", "WCONINJH", "WELOPEN", "WCONINJH"};
        std::vector<std::string> last_kw = {"WTEST", "WCONHIST", "WCONPROD", "WCONINJH", "WELOPEN", "WCONINJH"};
        std::vector<std::array<int,3>> start_time = {{2007, 5, 10},
                                                     {2007, 6, 10},
                                                     {2007, 7, 10},
                                                     {2007, 8, 10},
                                                     {2007, 9, 10},
                                                     {2007, 11,10}};

        for (std::size_t block_index = 0; block_index < sched_deck.size(); block_index++) {
            const auto& block = sched_deck[block_index];
            for (const auto& kw : block) {
                (void) kw;
            }
            BOOST_CHECK_EQUAL( block[0].name(), first_kw[block_index]);
            BOOST_CHECK_EQUAL( block[block.size() - 1].name(), last_kw[block_index]);
            BOOST_CHECK_MESSAGE( compare_dates(block.start_time(), start_time[block_index]), dates_msg(block.start_time(), start_time[block_index]));
        }
        {
            const auto& block = sched_deck[0];
            auto poro = block.get("PORO");
            BOOST_CHECK_MESSAGE(!poro, "The block does not have a PORO keyword and block.get(\"PORO\") should evaluate to false");

            auto welspecs = block.get("WELSPECS");
            BOOST_CHECK_MESSAGE(welspecs.has_value(), "The block contains a WELSPECS keyword and block.get(\"WELSPECS\") should evaluate to true");
        }
    }
}



BOOST_AUTO_TEST_CASE(WCONPROD_UDA) {
    const std::string deck_string = R"(
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

PORO
  300*0.3 /
PERMX
    300*1 /
PERMY
    300*0.1 /
PERMZ
    300*0.01 /

SCHEDULE

VFPPROD
-- table_num, datum_depth, flo, wfr, gfr, pressure, alq, unit, table_vals
42 7.0E+03 LIQ WCT GOR THP ' ' METRIC BHP /
1.0 / flo axis
0.0 1.0 / THP axis
0.0 / WFR axis
0.0 / GFR axis
0.0 / ALQ axis
-- Table itself: thp_idx wfr_idx gfr_idx alq_idx <vals>
1 1 1 1 0.0 /
2 1 1 1 1.0 /

VFPPROD
-- table_num, datum_depth, flo, wfr, gfr, pressure, alq, unit, table_vals
43 7.0E+03 LIQ WCT GOR THP 'GRAT' METRIC BHP /
1.0 / flo axis
0.0 1.0 / THP axis
0.0 / WFR axis
0.0 / GFR axis
0.0 / ALQ axis
-- Table itself: thp_idx wfr_idx gfr_idx alq_idx <vals>
1 1 1 1 0.0 /
2 1 1 1 1.0 /

WELSPECS -- 0
  'P1' 'G' 10 10 2005 'LIQ' /
  'P2' 'G' 10 10 2005 'LIQ' /
/

COMPDAT
  'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
  'P2'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

UDQ
ASSIGN FU_GAS 10000 /
/

WCONPROD
  'P1' 'OPEN' 'ORAT'  123.4  0.0  0.0  0.0  0.0 100 100 42 'FU_GAS' /
  'P2' 'OPEN' 'ORAT'  123.4  0.0  0.0  0.0  0.0 100 100 43 'FU_GAS' /
/

)";
    const auto deck = Parser{}.parseString(deck_string);
    const auto es    = EclipseState{ deck };
    auto       sched = Schedule{ deck, es };
    const auto& well1 = sched.getWell("P1", 0);
    const auto& well2 = sched.getWell("P2", 0);
    SummaryState st(TimeService::now(), 0.0);

    st.update("FU_GAS", 123);
    const auto& controls1 = well1.productionControls(st);
    BOOST_CHECK_EQUAL(controls1.alq_value, 123);

    auto dim = sched.getUnits().getDimension(UnitSystem::measure::gas_surface_rate);
    const auto& controls2 = well2.productionControls(st);
    BOOST_CHECK_CLOSE(controls2.alq_value, dim.convertRawToSi(123), 1e-13);

    BOOST_CHECK(!sched[0].has_gpmaint());
}

BOOST_AUTO_TEST_CASE(WCONHIST_WCONINJH_VFP) {
    const std::string deck_string = R"(
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

PORO
  300*0.3 /
PERMX
    300*1 /
PERMY
    300*0.1 /
PERMZ
    300*0.01 /

SCHEDULE

VFPPROD
-- table_num, datum_depth, flo, wfr, gfr, pressure, alq, unit, table_vals
42 7.0E+03 LIQ WCT GOR THP ' ' METRIC BHP /
1.0 / flo axis
0.0 1.0 / THP axis
0.0 / WFR axis
0.0 / GFR axis
0.0 / ALQ axis
-- Table itself: thp_idx wfr_idx gfr_idx alq_idx <vals>
1 1 1 1 0.0 /
2 1 1 1 1.0 /

VFPPROD
-- table_num, datum_depth, flo, wfr, gfr, pressure, alq, unit, table_vals
43 7.0E+03 LIQ WCT GOR THP 'GRAT' METRIC BHP /
1.0 / flo axis
0.0 1.0 / THP axis
0.0 / WFR axis
0.0 / GFR axis
0.0 / ALQ axis
-- Table itself: thp_idx wfr_idx gfr_idx alq_idx <vals>
1 1 1 1 0.0 /
2 1 1 1 1.0 /

VFPINJ
-- Table Depth  Rate   TAB  UNITS  BODY
-- ----- ----- ----- ----- ------ -----
       5  32.9   WAT   THP METRIC   BHP /
-- Rate axis
1 3 5 /
-- THP axis
7 11 /
-- Table data with THP# <values 1-num_rates>
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /

WELSPECS -- 0
  'P1' 'G' 10 10 2005 'LIQ' /
  'P2' 'G' 10 10 2005 'LIQ' /
/

COMPDAT
  'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
  'P2'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

WCONHIST
  'P1' 'OPEN' 'RESV'  0.0 0.0  0.0  42 10/
  'P2' 'OPEN' 'RESV'  0.0 0.0  0.0  43 100/
/

TSTEP
 1/

WCONHIST
  'P1' 'OPEN' 'RESV'  0.0 0.0  0.0  1* 20/
  'P2' 'OPEN' 'RESV'  0.0 0.0  0.0  0 200/
/

TSTEP
 1/

WCONINJH
  'P1' 'WAT' 'OPEN'  0.0 2* 1*/
  'P2' 'WAT' 'OPEN'  0.0 2* 5 /
/

TSTEP
 1/

WCONINJH
  'P1' 'WAT' 'OPEN'  0.0 2* 0 /
  'P2' 'WAT' 'OPEN'  0.0 2* 1* /
/
)";
    const auto deck = Parser{}.parseString(deck_string);
    const auto es    = EclipseState{ deck };
    auto       sched = Schedule{ deck, es };

    // step 0
    {
        const auto& well1 = sched.getWell("P1", 0);
        const auto& well2 = sched.getWell("P2", 0);
        BOOST_CHECK_EQUAL(well1.vfp_table_number(), 42);
        BOOST_CHECK_EQUAL(well2.vfp_table_number(), 43);
    }

    // step 1
    {
        const auto& well1 = sched.getWell("P1", 1);
        const auto& well2 = sched.getWell("P2", 1);
        BOOST_CHECK_EQUAL(well1.vfp_table_number(), 42);
        BOOST_CHECK_EQUAL(well2.vfp_table_number(), 0);
    }

    // step 2
    {
        const auto& well1 = sched.getWell("P1", 2);
        const auto& well2 = sched.getWell("P2", 2);
        BOOST_CHECK_EQUAL(well1.vfp_table_number(), 0);
        BOOST_CHECK_EQUAL(well2.vfp_table_number(), 5);
    }

    // step 3
    {
        const auto& well1 = sched.getWell("P1", 3);
        const auto& well2 = sched.getWell("P2", 3);
        BOOST_CHECK_EQUAL(well1.vfp_table_number(), 0);
        BOOST_CHECK_EQUAL(well2.vfp_table_number(), 5);
    }
}

BOOST_AUTO_TEST_CASE(SUMTHIN_IN_SUMMARY) {
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

SUMMARY
SUMTHIN
10.0 /

SCHEDULE
WELSPECS
     'W_1'  'OP'   30   37  3.33 'OIL'  7* /
/
DATES             -- 1, 2, 3
  10  'JUN'  2007 /
  10  JLY 2007 /
  10  AUG 2007 /
/
SUMTHIN
100.0 /
WELSPECS
     'WX2'        'OP'   30   37  3.33       'OIL'  7* /
     'W_3'        'OP'   20   51  3.92       'OIL'  7* /
/
DATES             -- 4,5
  10  SEP 2007 /
  10  OCT 2007 /
/
SUMTHIN
0.0 /
DATES             -- 6,7
  10  NOV 2007 /
  10  DEC 2007 /
/
END
)");

    const auto es = EclipseState { deck };
    const auto sched = Schedule { deck, es, std::make_shared<const Python>() };

    BOOST_REQUIRE_MESSAGE(sched[0].sumthin().has_value(),
                          R"("SUMTHIN" must be configured on report step 1)");

    BOOST_CHECK_CLOSE(sched[0].sumthin().value(), 10.0 * 86'400.0, 1.0e-10);

    BOOST_REQUIRE_MESSAGE(sched[1].sumthin().has_value(),
                          R"("SUMTHIN" must be configured on report step 2)");

    BOOST_CHECK_CLOSE(sched[1].sumthin().value(), 10.0 * 86'400.0, 1.0e-10);

    BOOST_REQUIRE_MESSAGE(sched[2].sumthin().has_value(),
                          R"("SUMTHIN" must be configured on report step 3)");

    BOOST_CHECK_CLOSE(sched[2].sumthin().value(), 10.0 * 86'400.0, 1.0e-10);

    BOOST_REQUIRE_MESSAGE(sched[3].sumthin().has_value(),
                          R"("SUMTHIN" must be configured on report step 4)");

    BOOST_CHECK_CLOSE(sched[3].sumthin().value(), 100.0 * 86'400.0, 1.0e-10);

    BOOST_REQUIRE_MESSAGE(sched[4].sumthin().has_value(),
                          R"("SUMTHIN" must be configured on report step 5)");

    BOOST_CHECK_CLOSE(sched[4].sumthin().value(), 100.0 * 86'400.0, 1.0e-10);

    BOOST_REQUIRE_MESSAGE(!sched[5].sumthin().has_value(),
                          R"("SUMTHIN" must NOT be configured on report step 6)");

    BOOST_CHECK_THROW(sched[5].sumthin().value(), std::bad_optional_access);

    BOOST_REQUIRE_MESSAGE(!sched[6].sumthin().has_value(),
                          R"("SUMTHIN" must NOT be configured on report step 7)");
}


BOOST_AUTO_TEST_CASE(MISORDERD_DATES) {
const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

SCHEDULE
DATES             -- 1, 2, 3
  10  JUN  2007 /
  10  MAY 2007 /
  10  AUG 2007 /
/
END
)");

const auto es = EclipseState { deck };
BOOST_CHECK_THROW(Schedule(deck, es), Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(NEGATIVE_TSTEPS) {
const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

SCHEDULE
DATES             -- 1, 2, 3
  10  MAY 2007 /
  10  JUN  2007 /
  10  AUG 2007 /
/
TSTEP
-1 /
END
)");

const auto es = EclipseState { deck };
BOOST_CHECK_THROW(Schedule(deck, es), Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(RPTONLY_IN_SUMMARY) {
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

SUMMARY
RPTONLY

SCHEDULE
WELSPECS
     'W_1'  'OP'   30   37  3.33 'OIL'  7* /
/
DATES             -- 1, 2
  10  'JUN'  2007 /
  10  JLY 2007 /
/
WELSPECS
     'WX2'        'OP'   30   37  3.33       'OIL'  7* /
     'W_3'        'OP'   20   51  3.92       'OIL'  7* /
/
RPTONLYO
DATES             -- 3, 4
  10  AUG 2007 /
  10  SEP 2007 /
/
END
)");

    const auto es = EclipseState { deck };
    const auto sched = Schedule { deck, es, std::make_shared<const Python>() };

    BOOST_CHECK_MESSAGE(sched[0].rptonly(),
                        R"("RPTONLY" must be configured on report step 1)");

    BOOST_CHECK_MESSAGE(sched[1].rptonly(),
                        R"("RPTONLY" must be configured on report step 2)");

    BOOST_CHECK_MESSAGE(! sched[2].rptonly(),
                        R"("RPTONLY" must NOT be configured on report step 3)");

    BOOST_CHECK_MESSAGE(! sched[3].rptonly(),
                        R"("RPTONLY" must NOT be configured on report step 4)");


}

BOOST_AUTO_TEST_CASE(DUMP_DECK) {
    const std::string part1 = R"(
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

SUMMARY
RPTONLY
)";
    const std::string schedule_string = R"(
SCHEDULE
WELSPECS
     'W_1'  'OP'   30   37  3.33 'OIL'  7* /
/
DATES             -- 1, 2
  10  'JUN'  2007 /
  10  JLY 2007 /
/
WELSPECS
     'WX2'        'OP'   30   37  3.33       'OIL'  7* /
     'W_3'        'OP'   20   51  3.92       'OIL'  7* /
/
RPTONLYO
DATES             -- 3, 4
  10  AUG 2007 /
  10  SEP 2007 /
/
END
)";
    WorkArea wa;
    {
        std::ofstream stream{"CASE1.DATA"};
        stream << part1 << std::endl << schedule_string;
    }
    const auto& deck1 = Parser{}.parseFile("CASE1.DATA");
    const auto es1 = EclipseState { deck1 };
    const auto sched1 = Schedule { deck1, es1, std::make_shared<const Python>() };

    {
        std::ofstream stream{"CASE2.DATA"};
        stream << part1 << std::endl << sched1;
    }
    const auto& deck2 = Parser{}.parseFile("CASE2.DATA");
    const auto es2 = EclipseState { deck2 };
    const auto sched2 = Schedule { deck2, es2, std::make_shared<const Python>() };

    // Can not do a full sched == sched2 because the Deck member will have embedded
    // keyword location information.
    for (std::size_t step = 0; step < sched1.size(); step++)
        BOOST_CHECK(sched1[step] == sched2[step]);
}


BOOST_AUTO_TEST_CASE(TestScheduleGrid) {
    EclipseGrid grid(10,10,10);
    CompletedCells cells(grid);
    std::string deck_string = R"(
GRID

PORO
   1000*0.10 /

PERMX
   1000*1 /

PERMY
   1000*0.1 /

PERMZ
   1000*0.01 /


)";
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, TableManager());
    const auto& unit_system = deck.getActiveUnitSystem();

    {
        ScheduleGrid sched_grid(grid, fp, cells);
        const auto& cell = sched_grid.get_cell(1,1,1);
        const auto& props_val = cell.props.value();
        BOOST_CHECK_EQUAL(cell.depth, 1.50);
        BOOST_CHECK_EQUAL(props_val.permx, unit_system.to_si(UnitSystem::measure::permeability, 1));
        BOOST_CHECK_EQUAL(props_val.permy, unit_system.to_si(UnitSystem::measure::permeability, 0.1));
        BOOST_CHECK_EQUAL(props_val.permz, unit_system.to_si(UnitSystem::measure::permeability, 0.01));
    }
    {
        ScheduleGrid sched_grid(cells);
        const auto& cell = sched_grid.get_cell(1,1,1);
        const auto& props_val = cell.props.value();
        BOOST_CHECK_EQUAL(cell.depth, 1.50);
        BOOST_CHECK_EQUAL(props_val.permx, unit_system.to_si(UnitSystem::measure::permeability, 1));
        BOOST_CHECK_EQUAL(props_val.permy, unit_system.to_si(UnitSystem::measure::permeability, 0.1));
        BOOST_CHECK_EQUAL(props_val.permz, unit_system.to_si(UnitSystem::measure::permeability, 0.01));

        BOOST_CHECK_THROW(sched_grid.get_cell(2,2,2), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(Test_wvfpexp) {
        std::string input = R"(
DIMENS
 10 10 10 /

START         -- 0
 19 JUN 2007 /

GRID

DXV
 10*100.0 /
DYV
 10*100.0 /
DZV
 10*10.0 /
DEPTHZ
 121*2000.0 /

SCHEDULE

DATES        -- 1
 10  OKT 2008 /
/
WELSPECS
 'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
 'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

WVFPEXP
 'W1' 1* 'NO' 'NO' /
 'W2' 'EXP' 'YES' 'YES1' /
/

END

)";
    Deck deck = Parser{}.parseString(input);
    const auto es = EclipseState { deck };
    const auto sched = Schedule { deck, es, std::make_shared<const Python>() };

    const auto& well1 = sched.getWell("W1", 1);
    const auto& well2 = sched.getWell("W2", 1);
    const auto& wvfpexp1 = well1.getWVFPEXP();
    const auto& wvfpexp2 = well2.getWVFPEXP();

    BOOST_CHECK(!wvfpexp1.explicit_lookup());
    BOOST_CHECK(!wvfpexp1.shut());
    BOOST_CHECK(!wvfpexp1.prevent());

    BOOST_CHECK(wvfpexp2.explicit_lookup());
    BOOST_CHECK(wvfpexp2.shut());
    BOOST_CHECK(wvfpexp2.prevent());
}


BOOST_AUTO_TEST_CASE(Test_wdfac) {
    const auto deck = Parser{}.parseString(R"(
DIMENS
 10 10 10 /

START         -- 0
 19 JUN 2007 /

GRID

DXV
 10*100.0 /
DYV
 10*100.0 /
DZV
 10*10.0 /
DEPTHZ
121*2000.0 /

PORO
    1000*0.3 /
PERMX
    1000*10 /
PERMY
    1000*10 /
PERMZ
    1000*10 /

SCHEDULE

DATES        -- 1
 10  OKT 2008 /
/
WELSPECS
 'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
 'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
 'W1'  3 3   1   1 'OPEN' 1*   1   0.216  200 1*  1*  'X'  /
 'W1'  3 3   2   2 'OPEN' 1*   2   0.216  200 1*  1*  'X'  /
 'W1'  3 3   3   3 'OPEN' 1*   3   0.216  200 1*  1*  'X'  /
 'W2'  3 3   3   3 'OPEN' 1*   1   0.216  200 1*  11  'X'  /
/

WDFAC
 'W1' 1 /
 'W2' 2 /
/

DATES        -- 2
 10  NOV 2008 /
/

COMPDAT
 'W1'  3 3   1   1 'OPEN' 1*   1*   0.216  200 1*  1*  'X'  /
 'W1'  3 3   2   2 'OPEN' 1*   1*   0.216  200 1*  1*  'X'  /
 'W1'  3 3   3   3 'OPEN' 1*   1*   0.216  200 1*  1*  'X'  /
 'W2'  3 3   3   3 'OPEN' 1*   1   0.216  200 1*  11  'X'  /
/

WDFACCOR
-- 'W1' 8.957e10 1.1045 0.0 /
   'W1' 1.984e-7 -1.1045 0.0 /
/

DATES        -- 3
 12  NOV 2008 /
/

COMPDAT
 'W1'  3 3   1   1 'OPEN' 1*   1   0.216  200 1*  1*  'X' /
 'W1'  3 3   2   2 'OPEN' 1*   2   0.216  200 1*  0  'X' /
 'W1'  3 3   3   3 'OPEN' 1*   3   0.216  200 1*  11  'X' /
 'W2'  3 3   3   3 'OPEN' 1*   1   0.216  200 1*  11  'X' /
/

END
)");

    const auto es = EclipseState { deck };
    const auto sched = Schedule { deck, es, std::make_shared<const Python>() };

    const auto dFacUnit = 1*unit::day/unit::cubic(unit::meter);

    auto rho = []() { return 1.0*unit::kilogram/unit::cubic(unit::meter); };
    auto mu  = []() { return 0.01*prefix::centi*unit::Poise; };

    {
        const auto& well11 = sched.getWell("W1", 1);
        const auto& well21 = sched.getWell("W2", 1);
        const auto& wdfac11 = well11.getWDFAC();
        const auto& wdfac21 = well21.getWDFAC();

        // WDFAC overwrites D factor in COMDAT
        BOOST_CHECK_MESSAGE(wdfac11.useDFactor(),
                            R"(Well "W1" must use D-Factors at step 1)");

        // Well-level D-factor scaled by connection transmissibility factor.
        BOOST_CHECK_CLOSE(wdfac11.getDFactor(rho, mu, well11.getConnections()[0]), 6*1.0*dFacUnit, 1e-12);
        BOOST_CHECK_CLOSE(wdfac21.getDFactor(rho, mu, well21.getConnections()[0]),   2.0*dFacUnit, 1e-12);
    }

    {
        const auto& well12 = sched.getWell("W1", 2);
        const auto& well22 = sched.getWell("W2", 2);
        const auto& wdfac12 = well12.getWDFAC();
        const auto& wdfac22 = well22.getWDFAC();

        BOOST_CHECK_CLOSE(wdfac12.getDFactor(rho, mu, well12.getConnections()[0]), 5.19e-1, 3);
        BOOST_CHECK_CLOSE(wdfac22.getDFactor(rho, mu, well22.getConnections()[0]), 2.0*dFacUnit, 1e-12);
    }

    {
        const auto& well13 = sched.getWell("W1", 3);
        const auto& well23 = sched.getWell("W2", 3);
        const auto& wdfac13 = well13.getWDFAC();
        const auto& wdfac23 = well23.getWDFAC();

        BOOST_CHECK_MESSAGE(wdfac13.useDFactor(),
                            R"(Well "W1" must use D-Factors at step 3)");

        BOOST_CHECK_CLOSE(well13.getConnections()[0].dFactor(),  0.0*dFacUnit, 1e-12);
        BOOST_CHECK_CLOSE(well13.getConnections()[1].dFactor(),  0.0*dFacUnit, 1e-12);
        BOOST_CHECK_CLOSE(well13.getConnections()[2].dFactor(), 11.0*dFacUnit, 1e-12);

        BOOST_CHECK_CLOSE(wdfac13.getDFactor(rho, mu, well13.getConnections()[2]), 6.0/3.0*11.0*dFacUnit, 1e-12);
        BOOST_CHECK_CLOSE(wdfac23.getDFactor(rho, mu, well23.getConnections()[0]),          2.0*dFacUnit, 1e-12);
    }
}

BOOST_AUTO_TEST_CASE(createDeckWithBC) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /

SOLUTION

SCHEDULE

BCPROP
1 RATE GAS 100.0 /
2 FREE /
/

DATES             -- 1
 10  OKT 2008 /
/
BCPROP
1 RATE GAS 200.0 /
2 FREE 4* /
/
)";

    const auto& schedule = make_schedule(input);
    {
        size_t currentStep = 0;
        const auto& bc = schedule[currentStep].bcprop;
        BOOST_CHECK_EQUAL(bc.size(), 2);
        const auto& bcface0 = bc[0];
        BOOST_CHECK_CLOSE(bcface0.rate * Opm::unit::day, 100, 1e-8 );
    }

    {
        size_t currentStep = 1;
        const auto& bc = schedule[currentStep].bcprop;
        BOOST_CHECK_EQUAL(bc.size(), 2);
        const auto& bcface0 = bc[0];
        BOOST_CHECK_CLOSE(bcface0.rate * Opm::unit::day, 200, 1e-8 );
    }
}

BOOST_AUTO_TEST_CASE(createDeckWithSource) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /

SOLUTION

SCHEDULE

SOURCE
 1 1 1 GAS 0.01 /
 1 1 1 WATER 0.01 /
/

DATES             -- 1
 10  OKT 2008 /
/
SOURCE
 1 1 1 GAS 0.02 /
 1 1 2 WATER 0.01 /
/
)";

    const auto& schedule = make_schedule(input);
    {
        size_t currentStep = 0;
        const auto& source = schedule[currentStep].source();
        BOOST_CHECK_EQUAL(source.size(), 1);  // num cells
        double rate11 = source.rate({0,0,0},Opm::SourceComponent::GAS);
        BOOST_CHECK_EQUAL(rate11,
                        schedule.getUnits().to_si("Mass/Time", 0.01));

        double rate12 = source.rate({0,0,0},Opm::SourceComponent::WATER);
        BOOST_CHECK_EQUAL(rate12,
                      schedule.getUnits().to_si("Mass/Time", 0.01));
    }

    {
        size_t currentStep = 1;
        const auto& source = schedule[currentStep].source();
        BOOST_CHECK_EQUAL(source.size(), 2);  // num cells
        double rate21 = source.rate({0,0,0},Opm::SourceComponent::GAS);
        BOOST_CHECK_EQUAL(rate21,
                        schedule.getUnits().to_si("Mass/Time", 0.02));
        double rate22 = source.rate({0,0,0},Opm::SourceComponent::WATER);
        BOOST_CHECK_EQUAL(rate22,
                        schedule.getUnits().to_si("Mass/Time", 0.01));

        double rate23 = source.rate({0,0,1},Opm::SourceComponent::WATER);
        BOOST_CHECK_EQUAL(rate23,
                      schedule.getUnits().to_si("Mass/Time", 0.01));
    }
}

BOOST_AUTO_TEST_CASE(clearEvent) {
    std::string input = R"(
START             -- 0
19 JUN 2007 /

SOLUTION

SCHEDULE
DATES             -- 1
 10  OKT 2008 /
/

NEXTSTEP
 10 /

DATES             -- 1
 10  NOV 2008 /
/
)";

    auto schedule = make_schedule(input);
    BOOST_CHECK(schedule[1].events().hasEvent(ScheduleEvents::TUNING_CHANGE));
    // TUNING_CHANGE because NEXTSTEP cleared
    BOOST_CHECK(schedule[2].events().hasEvent(ScheduleEvents::TUNING_CHANGE));
    schedule.clear_event(ScheduleEvents::TUNING_CHANGE, 1);
    BOOST_CHECK(!schedule[1].events().hasEvent(ScheduleEvents::TUNING_CHANGE));
}

BOOST_AUTO_TEST_CASE(Well_Fracture_Seeds)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

MECH

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /

SCHEDULE
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 /
 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 /
 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 /
 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 /
 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 /
/

DATES             -- 1, 2
  10  JUN 2007 /
  10  AUG 2007 /
/

WSEED
  'OP_1'  9 9 1   1.0   -1.0      1.0  /
  'OP_1'  9 9 2   0.0    0.0     17.29 /
  'OP_3'  7 7 2   3.1   41.592  653.5  /
/

DATES
  1 SEP 2007 /
/
END
)");

    const auto es    = EclipseState { deck };
    const auto sched = Schedule { deck, es };

    BOOST_CHECK_EQUAL(sched[0].wseed.size(), std::size_t{0});
    BOOST_CHECK_EQUAL(sched[1].wseed.size(), std::size_t{0});
    BOOST_CHECK_EQUAL(sched[2].wseed.size(), std::size_t{2});
    BOOST_CHECK_EQUAL(sched[3].wseed.size(), std::size_t{2});

    const auto& wseed = sched[2].wseed;
    BOOST_CHECK_MESSAGE(  wseed.has("OP_1"), R"(Well "OP_1" must have well fracturing seeds)");
    BOOST_CHECK_MESSAGE(! wseed.has("OP_2"), R"(Well "OP_2" must NOT have well fracturing seeds)");
    BOOST_CHECK_MESSAGE(  wseed.has("OP_3"), R"(Well "OP_3" must have well fracturing seeds)");

    {
        const auto& op_1 = wseed("OP_1");

        BOOST_CHECK_MESSAGE(! op_1.empty(), R"(Well fracturing seed container for "OP_1" must not be empty)");

        const auto expectSeedCell = std::vector {
            es.getInputGrid().getGlobalIndex(9-1, 9-1, 1-1),
            es.getInputGrid().getGlobalIndex(9-1, 9-1, 2-1),
        };

        const auto& seedCells = op_1.seedCells();

        BOOST_CHECK_EQUAL_COLLECTIONS(seedCells.begin(), seedCells.end(),
                                      expectSeedCell.begin(), expectSeedCell.end());

        const auto& n0 = op_1.getNormal(WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n0[0],  1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[2],  1.0, 1.0e-8);

        const auto& n1 = op_1.getNormal(WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n1[0],  0.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(n1[1],  0.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(n1[2], 17.29, 1.0e-8);
    }

    {
        const auto& op_3 = wseed("OP_3");

        BOOST_CHECK_MESSAGE(! op_3.empty(), R"(Well fracturing seed container for "OP_3" must not be empty)");

        const auto expectSeedCell = std::vector {
            es.getInputGrid().getGlobalIndex(7-1, 7-1, 2-1),
        };

        const auto& seedCells = op_3.seedCells();

        BOOST_CHECK_EQUAL_COLLECTIONS(seedCells.begin(), seedCells.end(),
                                      expectSeedCell.begin(), expectSeedCell.end());

        const auto& n0 = op_3.getNormal(WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n0[0],   3.1  , 1.0e-8);
        BOOST_CHECK_CLOSE(n0[1],  41.592, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[2], 653.5  , 1.0e-8);
    }

    const auto& wseed_back = sched[3].wseed;
    BOOST_CHECK_MESSAGE(  wseed_back.has("OP_1"), R"(Well "OP_1" must have well fracturing seeds)");
    BOOST_CHECK_MESSAGE(! wseed_back.has("OP_2"), R"(Well "OP_2" must NOT have well fracturing seeds)");
    BOOST_CHECK_MESSAGE(  wseed_back.has("OP_3"), R"(Well "OP_3" must have well fracturing seeds)");

    {
        const auto& op_1 = wseed_back("OP_1");

        BOOST_CHECK_MESSAGE(! op_1.empty(), R"(Well fracturing seed container for "OP_1" must not be empty)");

        const auto expectSeedCell = std::vector {
            es.getInputGrid().getGlobalIndex(9-1, 9-1, 1-1),
            es.getInputGrid().getGlobalIndex(9-1, 9-1, 2-1),
        };

        const auto& seedCells = op_1.seedCells();

        BOOST_CHECK_EQUAL_COLLECTIONS(seedCells.begin(), seedCells.end(),
                                      expectSeedCell.begin(), expectSeedCell.end());

        const auto& n0 = op_1.getNormal(WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n0[0],  1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[1], -1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[2],  1.0, 1.0e-8);

        const auto& n1 = op_1.getNormal(WellFractureSeeds::SeedIndex { 1 });

        BOOST_CHECK_CLOSE(n1[0],  0.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(n1[1],  0.0 , 1.0e-8);
        BOOST_CHECK_CLOSE(n1[2], 17.29, 1.0e-8);
    }

    {
        const auto& op_3 = wseed_back("OP_3");

        BOOST_CHECK_MESSAGE(! op_3.empty(), R"(Well fracturing seed container for "OP_3" must not be empty)");

        const auto expectSeedCell = std::vector {
            es.getInputGrid().getGlobalIndex(7-1, 7-1, 2-1),
        };

        const auto& seedCells = op_3.seedCells();

        BOOST_CHECK_EQUAL_COLLECTIONS(seedCells.begin(), seedCells.end(),
                                      expectSeedCell.begin(), expectSeedCell.end());

        const auto& n0 = op_3.getNormal(WellFractureSeeds::SeedIndex { 0 });

        BOOST_CHECK_CLOSE(n0[0],   3.1  , 1.0e-8);
        BOOST_CHECK_CLOSE(n0[1],  41.592, 1.0e-8);
        BOOST_CHECK_CLOSE(n0[2], 653.5  , 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Well_Fracture_Seeds_With_Size)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  10 10 10 /

START             -- 0
10 MAI 2007 /

-- WSEED's required keyword 'MECH' is missing (disabled) to check that
-- 'WSEED' throws when parsing this input deck
-- MECH

GRID
DXV
10*100.0 /
DYV
10*100.0 /
DZV
10*10.0 /
DEPTHZ
121*2000.0 /

PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /

SCHEDULE
WELSPECS
    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 /
 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 /
 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 /
 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 /
 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 /
/

DATES             -- 1, 2
  10  JUN 2007 /
  10  AUG 2007 /
/

WSEED
  'OP_1'  9 9 1   1.0   -1.0      1.0  0.12 34.567 891.01112 /
  'OP_1'  9 9 2   0.0    0.0     17.29 8.91 0.111 0.2222 /
  'OP_3'  7 7 2   3.1   41.592  653.5  12.13 14.151617 1819.202122 /
/

DATES
  1 SEP 2007 /
/
END
)");

    const auto es    = EclipseState { deck };
    const auto sched = Schedule { deck, es };

    const auto& wseed = sched[2].wseed;

    {
        const auto& op_1 = wseed("OP_1");

        const auto& seedCells = op_1.seedCells();

        const auto* s0 = op_1.getSize(WellFractureSeeds::SeedCell { seedCells[0] });

        BOOST_CHECK_CLOSE(s0->verticalExtent(), 0.12, 1.0e-8);
        BOOST_CHECK_CLOSE(s0->horizontalExtent(), 34.567, 1.0e-8);
        BOOST_CHECK_CLOSE(s0->width(), 891.01112, 1.0e-8);

        const auto* s1 = op_1.getSize(WellFractureSeeds::SeedCell { seedCells[1] });

        BOOST_CHECK_CLOSE(s1->verticalExtent(), 8.91 , 1.0e-8);
        BOOST_CHECK_CLOSE(s1->horizontalExtent(), 0.111, 1.0e-8);
        BOOST_CHECK_CLOSE(s1->width(), 0.2222, 1.0e-8);
    }

    {
        const auto& op_3 = wseed("OP_3");

        const auto& seedCells = op_3.seedCells();

        const auto* s = op_3.getSize(WellFractureSeeds::SeedCell { seedCells[0] });

        BOOST_CHECK_CLOSE(s->verticalExtent(), 12.13, 1.0e-8);
        BOOST_CHECK_CLOSE(s->horizontalExtent(), 14.151617, 1.0e-8);
        BOOST_CHECK_CLOSE(s->width(), 1819.202122, 1.0e-8);
    }
}
