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

#define BOOST_TEST_MODULE ScheduleIntegrationTests
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

using namespace Opm;


inline std::string pathprefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE(CreateSchedule) {
    ParseContext parseContext;
    Parser parser;
    EclipseGrid grid(10,10,10);
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE1");
    auto deck1 =  parser.parseFile(scheduleFile, parseContext);
    std::stringstream ss;
    ss << deck1;
    auto deck2 = parser.parseString( ss.str(), parseContext );
    for (const auto& deck : {deck1 , deck2}) {
        TableManager table ( deck );
        Eclipse3DProperties eclipseProperties ( deck , table, grid);

        Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
        const auto& timeMap = sched.getTimeMap();
        BOOST_CHECK_EQUAL(TimeMap::mkdate(2007 , 5 , 10), sched.getStartTime());
        BOOST_CHECK_EQUAL(9U, timeMap.size());
        BOOST_CHECK( deck.hasKeyword("NETBALAN") );
    }
}


BOOST_AUTO_TEST_CASE(CreateSchedule_Comments_After_Keywords) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_COMMENTS_AFTER_KEYWORDS");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
    const auto& timeMap = sched.getTimeMap();
    BOOST_CHECK_EQUAL(TimeMap::mkdate(2007, 5 , 10) , sched.getStartTime());
    BOOST_CHECK_EQUAL(9U, timeMap.size());
}


BOOST_AUTO_TEST_CASE(WCONPROD_MissingCmode) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_MISSING_CMODE");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_NO_THROW( Schedule(deck, grid , eclipseProperties, Phases(true, true, true), parseContext ) );
}


BOOST_AUTO_TEST_CASE(WCONPROD_Missing_DATA) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_CMODE_MISSING_DATA");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_THROW( Schedule(deck, grid , eclipseProperties, Phases(true, true, true), parseContext ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(WellTestRefDepth) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS2");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_EQUAL(3, 3);
    Schedule sched(deck , grid , eclipseProperties, Phases(true, true, true) , parseContext);
    BOOST_CHECK_EQUAL(4, 4);

    auto* well1 = sched.getWell("W_1");
    auto* well2 = sched.getWell("W_2");
    auto* well4 = sched.getWell("W_4");
    BOOST_CHECK_EQUAL( well1->getRefDepth() , grid.getCellDepth( 29 , 36 , 0 ));
    BOOST_CHECK_EQUAL( well2->getRefDepth() , 100 );
    BOOST_CHECK_THROW( well4->getRefDepth() , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(WellTestOpen) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS2");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    auto well1 = sched.getWell( "W_1" );
    auto well2 = sched.getWell( "W_2" );
    auto well3 = sched.getWell( "W_3" );

    {
        auto wells = sched.getOpenWells( 3 );
        BOOST_CHECK_EQUAL( 1U , wells.size() );
        BOOST_CHECK_EQUAL( well1 , wells[0] );
    }

    {
        auto wells = sched.getOpenWells(6);
        BOOST_CHECK_EQUAL( 3U , wells.size() );

        BOOST_CHECK_EQUAL( well1 , wells[0] );
        BOOST_CHECK_EQUAL( well2 , wells[1] );
        BOOST_CHECK_EQUAL( well3 , wells[2] );
    }

    {
        auto wells = sched.getOpenWells(12);
        BOOST_CHECK_EQUAL( 2U , wells.size() );

        BOOST_CHECK_EQUAL( well2 , wells[0] );
        BOOST_CHECK_EQUAL( well3 , wells[1] );
    }
}





