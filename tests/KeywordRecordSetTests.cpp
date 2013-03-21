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
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/data/KeywordRecordSet.hpp>

BOOST_AUTO_TEST_CASE(EmptyConstructorEmptyKeyword) {
    Opm::KeywordRecordSet keyword;
    BOOST_CHECK(keyword.getKeyword() == "");
}

BOOST_AUTO_TEST_CASE(KeywordToConstructorKeywordSet) {
    Opm::KeywordRecordSet keyword("KEYWORD");
    BOOST_CHECK(keyword.getKeyword() == "KEYWORD");
}

BOOST_AUTO_TEST_CASE(KeywordAndRecordsToConstructorSetCorrectly) {
    Opm::KeywordRecordSet keyword("KEYWORD", std::list<std::string>());
    BOOST_CHECK(keyword.getKeyword() == "KEYWORD");
}

BOOST_AUTO_TEST_CASE(SetTooLongKeywordNoError) {
    Opm::KeywordRecordSet keyword;
    keyword.setKeyword("TESTTOOLONG");
    BOOST_CHECK(keyword.getKeyword() == "TESTTOOLONG");
}

BOOST_AUTO_TEST_CASE(SetCorrectLenghtKeywordNoError) {
    Opm::KeywordRecordSet keyword;
    keyword.setKeyword("GOODONE");
    BOOST_CHECK(keyword.getKeyword() == "GOODONE");
}

BOOST_AUTO_TEST_CASE(SetKeywordWithTrailingWhitespaceKeywordNotTrimmed) {
    Opm::KeywordRecordSet keyword;
    keyword.setKeyword("GOODONE ");
    BOOST_CHECK(keyword.getKeyword() == "GOODONE ");
}

BOOST_AUTO_TEST_CASE(AddDataRecordNoKeywordNoError) {
    Opm::KeywordRecordSet keyword;
    keyword.addDataElement("This is a record");
}

BOOST_AUTO_TEST_CASE(AddDataRecordHasKeywordNoError) {
    Opm::KeywordRecordSet keyword;
    keyword.setKeyword("RECORD");
    keyword.addDataElement("This is a record");
    BOOST_CHECK(keyword.getKeyword() == "RECORD ");
}


