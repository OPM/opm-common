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

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserBoolItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    ParserItemSizeEnum sizeType = SINGLE;
    BOOST_CHECK_NO_THROW(ParserIntItem item1("ITEM1", sizeType));
    BOOST_CHECK_NO_THROW(ParserStringItem item1("ITEM1", sizeType));
    //    BOOST_CHECK_NO_THROW(ParserBoolItem item1("ITEM1", sizeType));
    //    BOOST_CHECK_NO_THROW(ParserDoubleItem item1("ITEM1", sizeType));
}

BOOST_AUTO_TEST_CASE(Initialize_Default) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItem item1("ITEM1", sizeType);
    ParserIntItem item2("ITEM1", sizeType, 88);
    BOOST_CHECK_EQUAL(item1.getDefault(), ParserItem::defaultInt());
    BOOST_CHECK_EQUAL(item2.getDefault(), 88);
}

/******************************************************************/
/* <Json> */
BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_missingName_throws) {
    Json::JsonObject jsonConfig("{\"nameX\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_missingSizeType_throws) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_typeX\" : \"ALL\"}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    ParserIntItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( "ITEM1" , item1.name() );
    BOOST_CHECK_EQUAL( ALL , item1.sizeType() );
    BOOST_CHECK_EQUAL( ParserItem::defaultInt() , item1.getDefault() );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_withDefault) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\", \"default\" : 100}");
    ParserIntItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( 100 , item1.getDefault() );
}



BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_withDefaultInvalid_throws) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\", \"default\" : \"100X\"}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}

/* </Json> */
/******************************************************************/


BOOST_AUTO_TEST_CASE(Name_ReturnsCorrectName) {
    ParserItemSizeEnum sizeType = ALL;

    ParserIntItem item1("ITEM1", sizeType);
    BOOST_CHECK_EQUAL("ITEM1", item1.name());

    ParserIntItem item2("", sizeType);
    BOOST_CHECK_EQUAL("", item2.name());
}

BOOST_AUTO_TEST_CASE(Size_ReturnsCorrectSizeType) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem item1("ITEM1", sizeType);
    BOOST_CHECK_EQUAL(sizeType, item1.sizeType());
}

