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

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <boost/test/test_tools.hpp>

#include "opm/parser/eclipse/RawDeck/RawKeyword.hpp"
#include "opm/parser/eclipse/Parser/ParserKeyword.hpp"


using namespace Opm;


const static auto SINGLE = ParserItem::item_size::SINGLE;
const static auto ALL = ParserItem::item_size::ALL;

BOOST_AUTO_TEST_CASE(DefaultConstructor_NoParams_NoThrow) {
    BOOST_CHECK_NO_THROW(ParserRecord record);
}

BOOST_AUTO_TEST_CASE(Size_NoElements_ReturnsZero) {
    ParserRecord record;
    BOOST_CHECK_EQUAL(0U, record.size());
}

BOOST_AUTO_TEST_CASE(Size_OneItem_Return1) {
    ParserItem itemInt("ITEM1", SINGLE );
    ParserRecord record;
    record.addItem(itemInt);
    BOOST_CHECK_EQUAL(1U, record.size());
}

BOOST_AUTO_TEST_CASE(Get_OneItem_Return1) {
    ParserItem itemInt("ITEM1", SINGLE);
    ParserRecord record;
    record.addItem(itemInt);

    BOOST_CHECK_EQUAL(record.get(0), itemInt);
}

BOOST_AUTO_TEST_CASE(Get_outOfRange_Throw) {
    BOOST_CHECK_THROW(ParserRecord{}.get(0), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(Get_KeyNotFound_Throw) {
    ParserRecord record;
    BOOST_CHECK_THROW(record.get("Hei"), std::out_of_range );
}

BOOST_AUTO_TEST_CASE(Get_KeyFound_OK) {
    ParserItem itemInt("ITEM1", SINGLE );
    ParserRecord record;
    record.addItem(itemInt);
    BOOST_CHECK_EQUAL(record.get("ITEM1"), itemInt);
}

BOOST_AUTO_TEST_CASE(Get_GetByNameAndIndex_OK) {
    ParserItem itemInt("ITEM1", SINGLE);
    ParserRecord record;
    record.addItem(itemInt);

    const auto& itemByName = record.get("ITEM1");
    const auto& itemByIndex = record.get(0);
    BOOST_CHECK_EQUAL(itemInt, itemByName);
    BOOST_CHECK_EQUAL(itemInt, itemByIndex);
}

BOOST_AUTO_TEST_CASE(addItem_SameName_Throw) {
    ParserItem itemInt1("ITEM1", SINGLE);
    ParserItem itemInt2("ITEM1", SINGLE);
    ParserRecord record;
    record.addItem(itemInt1);
    BOOST_CHECK_THROW(record.addItem(itemInt2), std::invalid_argument);
}

static ParserRecord createSimpleParserRecord() {
    ParserItem itemInt1("ITEM1", SINGLE, 0 );
    ParserItem itemInt2("ITEM2", SINGLE, 0 );
    ParserRecord record;

    record.addItem(itemInt1);
    record.addItem(itemInt2);
    return record;
}

BOOST_AUTO_TEST_CASE(parse_validRecord_noThrow) {
    auto record = createSimpleParserRecord();
    ParseContext parseContext;
    RawRecord raw( string_view( "100 443" ) );
    MessageContainer msgContainer;
    BOOST_CHECK_NO_THROW(record.parse(parseContext, msgContainer, raw ) );
}

BOOST_AUTO_TEST_CASE(parse_validRecord_deckRecordCreated) {
    auto record = createSimpleParserRecord();
    RawRecord rawRecord( string_view( "100 443" ) );
    ParseContext parseContext;
    MessageContainer msgContainer;
    const auto deckRecord = record.parse(parseContext , msgContainer, rawRecord);
    BOOST_CHECK_EQUAL(2U, deckRecord.size());
}


// INT INT DOUBLE DOUBLE INT DOUBLE

static ParserRecord createMixedParserRecord() {

    auto sizeType = SINGLE;
    ParserItem itemInt1( "INTITEM1", sizeType, 0 );
    ParserItem itemInt2( "INTITEM2", sizeType, 0 );
    ParserItem itemInt3( "INTITEM3", sizeType, 0 );
    ParserItem itemDouble1( "DOUBLEITEM1", sizeType, 0.0 );
    ParserItem itemDouble2( "DOUBLEITEM2", sizeType, 0.0 );
    ParserItem itemDouble3( "DOUBLEITEM3", sizeType, 0.0 );

    ParserRecord record;
    record.addItem(itemInt1);
    record.addItem(itemInt2);
    record.addItem(itemDouble1);
    record.addItem(itemDouble2);
    record.addItem(itemInt3);
    record.addItem(itemDouble3);

    return record;
}

BOOST_AUTO_TEST_CASE(parse_validMixedRecord_noThrow) {
    auto record = createMixedParserRecord();
    RawRecord rawRecord( string_view( "1 2 10.0 20.0 4 90.0") );
    ParseContext parseContext;
    MessageContainer msgContainer;
    BOOST_CHECK_NO_THROW(record.parse(parseContext , msgContainer, rawRecord));
}

BOOST_AUTO_TEST_CASE(Equal_Equal_ReturnsTrue) {
    auto record1 = createMixedParserRecord();
    auto record2 = createMixedParserRecord();

    BOOST_CHECK(record1.equal(record1));
    BOOST_CHECK(record1.equal(record2));
}

BOOST_AUTO_TEST_CASE(Equal_Different_ReturnsFalse) {
    auto sizeType = SINGLE;
    ParserItem    itemInt( "INTITEM1", sizeType, 0 );
    ParserItem itemDouble( "DOUBLEITEM1", sizeType, 0.0 );
    ParserItem itemString( "STRINGITEM1", sizeType, "" );
    ParserRecord record1;
    ParserRecord record2;
    ParserRecord record3;

    record1.addItem(itemInt);
    record1.addItem(itemDouble);

    record2.addItem(itemInt);
    record2.addItem(itemDouble);
    record2.addItem(itemString);

    record3.addItem(itemDouble);
    record3.addItem(itemInt);
    BOOST_CHECK(!record1.equal(record2));
    BOOST_CHECK(!record1.equal(record3));

}

BOOST_AUTO_TEST_CASE(ParseWithDefault_defaultAppliedCorrectInDeck) {
    ParserRecord parserRecord;
    ParserItem itemInt("ITEM1", SINGLE , 100 );
    ParserItem itemString("ITEM2", SINGLE , "DEFAULT" );
    ParserItem itemDouble("ITEM3", SINGLE , 3.14 );

    parserRecord.addItem(itemInt);
    parserRecord.addItem(itemString);
    parserRecord.addItem(itemDouble);

    // according to the RM, this is invalid ("an asterisk by itself is not sufficient"),
    // but it seems to appear in the wild. Thus, we interpret this as "1*"...
    {
        RawRecord rawRecord( "* " );
        const auto& deckStringItem = itemString.scan(rawRecord);
        const auto& deckIntItem = itemInt.scan(rawRecord);
        const auto& deckDoubleItem = itemDouble.scan(rawRecord);

        BOOST_CHECK(deckStringItem.size() == 1);
        BOOST_CHECK(deckIntItem.size() == 1);
        BOOST_CHECK(deckDoubleItem.size() == 1);

        BOOST_CHECK(deckStringItem.defaultApplied(0));
        BOOST_CHECK(deckIntItem.defaultApplied(0));
        BOOST_CHECK(deckDoubleItem.defaultApplied(0));
    }

    {
        RawRecord rawRecord( "" );
        const auto deckStringItem = itemString.scan(rawRecord);
        const auto deckIntItem = itemInt.scan(rawRecord);
        const auto deckDoubleItem = itemDouble.scan(rawRecord);

        BOOST_CHECK_EQUAL(deckStringItem.size(), 1);
        BOOST_CHECK_EQUAL(deckIntItem.size(), 1);
        BOOST_CHECK_EQUAL(deckDoubleItem.size(), 1);

        BOOST_CHECK(deckStringItem.defaultApplied(0));
        BOOST_CHECK(deckIntItem.defaultApplied(0));
        BOOST_CHECK(deckDoubleItem.defaultApplied(0));
    }


    {
        RawRecord rawRecord( "TRYGVE 10 2.9 " );

        // let the raw record be "consumed" by the items. Note that the scan() method
        // modifies the rawRecord object!
        const auto& deckStringItem = itemString.scan(rawRecord);
        const auto& deckIntItem = itemInt.scan(rawRecord);
        const auto& deckDoubleItem = itemDouble.scan(rawRecord);

        BOOST_CHECK_EQUAL(deckStringItem.size(), 1);
        BOOST_CHECK_EQUAL(deckIntItem.size(), 1);
        BOOST_CHECK_EQUAL(deckDoubleItem.size(), 1);

        BOOST_CHECK(!deckStringItem.defaultApplied(0));
        BOOST_CHECK(!deckIntItem.defaultApplied(0));
        BOOST_CHECK(!deckDoubleItem.defaultApplied(0));
    }

    // again this is invalid according to the RM, but it is used anyway in the wild...
    {
        RawRecord rawRecord( "* * *" );
        const auto deckStringItem = itemString.scan(rawRecord);
        const auto deckIntItem = itemInt.scan(rawRecord);
        const auto deckDoubleItem = itemDouble.scan(rawRecord);

        BOOST_CHECK_EQUAL(deckStringItem.size(), 1);
        BOOST_CHECK_EQUAL(deckIntItem.size(), 1);
        BOOST_CHECK_EQUAL(deckDoubleItem.size(), 1);

        BOOST_CHECK(deckStringItem.defaultApplied(0));
        BOOST_CHECK(deckIntItem.defaultApplied(0));
        BOOST_CHECK(deckDoubleItem.defaultApplied(0));
    }

    {
        RawRecord rawRecord(  "3*" );
        const auto deckStringItem = itemString.scan(rawRecord);
        const auto deckIntItem = itemInt.scan(rawRecord);
        const auto deckDoubleItem = itemDouble.scan(rawRecord);

        BOOST_CHECK_EQUAL(deckStringItem.size(), 1);
        BOOST_CHECK_EQUAL(deckIntItem.size(), 1);
        BOOST_CHECK_EQUAL(deckDoubleItem.size(), 1);

        BOOST_CHECK(deckStringItem.defaultApplied(0));
        BOOST_CHECK(deckIntItem.defaultApplied(0));
        BOOST_CHECK(deckDoubleItem.defaultApplied(0));
    }
}

BOOST_AUTO_TEST_CASE(Parse_RawRecordTooManyItems_Throws) {
    ParserRecord parserRecord;
    ParserItem itemI( "I", SINGLE, 0 );
    ParserItem itemJ( "J", SINGLE, 0 );
    ParserItem itemK( "K", SINGLE, 0 );
    ParseContext parseContext;

    parserRecord.addItem(itemI);
    parserRecord.addItem(itemJ);
    parserRecord.addItem(itemK);


    RawRecord rawRecord(  "3 3 3 " );
    MessageContainer msgContainer;

    BOOST_CHECK_NO_THROW(parserRecord.parse(parseContext , msgContainer, rawRecord));

    RawRecord rawRecordOneExtra(  "3 3 3 4 " );
    BOOST_CHECK_THROW(parserRecord.parse(parseContext , msgContainer, rawRecordOneExtra), std::invalid_argument);

    RawRecord rawRecordForgotRecordTerminator(  "3 3 3 \n 4 4 4 " );
    BOOST_CHECK_THROW(parserRecord.parse(parseContext , msgContainer, rawRecordForgotRecordTerminator), std::invalid_argument);

}


BOOST_AUTO_TEST_CASE(Parse_RawRecordTooFewItems) {
    ParserRecord parserRecord;
    ParserItem itemI( "I", SINGLE );
    ParserItem itemJ( "J", SINGLE );
    ParserItem itemK( "K", SINGLE );
    itemI.setType( int() );
    itemJ.setType( int() );
    itemK.setType( int() );

    parserRecord.addItem(itemI);
    parserRecord.addItem(itemJ);
    parserRecord.addItem(itemK);

    ParseContext parseContext;
    RawRecord rawRecord(  "3 3  " );
    // no default specified for the third item, record can be parsed just fine but trying
    // to access the data will raise an exception...
    MessageContainer msgContainer;
    BOOST_CHECK_NO_THROW(parserRecord.parse(parseContext , msgContainer, rawRecord));
    auto record = parserRecord.parse(parseContext , msgContainer, rawRecord);
    BOOST_CHECK_NO_THROW(record.getItem(2));
    BOOST_CHECK_THROW(record.getItem(2).get< int >(0), std::out_of_range);
}



BOOST_AUTO_TEST_CASE(ParseRecordHasDimensionCorrect) {
    ParserRecord parserRecord;
    ParserItem itemI( "I", SINGLE, 0.0 );

    BOOST_CHECK( !parserRecord.hasDimension() );

    parserRecord.addItem( itemI );
    BOOST_CHECK( !parserRecord.hasDimension() );

    ParserItem item2( "ID", SINGLE, 0.0 );
    item2.push_backDimension("Length*Length/Time");
    parserRecord.addItem( item2 );
    BOOST_CHECK( parserRecord.hasDimension() );
}


BOOST_AUTO_TEST_CASE(DefaultNotDataRecord) {
    ParserRecord record;
    BOOST_CHECK_EQUAL( false , record.isDataRecord() );
}

BOOST_AUTO_TEST_CASE(MixingDataAndItems_throws1) {
    ParserRecord record;
    ParserItem dataItem( "ACTNUM" , ALL );
    ParserItem item    ( "XXX" , ALL );
    record.addDataItem( dataItem );
    BOOST_CHECK_THROW( record.addItem( item ) , std::invalid_argument);
    BOOST_CHECK_THROW( record.addItem( dataItem ) , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MixingDataAndItems_throws2) {
    ParserRecord record;
    ParserItem dataItem( "ACTNUM" , ALL);
    ParserItem item    ( "XXX" , ALL);

    record.addItem( item );
    BOOST_CHECK_THROW( record.addDataItem( dataItem ) , std::invalid_argument);
}
