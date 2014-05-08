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

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <boost/test/test_tools.hpp>

#include "opm/parser/eclipse/RawDeck/RawKeyword.hpp"
#include "opm/parser/eclipse/Parser/ParserKeyword.hpp"

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
    ParserRecordConstPtr record(new ParserRecord());
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

static ParserRecordPtr createSimpleParserRecord() {
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
    rawRecord->dump();
    BOOST_CHECK_NO_THROW(record->parse(rawRecord));
}

BOOST_AUTO_TEST_CASE(parse_validRecord_deckRecordCreated) {
    ParserRecordPtr record = createSimpleParserRecord();
    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    DeckRecordConstPtr deckRecord = record->parse(rawRecord);
    BOOST_CHECK_EQUAL(2U, deckRecord->size());
}


// INT INT DOUBLE DOUBLE INT DOUBLE

static ParserRecordPtr createMixedParserRecord() {

    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt1(new ParserIntItem("INTITEM1", sizeType));
    ParserIntItemPtr itemInt2(new ParserIntItem("INTITEM2", sizeType));
    ParserDoubleItemPtr itemDouble1(new ParserDoubleItem("DOUBLEITEM1", sizeType));
    ParserDoubleItemPtr itemDouble2(new ParserDoubleItem("DOUBLEITEM2", sizeType));

    ParserIntItemPtr itemInt3(new ParserIntItem("INTITEM3", sizeType));
    ParserDoubleItemPtr itemDouble3(new ParserDoubleItem("DOUBLEITEM3", sizeType));

    ParserRecordPtr record(new ParserRecord());
    record->addItem(itemInt1);
    record->addItem(itemInt2);
    record->addItem(itemDouble1);
    record->addItem(itemDouble2);
    record->addItem(itemInt3);
    record->addItem(itemDouble3);

    return record;
}

BOOST_AUTO_TEST_CASE(parse_validMixedRecord_noThrow) {
    ParserRecordPtr record = createMixedParserRecord();
    RawRecordPtr rawRecord(new RawRecord("1 2 10.0 20.0 4 90.0 /"));
    BOOST_CHECK_NO_THROW(record->parse(rawRecord));
}

BOOST_AUTO_TEST_CASE(Equal_Equal_ReturnsTrue) {
    ParserRecordPtr record1 = createMixedParserRecord();
    ParserRecordPtr record2 = createMixedParserRecord();

    BOOST_CHECK(record1->equal(*record1));
    BOOST_CHECK(record1->equal(*record2));
}

BOOST_AUTO_TEST_CASE(Equal_Different_ReturnsFalse) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItemPtr itemInt(new ParserIntItem("INTITEM1", sizeType, 0));
    ParserDoubleItemPtr itemDouble(new ParserDoubleItem("DOUBLEITEM1", sizeType, 0));
    ParserStringItemPtr itemString(new ParserStringItem("STRINGITEM1", sizeType));
    ParserRecordPtr record1(new ParserRecord());
    ParserRecordPtr record2(new ParserRecord());
    ParserRecordPtr record3(new ParserRecord());

    record1->addItem(itemInt);
    record1->addItem(itemDouble);

    record2->addItem(itemInt);
    record2->addItem(itemDouble);
    record2->addItem(itemString);

    record3->addItem(itemDouble);
    record3->addItem(itemInt);
    BOOST_CHECK(!record1->equal(*record2));
    BOOST_CHECK(!record1->equal(*record3));

}

