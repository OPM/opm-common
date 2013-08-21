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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;

ParserPtr createWWCTParser() {
    ParserKeywordPtr parserKeyword(new ParserKeyword("WWCT"));
    {
        ParserRecordPtr wwctRecord = parserKeyword->getRecord();
        wwctRecord->addItem(ParserStringItemConstPtr(new ParserStringItem("WELL", ALL)));
    }

    ParserPtr parser(new Parser());
    parser->addKeyword(parserKeyword);
    return parser;
}

BOOST_AUTO_TEST_CASE(parse_fileWithWWCTKeyword_deckReturned) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/wwct.data");
    ParserPtr parser = createWWCTParser();

    BOOST_CHECK_NO_THROW(DeckPtr deck = parser->parse(singleKeywordFile.string()));
}

BOOST_AUTO_TEST_CASE(parse_fileWithWWCTKeyword_deckHasWWCT) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/wwct.data");
    ParserPtr parser = createWWCTParser();
    DeckPtr deck = parser->parse(singleKeywordFile.string());
    BOOST_CHECK(deck->hasKeyword("WWCT"));
}

BOOST_AUTO_TEST_CASE(parse_fileWithWWCTKeyword_dataIsCorrect) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/wwct.data");
    ParserPtr parser = createWWCTParser();
    DeckPtr deck = parser->parse(singleKeywordFile.string());
    BOOST_CHECK_EQUAL("WELL-1", deck->getKeyword("WWCT" , 0)->getRecord(0)->getItem(0)->getString(0));
    BOOST_CHECK_EQUAL("WELL-2", deck->getKeyword("WWCT" , 0)->getRecord(0)->getItem(0)->getString(1));
}

ParserPtr createBPRParser() {
    ParserKeywordPtr parserKeyword(new ParserKeyword("BPR"));
    {
        ParserRecordPtr bprRecord = parserKeyword->getRecord();
        bprRecord->addItem(ParserIntItemConstPtr(new ParserIntItem("I", SINGLE)));
        bprRecord->addItem(ParserIntItemConstPtr(new ParserIntItem("J", SINGLE)));
        bprRecord->addItem(ParserIntItemConstPtr(new ParserIntItem("K", SINGLE)));
    }

    ParserPtr parser(new Parser());
    parser->addKeyword(parserKeyword);
    return parser;
}

BOOST_AUTO_TEST_CASE(parse_fileWithBPRKeyword_deckReturned) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/bpr.data");
    ParserPtr parser = createBPRParser();

    BOOST_CHECK_NO_THROW(DeckPtr deck = parser->parse(singleKeywordFile.string()));
}

BOOST_AUTO_TEST_CASE(parse_fileWithBPRKeyword_DeckhasBRP) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/bpr.data");

    ParserPtr parser = createBPRParser();
    DeckPtr deck = parser->parse(singleKeywordFile.string());

    BOOST_CHECK_EQUAL(true, deck->hasKeyword("BPR"));
}

BOOST_AUTO_TEST_CASE(parse_fileWithBPRKeyword_dataiscorrect) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/bpr.data");

    ParserPtr parser = createBPRParser();
    DeckPtr deck = parser->parse(singleKeywordFile.string());

    DeckKeywordConstPtr keyword = deck->getKeyword("BPR" , 0);
    BOOST_CHECK_EQUAL(2U, keyword->size());

    DeckRecordConstPtr record1 = keyword->getRecord(0);
    BOOST_CHECK_EQUAL(3U, record1->size());

    DeckItemConstPtr I1 = record1->getItem(0);
    BOOST_CHECK_EQUAL(1, I1->getInt(0));
    I1 = record1->getItem("I");
    BOOST_CHECK_EQUAL(1, I1->getInt(0));

    DeckItemConstPtr J1 = record1->getItem(1);
    BOOST_CHECK_EQUAL(2, J1->getInt(0));
    J1 = record1->getItem("J");
    BOOST_CHECK_EQUAL(2, J1->getInt(0));

    DeckItemConstPtr K1 = record1->getItem(2);
    BOOST_CHECK_EQUAL(3, K1->getInt(0));
    K1 = record1->getItem("K");
    BOOST_CHECK_EQUAL(3, K1->getInt(0));


    DeckRecordConstPtr record2 = keyword->getRecord(0);
    BOOST_CHECK_EQUAL(3U, record2->size());

    I1 = record2->getItem(0);
    BOOST_CHECK_EQUAL(1, I1->getInt(0));
    I1 = record2->getItem("I");
    BOOST_CHECK_EQUAL(1, I1->getInt(0));

    J1 = record2->getItem(1);
    BOOST_CHECK_EQUAL(2, J1->getInt(0));
    J1 = record2->getItem("J");
    BOOST_CHECK_EQUAL(2, J1->getInt(0));

    K1 = record2->getItem(2);
    BOOST_CHECK_EQUAL(3, K1->getInt(0));
    K1 = record2->getItem("K");
    BOOST_CHECK_EQUAL(3, K1->getInt(0));
}


BOOST_AUTO_TEST_CASE(Parse_InvalidInputFile_Throws) {
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));
    BOOST_CHECK_THROW(parser->parse("nonexistingfile.asdf"), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(Parse_ValidInputFile_NoThrow) {
    boost::filesystem::path singleKeywordFile("testdata/integration_tests/small.data");
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));

    BOOST_CHECK_NO_THROW(parser->parse(singleKeywordFile.string()));
}

/***************** Testing non-recognized keywords ********************/
BOOST_AUTO_TEST_CASE(parse_unknownkeywordWithnonstrictparsing_keywordmarked) {
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));
    DeckPtr deck = parser->parse("testdata/integration_tests/someobscureelements.data", false);
    BOOST_CHECK_EQUAL(4U, deck->size());
    DeckKeywordConstPtr unknown = deck->getKeyword("GRUDINT");
    BOOST_CHECK(!unknown->isKnown());
}

BOOST_AUTO_TEST_CASE(parse_unknownkeywordWithstrictparsing_exceptionthrown) {
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));
    BOOST_CHECK_THROW(parser->parse("testdata/integration_tests/someobscureelements.data", true), std::invalid_argument);
}
