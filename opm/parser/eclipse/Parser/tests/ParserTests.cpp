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
        ParserKeywordPtr equilKeyword = ParserKeyword::createDynamicSized("EQUIL");
        parser.addKeyword(equilKeyword);
    }
}


BOOST_AUTO_TEST_CASE(canParseKeyword_canParseKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    parser->addKeyword(ParserKeyword::createDynamicSized("FJAS"));
    BOOST_CHECK(parser->canParseKeyword("FJAS"));
}


BOOST_AUTO_TEST_CASE(getKeyword_haskeyword_returnskeyword) {
    ParserPtr parser(new Parser());
    ParserKeywordConstPtr parserKeyword = ParserKeyword::createDynamicSized("FJAS");
    parser->addKeyword(parserKeyword);
    BOOST_CHECK_EQUAL(parserKeyword, parser->getKeyword("FJAS"));
}

BOOST_AUTO_TEST_CASE(getKeyword_hasnotkeyword_getKeywordThrowsException) {
    ParserPtr parser(new Parser());
    ParserKeywordConstPtr parserKeyword = ParserKeyword::createDynamicSized("FJAS");
    parser->addKeyword(parserKeyword);
    BOOST_CHECK_THROW(parser->getKeyword("FJASS"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getAllKeywords_hasTwoKeywords_returnsCompleteList) {
    ParserPtr parser(new Parser(false));
    std::cout << parser->getAllKeywords().size() << std::endl;
    ParserKeywordConstPtr firstParserKeyword = ParserKeyword::createDynamicSized("FJAS");
    parser->addKeyword(firstParserKeyword);
    ParserKeywordConstPtr secondParserKeyword = ParserKeyword::createDynamicSized("SAJF");
    parser->addKeyword(secondParserKeyword);
    BOOST_CHECK_EQUAL(2U, parser->getAllKeywords().size());
}

BOOST_AUTO_TEST_CASE(getAllKeywords_hasNoKeywords_returnsEmptyList) {
    ParserPtr parser(new Parser(false));
    BOOST_CHECK_EQUAL(0U, parser->getAllKeywords().size());
}



/************************ JSON config related tests **********************'*/


BOOST_AUTO_TEST_CASE(addKeywordJSON_canParseKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig("{\"name\": \"BPR\", \"size\" : 100 ,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"DOUBLE\"}]}");
    parser->addKeyword(ParserKeyword::createFromJson( jsonConfig ));
    BOOST_CHECK(parser->canParseKeyword("BPR"));
}


BOOST_AUTO_TEST_CASE(addKeywordJSON_size_isObject_allGood) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig("{\"name\": \"EQUIXL\", \"size\" : {\"keyword\":\"EQLDIMS\" , \"item\" : \"NTEQUL\"},  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"DOUBLE\"}]}");
    parser->addKeyword(ParserKeyword::createFromJson( jsonConfig ));
    BOOST_CHECK(parser->canParseKeyword("EQUIXL"));
}



BOOST_AUTO_TEST_CASE(loadKeywordsJSON_notArray_throw) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig( "{\"name\" : \"BPR\" , \"size\" : 100}");
    
    BOOST_CHECK_THROW(parser->loadKeywords( jsonConfig ) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(loadKeywordsJSON_canParseKeyword_returnstrue) {
    ParserPtr parser(new Parser());
    Json::JsonObject jsonConfig( "[{\"name\" : \"BPR\" , \"size\" : 100,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"DOUBLE\"}]}]");
    
    parser->loadKeywords( jsonConfig );
    BOOST_CHECK(parser->canParseKeyword("BPR"));
}


BOOST_AUTO_TEST_CASE(empty_sizeReturns0) {
    ParserPtr parser(new Parser( false ));
    BOOST_CHECK_EQUAL( 0U , parser->size());
}



BOOST_AUTO_TEST_CASE(loadKeywordsJSON_manyKeywords_returnstrue) {
    ParserPtr parser(new Parser( false ));
    Json::JsonObject jsonConfig( "[{\"name\" : \"BPR\" , \"size\" : 100 ,  \"items\" :[{\"name\":\"ItemX\" , \"size_type\":\"SINGLE\" , \"value_type\" : \"DOUBLE\"}]}, {\"name\" : \"WWCT\", \"size\" : 0} , {\"name\" : \"EQUIL\" , \"size\" : 0}]");
    
    parser->loadKeywords( jsonConfig );
    BOOST_CHECK(parser->canParseKeyword("BPR"));
    BOOST_CHECK(parser->canParseKeyword("WWCT"));
    BOOST_CHECK(parser->canParseKeyword("EQUIL"));
    BOOST_CHECK_EQUAL( 3U , parser->size() );
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
    ParserPtr parser(new Parser( false ));
    boost::filesystem::path configFile("testdata/json/BPR");
    BOOST_CHECK_EQUAL( true , parser->loadKeywordFromFile( configFile ));
    BOOST_CHECK_EQUAL( 1U , parser->size() );
    BOOST_CHECK_EQUAL( true , parser->canParseKeyword("BPR") );
}



BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_directoryDoesNotexist_throws) {
        ParserPtr parser(new Parser(false));
        boost::filesystem::path configPath("path/does/not/exist");
        BOOST_CHECK_THROW(parser->loadKeywordsFromDirectory( configPath), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_notRecursive_allNames) {
        ParserPtr parser(new Parser(false));
        BOOST_CHECK_EQUAL(false , parser->canParseKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, false));
        BOOST_CHECK(parser->canParseKeyword("WWCT"));
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("BPR"));
        BOOST_CHECK_EQUAL(false , parser->canParseKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_notRecursive_strictNames) {
        ParserPtr parser(new Parser(false));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, false));
        BOOST_CHECK(parser->canParseKeyword("WWCT"));
        // the file name for the following keyword is "Bpr", but that
        // does not matter
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("BPR"));
        BOOST_CHECK_EQUAL(false , parser->canParseKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_Recursive_allNames) {
        ParserPtr parser(new Parser(false));
        BOOST_CHECK_EQUAL(false , parser->canParseKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath, true));
        BOOST_CHECK(parser->canParseKeyword("WWCT"));
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("BPR"));
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(loadConfigFromDirectory_default) {
        ParserPtr parser(new Parser(false));
        BOOST_CHECK_EQUAL(false , parser->canParseKeyword("BPR"));
        boost::filesystem::path configPath("testdata/config/directory1");
        BOOST_CHECK_NO_THROW(parser->loadKeywordsFromDirectory( configPath ));
        BOOST_CHECK(parser->canParseKeyword("WWCT"));
        // the file name for the following keyword is "Bpr", but that
        // does not matter
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("BPR"));
        BOOST_CHECK_EQUAL(true , parser->canParseKeyword("DIMENS"));
}


