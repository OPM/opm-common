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
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateSchedule) {

    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE1");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));
    TimeMapConstPtr timeMap = sched->getTimeMap();
    BOOST_CHECK_EQUAL(boost::gregorian::date(2007, boost::gregorian::May, 10), sched->getStartDate());
    BOOST_CHECK_EQUAL(9U, timeMap->size());
    BOOST_CHECK( deck->hasKeyword("NETBALAN") );
}


BOOST_AUTO_TEST_CASE(CreateSchedule_Comments_After_Keywords) {

    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_COMMENTS_AFTER_KEYWORDS");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));
    TimeMapConstPtr timeMap = sched->getTimeMap();
    BOOST_CHECK_EQUAL(boost::gregorian::date(2007, boost::gregorian::May, 10), sched->getStartDate());
    BOOST_CHECK_EQUAL(9U, timeMap->size());

}


BOOST_AUTO_TEST_CASE(WellTesting) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELLS2");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));

    BOOST_CHECK_EQUAL(3U, sched->numWells());
    BOOST_CHECK(sched->hasWell("W_1"));
    BOOST_CHECK(sched->hasWell("W_2"));
    BOOST_CHECK(sched->hasWell("W_3"));

    {
        WellPtr well1 = sched->getWell("W_1");

        BOOST_CHECK(well1->isInPredictionMode(0));
        BOOST_CHECK_EQUAL(0, well1->getOilRate(0));

        BOOST_CHECK_EQUAL(0, well1->getOilRate(1));
        BOOST_CHECK_EQUAL(0, well1->getOilRate(2));

        BOOST_CHECK(!well1->isInPredictionMode(3));
        BOOST_CHECK_EQUAL(4000, well1->getOilRate(3));
        BOOST_CHECK_EQUAL(4000, well1->getOilRate(4));
        BOOST_CHECK_EQUAL(4000, well1->getOilRate(5));
        BOOST_CHECK_EQUAL(4     , well1->getWaterRate(3));
        BOOST_CHECK_EQUAL(12345 , well1->getGasRate(3));
        BOOST_CHECK_EQUAL(4     , well1->getWaterRate(4));
        BOOST_CHECK_EQUAL(12345 , well1->getGasRate(4));
        BOOST_CHECK_EQUAL(4     , well1->getWaterRate(5));
        BOOST_CHECK_EQUAL(12345 , well1->getGasRate(5));


        BOOST_CHECK(!well1->isInPredictionMode(6));
        BOOST_CHECK_EQUAL(14000, well1->getOilRate(6));

        BOOST_CHECK(well1->isInPredictionMode(7));
        BOOST_CHECK_EQUAL(11000, well1->getOilRate(7));
        BOOST_CHECK_EQUAL(44   , well1->getWaterRate(7));
        BOOST_CHECK_EQUAL(188  , well1->getGasRate(7));

        BOOST_CHECK(!well1->isInPredictionMode(8));
        BOOST_CHECK_EQUAL(13000, well1->getOilRate(8));
        
        BOOST_CHECK( well1->isInjector(9));
        BOOST_CHECK_EQUAL(20000, well1->getInjectionRate(9));
        BOOST_CHECK_EQUAL(5000, well1->getInjectionRate(10));
    }
}

BOOST_AUTO_TEST_CASE(WellTestCOMPDAT) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELLS2");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));

    BOOST_CHECK_EQUAL(3U, sched->numWells());
    BOOST_CHECK(sched->hasWell("W_1"));
    BOOST_CHECK(sched->hasWell("W_2"));
    BOOST_CHECK(sched->hasWell("W_3"));
    {
        WellPtr well1 = sched->getWell("W_1");
        BOOST_CHECK_EQUAL(13000, well1->getOilRate(8));
        CompletionSetConstPtr completions = well1->getCompletions(0);
        BOOST_CHECK_EQUAL(0U, completions->size());


        completions = well1->getCompletions(3);
        BOOST_CHECK_EQUAL(4U, completions->size());
        BOOST_CHECK_EQUAL(OPEN, completions->get(3)->getState());

        completions = well1->getCompletions(7);
        BOOST_CHECK_EQUAL(4U, completions->size());
        BOOST_CHECK_EQUAL(SHUT, completions->get(3)->getState());
    }
}

