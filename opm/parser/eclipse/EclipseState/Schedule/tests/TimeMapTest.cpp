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

#define BOOST_TEST_MODULE TimeMapTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

BOOST_AUTO_TEST_CASE(CreateTimeMap_InvalidThrow) {
    boost::gregorian::date startDate;
    BOOST_CHECK_THROW(Opm::TimeMap(boost::posix_time::ptime(startDate)) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CreateTimeMap) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap((boost::posix_time::ptime(startDate)));
    BOOST_CHECK_EQUAL(1U , timeMap.size());
}



BOOST_AUTO_TEST_CASE(AddDateBeforeThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap((boost::posix_time::ptime(startDate)));

    BOOST_CHECK_THROW( timeMap.addTime( boost::posix_time::ptime(boost::gregorian::date(2009,boost::gregorian::Feb,2))),
                       std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(GetStartDate) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    boost::posix_time::ptime startTime(startDate);
    Opm::TimeMap timeMap(startTime);
    BOOST_CHECK_EQUAL( startTime , timeMap.getStartTime(/*timeStepIdx=*/0));
}



BOOST_AUTO_TEST_CASE(AddDateAfterSizeCorrect) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap((boost::posix_time::ptime(startDate)));

    timeMap.addTime( boost::posix_time::ptime(boost::gregorian::date(2010,boost::gregorian::Feb,2)));
    BOOST_CHECK_EQUAL( 2U , timeMap.size());
}


