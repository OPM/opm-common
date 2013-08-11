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

#define BOOST_TEST_MODULE RawKeywordTests
#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorKeywordSet) {
    RawKeyword keyword("KEYYWORD");
    BOOST_CHECK(keyword.getKeywordName() == "KEYYWORD");
}

BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorTooLongThrows) {
    BOOST_CHECK_THROW(RawKeyword keyword("KEYYYWORD"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialWhitespaceInKeywordThrows) {
    BOOST_CHECK_THROW(RawKeyword(" TELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(constructor_mixedCaseName_throws) {
    BOOST_CHECK_THROW(RawKeyword("Test"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialTabInKeywordThrows) {
    BOOST_CHECK_THROW( RawKeyword("\tTELONG"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetCorrectLenghtKeywordNoError) {
    RawKeyword keyword("GOODONE");
    BOOST_CHECK(keyword.getKeywordName() == "GOODONE");
}

BOOST_AUTO_TEST_CASE(RawKeywordSet8CharKeywordWithTrailingWhitespaceKeywordTrimmed) {
    RawKeyword keyword("GOODONEE ");
    BOOST_CHECK(keyword.getKeywordName() == "GOODONEE");
}


BOOST_AUTO_TEST_CASE(addRecord_singleRecord_recordAdded) {
    RawKeyword keyword("TEST");
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK_EQUAL(1U, keyword.size());
}

BOOST_AUTO_TEST_CASE(getRecord_outOfRange_throws) {
    RawKeyword keyword("TEST");
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK_THROW(keyword.getRecord(1), std::range_error);
}



BOOST_AUTO_TEST_CASE(isFinished_undef_size) {
    RawKeyword keyword("TEST");

    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("test 1 3 4 /");
    keyword.addRawRecordString("test 1 3 4");
    keyword.addRawRecordString("test 1 3 4");
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("/");
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("/");
    BOOST_CHECK(  keyword.isFinished() );
}


BOOST_AUTO_TEST_CASE(isFinished_Fixedsize0) {
    RawKeyword keyword("TEST" , 0U);
    
    BOOST_CHECK(  keyword.isFinished() );
}


BOOST_AUTO_TEST_CASE(isFinished_Fixedsize1) {
    RawKeyword keyword("TEST" , 1U);
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK(  keyword.isFinished() );
}


BOOST_AUTO_TEST_CASE(isFinished_FixedsizeMulti) {
    RawKeyword keyword("TEST" , 4U);
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK(  !keyword.isFinished() );

    keyword.addRawRecordString("/");
    BOOST_CHECK(  !keyword.isFinished() );

    keyword.addRawRecordString("1 2 3 3 4");
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("1 2 3 3 4 /");
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("1 2 3 3 /");
    BOOST_CHECK(  keyword.isFinished() );
    
    RawRecordConstPtr record = keyword.getRecord(3);
    BOOST_CHECK_EQUAL( 4U , record->size() );

}



BOOST_AUTO_TEST_CASE(isKeywordTerminator) {
    BOOST_CHECK( RawKeyword::isTerminator("/"));
    BOOST_CHECK( RawKeyword::isTerminator("  /"));
    BOOST_CHECK( RawKeyword::isTerminator("/  "));
    BOOST_CHECK( RawKeyword::isTerminator("  /"));
    BOOST_CHECK( RawKeyword::isTerminator("  /"));

    BOOST_CHECK( !RawKeyword::isTerminator("  X/  "));
}


BOOST_AUTO_TEST_CASE(useLine) {
    BOOST_CHECK( !RawKeyword::useLine("                   "));
    BOOST_CHECK( !RawKeyword::useLine("-- ggg"));

    BOOST_CHECK( RawKeyword::useLine("Data -- ggg"));
    BOOST_CHECK( RawKeyword::useLine("/ -- ggg"));
    BOOST_CHECK( RawKeyword::useLine("/"));
}

