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
    BOOST_CHECK_EQUAL("WELL-1", deck->getKeyword("WWCT")->getRecord(0)->get(0)->getString(0));
    BOOST_CHECK_EQUAL("WELL-2", deck->getKeyword("WWCT")->getRecord(0)->get(0)->getString(1));
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

    DeckKeywordConstPtr keyword = deck->getKeyword("BPR");
    BOOST_CHECK_EQUAL(2U, keyword->size());

    DeckRecordConstPtr record1 = keyword->getRecord(0);
    BOOST_CHECK_EQUAL(3U, record1->size());

    DeckItemConstPtr I1 = record1->get(0);
    BOOST_CHECK_EQUAL(1, I1->getInt(0));
    I1 = record1->get("I");
    BOOST_CHECK_EQUAL(1, I1->getInt(0));

    DeckItemConstPtr J1 = record1->get(1);
    BOOST_CHECK_EQUAL(2, J1->getInt(0));
    J1 = record1->get("J");
    BOOST_CHECK_EQUAL(2, J1->getInt(0));

    DeckItemConstPtr K1 = record1->get(2);
    BOOST_CHECK_EQUAL(3, K1->getInt(0));
    K1 = record1->get("K");
    BOOST_CHECK_EQUAL(3, K1->getInt(0));


    DeckRecordConstPtr record2 = keyword->getRecord(0);
    BOOST_CHECK_EQUAL(3U, record2->size());

    I1 = record2->get(0);
    BOOST_CHECK_EQUAL(1, I1->getInt(0));
    I1 = record2->get("I");
    BOOST_CHECK_EQUAL(1, I1->getInt(0));

    J1 = record2->get(1);
    BOOST_CHECK_EQUAL(2, J1->getInt(0));
    J1 = record2->get("J");
    BOOST_CHECK_EQUAL(2, J1->getInt(0));

    K1 = record2->get(2);
    BOOST_CHECK_EQUAL(3, K1->getInt(0));
    K1 = record2->get("K");
    BOOST_CHECK_EQUAL(3, K1->getInt(0));

}

BOOST_AUTO_TEST_CASE(PrintToOStream_noThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));
    RawDeckPtr rawDeck = parser->readToRawDeck(singleKeywordFile.string());
    std::cout << *rawDeck << "\n";
}

BOOST_AUTO_TEST_CASE(Parse_InvalidInputFile_Throws) {
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));
    BOOST_CHECK_THROW(parser->readToRawDeck("nonexistingfile.asdf"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Parse_ValidInputFile_NoThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));

    BOOST_CHECK_NO_THROW(parser->readToRawDeck(singleKeywordFile.string()));
}

BOOST_AUTO_TEST_CASE(ParseFileWithOneKeyword) {

    boost::filesystem::path singleKeywordFile("testdata/mini.data");
    ParserPtr parser(new Parser(JSON_CONFIG_FILE));

    RawDeckPtr rawDeck = parser->readToRawDeck(singleKeywordFile.string());

    BOOST_CHECK_EQUAL(1U, rawDeck->size());
    RawKeywordConstPtr rawKeyword = rawDeck->getKeyword(0);

    BOOST_CHECK_EQUAL(1U, rawKeyword->size());
    RawRecordConstPtr record = rawKeyword->getRecord(rawKeyword->size() - 1);

    const std::string& recordString = record->getRecordString();
    BOOST_CHECK_EQUAL("'NODIR'  'REVERS'  1  20", recordString);

    BOOST_CHECK_EQUAL(4U, record->size());

    BOOST_CHECK_EQUAL("NODIR", record->getItem(0));
    BOOST_CHECK_EQUAL("REVERS", record->getItem(1));
    BOOST_CHECK_EQUAL("1", record->getItem(2));
    BOOST_CHECK_EQUAL("20", record->getItem(3));
}



BOOST_AUTO_TEST_CASE(ParseFileWithFewKeywords) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");

    ParserPtr parser(new Parser(JSON_CONFIG_FILE));

    RawDeckPtr rawDeck = parser->readToRawDeck(singleKeywordFile.string());

    BOOST_CHECK_EQUAL(7U, rawDeck->size());

    RawKeywordConstPtr matchingKeyword = rawDeck->getKeyword(0);
    BOOST_CHECK_EQUAL("OIL", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(0U, matchingKeyword->size());

    // The two next come in via the include of the include path/readthis.sch file
    matchingKeyword = rawDeck->getKeyword(1);
    BOOST_CHECK_EQUAL("GRUPTREE", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(2U, matchingKeyword->size());

    matchingKeyword = rawDeck->getKeyword(2);
    BOOST_CHECK_EQUAL("WHISTCTL", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(1U, matchingKeyword->size());

    matchingKeyword = rawDeck->getKeyword(3);
    BOOST_CHECK_EQUAL("METRIC", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(0U, matchingKeyword->size());

    matchingKeyword = rawDeck->getKeyword(4);
    BOOST_CHECK_EQUAL("GRIDUNIT", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(1U, matchingKeyword->size());

    matchingKeyword = rawDeck->getKeyword(5);
    BOOST_CHECK_EQUAL("RADFIN4", matchingKeyword->getKeywordName());
    BOOST_CHECK_EQUAL(1U, matchingKeyword->size());

    matchingKeyword = rawDeck->getKeyword(6);
    BOOST_CHECK_EQUAL("ABCDAD", matchingKeyword->getKeywordName());

    BOOST_CHECK_EQUAL(2U, matchingKeyword->size());
}
