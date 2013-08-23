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
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>


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
    DeckKeywordPtr keyword = DeckKeywordPtr(new DeckKeyword("Truls"));
    container->addKeyword(keyword);
    
    BOOST_CHECK_EQUAL(true, container->hasKeyword("Truls"));
    BOOST_CHECK_EQUAL(1U, container->size());
}

BOOST_AUTO_TEST_CASE(getKeyword_nosuchkeyword_throws) {
    KeywordContainerPtr container(new KeywordContainer());
    BOOST_CHECK_THROW(container->getKeyword("TRULS" , 0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getKeyword_singleKeyword_keywordReturned) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword = DeckKeywordPtr(new DeckKeyword("TRULS"));
    container->addKeyword(keyword);
    BOOST_CHECK_EQUAL(keyword, container->getKeyword("TRULS", 0));
    BOOST_CHECK_EQUAL(keyword, container->getKeyword("TRULS"));
}


BOOST_AUTO_TEST_CASE(getKeyword_multipleKeyword_keywordReturned) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    container->addKeyword(keyword1);
    container->addKeyword(keyword2);
    container->addKeyword(keyword3);

    BOOST_CHECK_EQUAL(keyword1, container->getKeyword("TRULS", 0));
    BOOST_CHECK_EQUAL(keyword3, container->getKeyword("TRULS", 2));
    BOOST_CHECK_EQUAL(keyword3, container->getKeyword("TRULS"));
}


BOOST_AUTO_TEST_CASE(getKeyword_outOfRange_throws) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword = DeckKeywordPtr(new DeckKeyword("TRULS"));
    container->addKeyword(keyword);
    BOOST_CHECK_THROW( container->getKeyword("TRULS" , 3) , std::out_of_range)
}


BOOST_AUTO_TEST_CASE(getKeywordList_notFound_throws) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword = DeckKeywordPtr(new DeckKeyword("TRULS"));
    container->addKeyword(keyword);
    BOOST_CHECK_THROW( container->getKeywordList("TRULSX") , std::invalid_argument)
}

BOOST_AUTO_TEST_CASE(getKeywordList_OK) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    container->addKeyword(keyword1);
    container->addKeyword(keyword2);
    container->addKeyword(keyword3);

    const std::vector<DeckKeywordConstPtr>& keywordList = container->getKeywordList("TRULS");
    BOOST_CHECK_EQUAL( 3U , keywordList.size() );
}





BOOST_AUTO_TEST_CASE(keywordList_getnum_OK) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    container->addKeyword(keyword1);
    container->addKeyword(keyword2);
    container->addKeyword(keyword3);

    BOOST_CHECK_EQUAL( 0U , container->numKeywords( "TRULSY" ));
    BOOST_CHECK_EQUAL( 2U , container->numKeywords( "TRULS" ));
    BOOST_CHECK_EQUAL( 1U , container->numKeywords( "TRULSX" ));
}


BOOST_AUTO_TEST_CASE(keywordList_getbyindexoutofbounds_exceptionthrown) {
    KeywordContainerPtr container(new KeywordContainer());
    BOOST_CHECK_THROW(container->getKeyword(0), std::out_of_range);
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    container->addKeyword(keyword1);
    container->addKeyword(keyword2);
    container->addKeyword(keyword3);
    BOOST_CHECK_NO_THROW(container->getKeyword(2));
    BOOST_CHECK_THROW(container->getKeyword(3), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(keywordList_getbyindex_correctkeywordreturned) {
    KeywordContainerPtr container(new KeywordContainer());
    DeckKeywordPtr keyword1 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword2 = DeckKeywordPtr(new DeckKeyword("TRULS"));
    DeckKeywordPtr keyword3 = DeckKeywordPtr(new DeckKeyword("TRULSX"));
    container->addKeyword(keyword1);
    container->addKeyword(keyword2);
    container->addKeyword(keyword3);
    BOOST_CHECK_EQUAL("TRULS", container->getKeyword(0)->name());
    BOOST_CHECK_EQUAL("TRULS", container->getKeyword(1)->name());
    BOOST_CHECK_EQUAL("TRULSX", container->getKeyword(2)->name());
}