BOOST_AUTO_TEST_CASE(WellTesting) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS2");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL(4U, sched.numWells());
    BOOST_CHECK(sched.hasWell("W_1"));
    BOOST_CHECK(sched.hasWell("W_2"));
    BOOST_CHECK(sched.hasWell("W_3"));

    {
        auto* well2 = sched.getWell("W_2");
        BOOST_CHECK_EQUAL( 0 , well2->getProductionPropertiesCopy(2).ResVRate);
        BOOST_CHECK_CLOSE( 777/Metric::Time , well2->getProductionPropertiesCopy(7).ResVRate , 0.0001);
        BOOST_CHECK_EQUAL( 0 , well2->getProductionPropertiesCopy(8).ResVRate);

        BOOST_CHECK_EQUAL( WellCommon::SHUT , well2->getStatus(3));
        BOOST_CHECK( !well2->getRFTActive( 2 ) );
        BOOST_CHECK( well2->getRFTActive( 3 ) );
        BOOST_CHECK( well2->getRFTActive( 4 ) );
        BOOST_CHECK( !well2->getRFTActive( 5 ) );
        {
            const WellProductionProperties& prop3 = well2->getProductionProperties(3);
            BOOST_CHECK_EQUAL( WellProducer::ORAT , prop3.controlMode);
            BOOST_CHECK(  prop3.hasProductionControl(WellProducer::ORAT));
            BOOST_CHECK(  !prop3.hasProductionControl(WellProducer::GRAT));
            BOOST_CHECK(  !prop3.hasProductionControl(WellProducer::WRAT));
        }

        // BOOST_CHECK( !well2->getProductionProperties(8).hasProductionControl(WellProducer::GRAT));
    }


    {
        auto* well3 = sched.getWell("W_3");

        BOOST_CHECK_EQUAL( WellCommon::AUTO , well3->getStatus(3));
        BOOST_CHECK_EQUAL( 0 , well3->getProductionPropertiesCopy(2).LiquidRate);

        {
            const WellProductionProperties& prop7 = well3->getProductionProperties(7);
            BOOST_CHECK_CLOSE( 999/Metric::Time , prop7.LiquidRate , 0.001);
            BOOST_CHECK_EQUAL( WellProducer::RESV, prop7.controlMode);
        }
        BOOST_CHECK_EQUAL( 0 , well3->getProductionPropertiesCopy(8).LiquidRate);
    }

    {
        auto* well1 = sched.getWell("W_1");

        BOOST_CHECK_EQUAL( well1->firstRFTOutput( ) , 3);
        BOOST_CHECK( well1->getRFTActive( 3 ) );
        BOOST_CHECK(well1->getProductionPropertiesCopy(0).predictionMode);
        BOOST_CHECK_EQUAL(0, well1->getProductionPropertiesCopy(0).OilRate);

        BOOST_CHECK_EQUAL(0, well1->getProductionPropertiesCopy(1).OilRate);
        BOOST_CHECK_EQUAL(0, well1->getProductionPropertiesCopy(2).OilRate);

        BOOST_CHECK(!well1->getProductionPropertiesCopy(3).predictionMode);
        BOOST_CHECK_CLOSE(4000/Metric::Time , well1->getProductionPropertiesCopy(3).OilRate , 0.001);
        BOOST_CHECK_CLOSE(4000/Metric::Time , well1->getProductionPropertiesCopy(4).OilRate , 0.001);
        BOOST_CHECK_CLOSE(4000/Metric::Time , well1->getProductionPropertiesCopy(5).OilRate , 0.001);
        BOOST_CHECK_CLOSE(4/Metric::Time      , well1->getProductionPropertiesCopy(3).WaterRate , 0.001);
        BOOST_CHECK_CLOSE(12345/Metric::Time  , well1->getProductionPropertiesCopy(3).GasRate , 0.001);
        BOOST_CHECK_CLOSE(4/Metric::Time      , well1->getProductionPropertiesCopy(4).WaterRate , 0.001);
        BOOST_CHECK_CLOSE(12345/Metric::Time  , well1->getProductionPropertiesCopy(4).GasRate , 0.001);
        BOOST_CHECK_CLOSE(4/Metric::Time      , well1->getProductionPropertiesCopy(5).WaterRate , 0.001);
        BOOST_CHECK_CLOSE(12345/Metric::Time  , well1->getProductionPropertiesCopy(5).GasRate , 0.001);


        BOOST_CHECK(!well1->getProductionPropertiesCopy(6).predictionMode);
        BOOST_CHECK_CLOSE(14000/Metric::Time , well1->getProductionPropertiesCopy(6).OilRate , 0.001);

        BOOST_CHECK(well1->getProductionPropertiesCopy(7).predictionMode);
        BOOST_CHECK_CLOSE(11000/Metric::Time , well1->getProductionPropertiesCopy(7).OilRate , 0.001);
        BOOST_CHECK_CLOSE(44/Metric::Time    , well1->getProductionPropertiesCopy(7).WaterRate , 0.001);
        BOOST_CHECK_CLOSE(188/Metric::Time   , well1->getProductionPropertiesCopy(7).GasRate , 0.001);

        BOOST_CHECK(!well1->getProductionPropertiesCopy(8).predictionMode);
        BOOST_CHECK_CLOSE(13000/Metric::Time , well1->getProductionPropertiesCopy(8).OilRate , 0.001);

        BOOST_CHECK_CLOSE(123.00 * Metric::Pressure , well1->getInjectionPropertiesCopy(10).BHPLimit, 0.001);
        BOOST_CHECK_CLOSE(678.00 * Metric::Pressure , well1->getInjectionPropertiesCopy(10).THPLimit, 0.001);

        {
            const WellInjectionProperties& prop11 = well1->getInjectionProperties(11);
            BOOST_CHECK_CLOSE(5000/Metric::Time , prop11.surfaceInjectionRate, 0.001);
            BOOST_CHECK_EQUAL( WellInjector::RATE  , prop11.controlMode);
            BOOST_CHECK_EQUAL( WellCommon::OPEN , well1->getStatus( 11 ));
        }



        BOOST_CHECK( well1->isInjector(9));
        {
            const WellInjectionProperties& prop9 = well1->getInjectionProperties(9);
            BOOST_CHECK_CLOSE(20000/Metric::Time ,  prop9.surfaceInjectionRate  , 0.001);
            BOOST_CHECK_CLOSE(200000/Metric::Time , prop9.reservoirInjectionRate, 0.001);
            BOOST_CHECK_CLOSE(6895 * Metric::Pressure , prop9.BHPLimit, 0.001);
            BOOST_CHECK_CLOSE(0 , prop9.THPLimit , 0.001);
            BOOST_CHECK_EQUAL( WellInjector::RESV  , prop9.controlMode);
            BOOST_CHECK(  prop9.hasInjectionControl(WellInjector::RATE ));
            BOOST_CHECK(  prop9.hasInjectionControl(WellInjector::RESV ));
            BOOST_CHECK( !prop9.hasInjectionControl(WellInjector::THP));
            BOOST_CHECK(  prop9.hasInjectionControl(WellInjector::BHP));
        }


        BOOST_CHECK_EQUAL( WellCommon::SHUT , well1->getStatus( 12 ));
        BOOST_CHECK(  well1->getInjectionPropertiesCopy(12).hasInjectionControl(WellInjector::RATE ));
        BOOST_CHECK( !well1->getInjectionPropertiesCopy(12).hasInjectionControl(WellInjector::RESV));
        BOOST_CHECK(  well1->getInjectionPropertiesCopy(12).hasInjectionControl(WellInjector::THP ));
        BOOST_CHECK(  well1->getInjectionPropertiesCopy(12).hasInjectionControl(WellInjector::BHP ));

    }
}


