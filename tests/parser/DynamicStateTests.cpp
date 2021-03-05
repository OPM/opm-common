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

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>


Opm::TimeMap make_timemap(int num) {
    std::vector<std::time_t> tp;
    for (int i = 0; i < num; i++)
        tp.push_back( Opm::asTimeT(Opm::TimeStampUTC(2010,1,i+1)));

    Opm::TimeMap timeMap{ tp };
    return timeMap;
}



BOOST_AUTO_TEST_CASE(CreateDynamicTest) {
    const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
    Opm::TimeMap timeMap({ startDate });
    Opm::DynamicState<double> state(timeMap , 9.99);
}


BOOST_AUTO_TEST_CASE(DynamicStateGetOutOfRangeThrows) {
    const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
    Opm::TimeMap timeMap({ startDate });
    Opm::DynamicState<double> state(timeMap , 9.99);
    BOOST_CHECK_THROW( state.get(1) , std::out_of_range );
}


BOOST_AUTO_TEST_CASE(DynamicStateGetDefault) {
    const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
    Opm::TimeMap timeMap( { startDate } );
    Opm::DynamicState<int> state(timeMap , 137);
    BOOST_CHECK_EQUAL( 137 , state.get(0));
    BOOST_CHECK_EQUAL( 137 , state.back() );
}


BOOST_AUTO_TEST_CASE(DynamicStateSetOutOfRangeThrows) {
    Opm::TimeMap timeMap = make_timemap(3);
    Opm::DynamicState<int> state(timeMap , 137);

    BOOST_CHECK_THROW( state.update(3 , 100) , std::out_of_range );
}


BOOST_AUTO_TEST_CASE(DynamicStateSetOK) {
    Opm::TimeMap timeMap = make_timemap(11);
    Opm::DynamicState<int> state(timeMap , 137);

    state.update(2 , 23 );
    BOOST_CHECK_EQUAL( 137 , state.get(0));
    BOOST_CHECK_EQUAL( 137 , state.get(1));
    BOOST_CHECK_EQUAL( 23 , state.get(2));
    BOOST_CHECK_EQUAL( 23 , state.get(5));

    state.update(2 , 17);
    BOOST_CHECK_EQUAL( 137 , state.get(0));
    BOOST_CHECK_EQUAL( 137 , state.get(1));
    BOOST_CHECK_EQUAL( 17 , state.get(2));
    BOOST_CHECK_EQUAL( 17 , state.get(5));

    state.update(6 , 60);
    BOOST_CHECK_EQUAL( 17 , state.get(2));
    BOOST_CHECK_EQUAL( 17 , state.get(5));
    BOOST_CHECK_EQUAL( 60 , state.get(6));
    BOOST_CHECK_EQUAL( 60 , state.get(8));
    BOOST_CHECK_EQUAL( 60 , state.get(9));
    BOOST_CHECK_EQUAL( 60 , state.back());
}


BOOST_AUTO_TEST_CASE( ResetGlobal ) {
    Opm::TimeMap timeMap = make_timemap(11);
    Opm::DynamicState<int> state(timeMap , 137);

    state.update(5 , 100);
    BOOST_CHECK_EQUAL( state.get(0) , 137 );
    BOOST_CHECK_EQUAL( state.get(4) , 137 );
    BOOST_CHECK_EQUAL( state.get(5) , 100 );
    BOOST_CHECK_EQUAL( state.get(9) , 100 );


    state.globalReset( 88 );
    BOOST_CHECK_EQUAL( state.get(0) , 88 );
    BOOST_CHECK_EQUAL( state.get(4) , 88 );
    BOOST_CHECK_EQUAL( state.get(5) , 88 );
    BOOST_CHECK_EQUAL( state.get(9) , 88 );
}


BOOST_AUTO_TEST_CASE( CheckReturn ) {
    Opm::TimeMap timeMap = make_timemap(11);
    Opm::DynamicState<int> state(timeMap , 137);

    BOOST_CHECK_EQUAL( false , state.update( 0 , 137 ));
    BOOST_CHECK_EQUAL( false , state.update( 3 , 137 ));
    BOOST_CHECK_EQUAL( true , state.update( 5 , 200 ));
}




