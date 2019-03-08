/*
  Copyright 2014 Statoil ASA.

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

#define BOOST_TEST_MODULE ScheduleTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>


BOOST_AUTO_TEST_CASE( check_empty) {
    Opm::OrderedMap<std::string, std::string> map;
    BOOST_CHECK_EQUAL( 0U , map.size() );
    BOOST_CHECK_THROW( map.iget( 0 ) , std::invalid_argument);
    BOOST_CHECK_THROW( map.get( "KEY" ) , std::invalid_argument);
    BOOST_CHECK_THROW( map.at("KEY"), std::invalid_argument);
    BOOST_CHECK_THROW( map.at(0), std::invalid_argument);
    BOOST_CHECK_EQUAL( map.count("NO_SUCH_KEY"), 0);
}

BOOST_AUTO_TEST_CASE( operator_square ) {
    Opm::OrderedMap<std::string, std::string> map;
    map.insert(std::make_pair("CKEY1" , "Value1"));
    map.insert(std::make_pair("BKEY2" , "Value2"));
    map.insert(std::make_pair("AKEY3" , "Value3"));

    const auto& value = map["CKEY1"];
    BOOST_CHECK_EQUAL( value, std::string("Value1"));
    BOOST_CHECK_EQUAL( map.size(), 3);

    auto& new_value = map["NEW_KEY"];
    BOOST_CHECK_EQUAL( map.size(), 4);
    BOOST_CHECK_EQUAL( new_value, "");
}

BOOST_AUTO_TEST_CASE( find ) {
    Opm::OrderedMap<std::string, std::string> map;
    map.insert(std::make_pair("CKEY1" , "Value1"));
    map.insert(std::make_pair("BKEY2" , "Value2"));
    map.insert(std::make_pair("AKEY3" , "Value3"));

    auto iter_end = map.find("NO_SUCH_KEY");
    BOOST_CHECK( iter_end == map.end());


    auto iter_found = map.find("CKEY1");
    BOOST_CHECK_EQUAL( iter_found->first, std::string("CKEY1"));
    BOOST_CHECK_EQUAL( iter_found->second, std::string("Value1"));
}

BOOST_AUTO_TEST_CASE( check_order ) {
    Opm::OrderedMap<std::string, std::string> map;
    map.insert(std::make_pair("CKEY1" , "Value1"));
    map.insert(std::make_pair("BKEY2" , "Value2"));
    map.insert(std::make_pair("AKEY3" , "Value3"));

    BOOST_CHECK_EQUAL( 3U , map.size() );
    BOOST_CHECK( map.count("CKEY1") > 0);
    BOOST_CHECK( map.count("BKEY2") > 0);
    BOOST_CHECK( map.count("AKEY3") > 0);

    BOOST_CHECK_EQUAL( "Value1" , map.get("CKEY1"));
    BOOST_CHECK_EQUAL( "Value1" , map.iget( 0 ));
    BOOST_CHECK_EQUAL( map.count("CKEY"), 0);

    BOOST_CHECK_EQUAL( "Value2" , map.get("BKEY2"));
    BOOST_CHECK_EQUAL( "Value2" , map.iget( 1 ));

    BOOST_CHECK_EQUAL( "Value3" , map.at("AKEY3"));
    BOOST_CHECK_EQUAL( "Value3" , map.at( 2 ));

    map.insert( std::make_pair("CKEY1" , "NewValue1"));
    BOOST_CHECK_EQUAL( "NewValue1" , map.get("CKEY1"));
    BOOST_CHECK_EQUAL( "NewValue1" , map.iget( 0 ));

    {
        std::vector<std::string> values;
        for (auto iter = map.begin(); iter != map.end(); ++iter)
            values.push_back( iter->second );

        BOOST_CHECK_EQUAL( values[0] , "NewValue1");
        BOOST_CHECK_EQUAL( values[1] , "Value2");
        BOOST_CHECK_EQUAL( values[2] , "Value3");
    }
}
