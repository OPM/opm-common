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


#define BOOST_TEST_MODULE DeckKWTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/DeckKW.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
    DeckKW deckKW1("KW");
    DeckKWPtr deckKW2(new DeckKW("KW"));
    DeckKWConstPtr deckKW3(new DeckKW("KW"));
}
BOOST_AUTO_TEST_CASE(name_nameSetInConstructor_nameReturned) {
    DeckKWPtr deckKW(new DeckKW("KW"));
    BOOST_CHECK_EQUAL("KW", deckKW->name());
}

BOOST_AUTO_TEST_CASE(size_noRecords_returnszero) {
    DeckKWPtr deckKW(new DeckKW("KW"));
    BOOST_CHECK_EQUAL(0U, deckKW->size());
}


BOOST_AUTO_TEST_CASE(addRecord_onerecord_recordadded) {
    DeckKWPtr deckKW(new DeckKW("KW"));
    deckKW->addRecord(DeckRecordConstPtr(new DeckRecord()));
    BOOST_CHECK_EQUAL(1U, deckKW->size());
}

BOOST_AUTO_TEST_CASE(getRecord_onerecord_recordretured) {
    DeckKWPtr deckKW(new DeckKW("KW"));
    DeckRecordConstPtr deckRecord(new DeckRecord());
    deckKW->addRecord(deckRecord);
    BOOST_CHECK_EQUAL(deckRecord, deckKW->getRecord(0));
}


BOOST_AUTO_TEST_CASE(getRecord_outofrange_exceptionthrown) {
    DeckKWPtr deckKW(new DeckKW("KW"));
    DeckRecordConstPtr deckRecord(new DeckRecord());
    deckKW->addRecord(deckRecord);
    BOOST_CHECK_THROW(deckKW->getRecord(1), std::range_error);
}