BOOST_AUTO_TEST_CASE(WellTestCOMPDAT_DEFAULTED_ITEMS) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_COMPDAT1");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
}


BOOST_AUTO_TEST_CASE(WellTestCOMPDAT) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS2");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL(4U, sched.numWells());
    BOOST_CHECK(sched.hasWell("W_1"));
    BOOST_CHECK(sched.hasWell("W_2"));
    BOOST_CHECK(sched.hasWell("W_3"));
    {
        auto* well1 = sched.getWell("W_1");
        BOOST_CHECK_CLOSE(13000/Metric::Time , well1->getProductionPropertiesCopy(8).OilRate , 0.0001);
        BOOST_CHECK_EQUAL(0U, well1->getCompletions( 0 ).size() );

        const auto& completions = well1->getCompletions(3);
        BOOST_CHECK_EQUAL(4U, completions.size());

        BOOST_CHECK_EQUAL(WellCompletion::OPEN, completions.get(3).getState());
        BOOST_CHECK_EQUAL(2.2836805555555556e-12 , completions.get(3).getConnectionTransmissibilityFactor());
        BOOST_CHECK_EQUAL(0.311/Metric::Length, completions.get(3).getDiameter());
        BOOST_CHECK_EQUAL(3.3, completions.get(3).getSkinFactor());

        BOOST_CHECK_EQUAL(4U, well1->getCompletions( 7 ).size() );
        BOOST_CHECK_EQUAL(WellCompletion::SHUT, well1->getCompletions( 7 ).get( 3 ).getState() );
    }
}

BOOST_AUTO_TEST_CASE(GroupTreeTest_GRUPTREE_with_explicit_L0_parenting) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_GRUPTREE_EXPLICIT_PARENTING");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    const auto& grouptree = sched.getGroupTree( 0 );

    BOOST_CHECK( grouptree.exists( "FIRST_LEVEL1" ) );
    BOOST_CHECK( grouptree.exists( "FIRST_LEVEL2" ) );
    BOOST_CHECK( grouptree.exists( "SECOND_LEVEL1" ) );
    BOOST_CHECK( grouptree.exists( "SECOND_LEVEL2" ) );
    BOOST_CHECK( grouptree.exists( "THIRD_LEVEL1" ) );

    BOOST_CHECK_EQUAL( "FIELD", grouptree.parent( "FIRST_LEVEL1" ) );
    BOOST_CHECK_EQUAL( "FIELD", grouptree.parent( "FIRST_LEVEL2" ) );
    BOOST_CHECK_EQUAL( "FIRST_LEVEL1", grouptree.parent( "SECOND_LEVEL1" ) );
    BOOST_CHECK_EQUAL( "FIRST_LEVEL2", grouptree.parent( "SECOND_LEVEL2" ) );
    BOOST_CHECK_EQUAL( "SECOND_LEVEL1", grouptree.parent( "THIRD_LEVEL1" ) );
}


BOOST_AUTO_TEST_CASE(GroupTreeTest_GRUPTREE_correct) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELSPECS_GRUPTREE");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK( schedule.hasGroup( "FIELD" ));
    BOOST_CHECK( schedule.hasGroup( "PROD" ));
    BOOST_CHECK( schedule.hasGroup( "INJE" ));
    BOOST_CHECK( schedule.hasGroup( "MANI-PROD" ));
    BOOST_CHECK( schedule.hasGroup( "MANI-INJ" ));
    BOOST_CHECK( schedule.hasGroup( "DUMMY-PROD" ));
    BOOST_CHECK( schedule.hasGroup( "DUMMY-INJ" ));
}



