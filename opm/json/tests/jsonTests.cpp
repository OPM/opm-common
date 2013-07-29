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

#define BOOST_TEST_MODULE jsonParserTests
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/json/JsonObject.hpp>





BOOST_AUTO_TEST_CASE(ParseValidJson) {
    std::string inline_json = "{\"key\": \"value\"}";
    BOOST_CHECK_NO_THROW(Json::JsonObject parser(inline_json));
}



BOOST_AUTO_TEST_CASE(ParseInvalidJSON_throw) {
    std::string inline_json = "{\"key\": \"value\"";
    BOOST_CHECK_THROW(Json::JsonObject parser(inline_json) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(ParsevalidJSON_getValue) {
    std::string inline_json = "{\"key\": \"value\"}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK_EQUAL( "value" , parser.get_string("key") );
}


BOOST_AUTO_TEST_CASE(ParsevalidJSON_hasItem) {
    std::string inline_json = "{\"key\": \"value\"}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK( parser.has_item("key"));
    BOOST_CHECK_EQUAL( false , parser.has_item("keyX"));
}



BOOST_AUTO_TEST_CASE(ParsevalidJSON_getMissingValue) {
    std::string inline_json = "{\"key\": \"value\"}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK_THROW( parser.get_string("keyX") , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(ParsevalidJSON_getNotScalar_throws) {
    std::string inline_json = "{\"key\": \"value\", \"list\": [1,2,3]}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK_EQUAL( "value" , parser.get_string("key"));
    BOOST_CHECK_THROW( parser.get_string("list") , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(ParsevalidJSON_getObject) {
    std::string inline_json = "{\"key\": \"value\", \"list\": [1,2,3]}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK_NO_THROW( Json::JsonObject object = parser.get_object("list") );
    BOOST_CHECK_NO_THROW( Json::JsonObject object = parser.get_object("key") );
}


BOOST_AUTO_TEST_CASE(ParsevalidJSON_getObject_missing_throw) {
    std::string inline_json = "{\"key\": \"value\", \"list\": [1,2,3]}";
    Json::JsonObject parser(inline_json);
    BOOST_CHECK_THROW( parser.get_object("listX") , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(ParsevalidJSON_CheckArraySize) {
    std::string inline_json = "{\"key\": \"value\", \"list\": [1,2,3]}";
    Json::JsonObject parser(inline_json);
    Json::JsonObject object = parser.get_object("list");
    BOOST_CHECK_EQUAL( 3U , object.size() );
}



BOOST_AUTO_TEST_CASE(Parse_fileDoesNotExist_Throws) {
    boost::filesystem::path jsonFile("file/does/not/exist");
    BOOST_CHECK_THROW( Json::JsonObject parser(jsonFile) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(Parse_fileExists_OK) {
    boost::filesystem::path jsonFile("testdata/json/example1.json");
    BOOST_CHECK_THROW( Json::JsonObject parser(jsonFile) , std::invalid_argument);
}
