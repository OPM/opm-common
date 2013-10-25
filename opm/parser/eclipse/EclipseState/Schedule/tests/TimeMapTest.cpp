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


BOOST_AUTO_TEST_CASE(CreateTimeMap_InvalidThrow) {
    boost::gregorian::date startDate;
    BOOST_CHECK_THROW(Opm::TimeMap timeMap(startDate) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CreateTimeMap) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);
    BOOST_CHECK_EQUAL(1U , timeMap.size());
}



BOOST_AUTO_TEST_CASE(AddDateBeforeThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);

    BOOST_CHECK_THROW( timeMap.addDate( boost::gregorian::date(2009,boost::gregorian::Feb,2))  , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(GetStartDate) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);
    BOOST_CHECK_EQUAL( startDate , timeMap.getStartDate());
}



BOOST_AUTO_TEST_CASE(AddDateAfterSizeCorrect) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);

    timeMap.addDate( boost::gregorian::date(2010,boost::gregorian::Feb,2));
    BOOST_CHECK_EQUAL( 2U , timeMap.size());
}


BOOST_AUTO_TEST_CASE(AddDateNegativeStepThrows) {
  boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);

    BOOST_CHECK_THROW( timeMap.addTStep( boost::posix_time::hours(-1)) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(AddStepSizeCorrect) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap( new Opm::TimeMap(startDate) );

    timeMap->addTStep( boost::posix_time::hours(1));
    timeMap->addTStep( boost::posix_time::hours(24));
    BOOST_CHECK_EQUAL( 3U , timeMap->size());
}


BOOST_AUTO_TEST_CASE( dateFromEclipseThrowsInvalidRecord ) {
    Opm::DeckRecordPtr  startRecord(new  Opm::DeckRecord());
    Opm::DeckIntItemPtr    dayItem( new  Opm::DeckIntItem("DAY") );
    Opm::DeckStringItemPtr monthItem(new Opm::DeckStringItem("MONTH") );
    Opm::DeckIntItemPtr    yearItem(new  Opm::DeckIntItem("YEAR"));
    Opm::DeckIntItemPtr    extraItem(new  Opm::DeckIntItem("EXTRA"));

    dayItem->push_back( 10 );
    yearItem->push_back(1987 );
    monthItem->push_back("FEB");

    BOOST_CHECK_THROW( Opm::TimeMap::dateFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( dayItem );
    BOOST_CHECK_THROW( Opm::TimeMap::dateFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( monthItem );
    BOOST_CHECK_THROW( Opm::TimeMap::dateFromEclipse( startRecord ) , std::invalid_argument );

    startRecord->addItem( yearItem );
    BOOST_CHECK_NO_THROW(Opm::TimeMap::dateFromEclipse( startRecord ));
  
    startRecord->addItem( extraItem );
    BOOST_CHECK_THROW( Opm::TimeMap::dateFromEclipse( startRecord ) , std::invalid_argument );
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

    BOOST_CHECK_THROW( Opm::TimeMap::dateFromEclipse( startRecord ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE( dateFromEclipseCheckMonthNames ) {

    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Jan , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "JAN" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Feb , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "FEB" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Mar , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "MAR" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Apr , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "APR" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::May , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "MAI" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::May , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "MAY" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Jun , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "JUN" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Jul , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "JUL" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Jul , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "JLY" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Aug , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "AUG" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Sep , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "SEP" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Oct , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "OKT" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Oct , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "OCT" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Nov , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "NOV" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Dec , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "DEC" , 2000));
    BOOST_CHECK_EQUAL( boost::gregorian::date( 2000 , boost::gregorian::Dec , 1 ) , Opm::TimeMap::dateFromEclipse( 1 , "DES" , 2000));

}



BOOST_AUTO_TEST_CASE( dateFromEclipseInputRecord ) {
    Opm::DeckRecordPtr  startRecord(new  Opm::DeckRecord());
    Opm::DeckIntItemPtr    dayItem( new  Opm::DeckIntItem("DAY") );
    Opm::DeckStringItemPtr monthItem(new Opm::DeckStringItem("MONTH") );
    Opm::DeckIntItemPtr    yearItem(new  Opm::DeckIntItem("YEAR"));

    dayItem->push_back( 10 );
    yearItem->push_back( 1987 );
    monthItem->push_back("JAN");

    startRecord->addItem( dayItem );
    startRecord->addItem( monthItem );
    startRecord->addItem( yearItem );

    BOOST_CHECK_EQUAL( boost::gregorian::date( 1987 , boost::gregorian::Jan , 10 ) , Opm::TimeMap::dateFromEclipse( startRecord ));
}