BOOST_AUTO_TEST_CASE(GroupTreeTest_WELSPECS_AND_GRUPTREE_correct_size ) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELSPECS_GROUPS");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck,  grid , eclipseProperties,Phases(true, true, true), parseContext);

    // Time 0, only from WELSPECS
    BOOST_CHECK_EQUAL( 2U, schedule.getGroupTree(0).children("FIELD").size() );

    // Time 1, a new group added in tree
    BOOST_CHECK_EQUAL( 3U, schedule.getGroupTree(1).children("FIELD").size() );
}

BOOST_AUTO_TEST_CASE(GroupTreeTest_WELSPECS_AND_GRUPTREE_correct_tree) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELSPECS_GROUPS");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    // Time 0, only from WELSPECS
    const auto& tree0 = schedule.getGroupTree( 0 );
    BOOST_CHECK( tree0.exists( "FIELD" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree0.parent( "GROUP_BJARNE" ) );
    BOOST_CHECK( tree0.exists("GROUP_ODD") );

    // Time 1, now also from GRUPTREE
    const auto& tree1 = schedule.getGroupTree( 1 );
    BOOST_CHECK( tree1.exists( "FIELD" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree1.parent( "GROUP_BJARNE" ) );
    BOOST_CHECK( tree1.exists("GROUP_ODD"));

    // - from GRUPTREE
    BOOST_CHECK( tree1.exists( "GROUP_BIRGER" ) );
    BOOST_CHECK_EQUAL( "GROUP_BJARNE", tree1.parent( "GROUP_BIRGER" ) );

    BOOST_CHECK( tree1.exists( "GROUP_NEW" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree1.parent( "GROUP_NEW" ) );

    BOOST_CHECK( tree1.exists( "GROUP_NILS" ) );
    BOOST_CHECK_EQUAL( "GROUP_NEW", tree1.parent( "GROUP_NILS" ) );
}

BOOST_AUTO_TEST_CASE(GroupTreeTest_GRUPTREE_WITH_REPARENT_correct_tree) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_GROUPS_REPARENT");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);


    const auto& tree0 = sched.getGroupTree( 0 );

    BOOST_CHECK( tree0.exists( "GROUP_BJARNE" ) );
    BOOST_CHECK( tree0.exists( "GROUP_NILS" ) );
    BOOST_CHECK( tree0.exists( "GROUP_NEW" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree0.parent( "GROUP_BJARNE" ) );
    BOOST_CHECK_EQUAL( "GROUP_BJARNE", tree0.parent( "GROUP_BIRGER" ) );
    BOOST_CHECK_EQUAL( "GROUP_NEW", tree0.parent( "GROUP_NILS" ) );
}

BOOST_AUTO_TEST_CASE( WellTestGroups ) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_GROUPS");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL( 3U , sched.numGroups() );
    BOOST_CHECK( sched.hasGroup( "INJ" ));
    BOOST_CHECK( sched.hasGroup( "OP" ));

    {
        auto& group = sched.getGroup("INJ");
        BOOST_CHECK_EQUAL( Phase::WATER , group.getInjectionPhase( 3 ));
        BOOST_CHECK_EQUAL( GroupInjection::VREP , group.getInjectionControlMode( 3 ));
        BOOST_CHECK_CLOSE( 10/Metric::Time , group.getSurfaceMaxRate( 3 ) , 0.001);
        BOOST_CHECK_CLOSE( 20/Metric::Time , group.getReservoirMaxRate( 3 ) , 0.001);
        BOOST_CHECK_EQUAL( 0.75 , group.getTargetReinjectFraction( 3 ));
        BOOST_CHECK_EQUAL( 0.95 , group.getTargetVoidReplacementFraction( 3 ));

        BOOST_CHECK_EQUAL( Phase::OIL , group.getInjectionPhase( 6 ));
        BOOST_CHECK_EQUAL( GroupInjection::RATE , group.getInjectionControlMode( 6 ));
        BOOST_CHECK_CLOSE( 1000/Metric::Time , group.getSurfaceMaxRate( 6 ) , 0.0001);

        BOOST_CHECK(group.isInjectionGroup(3));
    }

    {
        auto& group = sched.getGroup("OP");
        BOOST_CHECK_EQUAL( GroupProduction::ORAT , group.getProductionControlMode(3));
        BOOST_CHECK_CLOSE( 10/Metric::Time , group.getOilTargetRate(3) , 0.001);
        BOOST_CHECK_CLOSE( 20/Metric::Time , group.getWaterTargetRate(3) , 0.001);
        BOOST_CHECK_CLOSE( 30/Metric::Time , group.getGasTargetRate(3) , 0.001);
        BOOST_CHECK_CLOSE( 40/Metric::Time , group.getLiquidTargetRate(3) , 0.001);

        BOOST_CHECK(group.isProductionGroup(3));
    }

}


BOOST_AUTO_TEST_CASE( WellTestGroupAndWellRelation ) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS_AND_GROUPS");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    auto& group1 = sched.getGroup("GROUP1");
    auto& group2 = sched.getGroup("GROUP2");

    BOOST_CHECK(  group1.hasBeenDefined(0) );
    BOOST_CHECK( !group2.hasBeenDefined(0));
    BOOST_CHECK(  group2.hasBeenDefined(1));

    BOOST_CHECK(  group1.hasWell("W_1" , 0));
    BOOST_CHECK(  group1.hasWell("W_2" , 0));
    BOOST_CHECK( !group2.hasWell("W_1" , 0));
    BOOST_CHECK( !group2.hasWell("W_2" , 0));

    BOOST_CHECK(  group1.hasWell("W_1" , 1));
    BOOST_CHECK( !group1.hasWell("W_2" , 1));
    BOOST_CHECK( !group2.hasWell("W_1" , 1));
    BOOST_CHECK(  group2.hasWell("W_2" , 1));
}


