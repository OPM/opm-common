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


#define BOOST_TEST_MODULE DynamicStateTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>



BOOST_AUTO_TEST_CASE(CreateDynamicTest) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<double> state(timeMap , 9.99);
}


BOOST_AUTO_TEST_CASE(DynamicStateGetOutOfRangeThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<double> state(timeMap , 9.99);
    BOOST_CHECK_THROW( state.get(1) , std::range_error);
}


BOOST_AUTO_TEST_CASE(DynamicStateGetDefault) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<int> state(timeMap , 137);
    BOOST_CHECK_EQUAL( 137 , state.get(0));
}


BOOST_AUTO_TEST_CASE(DynamicStateSetOutOfRangeThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<int> state(timeMap , 137);
    for (size_t i = 0; i < 2; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    
    BOOST_CHECK_THROW( state.add(3 , 100) , std::range_error);
}


BOOST_AUTO_TEST_CASE(DynamicStateSetOK) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<int> state(timeMap , 137);
    for (size_t i = 0; i < 10; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    
    state.add(2 , 23 );
    BOOST_CHECK_EQUAL( 137 , state.get(0));
    BOOST_CHECK_EQUAL( 137 , state.get(1));
    BOOST_CHECK_EQUAL( 23 , state.get(2));
    BOOST_CHECK_EQUAL( 23 , state.get(5));

    state.add(2 , 17);
    BOOST_CHECK_EQUAL( 137 , state.get(0));
    BOOST_CHECK_EQUAL( 137 , state.get(1));
    BOOST_CHECK_EQUAL( 17 , state.get(2));
    BOOST_CHECK_EQUAL( 17 , state.get(5));
    
    state.add(6 , 60);
    BOOST_CHECK_EQUAL( 17 , state.get(2));
    BOOST_CHECK_EQUAL( 17 , state.get(5));
    BOOST_CHECK_EQUAL( 60 , state.get(6));
    BOOST_CHECK_EQUAL( 60 , state.get(8));
    BOOST_CHECK_EQUAL( 60 , state.get(9));
}



BOOST_AUTO_TEST_CASE(DynamicStateAddIndexAlreadySetThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<int> state(timeMap , 137);
    for (size_t i = 0; i < 10; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));

    state.add( 5 , 60);
    BOOST_CHECK_THROW( state.add(3 , 78) , std::invalid_argument);
}




BOOST_AUTO_TEST_CASE(DynamicStateCheckSize) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    Opm::DynamicState<int> state(timeMap , 137);
    for (size_t i = 0; i < 10; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));

    BOOST_CHECK_EQUAL( 0U , state.size() );

    state.add( 0 , 10 );
    BOOST_CHECK_EQUAL( 1U , state.size() );

    state.add( 2 , 10 );
    BOOST_CHECK_EQUAL( 3U , state.size() );
    state.add( 2 , 10 );
    BOOST_CHECK_EQUAL( 3U , state.size() );

    state.add( 6 , 10 );
    BOOST_CHECK_EQUAL( 7U , state.size() );
}
