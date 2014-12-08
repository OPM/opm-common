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


#define BOOST_TEST_MODULE DeckRecordTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <boost/test/test_tools.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    BOOST_CHECK_NO_THROW(DeckRecord deckRecord);
}

BOOST_AUTO_TEST_CASE(size_defaultConstructor_sizezero) {
    DeckRecord deckRecord;
    BOOST_CHECK_EQUAL(0U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_singleItem_sizeone) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem);
    BOOST_CHECK_EQUAL(1U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_multipleItems_sizecorrect) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    DeckIntItemPtr intItem2(new DeckIntItem("TEST2"));
    DeckIntItemPtr intItem3(new DeckIntItem("TEST3"));

    deckRecord.addItem(intItem1);
    deckRecord.addItem(intItem2);
    deckRecord.addItem(intItem3);

    BOOST_CHECK_EQUAL(3U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_sameItemTwoTimes_throws) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));

    deckRecord.addItem(intItem1);
    BOOST_CHECK_THROW(deckRecord.addItem(intItem1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(addItem_differentItemsSameName_throws) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    DeckIntItemPtr intItem2(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem1);
    BOOST_CHECK_THROW(deckRecord.addItem(intItem2), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(get_byIndex_returnsItem) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem1);
    BOOST_CHECK_NO_THROW(deckRecord.getItem(0U));
}

BOOST_AUTO_TEST_CASE(get_indexoutofbounds_throws) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem1);
    BOOST_CHECK_THROW(deckRecord.getItem(1), std::range_error);
}

BOOST_AUTO_TEST_CASE(get_byName_returnsItem) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem1);
    deckRecord.getItem("TEST");
}

BOOST_AUTO_TEST_CASE(get_byNameNonExisting_throws) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    deckRecord.addItem(intItem1);
    BOOST_CHECK_THROW(deckRecord.getItem("INVALID"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(get_oneoftwo_returnscorrectitem) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    DeckIntItemPtr intItem2(new DeckIntItem("TEST2"));
    deckRecord.addItem(intItem1);
    deckRecord.addItem(intItem2);
    BOOST_CHECK_EQUAL(intItem2, deckRecord.getItem(1));
    BOOST_CHECK_EQUAL(intItem1, deckRecord.getItem("TEST"));
}


BOOST_AUTO_TEST_CASE(StringsWithSpaceOK) {
    ParserStringItemPtr itemString(new ParserStringItem(std::string("STRINGITEM1")));
    ParserRecordPtr record1(new ParserRecord());
    RawRecordPtr rawRecord(new Opm::RawRecord(" ' VALUE ' /"));
    record1->addItem( itemString );


    DeckRecordConstPtr deckRecord = record1->parse( rawRecord );
    BOOST_CHECK_EQUAL(" VALUE " , deckRecord->getItem(0)->getString(0));
}