BOOST_AUTO_TEST_CASE(WellTestWELSPECSDataLoaded) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELLS2");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,60,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL(4U, sched.numWells());
    BOOST_CHECK(sched.hasWell("W_1"));
    BOOST_CHECK(sched.hasWell("W_2"));
    BOOST_CHECK(sched.hasWell("W_3"));
    {
        const auto* well1 = sched.getWell("W_1");
        BOOST_CHECK(!well1->hasBeenDefined(2));
        BOOST_CHECK(well1->hasBeenDefined(3));
        BOOST_CHECK_EQUAL(29, well1->getHeadI());
        BOOST_CHECK_EQUAL(36, well1->getHeadJ());

        const auto* well2 = sched.getWell("W_2");
        BOOST_CHECK(!well2->hasBeenDefined(2));
        BOOST_CHECK(well2->hasBeenDefined(3));
        BOOST_CHECK_EQUAL(19, well2->getHeadI());
        BOOST_CHECK_EQUAL(50, well2->getHeadJ());

        const auto* well3 = sched.getWell("W_3");
        BOOST_CHECK(!well3->hasBeenDefined(2));
        BOOST_CHECK(well3->hasBeenDefined(3));
        BOOST_CHECK_EQUAL(30, well3->getHeadI());
        BOOST_CHECK_EQUAL(17, well3->getHeadJ());
    }
}

/*
BOOST_AUTO_TEST_CASE(WellTestWELOPEN_ConfigWithIndexes_Throws) {
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELOPEN_INVALID");
    auto deck =  parser.parseFile(scheduleFile);
    std::shared_ptr<const EclipseGrid> grid = std::make_shared<const EclipseGrid>(10,10,3);
    BOOST_CHECK_THROW(Schedule(grid , deck), std::logic_error);
}


BOOST_AUTO_TEST_CASE(WellTestWELOPENControlsSet) {
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WELOPEN");
    auto deck =  parser.parseFile(scheduleFile);
    std::shared_ptr<const EclipseGrid> grid = std::make_shared<const EclipseGrid>( 10,10,10 );
    Schedule sched(grid , deck);

    const auto* well1 = sched.getWell("W_1");
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, sched.getWell("W_1")->getStatus(0));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, sched.getWell("W_1")->getStatus(1));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, sched.getWell("W_1")->getStatus(2));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::STOP, sched.getWell("W_1")->getStatus(3));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::AUTO, sched.getWell("W_1")->getStatus(4));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::STOP, sched.getWell("W_1")->getStatus(5));
}
*/



BOOST_AUTO_TEST_CASE(WellTestWGRUPCONWellPropertiesSet) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WGRUPCON");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    const auto* well1 = sched.getWell("W_1");
    BOOST_CHECK(well1->isAvailableForGroupControl(0));
    BOOST_CHECK_EQUAL(-1, well1->getGuideRate(0));
    BOOST_CHECK_EQUAL(GuideRate::OIL, well1->getGuideRatePhase(0));
    BOOST_CHECK_EQUAL(1.0, well1->getGuideRateScalingFactor(0));

    const auto* well2 = sched.getWell("W_2");
    BOOST_CHECK(!well2->isAvailableForGroupControl(0));
    BOOST_CHECK_EQUAL(-1, well2->getGuideRate(0));
    BOOST_CHECK_EQUAL(GuideRate::UNDEFINED, well2->getGuideRatePhase(0));
    BOOST_CHECK_EQUAL(1.0, well2->getGuideRateScalingFactor(0));

    const auto* well3 = sched.getWell("W_3");
    BOOST_CHECK(well3->isAvailableForGroupControl(0));
    BOOST_CHECK_EQUAL(100, well3->getGuideRate(0));
    BOOST_CHECK_EQUAL(GuideRate::RAT, well3->getGuideRatePhase(0));
    BOOST_CHECK_EQUAL(0.5, well3->getGuideRateScalingFactor(0));
}


