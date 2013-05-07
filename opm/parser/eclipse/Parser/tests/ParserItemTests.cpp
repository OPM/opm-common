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

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserBoolItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserItemSize.hpp>

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    ParserItemSizeConstPtr itemSize(new ParserItemSize(10));

    BOOST_REQUIRE_NO_THROW(ParserIntItem item1("ITEM1", itemSize));
    BOOST_REQUIRE_NO_THROW(ParserStringItem item1("ITEM1", itemSize));
    BOOST_REQUIRE_NO_THROW(ParserBoolItem item1("ITEM1", itemSize));
    BOOST_REQUIRE_NO_THROW(ParserDoubleItem item1("ITEM1", itemSize));
}

//BOOST_AUTO_TEST_CASE(TestScanInt) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(10));
//    ParserIntItem itemInt("ITEM2", itemSize);
//    
//    std::vector<std::string> singleItem;
//    singleItem.push_back("100");
//    DeckIntItemConstPtr deckIntItem = itemInt.scan(singleItem);
//
//    BOOST_REQUIRE_EQUAL(100, deckIntItem->getInt(0));

//    BOOST_REQUIRE(!itemInt.scanItem("200X", value));
//    BOOST_REQUIRE_EQUAL(value, 100);
//
//    BOOST_REQUIRE(!itemInt.scanItem("", value));
//    BOOST_REQUIRE_EQUAL(value, 100);
//}

//BOOST_AUTO_TEST_CASE(TestScanDouble) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(10));
//    ParserDoubleItem itemDouble("ITEM2", itemSize);
//    double value = 78;
//
//    BOOST_REQUIRE(itemDouble.scanItem("100.25", value));
//    BOOST_REQUIRE_EQUAL(value, 100.25);
//
//    BOOST_REQUIRE(!itemDouble.scanItem("200X", value));
//    BOOST_REQUIRE_EQUAL(value, 100.25);
//
//    BOOST_REQUIRE(!itemDouble.scanItem("", value));
//    BOOST_REQUIRE_EQUAL(value, 100.25);
//}
//
//BOOST_AUTO_TEST_CASE(TestScanString) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(10));
//    ParserStringItem itemString("ITEM2", itemSize);
//    std::string value = "Hei";
//
//    BOOST_REQUIRE(itemString.scanItem("100.25", value));
//    BOOST_REQUIRE_EQUAL(value, "100.25");
//
//    BOOST_REQUIRE(!itemString.scanItem("", value));
//    BOOST_REQUIRE_EQUAL(value, "100.25");
//}
//
//BOOST_AUTO_TEST_CASE(TestScanBool) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(10));
//    ParserBoolItem itemString("ITEM2", itemSize);
//    bool value = true;
//
//    BOOST_REQUIRE(itemString.scanItem("1", value));
//    BOOST_REQUIRE_EQUAL(value, true);
//
//    BOOST_REQUIRE(itemString.scanItem("0", value));
//    BOOST_REQUIRE_EQUAL(value, false);
//
//    BOOST_REQUIRE(!itemString.scanItem("", value));
//    BOOST_REQUIRE_EQUAL(value, false);
//
//    BOOST_REQUIRE(!itemString.scanItem("10", value));
//    BOOST_REQUIRE_EQUAL(value, false);
//}
//
///*****************************************************************/
//
//
//BOOST_AUTO_TEST_CASE(TestScanItemsFail) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(ITEM_BOX));
//    ParserIntItem itemInt("ITEM2", itemSize);
//    std::vector<int> values;
//
//    BOOST_REQUIRE_THROW(itemInt.scanItems("100 100 100", values), std::invalid_argument);
//}
//
//BOOST_AUTO_TEST_CASE(TestScanItemsInt) {
//    ParserItemSizeConstPtr itemSize(new ParserItemSize(4));
//    ParserIntItem itemInt("ITEM2", itemSize);
//    std::vector<int> values;
//
//    BOOST_REQUIRE_EQUAL(itemInt.scanItems("1 2 3 4", values), 4);
//    BOOST_REQUIRE_EQUAL(values[0], 1);
//    BOOST_REQUIRE_EQUAL(values[1], 2);
//    BOOST_REQUIRE_EQUAL(values[2], 3);
//    BOOST_REQUIRE_EQUAL(values[3], 4);
//}

