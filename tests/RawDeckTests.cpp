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
#define BOOST_TEST_MODULE RawDeckTests
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>
#include <opm/parser/eclipse/RawDeck/RawParserKWs.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <boost/test/test_tools.hpp>

BOOST_AUTO_TEST_CASE(GetNumberOfKeywords_EmptyDeck_RetunsZero) {
    Opm::RawParserKWsConstPtr fixedKeywords(new Opm::RawParserKWs());
    Opm::RawDeckPtr rawDeck(new Opm::RawDeck(fixedKeywords));
    BOOST_CHECK_EQUAL((unsigned) 0, rawDeck->getNumberOfKeywords());
}

BOOST_AUTO_TEST_CASE(HasKeyword_NotExisting_RetunsFalse) {
    Opm::RawParserKWsConstPtr fixedKeywords(new Opm::RawParserKWs());
    Opm::RawDeckPtr rawDeck(new Opm::RawDeck(fixedKeywords));
    BOOST_CHECK_EQUAL(false, rawDeck->hasKeyword("TEST"));
}

BOOST_AUTO_TEST_CASE(GetKeyword_EmptyDeck_ThrowsExeption) {
    Opm::RawParserKWsConstPtr fixedKeywords(new Opm::RawParserKWs());
    Opm::RawDeckPtr rawDeck(new Opm::RawDeck(fixedKeywords));
    BOOST_CHECK_THROW(rawDeck->getKeyword("TEST"), std::invalid_argument);
}
