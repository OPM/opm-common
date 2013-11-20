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


BOOST_AUTO_TEST_CASE( WellTestCOMPDAT ) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_WELLS2");
    DeckPtr deck = parser->parse(scheduleFile.string());
    ScheduleConstPtr sched( new Schedule(deck));

    BOOST_CHECK_EQUAL( 3U , sched->numWells());
    BOOST_CHECK( sched->hasWell("W_1"));
    BOOST_CHECK( sched->hasWell("W_2"));
    BOOST_CHECK( sched->hasWell("W_3"));
    {
        WellPtr well1 = sched->getWell("W_1");
        BOOST_CHECK_EQUAL( 13000 , well1->getOilRate( 8 ));
        CompletionSetConstPtr completions = well1->getCompletions( 0 );
        BOOST_CHECK_EQUAL( 0U , completions->size() );

        
        completions = well1->getCompletions( 3 );
        BOOST_CHECK_EQUAL( 4U , completions->size() );
        BOOST_CHECK_EQUAL( OPEN , completions->get(3)->getState() );

        completions = well1->getCompletions( 7 );
        BOOST_CHECK_EQUAL( 4U , completions->size() );
        BOOST_CHECK_EQUAL( SHUT , completions->get(3)->getState() );
    }
}
