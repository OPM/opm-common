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
#define BOOST_TEST_MODULE RawParserKeywordsTests
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>
#include <opm/parser/eclipse/RawDeck/RawParserKeywords.hpp>
using namespace Opm;

BOOST_AUTO_TEST_CASE(KeywordExists_KeywordNotPresent_ReturnsFalse) {
    RawParserKeywordsConstPtr parserKeywords(new RawParserKeywords());
    BOOST_CHECK_EQUAL(false, parserKeywords->keywordExists("FLASKE"));
}

BOOST_AUTO_TEST_CASE(KeywordExists_KeywordPresent_ReturnsTrue) {
    RawParserKeywordsConstPtr parserKeywords(new RawParserKeywords());
    BOOST_CHECK_EQUAL(true, parserKeywords->keywordExists("TITLE"));
}

BOOST_AUTO_TEST_CASE(GetFixedNumberOfRecords_KeywordNotPresent_ThrowsException) {
    RawParserKeywordsConstPtr parserKeywords(new RawParserKeywords());
    BOOST_CHECK_THROW(parserKeywords->getFixedNumberOfRecords("FLASKE"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(GetFixedNumberOfRecords_OneRecord_ReturnsOne) {
    RawParserKeywordsConstPtr parserKeywords(new RawParserKeywords());
    BOOST_CHECK_EQUAL((unsigned) 1, parserKeywords->getFixedNumberOfRecords("GRIDUNIT"));
}

BOOST_AUTO_TEST_CASE(GetFixedNumberOfRecords_ZeroRecords_ReturnsZero) {
    RawParserKeywordsConstPtr parserKeywords(new RawParserKeywords());
    BOOST_CHECK_EQUAL((unsigned) 0, parserKeywords->getFixedNumberOfRecords("METRIC"));
}
