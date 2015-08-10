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

#define BOOST_TEST_MODULE DeckTests

#include <opm/core/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <opm/core/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/OpmLog/CounterLog.hpp>
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
    BOOST_CHECK_THROW( deck.getKeyword("Bjarne") , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(addKeyword_singlekeyword_keywordAdded) {
    Deck deck;
    DeckKeywordPtr keyword(new DeckKeyword("BJARNE"));
    BOOST_CHECK_NO_THROW(deck.addKeyword(keyword));
}


BOOST_AUTO_TEST_CASE(getKeywordList_empty_list) {
    Deck deck;
    auto kw_list = deck.getKeywordList("TRULS");
    BOOST_CHECK_EQUAL( kw_list.size() , 0 );
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


BOOST_AUTO_TEST_CASE(getKeyword_multipleKeyword_keywordReturned) {
    Deck deck;
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);

    BOOST_CHECK_EQUAL(keyword1, deck.getKeyword("TRULS", 0));
    BOOST_CHECK_EQUAL(keyword3, deck.getKeyword("TRULS", 2));
    BOOST_CHECK_EQUAL(keyword3, deck.getKeyword("TRULS"));
}



BOOST_AUTO_TEST_CASE(getKeyword_outOfRange_throws) {
    Deck deck;
    DeckKeywordPtr keyword = DeckKeywordPtr(new DeckKeyword("TRULS"));
    deck.addKeyword(keyword);
    BOOST_CHECK_THROW( deck.getKeyword("TRULS" , 3) , std::out_of_range)
}



BOOST_AUTO_TEST_CASE(getKeywordList_OK) {
    Deck deck;
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);

    const std::vector<DeckKeywordConstPtr>& keywordList = deck.getKeywordList("TRULS");
    BOOST_CHECK_EQUAL( 3U , keywordList.size() );
}





BOOST_AUTO_TEST_CASE(keywordList_getnum_OK) {
    Deck deck;
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);

    BOOST_CHECK_EQUAL( 0U , deck.numKeywords( "TRULSY" ));
    BOOST_CHECK_EQUAL( 2U , deck.numKeywords( "TRULS" ));
    BOOST_CHECK_EQUAL( 1U , deck.numKeywords( "TRULSX" ));
}


BOOST_AUTO_TEST_CASE(keywordList_getbyindexoutofbounds_exceptionthrown) {
    Deck deck;
    BOOST_CHECK_THROW(deck.getKeyword(0), std::out_of_range);
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);
    BOOST_CHECK_NO_THROW(deck.getKeyword(2));
    BOOST_CHECK_THROW(deck.getKeyword(3), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(keywordList_getbyindex_correctkeywordreturned) {
    Deck deck;
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);
    BOOST_CHECK_EQUAL("TRULS",  deck.getKeyword(0)->name());
    BOOST_CHECK_EQUAL("TRULS",  deck.getKeyword(1)->name());
    BOOST_CHECK_EQUAL("TRULSX", deck.getKeyword(2)->name());
}


BOOST_AUTO_TEST_CASE(KeywordIndexCorrect) {
    Deck deck;
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword4 = DeckKeywordPtr(new DeckKeyword("TRULS4"));
    deck.addKeyword(keyword1);
    deck.addKeyword(keyword2);
    deck.addKeyword(keyword3);

    BOOST_CHECK_THROW( deck.getKeywordIndex( keyword4 ) , std::invalid_argument);

    BOOST_CHECK_EQUAL(0U , deck.getKeywordIndex(keyword1));
    BOOST_CHECK_EQUAL(1U , deck.getKeywordIndex(keyword2));
    BOOST_CHECK_EQUAL(2U , deck.getKeywordIndex(keyword3));
}





