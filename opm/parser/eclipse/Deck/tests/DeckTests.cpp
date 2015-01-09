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


#define BOOST_TEST_MODULE DeckTests

#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/OpmLog/MessageCounter.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    BOOST_REQUIRE_NO_THROW(Deck deck);
    BOOST_REQUIRE_NO_THROW(DeckPtr deckPtr(new Deck()));
    BOOST_REQUIRE_NO_THROW(DeckConstPtr deckConstPtr(new Deck()));
}

BOOST_AUTO_TEST_CASE(hasKeyword_empty_returnFalse) {
    Deck deck;
    BOOST_CHECK_EQUAL(false, deck.hasKeyword("Bjarne"));
}

BOOST_AUTO_TEST_CASE(addKeyword_singlekeyword_keywordAdded) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    BOOST_CHECK_NO_THROW(deck.addKeyword(keyword));
}

BOOST_AUTO_TEST_CASE(getKeyword_nosuchkeyword_throws) {
    Deck deck;
    BOOST_CHECK_THROW(deck.getKeyword("TRULS" , 0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getKeywordList_nosuchkeyword_throws) {
    Deck deck;
    BOOST_CHECK_THROW(deck.getKeywordList("TRULS"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getKeyword_singlekeyword_keywordreturned) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(keyword, deck.getKeyword("BJARNE" , 0));
}


BOOST_AUTO_TEST_CASE(getKeyword_singlekeyword_outRange_throws) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_THROW(deck.getKeyword("BJARNE" , 10) , std::out_of_range);
}


BOOST_AUTO_TEST_CASE(getKeywordList_returnOK) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_NO_THROW(deck.getKeywordList("BJARNE") );
}


BOOST_AUTO_TEST_CASE(getKeyword_indexok_returnskeyword) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_NO_THROW(deck.getKeyword(0));
}

BOOST_AUTO_TEST_CASE(numKeyword_singlekeyword_return1) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(1U , deck.numKeywords("BJARNE"));
}


BOOST_AUTO_TEST_CASE(numKeyword_twokeyword_return2) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(2U , deck.numKeywords("BJARNE"));
}


BOOST_AUTO_TEST_CASE(numKeyword_nokeyword_return0) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(0U , deck.numKeywords("BJARNEX"));
}


BOOST_AUTO_TEST_CASE(size_twokeyword_return2) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    deck.addKeyword(keyword);
    deck.addKeyword(keyword);
    BOOST_CHECK_EQUAL(2U , deck.size());
}


BOOST_AUTO_TEST_CASE(DECKWARNING_EMPTYOK) {
    MessageCounter logger;
    BOOST_CHECK_EQUAL(0U, logger.size());
}


BOOST_AUTO_TEST_CASE(DECKAddWarning) {
    MessageCounter logger;
    logger.addNote("FILE", 100U, "NOTE");
    BOOST_CHECK_EQUAL(1U, logger.size());

    logger.addWarning("FILE2", 200U, "WARNING");
    BOOST_CHECK_EQUAL(2U, logger.size());

    logger.addError("FILE3", 300U, "ERROR");
    BOOST_CHECK_EQUAL(3U, logger.size());

    BOOST_CHECK_EQUAL(logger.getMessageType(0), Log::MessageType::Note);
    BOOST_CHECK_EQUAL(logger.getDescription(0), "NOTE");
    BOOST_CHECK_EQUAL(logger.getFileName(0), "FILE");
    BOOST_CHECK_EQUAL(logger.getLineNumber(0), 100U);

    BOOST_CHECK_EQUAL(logger.getMessageType(1), Log::MessageType::Warning);
    BOOST_CHECK_EQUAL(logger.getDescription(1), "WARNING");
    BOOST_CHECK_EQUAL(logger.getFileName(1), "FILE2");
    BOOST_CHECK_EQUAL(logger.getLineNumber(1), 200U);

    BOOST_CHECK_EQUAL(logger.getMessageType(2), Log::MessageType::Error);
    BOOST_CHECK_EQUAL(logger.getDescription(2), "ERROR");
    BOOST_CHECK_EQUAL(logger.getFileName(2), "FILE3");
    BOOST_CHECK_EQUAL(logger.getLineNumber(2), 300U);

}


