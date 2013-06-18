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

#include "opm/parser/eclipse/Parser/ParserIntItem.hpp"

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initializing) {
    BOOST_CHECK_NO_THROW(Parser parser);
    BOOST_CHECK_NO_THROW(ParserPtr parserPtr(new Parser()));
    BOOST_CHECK_NO_THROW(ParserConstPtr parserConstPtr(new Parser()));
}

BOOST_AUTO_TEST_CASE(addKW_keyword_doesntfail) {
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

ParserKWPtr setupParserKWInt(int numberOfItems) {
    ParserKWPtr parserKw(new ParserKW("RANDOM"));
    ParserRecordPtr parserRecord(new ParserRecord());
    for (int i = 0; i < numberOfItems; i++) {
        std::string name = "ITEM_" + boost::lexical_cast<std::string>(i);
        ParserItemPtr intItem(new ParserIntItem(name, SINGLE));
        parserRecord->addItem(intItem);
    }

    parserKw->setRecord(parserRecord);

    return parserKw;
}

RawDeckPtr setupRawDeckInt(int numberOfRecords, int numberOfItems) {
    RawParserKWsConstPtr rawParserKWs(new RawParserKWs());
    RawDeckPtr rawDeck(new RawDeck(rawParserKWs));

    RawKeywordPtr rawKeyword(new RawKeyword("RANDOM"));
    for (int records = 0; records < numberOfRecords; records++) {
        for (int i = 0; i < numberOfItems; i++)
            rawKeyword->addRawRecordString("42 ");
        rawKeyword->addRawRecordString("/");
    }

    rawDeck->addKeyword(rawKeyword);

    return rawDeck;
}

BOOST_AUTO_TEST_CASE(parseFromRawDeck_singleRawSingleIntItem_deckReturned) {
    ParserPtr parser(new Parser());
    parser->addKW(setupParserKWInt(1));
    DeckPtr deck = parser->parseFromRawDeck(setupRawDeckInt(1,1));

    BOOST_CHECK(!deck->hasKeyword("ANDOM"));

    BOOST_CHECK(deck->hasKeyword("RANDOM"));
    BOOST_CHECK_EQUAL(1U, deck->getKeyword("RANDOM")->getRecord(0)->size());
}

BOOST_AUTO_TEST_CASE(parseFromRawDeck_singleRawRecordsSeveralIntItem_deckReturned) {
    ParserPtr parser(new Parser());
    parser->addKW(setupParserKWInt(50));
    DeckPtr deck = parser->parseFromRawDeck(setupRawDeckInt(1, 50));

    BOOST_CHECK(deck->hasKeyword("RANDOM"));
    BOOST_CHECK_EQUAL(50U, deck->getKeyword("RANDOM")->getRecord(0)->size());
}

BOOST_AUTO_TEST_CASE(parseFromRawDeck_severalRawRecordsSeveralIntItem_deckReturned) {
    ParserPtr parser(new Parser());
    parser->addKW(setupParserKWInt(50));
    DeckPtr deck = parser->parseFromRawDeck(setupRawDeckInt(10, 50));

    BOOST_CHECK(deck->hasKeyword("RANDOM"));
    BOOST_CHECK_EQUAL(10U, deck->getKeyword("RANDOM")->size());
    BOOST_CHECK_EQUAL(50U, deck->getKeyword("RANDOM")->getRecord(0)->size());
}