BOOST_AUTO_TEST_CASE(GroupTreeTest_GRUPTREE_with_explicit_L0_parenting) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_GRUPTREE_EXPLICIT_PARENTING");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));

    GroupTreeNodePtr rootNode = sched->getGroupTree(0)->getNode("FIELD");

    sched->getGroupTree(0)->printTree();

    BOOST_REQUIRE_EQUAL("FIELD", rootNode->name());

    BOOST_CHECK(rootNode->hasChildGroup("FIRST_LEVEL1"));
    GroupTreeNodePtr FIRST_LEVEL1 = rootNode->getChildGroup("FIRST_LEVEL1");
    BOOST_CHECK(rootNode->hasChildGroup("FIRST_LEVEL2"));
    GroupTreeNodePtr FIRST_LEVEL2 = rootNode->getChildGroup("FIRST_LEVEL2");

    BOOST_CHECK(FIRST_LEVEL1->hasChildGroup("SECOND_LEVEL1"));
    GroupTreeNodePtr SECOND_LEVEL1 = FIRST_LEVEL1->getChildGroup("SECOND_LEVEL1");

    BOOST_CHECK(FIRST_LEVEL2->hasChildGroup("SECOND_LEVEL2"));
    GroupTreeNodePtr SECOND_LEVEL2 = FIRST_LEVEL2->getChildGroup("SECOND_LEVEL2");

    BOOST_CHECK(SECOND_LEVEL1->hasChildGroup("THIRD_LEVEL1"));
    GroupTreeNodePtr THIRD_LEVEL1 = SECOND_LEVEL1->getChildGroup("THIRD_LEVEL1");
}

BOOST_AUTO_TEST_CASE(GroupTreeTest_WELSPECS_AND_GRUPTREE_correct_tree) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELSPECS_GROUPS");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr schedule(new Schedule(deck));

    // Time 0, only from WELSPECS
    GroupTreeNodePtr root0 = schedule->getGroupTree(0)->getNode("FIELD");
    BOOST_REQUIRE_EQUAL("FIELD", root0->name());

    BOOST_CHECK(root0->hasChildGroup("GROUP_BJARNE"));
    GroupTreeNodePtr GROUP_BJARNE = root0->getChildGroup("GROUP_BJARNE");
    BOOST_CHECK_EQUAL("GROUP_BJARNE", GROUP_BJARNE->name());

    BOOST_CHECK(root0->hasChildGroup("GROUP_ODD"));
    GroupTreeNodePtr GROUP_ODD = root0->getChildGroup("GROUP_ODD");
    BOOST_CHECK_EQUAL("GROUP_ODD", GROUP_ODD->name());

    // Time 1, now also from GRUPTREE
    GroupTreeNodePtr root1 = schedule->getGroupTree(1)->getNode("FIELD");
    BOOST_REQUIRE_EQUAL("FIELD", root1->name());
    BOOST_CHECK(root1->hasChildGroup("GROUP_BJARNE"));
    GroupTreeNodePtr GROUP_BJARNE1 = root1->getChildGroup("GROUP_BJARNE");
    BOOST_CHECK_EQUAL("GROUP_BJARNE", GROUP_BJARNE1->name());

    BOOST_CHECK(root1->hasChildGroup("GROUP_ODD"));
    GroupTreeNodePtr GROUP_ODD1 = root1->getChildGroup("GROUP_ODD");
    BOOST_CHECK_EQUAL("GROUP_ODD", GROUP_ODD1->name());

    // - from GRUPTREE
    
    BOOST_CHECK(GROUP_BJARNE1->hasChildGroup("GROUP_BIRGER"));
    GroupTreeNodePtr GROUP_BIRGER = GROUP_BJARNE1->getChildGroup("GROUP_BIRGER");
    BOOST_CHECK_EQUAL("GROUP_BIRGER", GROUP_BIRGER->name());

    BOOST_CHECK(root1->hasChildGroup("GROUP_NEW"));
    GroupTreeNodePtr GROUP_NEW = root1->getChildGroup("GROUP_NEW");
    BOOST_CHECK_EQUAL("GROUP_NEW", GROUP_NEW->name());
    
    BOOST_CHECK(GROUP_NEW->hasChildGroup("GROUP_NILS"));
    GroupTreeNodePtr GROUP_NILS = GROUP_NEW->getChildGroup("GROUP_NILS");
    BOOST_CHECK_EQUAL("GROUP_NILS", GROUP_NILS->name());
};