BOOST_AUTO_TEST_CASE(TestDefaultedCOMPDATIJ) {
    ParseContext parseContext;
    Parser parser;
    const char * deckString = "\n\
START\n\
\n\
10 MAI 2007 /\n\
\n\
SCHEDULE\n\
WELSPECS \n\
     'W1'        'OP'   11   21  3.33       'OIL'  7* /   \n\
/\n\
COMPDAT \n\
     'W1'   2*    1    1      'OPEN'  1*     32.948      0.311   3047.839  2*         'X'     22.100 /\n\
/\n";
    auto deck =  parser.parseString(deckString, parseContext);
    EclipseGrid grid(30,30,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
    const auto* well = sched.getWell("W1");
    const auto& completions = well->getCompletions(0);
    BOOST_CHECK_EQUAL( 10 , completions.get(0).getI() );
    BOOST_CHECK_EQUAL( 20 , completions.get(0).getJ() );
}


/**
   This is a deck used in the opm-core wellsManager testing; just be
   certain we can parse it.
*/
BOOST_AUTO_TEST_CASE(OpmCode) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/wells_group.data");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(10,10,3);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_NO_THROW( Schedule(deck , grid , eclipseProperties, Phases(true, true, true), parseContext) );
}



BOOST_AUTO_TEST_CASE(WELLS_SHUT) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_SHUT_WELL");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(20,40,1);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);


    const auto* well1 = sched.getWell("W1");
    const auto* well2 = sched.getWell("W2");
    const auto* well3 = sched.getWell("W3");

    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::OPEN , well1->getStatus(1));
    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::OPEN , well2->getStatus(1));
    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::OPEN , well3->getStatus(1));


    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::SHUT , well1->getStatus(2));
    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::SHUT , well2->getStatus(2));
    BOOST_CHECK_EQUAL( WellCommon::StatusEnum::SHUT , well3->getStatus(2));
}


