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


#define BOOST_TEST_MODULE KeywordContainerTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/KeywordContainer.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKW.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    KeywordContainer container;
    KeywordContainerPtr ptrContainer(new KeywordContainer());
    KeywordContainerConstPtr constPtrContainer(new KeywordContainer());
}

BOOST_AUTO_TEST_CASE(hasKeyword_empty_returnsfalse) {
    KeywordContainerPtr container(new KeywordContainer());
    BOOST_CHECK_EQUAL(false, container->hasKeyword("Truls"));
}


BOOST_AUTO_TEST_CASE(addKeyword_keywordAdded_keywordAdded) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKWPtr keyword = DeckKWPtr(new DeckKW("Truls"));
    container->addKeyword(keyword);
    
    BOOST_CHECK_EQUAL(true, container->hasKeyword("Truls"));
    BOOST_CHECK_EQUAL(1U, container->size());
}

BOOST_AUTO_TEST_CASE(getKeyword_nosuchkeyword_throws) {
    KeywordContainerPtr container(new KeywordContainer());
    BOOST_CHECK_THROW(container->getKeyword("TRULS"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getKeyword_singleKeyword_keywordReturned) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKWPtr keyword = DeckKWPtr(new DeckKW("TRULS"));
    container->addKeyword(keyword);
    BOOST_CHECK_EQUAL(keyword, container->getKeyword("TRULS"));
}







