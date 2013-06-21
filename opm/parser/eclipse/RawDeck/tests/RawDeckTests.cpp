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

#include <map>
#include <string>
#include <iostream>
#define BOOST_TEST_MODULE RawDeckTests
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <boost/test/test_tools.hpp>

#include "opm/parser/eclipse/Parser/Parser.hpp"

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize_NoThrow) {
    BOOST_CHECK_NO_THROW(RawDeck rawDeck);
}


BOOST_AUTO_TEST_CASE(GetNumberOfKeywords_EmptyDeck_RetunsZero) {
    RawDeckPtr rawDeck(new RawDeck);
    BOOST_CHECK_EQUAL((unsigned) 0, rawDeck->size());
}

BOOST_AUTO_TEST_CASE(GetKeyword_EmptyDeck_ThrowsExeption) {
    RawDeckPtr rawDeck(new RawDeck);
    BOOST_CHECK_THROW(rawDeck->getKeyword(0), std::range_error);
}

BOOST_AUTO_TEST_CASE(addKeyword_withkeywords_keywordAdded) {
    RawDeckPtr rawDeck(new RawDeck());
    
    RawKeywordPtr keyword(new RawKeyword("BJARNE"));
    rawDeck->addKeyword(keyword);
    BOOST_CHECK_EQUAL(keyword, rawDeck->getKeyword(0));
    
    RawKeywordPtr keyword2(new RawKeyword("BJARNE2"));
    rawDeck->addKeyword(keyword2);
    BOOST_CHECK_EQUAL(keyword2, rawDeck->getKeyword(1));
    
    BOOST_CHECK_EQUAL(2U, rawDeck->size());
}