BOOST_AUTO_TEST_CASE(WellTestWPOLYMER) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_POLYMER");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(30,30,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);


    BOOST_CHECK_EQUAL(4U, sched.numWells());
    BOOST_CHECK(sched.hasWell("INJE01"));
    BOOST_CHECK(sched.hasWell("PROD01"));

    const auto* well1 = sched.getWell("INJE01");
    BOOST_CHECK( well1->isInjector(0));
    {
        const WellPolymerProperties& props_well10 = well1->getPolymerProperties(0);
        BOOST_CHECK_CLOSE(1.5*Metric::PolymerDensity, props_well10.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well11 = well1->getPolymerProperties(1);
        BOOST_CHECK_CLOSE(1.0*Metric::PolymerDensity, props_well11.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well12 = well1->getPolymerProperties(2);
        BOOST_CHECK_CLOSE(0.1*Metric::PolymerDensity, props_well12.m_polymerConcentration, 0.0001);
    }

    const auto* well2 = sched.getWell("INJE02");
    BOOST_CHECK( well2->isInjector(0));
    {
        const WellPolymerProperties& props_well20 = well2->getPolymerProperties(0);
        BOOST_CHECK_CLOSE(2.0*Metric::PolymerDensity, props_well20.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well21 = well2->getPolymerProperties(1);
        BOOST_CHECK_CLOSE(1.5*Metric::PolymerDensity, props_well21.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well22 = well2->getPolymerProperties(2);
        BOOST_CHECK_CLOSE(0.2*Metric::PolymerDensity, props_well22.m_polymerConcentration, 0.0001);
    }

    const auto* well3 = sched.getWell("INJE03");
    BOOST_CHECK( well3->isInjector(0));
    {
        const WellPolymerProperties& props_well30 = well3->getPolymerProperties(0);
        BOOST_CHECK_CLOSE(2.5*Metric::PolymerDensity, props_well30.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well31 = well3->getPolymerProperties(1);
        BOOST_CHECK_CLOSE(2.0*Metric::PolymerDensity, props_well31.m_polymerConcentration, 0.0001);
        const WellPolymerProperties& props_well32 = well3->getPolymerProperties(2);
        BOOST_CHECK_CLOSE(0.3*Metric::PolymerDensity, props_well32.m_polymerConcentration, 0.0001);
    }
}


BOOST_AUTO_TEST_CASE(WellTestWECON) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_WECON");
    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(30,30,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL(3U, sched.numWells());
    BOOST_CHECK(sched.hasWell("INJE01"));
    BOOST_CHECK(sched.hasWell("PROD01"));
    BOOST_CHECK(sched.hasWell("PROD02"));

    const auto* prod1 = sched.getWell("PROD01");
    {
        const WellEconProductionLimits& econ_limit1 = prod1->getEconProductionLimits(0);
        BOOST_CHECK(econ_limit1.onMinOilRate());
        BOOST_CHECK(econ_limit1.onMaxWaterCut());
        BOOST_CHECK(!(econ_limit1.onMinGasRate()));
        BOOST_CHECK(!(econ_limit1.onMaxGasOilRatio()));
        BOOST_CHECK_EQUAL(econ_limit1.maxWaterCut(), 0.95);
        BOOST_CHECK_EQUAL(econ_limit1.minOilRate(), 50.0/86400.);
        BOOST_CHECK_EQUAL(econ_limit1.minGasRate(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.maxGasOilRatio(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.endRun(), false);
        BOOST_CHECK_EQUAL(econ_limit1.followonWell(), "'");
        BOOST_CHECK_EQUAL(econ_limit1.quantityLimit(), WellEcon::RATE);
        BOOST_CHECK_EQUAL(econ_limit1.workover(), WellEcon::CON);
        BOOST_CHECK_EQUAL(econ_limit1.workoverSecondary(), WellEcon::CON);
        BOOST_CHECK(econ_limit1.requireWorkover());
        BOOST_CHECK(econ_limit1.requireSecondaryWorkover());
        BOOST_CHECK(!(econ_limit1.validFollowonWell()));
        BOOST_CHECK(!(econ_limit1.endRun()));
        BOOST_CHECK(econ_limit1.onAnyRatioLimit());
        BOOST_CHECK(econ_limit1.onAnyRateLimit());
        BOOST_CHECK(econ_limit1.onAnyEffectiveLimit());

        const WellEconProductionLimits& econ_limit2 = prod1->getEconProductionLimits(1);
        BOOST_CHECK(!(econ_limit2.onMinOilRate()));
        BOOST_CHECK(econ_limit2.onMaxWaterCut());
        BOOST_CHECK(econ_limit2.onMinGasRate());
        BOOST_CHECK(!(econ_limit2.onMaxGasOilRatio()));
        BOOST_CHECK_EQUAL(econ_limit2.maxWaterCut(), 0.95);
        BOOST_CHECK_EQUAL(econ_limit2.minOilRate(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit2.minGasRate(), 1000./86400.);
        BOOST_CHECK_EQUAL(econ_limit2.maxGasOilRatio(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit2.endRun(), false);
        BOOST_CHECK_EQUAL(econ_limit2.followonWell(), "'");
        BOOST_CHECK_EQUAL(econ_limit2.quantityLimit(), WellEcon::RATE);
        BOOST_CHECK_EQUAL(econ_limit2.workover(), WellEcon::CON);
        BOOST_CHECK_EQUAL(econ_limit2.workoverSecondary(), WellEcon::CON);
        BOOST_CHECK(econ_limit2.requireWorkover());
        BOOST_CHECK(econ_limit2.requireSecondaryWorkover());
        BOOST_CHECK(!(econ_limit2.validFollowonWell()));
        BOOST_CHECK(!(econ_limit2.endRun()));
        BOOST_CHECK(econ_limit2.onAnyRatioLimit());
        BOOST_CHECK(econ_limit2.onAnyRateLimit());
        BOOST_CHECK(econ_limit2.onAnyEffectiveLimit());
    }

    const auto* prod2 = sched.getWell("PROD02");
    {
        const WellEconProductionLimits& econ_limit1 = prod2->getEconProductionLimits(0);
        BOOST_CHECK(!(econ_limit1.onMinOilRate()));
        BOOST_CHECK(!(econ_limit1.onMaxWaterCut()));
        BOOST_CHECK(!(econ_limit1.onMinGasRate()));
        BOOST_CHECK(!(econ_limit1.onMaxGasOilRatio()));
        BOOST_CHECK_EQUAL(econ_limit1.maxWaterCut(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.minOilRate(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.minGasRate(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.maxGasOilRatio(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit1.endRun(), false);
        BOOST_CHECK_EQUAL(econ_limit1.followonWell(), "'");
        BOOST_CHECK_EQUAL(econ_limit1.quantityLimit(), WellEcon::RATE);
        BOOST_CHECK_EQUAL(econ_limit1.workover(), WellEcon::NONE);
        BOOST_CHECK_EQUAL(econ_limit1.workoverSecondary(), WellEcon::NONE);
        BOOST_CHECK(!(econ_limit1.requireWorkover()));
        BOOST_CHECK(!(econ_limit1.requireSecondaryWorkover()));
        BOOST_CHECK(!(econ_limit1.validFollowonWell()));
        BOOST_CHECK(!(econ_limit1.endRun()));
        BOOST_CHECK(!(econ_limit1.onAnyRatioLimit()));
        BOOST_CHECK(!(econ_limit1.onAnyRateLimit()));
        BOOST_CHECK(!(econ_limit1.onAnyEffectiveLimit()));

        const WellEconProductionLimits& econ_limit2 = prod2->getEconProductionLimits(1);
        BOOST_CHECK(!(econ_limit2.onMinOilRate()));
        BOOST_CHECK(econ_limit2.onMaxWaterCut());
        BOOST_CHECK(econ_limit2.onMinGasRate());
        BOOST_CHECK(!(econ_limit2.onMaxGasOilRatio()));
        BOOST_CHECK_EQUAL(econ_limit2.maxWaterCut(), 0.95);
        BOOST_CHECK_EQUAL(econ_limit2.minOilRate(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit2.minGasRate(), 1000.0/86400.);
        BOOST_CHECK_EQUAL(econ_limit2.maxGasOilRatio(), 0.0);
        BOOST_CHECK_EQUAL(econ_limit2.endRun(), false);
        BOOST_CHECK_EQUAL(econ_limit2.followonWell(), "'");
        BOOST_CHECK_EQUAL(econ_limit2.quantityLimit(), WellEcon::RATE);
        BOOST_CHECK_EQUAL(econ_limit2.workover(), WellEcon::CON);
        BOOST_CHECK_EQUAL(econ_limit2.workoverSecondary(), WellEcon::CON);
        BOOST_CHECK(econ_limit2.requireWorkover());
        BOOST_CHECK(econ_limit2.requireSecondaryWorkover());
        BOOST_CHECK(!(econ_limit2.validFollowonWell()));
        BOOST_CHECK(!(econ_limit2.endRun()));
        BOOST_CHECK(econ_limit2.onAnyRatioLimit());
        BOOST_CHECK(econ_limit2.onAnyRateLimit());
        BOOST_CHECK(econ_limit2.onAnyEffectiveLimit());
    }
}


BOOST_AUTO_TEST_CASE(TestEvents) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_EVENTS");

    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,40,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck , grid , eclipseProperties, Phases(true, true, true), parseContext);
    const Events& events = sched.getEvents();

    BOOST_CHECK(  events.hasEvent(ScheduleEvents::NEW_WELL , 0 ) );
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::NEW_WELL , 1 ) );
    BOOST_CHECK(  events.hasEvent(ScheduleEvents::NEW_WELL , 2 ) );
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::NEW_WELL , 3 ) );

    BOOST_CHECK(  events.hasEvent(ScheduleEvents::COMPLETION_CHANGE , 0 ) );
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::COMPLETION_CHANGE , 1) );
    BOOST_CHECK(  events.hasEvent(ScheduleEvents::COMPLETION_CHANGE , 5 ) );

    BOOST_CHECK(  events.hasEvent(ScheduleEvents::WELL_STATUS_CHANGE , 1 ));
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::WELL_STATUS_CHANGE , 2 ));
    BOOST_CHECK( events.hasEvent(ScheduleEvents::WELL_STATUS_CHANGE , 3 ));
    BOOST_CHECK( events.hasEvent(ScheduleEvents::COMPLETION_CHANGE , 5) );

    BOOST_CHECK(  events.hasEvent(ScheduleEvents::GROUP_CHANGE , 0 ));
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::GROUP_CHANGE , 1 ));
    BOOST_CHECK(  events.hasEvent(ScheduleEvents::GROUP_CHANGE , 3 ) );
    BOOST_CHECK( !events.hasEvent(ScheduleEvents::NEW_GROUP , 2 ) );
    BOOST_CHECK(  events.hasEvent(ScheduleEvents::NEW_GROUP , 3 ) );
}


BOOST_AUTO_TEST_CASE(TestWellEvents) {
    ParseContext parseContext;
    Parser parser;
    std::string scheduleFile(pathprefix() + "SCHEDULE/SCHEDULE_EVENTS");

    auto deck =  parser.parseFile(scheduleFile, parseContext);
    EclipseGrid grid(40,40,30);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule sched(deck , grid , eclipseProperties, Phases(true, true, true), parseContext);
    const auto& w1 = sched.getWell( "W_1");
    const auto& w2 = sched.getWell( "W_2");


    BOOST_CHECK( w1->hasEvent( ScheduleEvents::NEW_WELL , 0 ));
    BOOST_CHECK( w2->hasEvent( ScheduleEvents::NEW_WELL , 2 ));
    BOOST_CHECK( !w2->hasEvent( ScheduleEvents::NEW_WELL , 3 ));
    BOOST_CHECK( w2->hasEvent( ScheduleEvents::WELL_WELSPECS_UPDATE , 3 ));


    BOOST_CHECK( w1->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 0 ));
    BOOST_CHECK( w1->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 1 ));
    BOOST_CHECK( w1->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 3 ));
    BOOST_CHECK( w1->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 4 ));
    BOOST_CHECK( !w1->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 5 ));

    BOOST_CHECK( w1->hasEvent( ScheduleEvents::COMPLETION_CHANGE , 0 ));
    BOOST_CHECK( w1->hasEvent( ScheduleEvents::COMPLETION_CHANGE , 5 ));

    BOOST_CHECK_EQUAL( w1->firstTimeStep( ) , 0 );
    BOOST_CHECK_EQUAL( w2->firstTimeStep( ) , 2 );
}
