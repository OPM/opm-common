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

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>

#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>


using namespace Opm;

/************************Basic structural tests**********************'*/

BOOST_AUTO_TEST_CASE(Initializing) {
    BOOST_CHECK_NO_THROW(Parser parser);
    BOOST_CHECK_NO_THROW(Parser parser);
    BOOST_CHECK_NO_THROW(ParserPtr parserPtr(new Parser()));
    BOOST_CHECK_NO_THROW(ParserConstPtr parserConstPtr(new Parser()));
}

BOOST_AUTO_TEST_CASE(addKeyword_keyword_doesntfail) {
    Parser parser;
    {
        ParserKeywordPtr equilKeyword(new ParserKeyword("EQUIL"));
        parser.addKeyword(equilKeyword);
    }
}


BOOST_AUTO_TEST_CASE(hasKeyword_hasKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    parser->addKeyword(ParserKeywordConstPtr(new ParserKeyword("FJAS")));
    BOOST_CHECK(parser->hasKeyword("FJAS"));
}


BOOST_AUTO_TEST_CASE(Keyword_getKeyword_returnskeyword) {
    ParserPtr parser(new Parser());
    ParserKeywordConstPtr parserKeyword(new ParserKeyword("FJAS"));
    parser->addKeyword(parserKeyword);
    BOOST_CHECK_EQUAL(parserKeyword, parser->getKeyword("FJAS"));
}


/************************ JSON config related tests **********************'*/


BOOST_AUTO_TEST_CASE(addKeywordJSON_hasKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig("{\"name\": \"BPR\", \"size\" : 100 ,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"FLOAT\"}]}");
    parser->addKeyword(ParserKeywordConstPtr(new ParserKeyword( jsonConfig )));
    BOOST_CHECK(parser->hasKeyword("BPR"));
}


BOOST_AUTO_TEST_CASE(addKeywordJSON_size_isObject_allGood) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig("{\"name\": \"EQUIXL\", \"size\" : {\"keyword\":\"EQLDIMS\" , \"item\" : \"NTEQUL\"},  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"FLOAT\"}]}");
    parser->addKeyword(ParserKeywordConstPtr(new ParserKeyword( jsonConfig )));
    BOOST_CHECK(parser->hasKeyword("EQUIXL"));
}



BOOST_AUTO_TEST_CASE(loadKeywordsJSON_notArray_throw) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig( "{\"name\" : \"BPR\" , \"size\" : 100}");
    
    BOOST_CHECK_THROW(parser->loadKeywords( jsonConfig ) , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(empty_sizeReturns0) {
    ParserPtr parser(new Parser());
    BOOST_CHECK_EQUAL( 0U , parser->size());
}



BOOST_AUTO_TEST_CASE(loadKeywordsJSON_hasKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig( "[{\"name\" : \"BPR\" , \"size\" : 100,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"FLOAT\"}]}]");
    
    parser->loadKeywords( jsonConfig );
    BOOST_CHECK(parser->hasKeyword("BPR"));
}


BOOST_AUTO_TEST_CASE(loadKeywordsJSON_manyKeywords_returnstrue) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig( "[{\"name\" : \"BPR\" , \"size\" : 100 ,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"FLOAT\"}]}, {\"name\" : \"WWCT\", \"size\" : 0} , {\"name\" : \"EQUIL\" , \"size\" : 0}]");
    
    parser->loadKeywords( jsonConfig );
    BOOST_CHECK(parser->hasKeyword("BPR"));
    BOOST_CHECK(parser->hasKeyword("WWCT"));
    BOOST_CHECK(parser->hasKeyword("EQUIL"));
    BOOST_CHECK_EQUAL( 3U , parser->size() );
}



BOOST_AUTO_TEST_CASE(inititalizeFromFile) {
    ParserPtr parser(new Parser());
    boost::filesystem::path jsonFile("testdata/json/example1.json");
    BOOST_CHECK_NO_THROW(parser->initializeFromJsonFile( jsonFile ));
    BOOST_CHECK(parser->hasKeyword("BPR"));
    BOOST_CHECK(parser->hasKeyword("WWCT"));
}


