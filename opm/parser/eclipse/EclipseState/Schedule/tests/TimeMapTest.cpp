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
#include <opm/parser/eclipse/Parser/ParseMode.hpp>

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
    std::shared_ptr<Opm::TimeMap> timeMap = std::make_shared<Opm::TimeMap>(boost::posix_time::ptime(boost::posix_time::ptime(startDate)));

    timeMap->addTStep( boost::posix_time::hours(1));
    timeMap->addTStep( boost::posix_time::hours(23));
    BOOST_CHECK_EQUAL( 3U , timeMap->size());

    BOOST_CHECK_THROW( (*timeMap)[3] , std::invalid_argument );
    BOOST_CHECK_EQUAL( (*timeMap)[0] , boost::posix_time::ptime(boost::posix_time::ptime(startDate)));
    BOOST_CHECK_EQUAL( (*timeMap)[2] , boost::posix_time::ptime(boost::posix_time::ptime( boost::gregorian::date( 2010 , boost::gregorian::Jan , 2 ))));
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
    Opm::DeckPtr deck = parser->parseString(deckData, Opm::ParseMode());
    Opm::TimeMap tmap(deck);

    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/0),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/0), 1*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimePassedUntil(/*timeLevelIdx=*/1), 1.0*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/1),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21),
                                               boost::posix_time::time_duration(1*24, 0, 0)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/1), 2*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimePassedUntil(/*timeLevelIdx=*/2), (1.0 + 2.0)*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/2),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21),
                                               boost::posix_time::time_duration((1 + 2)*24, 0, 0)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/2), 3*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimePassedUntil(/*timeLevelIdx=*/3), (1.0 + 2.0 + 3.0)*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/3),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21),
                                               boost::posix_time::time_duration((1 + 2 + 3)*24, 0, 0)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/3), 4*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimePassedUntil(/*timeLevelIdx=*/4), (1.0 + 2.0 + 3.0 + 4.0)*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/4),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21),
                                               boost::posix_time::time_duration((1 + 2 + 3 + 4)*24, 0, 0)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/4), 5*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimePassedUntil(/*timeLevelIdx=*/5), (1.0 + 2.0 + 3.0 + 4.0 + 5.0)*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIdx=*/5),
                      boost::posix_time::ptime(boost::gregorian::date(1981, 5, 21),
                                               boost::posix_time::time_duration((1 + 2 + 3 + 4 + 5)*24, 0, 0)));
    // timestep 5 is the period between the last step specified using
    // of the TIMES keyword and the first record of DATES
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIndex=*/6),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 1)));
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIndex=*/7),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 1),
                                               boost::posix_time::time_duration(13, 55, 44)));
    BOOST_CHECK_EQUAL(tmap.getStartTime(/*timeLevelIndex=*/8),
                      boost::posix_time::ptime(boost::gregorian::date(1982, 1, 3),
                                               boost::posix_time::time_duration(14, 56, 45) +
                                               boost::posix_time::milliseconds(123)));
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/8), 6*24*60*60);
    BOOST_CHECK_EQUAL(tmap.getTimeStepLength(/*index=*/9), 7*24*60*60);
}


BOOST_AUTO_TEST_CASE(initTimestepsYearsAndMonths) {
    const char *deckData =
        "START\n"
        " 21 MAY 1981 /\n"
        "\n"
        "TSTEP\n"
        " 1 2 3 4 5 /\n"
        "\n"
        "DATES\n"
        " 5 JUL 1981 /\n"
        " 6 JUL 1981 /\n"
        " 5 AUG 1981 /\n"
        " 5 SEP 1981 /\n"
        " 1 OCT 1981 /\n"
        " 1 NOV 1981 /\n"
        " 1 DEC 1981 /\n"
        " 1 JAN 1982 /\n"
        " 1 JAN 1982 13:55:44 /\n"
        " 3 JAN 1982 14:56:45.123 /\n"
        "/\n"
        "\n"
        "TSTEP\n"
        " 6 7 /\n";

    Opm::ParserPtr parser(new Opm::Parser(/*addDefault=*/true));
    Opm::DeckPtr deck = parser->parseString(deckData, Opm::ParseMode());
    Opm::TimeMap tmap(deck);

    /*deckData timesteps:
    0   21 may  1981 START
    1   22 may  1981
    2   24 may  1981
    3   27 may  1981
    4   31 may  1981
    5   5  jun 1981
    6   5  jul 1981
    7   6  jul 1981
    8   5  aug 1981
    9   5  sep 1981
    10  1  oct 1981
    11  1  nov 1981
    12  1  dec 1981
    13  1  jan  1982
    14  1  jan  1982
    15  3  jan  1982
    16  9  jan  1982
    17  16 jan  1982
   */

    std::vector<size_t> first_timestep_of_each_month;
    tmap.initFirstTimestepsMonths(first_timestep_of_each_month);
    BOOST_CHECK_EQUAL(8, first_timestep_of_each_month.size());
    int expected_results[8] = {5,6,8,9,10,11,12,13};
    BOOST_CHECK_EQUAL_COLLECTIONS(expected_results, expected_results+8, first_timestep_of_each_month.begin(), first_timestep_of_each_month.end());

    first_timestep_of_each_month.clear();
    tmap.initFirstTimestepsMonths(first_timestep_of_each_month, 6);
    BOOST_CHECK_EQUAL(7, first_timestep_of_each_month.size());
    int expected_results3[7] = {6,8,9,10,11,12,13};
    BOOST_CHECK_EQUAL_COLLECTIONS(expected_results3, expected_results3+7, first_timestep_of_each_month.begin(), first_timestep_of_each_month.end());

    std::vector<size_t> first_timestep_of_each_year;
    tmap.initFirstTimestepsYears(first_timestep_of_each_year);
    BOOST_CHECK_EQUAL(1, first_timestep_of_each_year.size());
    int expected_results_years[1] = {13};
    BOOST_CHECK_EQUAL_COLLECTIONS(expected_results_years, expected_results_years+1, first_timestep_of_each_year.begin(), first_timestep_of_each_year.end());

    first_timestep_of_each_year.clear();
    tmap.initFirstTimestepsYears(first_timestep_of_each_year, 13);
    BOOST_CHECK_EQUAL(1, first_timestep_of_each_year.size());
}

