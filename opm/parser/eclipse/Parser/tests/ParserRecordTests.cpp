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

#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <boost/test/test_tools.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(DefaultConstructor_NoParams_NoThrow) {
    BOOST_CHECK_NO_THROW(ParserRecord record);
}

BOOST_AUTO_TEST_CASE(InitSharedPointer_NoThrow) {
    BOOST_CHECK_NO_THROW(ParserRecordConstPtr ptr(new ParserRecord()));
    BOOST_CHECK_NO_THROW(ParserRecordPtr ptr(new ParserRecord()));
}

BOOST_AUTO_TEST_CASE(Size_NoElements_ReturnsZero) {
    ParserRecord record;
    BOOST_CHECK_EQUAL(0U, record.size());
}

BOOST_AUTO_TEST_CASE(Size_OneItem_Return1) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt(new ParserIntItem("ITEM1", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt);
    BOOST_CHECK_EQUAL(1U, record->size());
}

BOOST_AUTO_TEST_CASE(Get_OneItem_Return1) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt(new ParserIntItem("ITEM1", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt);
    {
        ParserItemConstPtr item = record->get(0);
        BOOST_CHECK_EQUAL(item, itemInt);
    }
}

BOOST_AUTO_TEST_CASE(Get_outOfRange_Throw) {
    ParserRecordPtr record(new ParserRecord());
    BOOST_CHECK_THROW(record->get(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(Get_KeyNotFound_Throw) {
    ParserRecordPtr record(new ParserRecord());
    BOOST_CHECK_THROW(record->get("Hei"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Get_KeyFound_OK) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt(new ParserIntItem("ITEM1", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt);
    {
        ParserItemConstPtr item = record->get("ITEM1");
        BOOST_CHECK_EQUAL(item, itemInt);
    }
}

BOOST_AUTO_TEST_CASE(Get_GetByNameAndIndex_OK) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt(new ParserIntItem("ITEM1", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt);
    {
        ParserItemConstPtr itemByName = record->get("ITEM1");
        ParserItemConstPtr itemByIndex = record->get(0);
        BOOST_CHECK_EQUAL(itemInt, itemByName);
        BOOST_CHECK_EQUAL(itemInt, itemByIndex);
    }
}

BOOST_AUTO_TEST_CASE(addItem_SameName_Throw) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt1(new ParserIntItem("ITEM1", sizeType));
    ParserIntItemPtr itemInt2(new ParserIntItem("ITEM1", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt1);
    BOOST_CHECK_THROW(record->addItem(itemInt2), std::invalid_argument);
}

ParserRecordPtr createSimpleParserRecord() {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt1(new ParserIntItem("ITEM1", sizeType));
    ParserIntItemPtr itemInt2(new ParserIntItem("ITEM2", sizeType));
    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt1);
    record->addItem(itemInt2);

    return record;
}

BOOST_AUTO_TEST_CASE(parse_validRecord_noThrow) {
    ParserRecordPtr record = createSimpleParserRecord();
    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    BOOST_CHECK_NO_THROW(record->parse(rawRecord));
}

BOOST_AUTO_TEST_CASE(parse_validRecord_deckRecordCreated) {
    ParserRecordPtr record = createSimpleParserRecord();
    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    DeckRecordConstPtr deckRecord = record->parse(rawRecord);
    BOOST_CHECK_EQUAL(2U, deckRecord->size());
}





