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


#define BOOST_TEST_MODULE DeckItemTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    BOOST_REQUIRE_NO_THROW(DeckIntItem deckIntItem("TEST"));
}

BOOST_AUTO_TEST_CASE(GetIntAtIndex_NoData_ExceptionThrown) {
    DeckIntItem deckIntItem("TEST");
    deckIntItem.push_back(100);
    BOOST_CHECK_THROW(deckIntItem.getInt(1), std::out_of_range);
}




 
BOOST_AUTO_TEST_CASE(InitializeDefaultApplied) {
    DeckIntItem deckIntItem("TEST");
    BOOST_REQUIRE_NO_THROW( deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK( !deckIntItem.wasSetInDeck(0));
}


BOOST_AUTO_TEST_CASE(InitializeDefaultApplied_Throws_for_nonScalar) {
    DeckIntItem deckIntItem("TEST" , false);
    BOOST_REQUIRE_NO_THROW( deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK( !deckIntItem.wasSetInDeck(0));
}



BOOST_AUTO_TEST_CASE(PushBack_VectorPushed_ElementsCorrect) {
    DeckIntItem deckIntItem("TEST");
    std::deque<int> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    deckIntItem.push_back(pushThese);
    BOOST_CHECK_EQUAL(13, deckIntItem.getInt(0));
    BOOST_CHECK_EQUAL(33, deckIntItem.getInt(1));
}

BOOST_AUTO_TEST_CASE(PushBack_subVectorPushed_ElementsCorrect) {
    DeckIntItem deckIntItem("TEST");
    std::deque<int> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    pushThese.push_back(47);
    deckIntItem.push_back(pushThese , 2);
    BOOST_CHECK_EQUAL(13 , deckIntItem.getInt(0));
    BOOST_CHECK_EQUAL(33 , deckIntItem.getInt(1));
    BOOST_CHECK_EQUAL( 2U , deckIntItem.size());
}


BOOST_AUTO_TEST_CASE(size_correct) {
    DeckIntItem deckIntItem("TEST");
    
    BOOST_CHECK_EQUAL( 0U , deckIntItem.size());
    deckIntItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 1U , deckIntItem.size());
    
    deckIntItem.push_back( 100 );
    deckIntItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 3U , deckIntItem.size());
}

BOOST_AUTO_TEST_CASE(SetInDeck) {
    DeckIntItem deckIntItem("TEST");
    BOOST_CHECK( !deckIntItem.wasSetInDeck(0) );

    deckIntItem.push_back( 100 );
    BOOST_CHECK( deckIntItem.wasSetInDeck(0) );
}


BOOST_AUTO_TEST_CASE(SetInDeckData) {
    DeckIntItem deckIntItem("TEST");
    BOOST_CHECK_EQUAL( false , deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK_EQUAL( false , deckIntItem.defaultApplied(0));
    BOOST_CHECK_EQUAL( false , deckIntItem.hasData(0));
    deckIntItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( false , deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK_EQUAL( true  , deckIntItem.defaultApplied(0));
    BOOST_CHECK_EQUAL( true , deckIntItem.hasData(0));
    deckIntItem.push_back( 10 );
    BOOST_CHECK_EQUAL( true , deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK_EQUAL( true  , deckIntItem.defaultApplied(0));
    BOOST_CHECK_EQUAL( true  , deckIntItem.hasData(0));
    deckIntItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( true , deckIntItem.wasSetInDeck(0) );
    BOOST_CHECK_EQUAL( true  , deckIntItem.hasData(0));
}


BOOST_AUTO_TEST_CASE(PushBackMultiple) {
    DeckIntItem item("HEI");
    item.push_backMultiple(10 , 100U );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL(10 , item.getInt(i));
}



