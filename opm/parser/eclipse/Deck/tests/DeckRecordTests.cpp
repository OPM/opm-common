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
    intItem->push_back(3);
    deckRecord.addItem(intItem);
    BOOST_CHECK_EQUAL(1U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_multipleItems_sizecorrect) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    DeckIntItemPtr intItem2(new DeckIntItem("TEST"));
    DeckIntItemPtr intItem3(new DeckIntItem("TEST"));

    intItem1->push_back(3);
    intItem2->push_back(3);
    intItem3->push_back(3);

    deckRecord.addItem(intItem1);
    deckRecord.addItem(intItem2);
    deckRecord.addItem(intItem3);

    BOOST_CHECK_EQUAL(3U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_sameItemTimes_throws) {
    DeckRecord deckRecord;
    DeckIntItemPtr intItem1(new DeckIntItem("TEST"));
    intItem1->push_back(3);

    deckRecord.addItem(intItem1);
    BOOST_CHECK_THROW(deckRecord.addItem(intItem1), std::invalid_argument);
}
