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

#define BOOST_TEST_MODULE ParserTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>


BOOST_AUTO_TEST_CASE(CreateTimeMap_InvalidThrow) {
    boost::gregorian::date startDate;
    BOOST_CHECK_THROW(Opm::TimeMap timeMap(startDate) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CreateTimeMap) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);
    BOOST_CHECK_EQUAL(1U , timeMap.size());
}



BOOST_AUTO_TEST_CASE(AddDateBeforThrows) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMap timeMap(startDate);

    BOOST_CHECK_THROW( timeMap.addDate( boost::gregorian::date(2009,boost::gregorian::Feb,2))  , std::invalid_argument);
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
    Opm::TimeMap timeMap(startDate);

    timeMap.addTStep( boost::posix_time::hours(1));
    timeMap.addTStep( boost::posix_time::hours(24));
    BOOST_CHECK_EQUAL( 3U , timeMap.size());
}







