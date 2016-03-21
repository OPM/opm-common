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

#include <opm/parser/eclipse/RawDeck/RawEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorKeywordSet) {
    RawKeyword keyword("KEYYWORD", Raw::SLASH_TERMINATED , "FILE" , 10U);
    BOOST_CHECK(keyword.getKeywordName() == "KEYYWORD");
    BOOST_CHECK_EQUAL(Raw::SLASH_TERMINATED , keyword.getSizeType());
}

BOOST_AUTO_TEST_CASE(RawKeywordSizeTypeInvalidThrows) {
    BOOST_CHECK_THROW( RawKeyword("KEYYWORD", Raw::FIXED , "FILE" , 0U) , std::invalid_argument);
    BOOST_CHECK_THROW( RawKeyword("KEYYWORD", Raw::TABLE_COLLECTION , "FILE" , 10U) , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordFinalizeWrongSizeTYpeThrows) {
    RawKeyword kw("KEYYWORD", Raw::SLASH_TERMINATED , "FILE" , 0U);
    BOOST_CHECK_THROW(     kw.finalizeUnknownSize() , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(RawKeywordFinalizeUnknownSize) {
    RawKeyword kw("KEYYWORD", Raw::UNKNOWN , "FILE" , 0U);
    BOOST_CHECK( !kw.isFinished() );
    kw.finalizeUnknownSize();
    BOOST_CHECK( kw.isFinished() );
}




BOOST_AUTO_TEST_CASE(RawKeywordGiveKeywordToConstructorTooLongThrows) {
    BOOST_CHECK_THROW(RawKeyword keyword("KEYYYWORD", Raw::SLASH_TERMINATED , "FILE" , 10U), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialWhitespaceInKeywordThrows) {
    BOOST_CHECK_THROW(RawKeyword(" TELONG", Raw::SLASH_TERMINATED, "FILE" , 10U), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(constructor_mixedCaseName_throws) {
    BOOST_CHECK_NO_THROW(RawKeyword("Test", Raw::SLASH_TERMINATED , "FILE" , 10U));
}

BOOST_AUTO_TEST_CASE(RawKeywordSetKeywordInitialTabInKeywordThrows) {
    BOOST_CHECK_THROW( RawKeyword("\tTELONG", Raw::SLASH_TERMINATED , "FILE" , 10U), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RawKeywordSetCorrectLenghtKeywordNoError) {
    RawKeyword keyword("GOODONE", Raw::SLASH_TERMINATED , "FILE" , 10U);
    BOOST_CHECK(keyword.getKeywordName() == "GOODONE");
}

BOOST_AUTO_TEST_CASE(RawKeywordSet8CharKeywordWithTrailingWhitespaceKeywordTrimmed) {
    RawKeyword keyword("GOODONEE ", Raw::SLASH_TERMINATED , "FILE" , 10U);
    BOOST_CHECK(keyword.getKeywordName() == "GOODONEE");
}


BOOST_AUTO_TEST_CASE(addRecord_singleRecord_recordAdded) {
    RawKeyword keyword("TEST", Raw::SLASH_TERMINATED , "FILE" , 10U);
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK_EQUAL(1U, keyword.size());
}

BOOST_AUTO_TEST_CASE(getRecord_outOfRange_throws) {
    RawKeyword keyword("TEST", Raw::SLASH_TERMINATED , "FILE" , 10U);
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK_THROW(keyword.getRecord(1), std::out_of_range);
}



BOOST_AUTO_TEST_CASE(isFinished_undef_size) {
    RawKeyword keyword("TEST", Raw::SLASH_TERMINATED , "FILE" , 10U);

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
    RawKeyword keyword("TEST" , "FILE" , 10U , 0U);

    BOOST_CHECK(  keyword.isFinished() );
}


BOOST_AUTO_TEST_CASE(isFinished_Fixedsize1) {
    RawKeyword keyword("TEST" , "FILE" , 10U, 1U);
    BOOST_CHECK(  !keyword.isFinished() );
    keyword.addRawRecordString("test 1 3 4 /");
    BOOST_CHECK(  keyword.isFinished() );
}


BOOST_AUTO_TEST_CASE(isFinished_FixedsizeMulti) {
    RawKeyword keyword("TEST", "FILE" , 10U , 4U);
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

    const auto& record = keyword.getRecord(3);
    BOOST_CHECK_EQUAL( 4U , record.size() );

}

BOOST_AUTO_TEST_CASE(isTableCollection) {
    RawKeyword keyword1("TEST" , "FILE" , 10U, 4U , false);
    RawKeyword keyword2("TEST2", Raw::SLASH_TERMINATED , "FILE" , 10U);
    BOOST_CHECK_EQUAL( Raw::FIXED , keyword1.getSizeType());
    BOOST_CHECK_EQUAL( Raw::SLASH_TERMINATED , keyword2.getSizeType());
 }


BOOST_AUTO_TEST_CASE(CreateTableCollection) {
    RawKeyword keyword1("TEST" , "FILE" , 10U, 2, true);
    BOOST_CHECK_EQUAL( Raw::TABLE_COLLECTION , keyword1.getSizeType());
}


BOOST_AUTO_TEST_CASE(CreateWithFileAndLine) {
    RawKeyword keyword1("TEST" , Raw::SLASH_TERMINATED , "XXX", 100);
    BOOST_CHECK_EQUAL( "XXX" , keyword1.getFilename());
    BOOST_CHECK_EQUAL( 100U , keyword1.getLineNR() );
}

BOOST_AUTO_TEST_CASE(isUnknownSize) {
    RawKeyword keyword("TEST2", Raw::UNKNOWN , "FILE" , 10U);
    BOOST_CHECK_EQUAL( Raw::UNKNOWN  , keyword.getSizeType( ));
 }