BOOST_AUTO_TEST_CASE(DropKeyword) {
    ParserPtr parser(new Parser());
    BOOST_CHECK_EQUAL(false , parser->dropParserKeyword("DoesNotHaveThis"));
    BOOST_CHECK_EQUAL(true , parser->canParseKeyword("BPR"));
    BOOST_CHECK_EQUAL(true  , parser->dropParserKeyword("BLOCK_PROBE"));
    BOOST_CHECK_EQUAL(false  , parser->dropParserKeyword("BLOCK_PROBE"));
    BOOST_CHECK_EQUAL(false , parser->canParseKeyword("BPR"));

    BOOST_CHECK_EQUAL(true , parser->canParseKeyword("TVDPX"));
    BOOST_CHECK_EQUAL(true , parser->dropKeyword("TVDP*"));
    BOOST_CHECK_EQUAL(false , parser->canParseKeyword("TVDPX"));
}


BOOST_AUTO_TEST_CASE(ReplaceKeyword) {
    ParserPtr parser(new Parser());
    ParserKeywordConstPtr eqldims = parser->getKeyword("EQLDIMS");

    BOOST_CHECK_EQUAL( 5U , eqldims->numItems());
    BOOST_CHECK( parser->loadKeywordFromFile( "testdata/parser/EQLDIMS2" ) );
    

    eqldims = parser->getKeyword("EQLDIMS");
    BOOST_CHECK_EQUAL( 1U , eqldims->numItems());
}


BOOST_AUTO_TEST_CASE(WildCardTest) {
    ParserPtr parser(new Parser());
    BOOST_CHECK(!parser->canParseKeyword("TVDP*"));
    BOOST_CHECK(!parser->canParseKeyword("TVDP"));
    BOOST_CHECK(parser->canParseKeyword("TVDPXXX"));
    BOOST_CHECK(!parser->canParseKeyword("TVDPIAMTOOLONG"));
    BOOST_CHECK(!parser->canParseKeyword("TVD"));

    BOOST_CHECK(!parser->canParseKeyword("TVDP"));

    ParserKeywordConstPtr keyword1 = parser->getParserKeyword("TVDPA");
    ParserKeywordConstPtr keyword2 = parser->getParserKeyword("TVDPBC");
    ParserKeywordConstPtr keyword3 = parser->getParserKeyword("TVDPXXX");

    BOOST_CHECK_EQUAL( keyword1 , keyword2 );
    BOOST_CHECK_EQUAL( keyword1 , keyword3 );
}



/***************** Simple Int parsing ********************************/

static ParserKeywordPtr __attribute__((unused)) setupParserKeywordInt(std::string name, int numberOfItems) {
    ParserKeywordPtr parserKeyword = ParserKeyword::createDynamicSized(name);
    ParserRecordPtr parserRecord = parserKeyword->getRecord();

    for (int i = 0; i < numberOfItems; i++) {
        std::string another_name = "ITEM_" + boost::lexical_cast<std::string>(i);
        ParserItemPtr intItem(new ParserIntItem(another_name, SINGLE));
        parserRecord->addItem(intItem);
    }

    return parserKeyword;
}