BOOST_AUTO_TEST_CASE(AddDateNegativeStepThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap((boost::posix_time::ptime(startDate)));

    BOOST_CHECK_THROW( timeMap.addTStep( boost::posix_time::hours(-1)) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(AddStepSizeCorrect) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap( new Opm::TimeMap(boost::posix_time::ptime(boost::posix_time::ptime(startDate))) );

    timeMap->addTStep( boost::posix_time::hours(1));
    timeMap->addTStep( boost::posix_time::hours(24));
    BOOST_CHECK_EQUAL( 3U , timeMap->size());
}


BOOST_AUTO_TEST_CASE( dateFromEclipseThrowsInvalidRecord ) {
    Opm::DeckRecordPtr  startRecord(new  Opm::DeckRecord());
    Opm::DeckIntItemPtr    dayItem( new  Opm::DeckIntItem("DAY") );
    Opm::DeckStringItemPtr monthItem(new Opm::DeckStringItem("MONTH") );
    Opm::DeckIntItemPtr    yearItem(new  Opm::DeckIntItem("YEAR"));
    Opm::DeckStringItemPtr timeItem(new Opm::DeckStringItem("TIME") );
    Opm::DeckIntItemPtr    extraItem(new  Opm::DeckIntItem("EXTRA"));

    dayItem->push_back( 10 );
    yearItem->push_back(1987 );
    monthItem->push_back("FEB");
    timeItem->push_back("00:00:00.000");

    BOOST_CHECK_THROW( Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( dayItem );
    BOOST_CHECK_THROW( Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( monthItem );
    BOOST_CHECK_THROW( Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( yearItem );
    BOOST_CHECK_THROW(Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( timeItem );
    BOOST_CHECK_NO_THROW(Opm::TimeMap::timeFromEclipse( startRecord ));

    startRecord->addItem( extraItem );
    BOOST_CHECK_THROW( Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE( dateFromEclipseInvalidMonthThrows ) {
    Opm::DeckRecordPtr  startRecord(new  Opm::DeckRecord());
    Opm::DeckIntItemPtr    dayItem( new  Opm::DeckIntItem("DAY") );
    Opm::DeckStringItemPtr monthItem(new Opm::DeckStringItem("MONTH") );
    Opm::DeckIntItemPtr    yearItem(new  Opm::DeckIntItem("YEAR"));

    dayItem->push_back( 10 );
    yearItem->push_back(1987 );
    monthItem->push_back("XXX");

    startRecord->addItem( dayItem );
    startRecord->addItem( monthItem );
    startRecord->addItem( yearItem );

    BOOST_CHECK_THROW( Opm::TimeMap::timeFromEclipse( startRecord ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE( timeFromEclipseCheckMonthNames ) {

    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Jan , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "JAN" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Feb , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "FEB" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Mar , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "MAR" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Apr , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "APR" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::May , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "MAI" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::May , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "MAY" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Jun , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "JUN" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Jul , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "JUL" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Jul , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "JLY" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Aug , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "AUG" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Sep , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "SEP" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Oct , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "OKT" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Oct , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "OCT" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Nov , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "NOV" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Dec , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "DEC" , 2000));
    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 2000 , boost::gregorian::Dec , 1 )) , Opm::TimeMap::timeFromEclipse( 1 , "DES" , 2000));

}



BOOST_AUTO_TEST_CASE( timeFromEclipseInputRecord ) {
    Opm::DeckRecordPtr  startRecord(new  Opm::DeckRecord());
    Opm::DeckIntItemPtr    dayItem( new  Opm::DeckIntItem("DAY") );
    Opm::DeckStringItemPtr monthItem(new Opm::DeckStringItem("MONTH") );
    Opm::DeckIntItemPtr    yearItem(new  Opm::DeckIntItem("YEAR"));
    Opm::DeckStringItemPtr timeItem(new Opm::DeckStringItem("TIME") );

    dayItem->push_back( 10 );
    yearItem->push_back( 1987 );
    monthItem->push_back("JAN");
    timeItem->push_back("00:00:00.000");

    startRecord->addItem( dayItem );
    startRecord->addItem( monthItem );
    startRecord->addItem( yearItem );
    startRecord->addItem( timeItem );

    BOOST_CHECK_EQUAL( boost::posix_time::ptime(boost::gregorian::date( 1987 , boost::gregorian::Jan , 10 )) , Opm::TimeMap::timeFromEclipse( startRecord ));
}



BOOST_AUTO_TEST_CASE( addDATESFromWrongKeywordThrows ) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap((boost::posix_time::ptime(startDate)));
    Opm::DeckKeywordConstPtr deckKeyword(new Opm::DeckKeyword("NOTDATES"));
    BOOST_CHECK_THROW( timeMap.addFromDATESKeyword( deckKeyword ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE( addTSTEPFromWrongKeywordThrows ) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    boost::posix_time::ptime ptime(startDate);
    Opm::TimeMap timeMap(ptime);
    Opm::DeckKeywordConstPtr deckKeyword(new Opm::DeckKeyword("NOTTSTEP"));
    BOOST_CHECK_THROW( timeMap.addFromTSTEPKeyword( deckKeyword ) , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(TimeStepsCorrect) {
    const char *deckData =
        "START\n"
        " 21 MAY 1981 /\n"
        "\n"
        "TSTEP\n"
        " 1 2 3 4 5 /\n"
        "\n"
        "DATES\n"
        " 1 JAN 1982 /\n"
        " 1 JAN 1982 13:55:44 /\n"
        " 3 JAN 1982 14:56:45.123 /\n"
        "/\n"
        "\n"
        "TSTEP\n"
        " 6 7 /\n";

    Opm::ParserPtr parser(new Opm::Parser(/*addDefault=*/true));
    Opm::DeckPtr deck = parser->parseString(deckData);
    Opm::TimeMap tmap(deck);

    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeStepIdx=*/0),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/0), 1*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/1), 2*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/2), 3*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/3), 4*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/4), 5*24*60*60);
    // timestep 5 is the period between the last step specified using
    // of the TIMES keyword and the first record of DATES
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeStepIndex=*/6),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 1)));
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeStepIndex=*/7),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 1),
                                               boost::posix_time::time_duration(13, 55, 44)));
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeStepIndex=*/8),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 3),
                                               boost::posix_time::time_duration(14, 56, 45) +
                                               boost::posix_time::milliseconds(123)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/8), 6*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/9), 7*24*60*60);
}
