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

#define BOOST_TEST_MODULE WellTest
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

Opm::TimeMapPtr createXDaysTimeMap(size_t numDays) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(startDate));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}

BOOST_AUTO_TEST_CASE(CreateWell_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , timeMap);
    BOOST_CHECK_EQUAL( "WELL1" , well.name() );
    BOOST_CHECK_EQUAL(0.0 , well.getOilRate( 5 ));
}

BOOST_AUTO_TEST_CASE(setOilRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , timeMap);
    
    BOOST_CHECK_EQUAL(0.0 , well.getOilRate( 5 ));
    well.setOilRate( 5 , 99 );
    BOOST_CHECK_EQUAL(99 , well.getOilRate( 5 ));
    BOOST_CHECK_EQUAL(99 , well.getOilRate( 8 ));
}

BOOST_AUTO_TEST_CASE(setPredictionMode_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , timeMap);
    
    BOOST_CHECK_EQUAL( true, well.isInPredictionMode( 5 ));
    well.setInPredictionMode( 5 , false ); // Go to history mode
    BOOST_CHECK_EQUAL(false , well.isInPredictionMode( 5 ));
    BOOST_CHECK_EQUAL(false , well.isInPredictionMode( 8 ));
}
