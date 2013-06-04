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


#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKW.hpp>
#include <opm/parser/eclipse/Parser/ParserRecordSize.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initializing) {
    BOOST_CHECK_NO_THROW(Parser parser);
    BOOST_CHECK_NO_THROW(ParserPtr parserPtr(new Parser()));
    BOOST_CHECK_NO_THROW(ParserConstPtr parserConstPtr(new Parser()));
}

BOOST_AUTO_TEST_CASE(ParserAddKW) {
    Parser parser;
    {
        ParserRecordSizePtr recordSize(new ParserRecordSize(9));
        ParserKWPtr equilKW(new ParserKW("EQUIL", recordSize));

        parser.addKW(equilKW);
    }
}

BOOST_AUTO_TEST_CASE(hasKeyword_hasKeyword_returnstrue) {
     ParserPtr parser(new Parser());
     parser->addKW(ParserKWConstPtr(new ParserKW("FJAS")));
     BOOST_CHECK(parser->hasKeyword("FJAS"));
}


BOOST_AUTO_TEST_CASE(PrintToOStream_noThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");
    ParserPtr parser(new Parser());
    RawDeckPtr rawDeck = parser->readToRawDeck(singleKeywordFile.string());
    std::cout << *rawDeck << "\n";
}

BOOST_AUTO_TEST_CASE(Parse_InvalidInputFile_Throws) {
    ParserPtr parser(new Parser());
    BOOST_CHECK_THROW(parser->readToRawDeck("nonexistingfile.asdf"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Parse_ValidInputFile_NoThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");
    ParserPtr parser(new Parser());

    BOOST_CHECK_NO_THROW(parser->readToRawDeck(singleKeywordFile.string()));
}

BOOST_AUTO_TEST_CASE(ParseFileWithOneKeyword) {

    boost::filesystem::path singleKeywordFile("testdata/mini.data");
    ParserPtr parser(new Parser());

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

    ParserPtr parser(new Parser());

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




