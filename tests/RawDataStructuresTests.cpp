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
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

BOOST_AUTO_TEST_CASE(RawKeywordEmptyConstructorEmptyKeyword) {
    Opm::RawKeyword keyword;
    BOOST_CHECK(keyword.getKeyword() == "");
}

BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorKeywordSet) {
    Opm::RawKeyword keyword("KEYYWORD");
    BOOST_CHECK(keyword.getKeyword() == "KEYYWORD");
}

BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorTooLongThrows) {
    BOOST_CHECK_THROW(Opm::RawKeyword keyword("KEYYYWORD"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetTooLongKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword("TESTTOOLONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialWhitespaceInKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword(" TELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialTabInKeywordThrows) {
    Opm::RawKeyword keyword;
    BOOST_CHECK_THROW(keyword.setKeyword("\tTELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetCorrectLenghtKeywordNoError) {
    Opm::RawKeyword keyword;
    keyword.setKeyword("GOODONE");
    BOOST_CHECK(keyword.getKeyword() == "GOODONE");
}

BOOST_AUTO_TEST_CASE(RawKeywordSet8CharKeywordWithTrailingWhitespaceKeywordTrimmed) {
    Opm::RawKeyword keyword;
    keyword.setKeyword("GOODONEE ");
    BOOST_CHECK(keyword.getKeyword() == "GOODONEE");
}


BOOST_AUTO_TEST_CASE(RawRecordGetRecordStringReturnsTrimmedString) {
    Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20                                       /"));
    const std::string& recordString = record->getRecordString();
    BOOST_REQUIRE_EQUAL("'NODIR '  'REVERS'  1  20", recordString);
}



BOOST_AUTO_TEST_CASE(RawRecordGetRecordsCorrectElementsReturned) {
    Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20                                       /"));

    const std::vector<std::string>& recordElements = record->getRecords();
    BOOST_REQUIRE_EQUAL((unsigned) 4, recordElements.size());

    BOOST_REQUIRE_EQUAL("NODIR ", recordElements[0]);
    BOOST_REQUIRE_EQUAL("REVERS", recordElements[1]);
    BOOST_REQUIRE_EQUAL("1", recordElements[2]);
    BOOST_REQUIRE_EQUAL("20", recordElements[3]);
}

BOOST_AUTO_TEST_CASE(RawRecordIsCompleteRecordCompleteRecordReturnsTrue) {
    bool isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS'  1  20                                       /");
    BOOST_REQUIRE_EQUAL(true, isComplete);
}

BOOST_AUTO_TEST_CASE(RawRecordIsCompleteRecordInCompleteRecordReturnsFalse) {
    bool isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS'  1  20                                       ");
    BOOST_REQUIRE_EQUAL(false, isComplete);
    isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS  1  20 /");
    BOOST_REQUIRE_EQUAL(false, isComplete);
}