BOOST_AUTO_TEST_CASE(ParseWithDefault_defaultAppliedCorrectInDeck) {
    ParserRecord parserRecord;
    ParserIntItemConstPtr itemInt(new ParserIntItem("ITEM1", SINGLE));
    ParserStringItemConstPtr itemString(new ParserStringItem("ITEM2", SINGLE));
    ParserDoubleItemConstPtr itemDouble(new ParserDoubleItem("ITEM3", SINGLE));

    parserRecord.addItem(itemInt);
    parserRecord.addItem(itemString);
    parserRecord.addItem(itemDouble);

    {
        RawRecordPtr rawRecord(new RawRecord("* /"));
        DeckItemConstPtr deckStringItem = itemString->scan(rawRecord);
        DeckItemConstPtr deckIntItem = itemInt->scan(rawRecord);
        DeckItemConstPtr deckDoubleItem = itemDouble->scan(rawRecord);

        BOOST_CHECK(deckStringItem->defaultApplied());
        BOOST_CHECK(deckIntItem->defaultApplied());
        BOOST_CHECK(deckDoubleItem->defaultApplied());
    }


    {
        RawRecordPtr rawRecord(new RawRecord("/"));
        DeckItemConstPtr deckStringItem = itemString->scan(rawRecord);
        DeckItemConstPtr deckIntItem = itemInt->scan(rawRecord);
        DeckItemConstPtr deckDoubleItem = itemDouble->scan(rawRecord);

        BOOST_CHECK(deckStringItem->defaultApplied());
        BOOST_CHECK(deckIntItem->defaultApplied());
        BOOST_CHECK(deckDoubleItem->defaultApplied());
    }


    {
        RawRecordPtr rawRecord(new RawRecord("TRYGVE 10 2.9 /"));
        DeckItemConstPtr deckStringItem = itemString->scan(rawRecord);
        DeckItemConstPtr deckIntItem = itemInt->scan(rawRecord);
        DeckItemConstPtr deckDoubleItem = itemDouble->scan(rawRecord);

        BOOST_CHECK_EQUAL(false, deckStringItem->defaultApplied());
        BOOST_CHECK_EQUAL(false, deckIntItem->defaultApplied());
        BOOST_CHECK_EQUAL(false, deckDoubleItem->defaultApplied());
    }


    {
        RawRecordPtr rawRecord(new RawRecord("* * * /"));
        DeckItemConstPtr deckStringItem = itemString->scan(rawRecord);
        DeckItemConstPtr deckIntItem = itemInt->scan(rawRecord);
        DeckItemConstPtr deckDoubleItem = itemDouble->scan(rawRecord);

        BOOST_CHECK_EQUAL(true, deckStringItem->defaultApplied());
        BOOST_CHECK_EQUAL(true, deckIntItem->defaultApplied());
        BOOST_CHECK_EQUAL(true, deckDoubleItem->defaultApplied());
    }

    {
        RawRecordPtr rawRecord(new RawRecord("3* /"));
        DeckItemConstPtr deckStringItem = itemString->scan(rawRecord);
        DeckItemConstPtr deckIntItem = itemInt->scan(rawRecord);
        DeckItemConstPtr deckDoubleItem = itemDouble->scan(rawRecord);

        BOOST_CHECK_EQUAL(true, deckStringItem->defaultApplied());
        BOOST_CHECK_EQUAL(true, deckIntItem->defaultApplied());
        BOOST_CHECK_EQUAL(true, deckDoubleItem->defaultApplied());
    }
}

BOOST_AUTO_TEST_CASE(Parse_RawRecordTooManyItems_Throws) {
    ParserRecordPtr parserRecord(new ParserRecord());
    ParserIntItemConstPtr itemI(new ParserIntItem("I", SINGLE));
    ParserIntItemConstPtr itemJ(new ParserIntItem("J", SINGLE));
    ParserIntItemConstPtr itemK(new ParserIntItem("K", SINGLE));

    parserRecord->addItem(itemI);
    parserRecord->addItem(itemJ);
    parserRecord->addItem(itemK);

        
    RawRecordPtr rawRecord(new RawRecord("3 3 3 /"));
    BOOST_CHECK_NO_THROW(parserRecord->parse(rawRecord));
    
    RawRecordPtr rawRecordOneExtra(new RawRecord("3 3 3 4 /"));
    BOOST_CHECK_THROW(parserRecord->parse(rawRecordOneExtra), std::invalid_argument);

    RawRecordPtr rawRecordForgotRecordTerminator(new RawRecord("3 3 3 \n 4 4 4 /"));
    BOOST_CHECK_THROW(parserRecord->parse(rawRecordForgotRecordTerminator), std::invalid_argument);

}


BOOST_AUTO_TEST_CASE(Parse_RawRecordTooFewItems_ThrowsNot) {
    ParserRecordPtr parserRecord(new ParserRecord());
    ParserIntItemConstPtr itemI(new ParserIntItem("I", SINGLE));
    ParserIntItemConstPtr itemJ(new ParserIntItem("J", SINGLE));
    ParserIntItemConstPtr itemK(new ParserIntItem("K", SINGLE));

    parserRecord->addItem(itemI);
    parserRecord->addItem(itemJ);
    parserRecord->addItem(itemK);

    RawRecordPtr rawRecord(new RawRecord("3 3  /"));
    BOOST_CHECK_NO_THROW(parserRecord->parse(rawRecord));
}



BOOST_AUTO_TEST_CASE(ParseRecordHasDimensionCorrect) {
    ParserRecordPtr parserRecord(new ParserRecord());
    ParserIntItemConstPtr itemI(new ParserIntItem("I", SINGLE));
    ParserDoubleItemPtr item2(new ParserDoubleItem("ID", SINGLE));
    
    BOOST_CHECK_EQUAL( false , parserRecord->hasDimension());
    
    parserRecord->addItem( itemI );
    parserRecord->addItem( item2 );
    BOOST_CHECK_EQUAL( false , parserRecord->hasDimension());
    
    item2->push_backDimension("Length*Length/Time");
    BOOST_CHECK_EQUAL( true , parserRecord->hasDimension());
}