BOOST_AUTO_TEST_CASE(constructFromJsonFile) {
    boost::filesystem::path jsonFile("testdata/json/example1.json");
    ParserPtr parser(new Parser(jsonFile));
    BOOST_CHECK(parser->hasKeyword("BPR"));
    BOOST_CHECK(parser->hasKeyword("WWCT"));
}


BOOST_AUTO_TEST_CASE(inititalizeFromFile_doesNotExist_throw) {
    ParserPtr parser(new Parser());
    boost::filesystem::path jsonFile("Does/not/exist");
    BOOST_CHECK_THROW( parser->initializeFromJsonFile( jsonFile ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(inititalizeFromFile_missing_keywords_throw) {
    ParserPtr parser(new Parser());
    boost::filesystem::path jsonFile("testdata/json/example_missing_keyword.json");
    BOOST_CHECK_THROW( parser->initializeFromJsonFile( jsonFile ) , std::invalid_argument );
}

/*****************************************************************/


BOOST_AUTO_TEST_CASE(loadKeywordFromFile_fileDoesNotExist_returnsFalse) {
    ParserPtr parser(new Parser());
    boost::filesystem::path configFile("File/does/not/exist");
    BOOST_CHECK_EQUAL( false , parser->loadKeywordFromFile( configFile ));
}


BOOST_AUTO_TEST_CASE(loadKeywordFromFile_invalidJson_returnsFalse) {
    ParserPtr parser(new Parser());
    boost::filesystem::path configFile("testdata/json/example_invalid_json");
    BOOST_CHECK_EQUAL( false , parser->loadKeywordFromFile( configFile ));
}


BOOST_AUTO_TEST_CASE(loadKeywordFromFile_invalidConfig_returnsFalse) {
    ParserPtr parser(new Parser());
    boost::filesystem::path configFile("testdata/json/example_missing_name.json");
    BOOST_CHECK_EQUAL( false , parser->loadKeywordFromFile( configFile ));
}


BOOST_AUTO_TEST_CASE(loadKeywordFromFile_validKeyword_returnsTrueHasKeyword) {
    ParserPtr parser(new Parser());
    boost::filesystem::path configFile("testdata/json/BPR");
    BOOST_CHECK_EQUAL( true , parser->loadKeywordFromFile( configFile ));
    BOOST_CHECK_EQUAL( 1U , parser->size() );
    BOOST_CHECK_EQUAL( true , parser->hasKeyword("BPR") );
}



BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_directoryDoesNotexist_throws) {
        ParserPtr parser(new Parser());
        boost::filesystem::path configPath("path/does/not/exist");
        BOOST_CHECK_THROW(parser->loadKeywordsFromDirectory( configPath), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_notRecursive_allNames) {
        ParserPtr parser(new Parser());
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, false , false));
        BOOST_CHECK(parser->hasKeyword("WWCT"));
        BOOST_CHECK_EQUAL(true , parser->hasKeyword("BPR"));
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_notRecursive_strictNames) {
        ParserPtr parser(new Parser());
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, false , true ));
        BOOST_CHECK(parser->hasKeyword("WWCT"));
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("BPR"));
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_Recursive_allNames) {
        ParserPtr parser(new Parser());
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, true, false));
        BOOST_CHECK(parser->hasKeyword("WWCT"));
        BOOST_CHECK_EQUAL(true , parser->hasKeyword("BPR"));
        BOOST_CHECK_EQUAL(true , parser->hasKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_default) {
        ParserPtr parser(new Parser());
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath ));
        BOOST_CHECK(parser->hasKeyword("WWCT"));
        BOOST_CHECK_EQUAL(false , parser->hasKeyword("BPR"));
        BOOST_CHECK_EQUAL(true , parser->hasKeyword("DIMENS"));
}


/***************** Simple Int parsing ********************************/

ParserKeywordPtr setupParserKeywordInt(std::string name, int numberOfItems) {
    ParserKeywordPtr parserKeyword(new ParserKeyword(name));
    ParserRecordPtr parserRecord = parserKeyword->getRecord();

    for (int i = 0; i < numberOfItems; i++) {
        std::string name = "ITEM_" + boost::lexical_cast<std::string>(i);
        ParserItemPtr intItem(new ParserIntItem(name, SINGLE));
        parserRecord->addItem(intItem);
    }

    return parserKeyword;
}