BOOST_AUTO_TEST_CASE(GroupTreeTest_GRUPTREE_WITH_REPARENT_correct_tree) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_GROUPS_REPARENT");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr schedule(new Schedule(deck));

    schedule->getGroupTree(0)->printTree();
    std::cout << std::endl << std::endl;
    schedule->getGroupTree(1)->printTree();

    // Time , from  first GRUPTREE
    GroupTreeNodePtr root0 = schedule->getGroupTree(0)->getNode("FIELD");
    BOOST_REQUIRE_EQUAL("FIELD", root0->name());
    BOOST_CHECK(root0->hasChildGroup("GROUP_BJARNE"));
    GroupTreeNodePtr GROUP_BJARNE0 = root0->getChildGroup("GROUP_BJARNE");
    BOOST_CHECK_EQUAL("GROUP_BJARNE", GROUP_BJARNE0->name());

    BOOST_CHECK(root0->hasChildGroup("GROUP_NEW"));
    GroupTreeNodePtr GROUP_NEW0 = root0->getChildGroup("GROUP_NEW");
    BOOST_CHECK_EQUAL("GROUP_NEW", GROUP_NEW0->name());

    
    BOOST_CHECK(GROUP_BJARNE0->hasChildGroup("GROUP_BIRGER"));
    GroupTreeNodePtr GROUP_BIRGER0 = GROUP_BJARNE0->getChildGroup("GROUP_BIRGER");
    BOOST_CHECK_EQUAL("GROUP_BIRGER", GROUP_BIRGER0->name());

    BOOST_CHECK(GROUP_NEW0->hasChildGroup("GROUP_NILS"));
    GroupTreeNodePtr GROUP_NILS0 = GROUP_NEW0->getChildGroup("GROUP_NILS");
    BOOST_CHECK_EQUAL("GROUP_NILS", GROUP_NILS0->name());
    
    // SÅ den nye strukturen med et barneflytt
};


BOOST_AUTO_TEST_CASE(GroupTreeTest_PrintGrouptree) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELSPECS_GROUPS");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched(new Schedule(deck));

    GroupTreePtr rootNode = sched->getGroupTree(0);
    rootNode->printTree();

}


BOOST_AUTO_TEST_CASE( WellTestGroups ) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_GROUPS");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched( new Schedule(deck));
    
    BOOST_CHECK_EQUAL( 3U , sched->numGroups() );
    BOOST_CHECK( sched->hasGroup( "INJ" ));
    BOOST_CHECK( sched->hasGroup( "OP" ));

    {
        GroupPtr group = sched->getGroup("INJ");
        BOOST_CHECK_EQUAL( WATER , group->getInjectionPhase( 3 ));
        BOOST_CHECK_EQUAL( GroupInjection::VREP , group->getInjectionControlMode( 3 ));
        BOOST_CHECK_EQUAL( 10 , group->getSurfaceMaxRate( 3 ));
        BOOST_CHECK_EQUAL( 20 , group->getReservoirMaxRate( 3 ));
        BOOST_CHECK_EQUAL( 0.75 , group->getTargetReinjectFraction( 3 ));
        BOOST_CHECK_EQUAL( 0.95 , group->getTargetVoidReplacementFraction( 3 ));
    
        BOOST_CHECK_EQUAL( OIL , group->getInjectionPhase( 6 ));
        BOOST_CHECK_EQUAL( GroupInjection::RATE , group->getInjectionControlMode( 6 ));
        BOOST_CHECK_EQUAL( 1000 , group->getSurfaceMaxRate( 6 ));
    }
    
    {
        GroupPtr group = sched->getGroup("OP");
        BOOST_CHECK_EQUAL( GroupProduction::ORAT , group->getProductionControlMode(3));
        BOOST_CHECK_EQUAL( 10 , group->getOilTargetRate(3));
        BOOST_CHECK_EQUAL( 20 , group->getWaterTargetRate(3));
        BOOST_CHECK_EQUAL( 30 , group->getGasTargetRate(3));
        BOOST_CHECK_EQUAL( 40 , group->getLiquidTargetRate(3));
    }

}


BOOST_AUTO_TEST_CASE( WellTestGroupAndWellRelation ) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELLS_AND_GROUPS");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched( new Schedule(deck));

    GroupPtr group1 = sched->getGroup("GROUP1");
    GroupPtr group2 = sched->getGroup("GROUP2");
    
    BOOST_CHECK( group1->hasBeenDefined(0) );
    BOOST_CHECK_EQUAL(false , group2->hasBeenDefined(0));
    BOOST_CHECK( group2->hasBeenDefined(1));

    BOOST_CHECK_EQUAL( true , group1->hasWell("W_1" , 0));
    BOOST_CHECK_EQUAL( true , group1->hasWell("W_2" , 0));
    BOOST_CHECK_EQUAL( false, group2->hasWell("W_1" , 0));
    BOOST_CHECK_EQUAL( false, group2->hasWell("W_2" , 0));
    


    BOOST_CHECK_EQUAL( true  , group1->hasWell("W_1" , 1));
    BOOST_CHECK_EQUAL( false , group1->hasWell("W_2" , 1));
    BOOST_CHECK_EQUAL( false , group2->hasWell("W_1" , 1));
    BOOST_CHECK_EQUAL( true  , group2->hasWell("W_2" , 1));
}

