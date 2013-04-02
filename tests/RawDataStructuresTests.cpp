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

#define BOOST_TEST_MODULE ParserTests
#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/data/RawKeyword.hpp>
#include <opm/parser/eclipse/data/RawRecord.hpp>


BOOST_AUTO_TEST_CASE(EmptyConstructorEmptyKeyword) {
    Opm::RawKeyword keyword;
    BOOST_CHECK(keyword.getKeyword() == "");
}

BOOST_AUTO_TEST_CASE(KeywordToConstructorKeywordSet) {
    Opm::RawKeyword keyword("KEYYWORD");
    BOOST_CHECK(keyword.getKeyword() == "KEYYWORD");
}

BOOST_AUTO_TEST_CASE(KeywordToConstructorTooLongThrows) {
    BOOST_CHECK_THROW(Opm::RawKeyword keyword("KEYYYWORD"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(SetTooLongKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword("TESTTOOLONG"), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(SetKeywordInitialWhitespaceInKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword(" TELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(SetKeywordInitialTabInKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword("\tTELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(SetCorrectLenghtKeywordNoError) {
    Opm::RawKeyword keyword;
    keyword.setKeyword("GOODONE");
    BOOST_CHECK(keyword.getKeyword() == "GOODONE");
}

BOOST_AUTO_TEST_CASE(Set8CharKeywordWithTrailingWhitespaceKeywordTrimmed) {
    Opm::RawKeyword keyword;
    keyword.setKeyword("GOODONEE ");
    BOOST_CHECK(keyword.getKeyword() == "GOODONEE");
}

BOOST_AUTO_TEST_CASE(RawRecordSetRecordCheckCorrectTrimAndSplit) {
    Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20                                       /"));
    std::string recordString;
    record -> getRecordString(recordString);
    BOOST_REQUIRE_EQUAL("'NODIR '  'REVERS'  1  20", recordString);
   
    std::vector<std::string> recordElements;
    record -> getRecords(recordElements);
    BOOST_REQUIRE_EQUAL((unsigned)4, recordElements.size());
    
    BOOST_REQUIRE_EQUAL("NODIR ", recordElements[0]);
    BOOST_REQUIRE_EQUAL("REVERS", recordElements[1]);
    BOOST_REQUIRE_EQUAL("1", recordElements[2]);
    BOOST_REQUIRE_EQUAL("20", recordElements[3]);
}