BOOST_AUTO_TEST_CASE(Scan_WrongSizeType_ExceptionThrown) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    BOOST_CHECK_THROW(itemInt.scan(rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_All_CorrectIntSetInDeckItem) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemInt("ITEM", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 10* 10*1 25/"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(23U, deckIntItem->size());
    BOOST_CHECK_EQUAL(1, deckIntItem->getInt(21));
    BOOST_CHECK_EQUAL(25, deckIntItem->getInt(22));
}

BOOST_AUTO_TEST_CASE(Scan_ScalarMultipleItems_ExceptionThrown) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    BOOST_CHECK_THROW(itemInt.scan(2, rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_NoData_OK) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(0, rawRecord);
    BOOST_CHECK_EQUAL(0U, deckIntItem->size());
}

BOOST_AUTO_TEST_CASE(Scan_SINGLE_CorrectIntSetInDeckItem) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 44.3 'Heisann' /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(100, deckIntItem->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_SeveralInts_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 338932 222.33 'Heisann' /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(3, rawRecord);
    BOOST_CHECK_EQUAL(100, deckIntItem->getInt(0));
    BOOST_CHECK_EQUAL(443, deckIntItem->getInt(1));
    BOOST_CHECK_EQUAL(338932, deckIntItem->getInt(2));
}

BOOST_AUTO_TEST_CASE(Scan_Default_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    int defaultValue = 199;
    ParserIntItem itemInt("ITEM2", sizeType, defaultValue);

    RawRecordPtr rawRecord1(new RawRecord("* /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(1, rawRecord1);
    BOOST_CHECK_EQUAL(1U, deckIntItem->size());
    BOOST_CHECK_EQUAL(defaultValue, deckIntItem->getInt(0));


    RawRecordPtr rawRecord2(new RawRecord("20* /"));
    deckIntItem = itemInt.scan(20, rawRecord2);
    BOOST_CHECK_EQUAL(defaultValue, deckIntItem->getInt(0));
    BOOST_CHECK_EQUAL(20U, deckIntItem->size());
    BOOST_CHECK_EQUAL(defaultValue, deckIntItem->getInt(19));
    BOOST_CHECK_EQUAL(defaultValue, deckIntItem->getInt(9));
}

BOOST_AUTO_TEST_CASE(Scan_Multiplier_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("3*4 /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(3, rawRecord);
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(0));
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(1));
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(2));
}

BOOST_AUTO_TEST_CASE(Scan_StarNoMultiplier_ExceptionThrown) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("*45 /"));
    BOOST_CHECK_THROW(itemInt.scan(1, rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MultipleItems_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType);
    ParserIntItem itemInt2("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("10 20 /"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(1, rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(1, rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MultipleDefault_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);
    ParserIntItem itemInt2("ITEM2", sizeType, 20);

    RawRecordPtr rawRecord(new RawRecord("* * /"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(1, rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(1, rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MultipleWithMultiplier_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);
    ParserIntItem itemInt2("ITEM2", sizeType, 20);

    RawRecordPtr rawRecord(new RawRecord("2*30/"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(1, rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(1, rawRecord);

    BOOST_CHECK_EQUAL(30, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(30, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MalformedMultiplier_Throw) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);

    RawRecordPtr rawRecord(new RawRecord("2.10*30/"));
    BOOST_CHECK_THROW(itemInt1.scan(1, rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MalformedMultiplierChar_Throw) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);

    RawRecordPtr rawRecord(new RawRecord("210X30/"));
    BOOST_CHECK_THROW(itemInt1.scan(1, rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MultipleWithMultiplierDefault_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);
    ParserIntItem itemInt2("ITEM2", sizeType, 20);

    RawRecordPtr rawRecord(new RawRecord("2*/"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(1, rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(1, rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MultipleWithMultiplierDefault2_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt1("ITEM1", sizeType, 10);
    ParserIntItem itemInt2("ITEM2", sizeType, 20);

    RawRecordPtr rawRecord(new RawRecord("15*  5*77/")); // * * * * * * * * * * ^ * * * * * 77 77 77 77 77 
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(10, rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(10, rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(9));

    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
    BOOST_CHECK_EQUAL(77, deckIntItem2->getInt(9));
}

BOOST_AUTO_TEST_CASE(Scan_RawRecordErrorInRawData_ExceptionThrown) {
    ParserItemSizeEnum sizeType = BOX;
    ParserIntItem itemInt("ITEM2", sizeType);

    // Too few elements
    RawRecordPtr rawRecord1(new RawRecord("100 443 /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord1), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord2(new RawRecord("100 443 333.2 /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord2), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord3(new RawRecord("100X 443 3332 /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord3), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord4(new RawRecord("100U 443 3332 /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord4), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord5(new RawRecord("galneslig 443 3332 /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord5), std::invalid_argument);

    // Too few elements
    RawRecordPtr rawRecord6(new RawRecord("2*2 2*1 /"));
    BOOST_CHECK_THROW(itemInt.scan(5, rawRecord6), std::invalid_argument);

    // Too few elements
    RawRecordPtr rawRecord7(new RawRecord("2* /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord7), std::invalid_argument);

    // Too few elements
    RawRecordPtr rawRecord8(new RawRecord("* /"));
    BOOST_CHECK_THROW(itemInt.scan(3, rawRecord8), std::invalid_argument);
}

/*********************String************************'*/
BOOST_AUTO_TEST_CASE(scan_boxWithoutExpected_ExceptionThrown) {
    ParserItemSizeEnum sizeType = BOX;
    ParserStringItem itemString("ITEM1", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 'WELL2' /"));
    BOOST_CHECK_THROW(itemString.scan(rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(scan_singleWithMoreThenOneExpected_ExceptionThrown) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserStringItem itemString("ITEM1", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 'WELL2' /"));
    BOOST_CHECK_THROW(itemString.scan(2, rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(init_defaultvalue_defaultset) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserStringItem itemString("ITEM1", sizeType);
    RawRecordPtr rawRecord(new RawRecord(("'1*'/")));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(itemString.defaultString(), deckItem->getString(0));

    rawRecord.reset(new RawRecord("13*/"));
    deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(itemString.defaultString(), deckItem->getString(0));

    rawRecord.reset(new RawRecord(("*/")));
    deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(itemString.defaultString(), deckItem->getString(0));

    ParserStringItem itemStringDefaultChanged("ITEM2", sizeType, "SPECIAL");
    rawRecord.reset(new RawRecord(("*/")));
    deckItem = itemStringDefaultChanged.scan(rawRecord);
    BOOST_CHECK_EQUAL("SPECIAL", deckItem->getString(0));
}

BOOST_AUTO_TEST_CASE(scan_all_valuesCorrect) {
    ParserItemSizeEnum sizeType = ALL;
    ParserStringItem itemString("ITEMWITHMANY", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' '*' FISK BANAN 3* OPPLEGG_FOR_DATAANALYSE /"));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(8U, deckItem->size());

    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(1));
    BOOST_CHECK_EQUAL("FISK", deckItem->getString(2));
    BOOST_CHECK_EQUAL("BANAN", deckItem->getString(3));
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(4));
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(5));
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(6));
    BOOST_CHECK_EQUAL("OPPLEGG_FOR_DATAANALYSE", deckItem->getString(7));
}

BOOST_AUTO_TEST_CASE(scan_givenNumber_valuesCorrect) {
    ParserItemSizeEnum sizeType = BOX;
    ParserStringItem itemString("ITEMWITHMANY", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' '*' FISK BANAN 3* OPPLEGG_FOR_DATAANALYSE /"));
    DeckItemConstPtr deckItem = itemString.scan(3, rawRecord);
    BOOST_CHECK_EQUAL(3U, deckItem->size());

    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(1));
    BOOST_CHECK_EQUAL("FISK", deckItem->getString(2));
}

BOOST_AUTO_TEST_CASE(scan_single_dataCorrect) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserStringItem itemString("ITEM1", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 'WELL2' /"));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(1U, deckItem->size());
    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
}

BOOST_AUTO_TEST_CASE(scan_singleWithMixedRecord_dataCorrect) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserStringItem itemString("ITEM1", sizeType);
    ParserStringItem itemInt("ITEM1", sizeType);

    RawRecordPtr rawRecord(new RawRecord("2 'WELL1' /"));
    itemInt.scan(rawRecord);
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
}

/******************String and int**********************/
BOOST_AUTO_TEST_CASE(scan_intsAndStrings_dataCorrect) {
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 2 2 2*3 3*FLASKEHALS /"));

    ParserItemSizeEnum sizeTypeSingle = SINGLE;
    ParserItemSizeEnum sizeTypeItemBoxed = BOX;

    ParserStringItem itemSingleString("ITEM1", sizeTypeSingle);
    DeckItemConstPtr deckItemWell1 = itemSingleString.scan(rawRecord);
    BOOST_CHECK_EQUAL("WELL1", deckItemWell1->getString(0));

    ParserIntItem itemSomeInts("SOMEINTS", sizeTypeItemBoxed);
    DeckItemConstPtr deckItemInts = itemSomeInts.scan(4, rawRecord);
    BOOST_CHECK_EQUAL(2, deckItemInts->getInt(0));
    BOOST_CHECK_EQUAL(2, deckItemInts->getInt(1));
    BOOST_CHECK_EQUAL(3, deckItemInts->getInt(2));
    BOOST_CHECK_EQUAL(3, deckItemInts->getInt(3));

    ParserStringItem itemTripleString("ITEM1", sizeTypeItemBoxed);
    DeckItemConstPtr deckItemTripleFlaskehals = itemTripleString.scan(3, rawRecord);
    BOOST_CHECK_EQUAL("FLASKEHALS", deckItemTripleFlaskehals->getString(0));
    BOOST_CHECK_EQUAL("FLASKEHALS", deckItemTripleFlaskehals->getString(1));
    BOOST_CHECK_EQUAL("FLASKEHALS", deckItemTripleFlaskehals->getString(2));
}
