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


#define BOOST_TEST_MODULE DeckStringItemTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(InitializeString) {
    DeckStringItem stringItem("TEST");
    BOOST_CHECK_EQUAL("TEST", stringItem.name());
}

BOOST_AUTO_TEST_CASE(GetStringAtIndex_NoData_ExceptionThrown) {
    const DeckStringItem deckStringItem("TEST");
    BOOST_CHECK_THROW(deckStringItem.getString(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(PushBack_VectorPushed_ElementsCorrect) {
    DeckStringItem deckStringItem("TEST");
    std::deque<std::string> pushThese;
    pushThese.push_back("hei");
    pushThese.push_back("trygve-hei");
    deckStringItem.push_back(pushThese);
    BOOST_CHECK_EQUAL("hei", deckStringItem.getString(0));
    BOOST_CHECK_EQUAL("trygve-hei", deckStringItem.getString(1));
}

BOOST_AUTO_TEST_CASE(PushBack_subVectorPushed_ElementsCorrect) {
    DeckStringItem deckStringItem("TEST");
    std::deque<std::string> pushThese;
    pushThese.push_back("Well-1");
    pushThese.push_back("Well-2");
    pushThese.push_back("Well-3");
    deckStringItem.push_back(pushThese, 2);
    BOOST_CHECK_EQUAL("Well-1", deckStringItem.getString(0));
    BOOST_CHECK_EQUAL("Well-2", deckStringItem.getString(1));
    BOOST_CHECK_EQUAL(2U, deckStringItem.size());
}

BOOST_AUTO_TEST_CASE(size_variouspushes_sizecorrect) {
    DeckStringItem deckStringItem("TEST");

    BOOST_CHECK_EQUAL(0U, deckStringItem.size());
    deckStringItem.push_back("WELL-3");
    BOOST_CHECK_EQUAL(1U, deckStringItem.size());

    deckStringItem.push_back("WELL-4");
    deckStringItem.push_back("WELL-5");
    BOOST_CHECK_EQUAL(3U, deckStringItem.size());
}


BOOST_AUTO_TEST_CASE(SetInDeckData) {
    DeckStringItem deckStringItem("TEST");
    BOOST_CHECK_EQUAL( false , deckStringItem.setInDeck() );
    deckStringItem.push_backDefault( "Default" );
    BOOST_CHECK_EQUAL( false , deckStringItem.setInDeck() );
    deckStringItem.push_back( "Value" );
    BOOST_CHECK_EQUAL( true , deckStringItem.setInDeck() );
    deckStringItem.push_backDefault( "Deafult" );
    BOOST_CHECK_EQUAL( true , deckStringItem.setInDeck() );
}


BOOST_AUTO_TEST_CASE(PushBackMultiple) {
    DeckStringItem item("HEI");
    item.push_backMultiple("Heisann ", 100U );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL("Heisann " , item.getString(i));
}
