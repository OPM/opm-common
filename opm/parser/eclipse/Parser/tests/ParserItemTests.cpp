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


#define BOOST_TEST_MODULE ParserItemTests
#include <boost/test/unit_test.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

#include <cmath>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    ParserItemSizeEnum sizeType = SINGLE;
    BOOST_CHECK_NO_THROW(ParserIntItem item1("ITEM1", sizeType));
    BOOST_CHECK_NO_THROW(ParserStringItem item1("ITEM1", sizeType));
    BOOST_CHECK_NO_THROW(ParserDoubleItem item1("ITEM1", sizeType));
    BOOST_CHECK_NO_THROW(ParserFloatItem item1("ITEM1", sizeType));
}

BOOST_AUTO_TEST_CASE(ScalarCheck) {
    ParserIntItem item1("ITEM1", SINGLE);
    ParserIntItem item2("ITEM1", ALL);

    BOOST_CHECK( item1.scalar());
    BOOST_CHECK( !item2.scalar());
}

BOOST_AUTO_TEST_CASE(Initialize_DefaultSizeType) {
    ParserIntItem item1(std::string("ITEM1"));
    ParserStringItem item2(std::string("ITEM1"));
    ParserDoubleItem item3(std::string("ITEM1"));
    ParserFloatItem item4(std::string("ITEM1"));

    BOOST_CHECK_EQUAL( SINGLE , item1.sizeType());
    BOOST_CHECK_EQUAL( SINGLE , item2.sizeType());
    BOOST_CHECK_EQUAL( SINGLE , item3.sizeType());
    BOOST_CHECK_EQUAL( SINGLE , item4.sizeType());
}



BOOST_AUTO_TEST_CASE(Initialize_Default) {
    ParserIntItem item1(std::string("ITEM1"));
    ParserIntItem item2(std::string("ITEM1"), 88);
    BOOST_CHECK(item1.getDefault() < 0);
    BOOST_CHECK_EQUAL(item2.getDefault(), 88);
}


BOOST_AUTO_TEST_CASE(Initialize_Default_Double) {
    ParserDoubleItem item1(std::string("ITEM1"));
    ParserDoubleItem item2("ITEM1",  88.91);
    BOOST_CHECK(!std::isfinite(item1.getDefault()));
    BOOST_CHECK_EQUAL( 88.91 , item2.getDefault());
}

BOOST_AUTO_TEST_CASE(Initialize_Default_Float) {
    ParserFloatItem item1(std::string("ITEM1"));
    ParserFloatItem item2("ITEM1",  88.91F);
    BOOST_CHECK(!std::isfinite(item1.getDefault()));
    BOOST_CHECK_EQUAL( 88.91F , item2.getDefault());
}


BOOST_AUTO_TEST_CASE(Initialize_Default_String) {
    ParserStringItem item1(std::string("ITEM1"));
    BOOST_CHECK(item1.getDefault() == "");

    ParserStringItem item2("ITEM1",  "String");
    BOOST_CHECK_EQUAL( "String" , item2.getDefault());
}

BOOST_AUTO_TEST_CASE(scan_PreMatureTerminator_defaultUsed) {
    ParserIntItem itemInt(std::string("ITEM2"));

    RawRecordPtr rawRecord1(new RawRecord("/"));
    DeckItemConstPtr defaulted = itemInt.scan(rawRecord1);
    // an item is always present even if the record was ended. If the deck specified no
    // data and the item does not have a meaningful default, the item gets assigned a NaN
    // (for float and double items), -1 (for integer items) and "" (for string items)
    // whit the defaultApplied(0) method returning true...
    BOOST_CHECK(defaulted->defaultApplied(0));
    BOOST_CHECK(defaulted->getInt(0) < 0);
}    

BOOST_AUTO_TEST_CASE(InitializeIntItem_setDescription_canReadBack) {
    ParserIntItem itemInt(std::string("ITEM1"));
    std::string description("This is the description");
    itemInt.setDescription(description);

    BOOST_CHECK_EQUAL( description, itemInt.getDescription() );
}


/******************************************************************/
/* <Json> */
BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_missingName_throws) {
    Json::JsonObject jsonConfig("{\"nameX\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_defaultSizeType) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" }");
    ParserIntItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( SINGLE , item1.sizeType());
}



BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    ParserIntItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( "ITEM1" , item1.name() );
    BOOST_CHECK_EQUAL( ALL , item1.sizeType() );
    BOOST_CHECK(item1.getDefault() < 0);
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_withDefault) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"SINGLE\", \"default\" : 100}");
    ParserIntItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( 100 , item1.getDefault() );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_withDefaultInvalid_throws) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"SINGLE\", \"default\" : \"100X\"}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_FromJsonObject_withSizeTypeALL_throws) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\", \"default\" : 100}");
    BOOST_CHECK_THROW( ParserIntItem item1( jsonConfig ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(InitializeIntItem_WithDescription_DescriptionPropertyShouldBePopulated) {
    std::string description("Description goes here");
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"description\" : \"Description goes here\"}");
    ParserIntItem item(jsonConfig);

    BOOST_CHECK_EQUAL( "Description goes here", item.getDescription() );
}


BOOST_AUTO_TEST_CASE(InitializeIntItem_WithoutDescription_DescriptionPropertyShouldBeEmpty) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\"}");
    ParserIntItem item(jsonConfig);

    BOOST_CHECK_EQUAL( "", item.getDescription() );
}



/* </Json> */
/******************************************************************/

/* EQUAL */


BOOST_AUTO_TEST_CASE(IntItem_Equal_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem item1("ITEM1", sizeType);
    ParserIntItem item2("ITEM1", sizeType);
    ParserIntItem item3 = item1;

    BOOST_CHECK( item1.equal( item2 ));
    BOOST_CHECK( item1.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(IntItem_Different_ReturnsFalse) {
    ParserIntItem item1("ITEM1", ALL);
    ParserIntItem item2("ITEM2", ALL);
    ParserIntItem item3(std::string("ITEM1"));
    ParserIntItem item4("ITEM1" , 42);

    BOOST_CHECK( !item1.equal( item2 ));
    BOOST_CHECK( !item1.equal( item3 ));
    BOOST_CHECK( !item2.equal( item3 ));
    BOOST_CHECK( !item4.equal( item3 ));
}

BOOST_AUTO_TEST_CASE(DoubleItem_Equal_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserDoubleItem item1("ITEM1", sizeType);
    ParserDoubleItem item2("ITEM1", sizeType);
    ParserDoubleItem item3 = item1;

    BOOST_CHECK( item1.equal( item2 ));
    BOOST_CHECK( item1.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(FloatItem_Equal_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserFloatItem item1("ITEM1", sizeType);
    ParserFloatItem item2("ITEM1", sizeType);
    ParserFloatItem item3 = item1;

    BOOST_CHECK( item1.equal( item2 ));
    BOOST_CHECK( item1.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(DoubleItem_DimEqual_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserDoubleItem item1("ITEM1", sizeType);
    ParserDoubleItem item2("ITEM1", sizeType);

    item1.push_backDimension("Length*Length");
    item2.push_backDimension("Length*Length");
    
    BOOST_CHECK( item1.equal( item2 ));
}


BOOST_AUTO_TEST_CASE(FloatItem_DimEqual_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserFloatItem item1("ITEM1", sizeType);
    ParserFloatItem item2("ITEM1", sizeType);

    item1.push_backDimension("Length*Length");
    item2.push_backDimension("Length*Length");

    BOOST_CHECK( item1.equal( item2 ));
}


BOOST_AUTO_TEST_CASE(DoubleItem_DimDifferent_ReturnsFalse) {
    ParserItemSizeEnum sizeType = ALL;
    ParserDoubleItem item1("ITEM1", sizeType);    // Dim: []
    ParserDoubleItem item2("ITEM1", sizeType);    // Dim: [Length]
    ParserDoubleItem item3("ITEM1", sizeType);    // Dim: [Length ,Length]
    ParserDoubleItem item4("ITEM1", sizeType);    // Dim: [t]

    item2.push_backDimension("Length");

    item3.push_backDimension("Length");
    item3.push_backDimension("Length");
    
    item4.push_backDimension("Time");

    BOOST_CHECK_EQUAL(false , item1.equal( item2 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item3 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item1 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item4 ));
    BOOST_CHECK_EQUAL(false , item1.equal( item3 ));
    BOOST_CHECK_EQUAL(false , item3.equal( item1 ));
    BOOST_CHECK_EQUAL(false , item4.equal( item2 ));
}


BOOST_AUTO_TEST_CASE(FloatItem_DimDifferent_ReturnsFalse) {
    ParserItemSizeEnum sizeType = ALL;
    ParserFloatItem item1("ITEM1", sizeType);    // Dim: []
    ParserFloatItem item2("ITEM1", sizeType);    // Dim: [Length]
    ParserFloatItem item3("ITEM1", sizeType);    // Dim: [Length ,Length]
    ParserFloatItem item4("ITEM1", sizeType);    // Dim: [t]

    item2.push_backDimension("Length");

    item3.push_backDimension("Length");
    item3.push_backDimension("Length");

    item4.push_backDimension("Time");

    BOOST_CHECK_EQUAL(false , item1.equal( item2 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item3 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item1 ));
    BOOST_CHECK_EQUAL(false , item2.equal( item4 ));
    BOOST_CHECK_EQUAL(false , item1.equal( item3 ));
    BOOST_CHECK_EQUAL(false , item3.equal( item1 ));
    BOOST_CHECK_EQUAL(false , item4.equal( item2 ));

}


BOOST_AUTO_TEST_CASE(DoubleItem_Different_ReturnsFalse) {
    ParserDoubleItem item1("ITEM1", ALL);
    ParserDoubleItem item2("ITEM2", ALL);
    ParserDoubleItem item3(std::string("ITEM1") );
    ParserDoubleItem item4("ITEM1" , 42.89);

    BOOST_CHECK( !item1.equal( item2 ));
    BOOST_CHECK( !item1.equal( item3 ));
    BOOST_CHECK( !item2.equal( item3 ));
    BOOST_CHECK( !item4.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(FloatItem_Different_ReturnsFalse) {
    ParserFloatItem item1("ITEM1", ALL);
    ParserFloatItem item2("ITEM2", ALL);
    ParserFloatItem item3(std::string("ITEM1") );
    ParserFloatItem item4("ITEM1" , 42.89);

    BOOST_CHECK( !item1.equal( item2 ));
    BOOST_CHECK( !item1.equal( item3 ));
    BOOST_CHECK( !item2.equal( item3 ));
    BOOST_CHECK( !item4.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(StringItem_Equal_ReturnsTrue) {
    ParserItemSizeEnum sizeType = ALL;
    ParserStringItem item1("ITEM1", sizeType);
    ParserStringItem item2("ITEM1", sizeType);
    ParserStringItem item3 = item1;

    BOOST_CHECK( item1.equal( item2 ));
    BOOST_CHECK( item1.equal( item3 ));
}


BOOST_AUTO_TEST_CASE(StringItem_Different_ReturnsFalse) {
    ParserStringItem item1("ITEM1", ALL);
    ParserStringItem item2("ITEM2", ALL);
    ParserStringItem item3(std::string("ITEM1") );
    ParserStringItem item4("ITEM1"  , "42.89");

    BOOST_CHECK( !item1.equal( item2 ));
    BOOST_CHECK( !item1.equal( item3 ));
    BOOST_CHECK( !item2.equal( item3 ));
    BOOST_CHECK( !item4.equal( item3 ));
}




/******************************************************************/

BOOST_AUTO_TEST_CASE(Name_ReturnsCorrectName) {
    ParserItemSizeEnum sizeType = ALL;

    ParserIntItem item1("ITEM1", sizeType);
    BOOST_CHECK_EQUAL("ITEM1", item1.name());

    ParserIntItem item2("", sizeType);
    BOOST_CHECK_EQUAL("", item2.name());
}

BOOST_AUTO_TEST_CASE(Size_ReturnsCorrectSizeTypeSingle) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItem item1("ITEM1", sizeType);
    BOOST_CHECK_EQUAL(sizeType, item1.sizeType());
}

BOOST_AUTO_TEST_CASE(Size_ReturnsCorrectSizeTypeAll) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem item1("ITEM1", sizeType);
    BOOST_CHECK_EQUAL(sizeType, item1.sizeType());
}

BOOST_AUTO_TEST_CASE(Scan_All_CorrectIntSetInDeckItem) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemInt("ITEM", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 443 10*77 10*1 25/"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(23U, deckIntItem->size());
    BOOST_CHECK_EQUAL(77, deckIntItem->getInt(3));
    BOOST_CHECK_EQUAL(1, deckIntItem->getInt(21));
    BOOST_CHECK_EQUAL(25, deckIntItem->getInt(22));
}

BOOST_AUTO_TEST_CASE(Scan_All_WithDefaults) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemInt("ITEM", sizeType);

    RawRecordPtr rawRecord(new RawRecord("100 10* 10*1 25/"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(22U, deckIntItem->size());
    BOOST_CHECK(!deckIntItem->defaultApplied(0));
    BOOST_CHECK(deckIntItem->defaultApplied(1));
    BOOST_CHECK(!deckIntItem->defaultApplied(11));
    BOOST_CHECK(!deckIntItem->defaultApplied(21));
    BOOST_CHECK_EQUAL(1, deckIntItem->getInt(20));
    BOOST_CHECK_EQUAL(25, deckIntItem->getInt(21));
}

BOOST_AUTO_TEST_CASE(Scan_SINGLE_CorrectIntSetInDeckItem) {
    ParserIntItem itemInt(std::string("ITEM2"));

    RawRecordPtr rawRecord(new RawRecord("100 44.3 'Heisann' /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(100, deckIntItem->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_SeveralInts_CorrectIntsSetInDeckItem) {
    ParserIntItem itemInt1(std::string("ITEM1"));
    ParserIntItem itemInt2(std::string("ITEM2"));
    ParserIntItem itemInt3(std::string("ITEM3"));

    RawRecordPtr rawRecord(new RawRecord("100 443 338932 222.33 'Heisann' /"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(rawRecord);
    BOOST_CHECK_EQUAL(100, deckIntItem1->getInt(0));
        
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(rawRecord);
    BOOST_CHECK_EQUAL(443, deckIntItem2->getInt(0));

    DeckItemConstPtr deckIntItem3 = itemInt3.scan(rawRecord);
    BOOST_CHECK_EQUAL(338932, deckIntItem3->getInt(0));
}





BOOST_AUTO_TEST_CASE(Scan_Multiplier_CorrectIntsSetInDeckItem) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemInt("ITEM2", sizeType);

    RawRecordPtr rawRecord(new RawRecord("3*4 /"));
    DeckItemConstPtr deckIntItem = itemInt.scan(rawRecord);
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(0));
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(1));
    BOOST_CHECK_EQUAL(4, deckIntItem->getInt(2));
}

BOOST_AUTO_TEST_CASE(Scan_StarNoMultiplier_ExceptionThrown) {
    ParserItemSizeEnum sizeType = SINGLE;
    ParserIntItem itemInt("ITEM2", sizeType , 100);

    RawRecordPtr rawRecord(new RawRecord("*45 /"));
    BOOST_CHECK_THROW(itemInt.scan(rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MultipleItems_CorrectIntsSetInDeckItem) {
    ParserIntItem itemInt1(std::string("ITEM1"));
    ParserIntItem itemInt2(std::string("ITEM2"));

    RawRecordPtr rawRecord(new RawRecord("10 20 /"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MultipleDefault_CorrectIntsSetInDeckItem) {
    ParserIntItem itemInt1("ITEM1", 10);
    ParserIntItem itemInt2("ITEM2", 20);

    RawRecordPtr rawRecord(new RawRecord("* * /"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MultipleWithMultiplier_CorrectIntsSetInDeckItem) {
    ParserIntItem itemInt1("ITEM1", 10);
    ParserIntItem itemInt2("ITEM2", 20);

    RawRecordPtr rawRecord(new RawRecord("2*30/"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(rawRecord);

    BOOST_CHECK_EQUAL(30, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(30, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_MalformedMultiplier_Throw) {
    ParserIntItem itemInt1("ITEM1" , 10);

    RawRecordPtr rawRecord(new RawRecord("2.10*30/"));
    BOOST_CHECK_THROW(itemInt1.scan(rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MalformedMultiplierChar_Throw) {
    ParserIntItem itemInt1("ITEM1", 10);

    RawRecordPtr rawRecord(new RawRecord("210X30/"));
    BOOST_CHECK_THROW(itemInt1.scan(rawRecord), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Scan_MultipleWithMultiplierDefault_CorrectIntsSetInDeckItem) {
    ParserIntItem itemInt1("ITEM1", 10);
    ParserIntItem itemInt2("ITEM2", 20);

    RawRecordPtr rawRecord(new RawRecord("2*/"));
    DeckItemConstPtr deckIntItem1 = itemInt1.scan(rawRecord);
    DeckItemConstPtr deckIntItem2 = itemInt2.scan(rawRecord);

    BOOST_CHECK_EQUAL(10, deckIntItem1->getInt(0));
    BOOST_CHECK_EQUAL(20, deckIntItem2->getInt(0));
}

BOOST_AUTO_TEST_CASE(Scan_RawRecordErrorInRawData_ExceptionThrown) {
    ParserIntItem itemInt(std::string("ITEM2"));

    // Wrong type
    RawRecordPtr rawRecord2(new RawRecord("333.2 /"));
    BOOST_CHECK_THROW(itemInt.scan(rawRecord2), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord3(new RawRecord("100X /"));
    BOOST_CHECK_THROW(itemInt.scan(rawRecord3), std::invalid_argument);

    // Wrong type
    RawRecordPtr rawRecord5(new RawRecord("astring /"));
    BOOST_CHECK_THROW(itemInt.scan(rawRecord5), std::invalid_argument);
}

/*********************String************************'*/
/*****************************************************************/
/*</json>*/

BOOST_AUTO_TEST_CASE(InitializeStringItem_FromJsonObject_missingName_throws) {
    Json::JsonObject jsonConfig("{\"nameX\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    BOOST_CHECK_THROW( ParserStringItem item1( jsonConfig ) , std::invalid_argument );
}




BOOST_AUTO_TEST_CASE(InitializeStringItem_FromJsonObject) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\"}");
    ParserStringItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( "ITEM1" , item1.name() );
    BOOST_CHECK_EQUAL( ALL , item1.sizeType() );
    BOOST_CHECK(item1.getDefault() == "");
}


BOOST_AUTO_TEST_CASE(InitializeStringItem_FromJsonObject_withDefault) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"SINGLE\", \"default\" : \"100\"}");
    ParserStringItem item1( jsonConfig );
    BOOST_CHECK_EQUAL( "100" , item1.getDefault() );
}



BOOST_AUTO_TEST_CASE(InitializeStringItem_FromJsonObject_withDefaultInvalid_throws) {
    Json::JsonObject jsonConfig("{\"name\": \"ITEM1\" , \"size_type\" : \"ALL\", \"default\" : [1,2,3]}");
    BOOST_CHECK_THROW( ParserStringItem item1( jsonConfig ) , std::invalid_argument );
}
/*</json>*/
/*****************************************************************/

BOOST_AUTO_TEST_CASE(init_defaultvalue_defaultset) {
    ParserStringItem itemString(std::string("ITEM1") , "DEFAULT");
    RawRecordPtr rawRecord(new RawRecord(("'1*'/")));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL("1*", deckItem->getString(0));

    rawRecord.reset(new RawRecord("13*/"));
    deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL("DEFAULT" , deckItem->getString(0));

    rawRecord.reset(new RawRecord(("*/")));
    deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL("DEFAULT", deckItem->getString(0));

    ParserStringItem itemStringDefaultChanged("ITEM2", "SPECIAL");
    rawRecord.reset(new RawRecord(("*/")));
    deckItem = itemStringDefaultChanged.scan(rawRecord);
    BOOST_CHECK_EQUAL("SPECIAL", deckItem->getString(0));
}

BOOST_AUTO_TEST_CASE(scan_all_valuesCorrect) {
    ParserItemSizeEnum sizeType = ALL;
    ParserStringItem itemString("ITEMWITHMANY", sizeType);
    RawRecordPtr rawRecord(new RawRecord("'WELL1' FISK BANAN 3*X OPPLEGG_FOR_DATAANALYSE 'Foo$*!% BAR' /"));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(8U, deckItem->size());

    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
    BOOST_CHECK_EQUAL("FISK", deckItem->getString(1));
    BOOST_CHECK_EQUAL("BANAN", deckItem->getString(2));
    BOOST_CHECK_EQUAL("X", deckItem->getString(3));
    BOOST_CHECK_EQUAL("X", deckItem->getString(4));
    BOOST_CHECK_EQUAL("X", deckItem->getString(5));
    BOOST_CHECK_EQUAL("OPPLEGG_FOR_DATAANALYSE", deckItem->getString(6));
    BOOST_CHECK_EQUAL("Foo$*!% BAR", deckItem->getString(7));
}

BOOST_AUTO_TEST_CASE(scan_all_withdefaults) {
    ParserItemSizeEnum sizeType = ALL;
    ParserIntItem itemString("ITEMWITHMANY", sizeType);
    RawRecordPtr rawRecord(new RawRecord("10*1 10* 10*2 /"));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);

    BOOST_CHECK_EQUAL(30U, deckItem->size());

    BOOST_CHECK_EQUAL(false, deckItem->defaultApplied(0));
    BOOST_CHECK_EQUAL(false, deckItem->defaultApplied(9));
    BOOST_CHECK_EQUAL(true, deckItem->defaultApplied(10));
    BOOST_CHECK_EQUAL(true, deckItem->defaultApplied(19));
    BOOST_CHECK_EQUAL(false, deckItem->defaultApplied(20));
    BOOST_CHECK_EQUAL(false, deckItem->defaultApplied(29));

    BOOST_CHECK_THROW(deckItem->getInt(30), std::out_of_range);
    BOOST_CHECK_THROW(deckItem->defaultApplied(30), std::out_of_range);

    BOOST_CHECK_EQUAL(1, deckItem->getInt(0));
    BOOST_CHECK_EQUAL(1, deckItem->getInt(9));
    BOOST_CHECK_EQUAL(2, deckItem->getInt(20));
    BOOST_CHECK_EQUAL(2, deckItem->getInt(29));
}

BOOST_AUTO_TEST_CASE(scan_single_dataCorrect) {
    ParserStringItem itemString(std::string("ITEM1"));
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 'WELL2' /"));
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL(1U, deckItem->size());
    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
}

BOOST_AUTO_TEST_CASE(scan_singleWithMixedRecord_dataCorrect) {
    ParserStringItem itemString(std::string("ITEM1"));
    ParserStringItem itemInt(std::string("ITEM1"));

    RawRecordPtr rawRecord(new RawRecord("2 'WELL1' /"));
    itemInt.scan(rawRecord);
    DeckItemConstPtr deckItem = itemString.scan(rawRecord);
    BOOST_CHECK_EQUAL("WELL1", deckItem->getString(0));
}

/******************String and int**********************/
BOOST_AUTO_TEST_CASE(scan_intsAndStrings_dataCorrect) {
    RawRecordPtr rawRecord(new RawRecord("'WELL1' 2 2 2*3 /"));

    ParserItemSizeEnum sizeTypeItemBoxed = ALL;

    ParserStringItem itemSingleString(std::string("ITEM1"));
    DeckItemConstPtr deckItemWell1 = itemSingleString.scan(rawRecord);
    BOOST_CHECK_EQUAL("WELL1", deckItemWell1->getString(0));

    ParserIntItem itemSomeInts("SOMEINTS", sizeTypeItemBoxed);
    DeckItemConstPtr deckItemInts = itemSomeInts.scan(rawRecord);
    BOOST_CHECK_EQUAL(2, deckItemInts->getInt(0));
    BOOST_CHECK_EQUAL(2, deckItemInts->getInt(1));
    BOOST_CHECK_EQUAL(3, deckItemInts->getInt(2));
    BOOST_CHECK_EQUAL(3, deckItemInts->getInt(3));
}




BOOST_AUTO_TEST_CASE(ParserItemCheckEqualsOverride) {
    ParserItemConstPtr itemDefault10( new ParserIntItem("ITEM" ,  10) );
    ParserItemConstPtr itemDefault20( new ParserIntItem("ITEM" ,  20) );
    
    BOOST_CHECK( itemDefault10->equal( *itemDefault10 ));
    BOOST_CHECK_EQUAL( false , itemDefault10->equal( *itemDefault20 ));
}

/*****************************************************************/


BOOST_AUTO_TEST_CASE(ParserDefaultHasDimensionReturnsFalse) {
    ParserIntItem intItem(std::string("SOMEINTS"));
    ParserStringItem stringItem(std::string("SOMESTRING"));
    ParserDoubleItem doubleItem(std::string("SOMEDOUBLE"));
    ParserFloatItem floatItem(std::string("SOMEFLOAT"));

    BOOST_CHECK_EQUAL( false, intItem.hasDimension());
    BOOST_CHECK_EQUAL( false, stringItem.hasDimension());
    BOOST_CHECK_EQUAL( false, doubleItem.hasDimension());
    BOOST_CHECK_EQUAL( false, floatItem.hasDimension());
}

BOOST_AUTO_TEST_CASE(ParserIntItemGetDimensionThrows) {
    ParserIntItem intItem(std::string("SOMEINT"));

    BOOST_CHECK_THROW( intItem.getDimension(0) , std::invalid_argument );
    BOOST_CHECK_THROW( intItem.push_backDimension("Length") , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(ParserDoubleItemAddMultipleDimensionToSIngleSizeThrows) {
    ParserDoubleItem doubleItem(std::string("SOMEDOUBLE"));
    
    doubleItem.push_backDimension("Length*Length");
    BOOST_CHECK_THROW( doubleItem.push_backDimension("Length*Length"), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(ParserFloatItemAddMultipleDimensionToSIngleSizeThrows) {
    ParserFloatItem floatItem(std::string("SOMEFLOAT"));

    floatItem.push_backDimension("Length*Length");
    BOOST_CHECK_THROW( floatItem.push_backDimension("Length*Length"), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(ParserDoubleItemWithDimensionHasReturnsCorrect) {
    ParserDoubleItem doubleItem(std::string("SOMEDOUBLE"));

    BOOST_CHECK_EQUAL( false , doubleItem.hasDimension() );
    doubleItem.push_backDimension("Length*Length");
    BOOST_CHECK_EQUAL( true , doubleItem.hasDimension() );
}


BOOST_AUTO_TEST_CASE(ParserFloatItemWithDimensionHasReturnsCorrect) {
    ParserFloatItem floatItem(std::string("SOMEFLOAT"));

    BOOST_CHECK_EQUAL( false , floatItem.hasDimension() );
    floatItem.push_backDimension("Length*Length");
    BOOST_CHECK_EQUAL( true , floatItem.hasDimension() );
}


BOOST_AUTO_TEST_CASE(ParserDoubleItemGetDimension) {
    ParserDoubleItem doubleItem(std::string("SOMEDOUBLE") , ALL);

    BOOST_CHECK_THROW( doubleItem.getDimension( 10 ) , std::invalid_argument );
    BOOST_CHECK_THROW( doubleItem.getDimension(  0 ) , std::invalid_argument );

    doubleItem.push_backDimension("Length");
    doubleItem.push_backDimension("Length*Length");
    doubleItem.push_backDimension("Length*Length*Length");

    BOOST_CHECK_EQUAL( "Length" , doubleItem.getDimension(0));
    BOOST_CHECK_EQUAL( "Length*Length" , doubleItem.getDimension(1));
    BOOST_CHECK_EQUAL( "Length*Length*Length" , doubleItem.getDimension(2));
    BOOST_CHECK_THROW( doubleItem.getDimension( 3 ) , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(ParserFloatItemGetDimension) {
    ParserFloatItem floatItem(std::string("SOMEFLOAT") , ALL);

    BOOST_CHECK_THROW( floatItem.getDimension( 10 ) , std::invalid_argument );
    BOOST_CHECK_THROW( floatItem.getDimension(  0 ) , std::invalid_argument );

    floatItem.push_backDimension("Length");
    floatItem.push_backDimension("Length*Length");
    floatItem.push_backDimension("Length*Length*Length");

    BOOST_CHECK_EQUAL( "Length" , floatItem.getDimension(0));
    BOOST_CHECK_EQUAL( "Length*Length" , floatItem.getDimension(1));
    BOOST_CHECK_EQUAL( "Length*Length*Length" , floatItem.getDimension(2));
    BOOST_CHECK_THROW( floatItem.getDimension( 3 ) , std::invalid_argument );
}

