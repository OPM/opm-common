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

#define BOOST_TEST_MODULE GroupTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

Opm::TimeMapPtr createXDaysTimeMap(size_t numDays) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(startDate));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}



BOOST_AUTO_TEST_CASE(CreateGroup_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap);
    BOOST_CHECK_EQUAL( "G1" , group.name() );
}


BOOST_AUTO_TEST_CASE(InjectRateOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap);
    BOOST_CHECK_EQUAL( 0 , group.getInjectionRate( 0 ));
    group.setInjectionRate( 2 , 100 );
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 2 ));
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 8 ));
}


BOOST_AUTO_TEST_CASE(ControlModeOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap);
    BOOST_CHECK_EQUAL( Opm::NONE , group.getInjectionControlMode( 0 ));
    group.setInjectionControlMode( 2 , Opm::RESV );
    BOOST_CHECK_EQUAL( Opm::RESV , group.getInjectionControlMode( 2 ));
    BOOST_CHECK_EQUAL( Opm::RESV , group.getInjectionControlMode( 8 ));
}



BOOST_AUTO_TEST_CASE(GroupChangePhaseSameTimeThrows) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap);
    BOOST_CHECK_EQUAL( Opm::WATER , group.getInjectionPhase( 0 )); // Default phase - assumed WATER
    group.setInjectionPhase(5 , Opm::WATER );
    BOOST_CHECK_THROW( group.setInjectionPhase( 5 , Opm::GAS ) , std::invalid_argument );
    BOOST_CHECK_NO_THROW( group.setInjectionPhase( 5 , Opm::WATER ));
    BOOST_CHECK_NO_THROW( group.setInjectionPhase( 6 , Opm::GAS ));
    BOOST_CHECK_EQUAL( Opm::GAS , group.getInjectionPhase( 6 ));
    BOOST_CHECK_EQUAL( Opm::GAS , group.getInjectionPhase( 8 ));
}


