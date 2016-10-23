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
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
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
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_EQUAL(1U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_multipleItems_sizecorrect) {

    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    deckRecord.addItem( DeckItem { "TEST2", int() } );
    deckRecord.addItem( DeckItem { "TEST3", int() } );

    BOOST_CHECK_EQUAL(3U, deckRecord.size());
}

BOOST_AUTO_TEST_CASE(addItem_differentItemsSameName_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW( deckRecord.addItem( DeckItem { "TEST", int() } ), std::invalid_argument );
    std::vector< DeckItem > items = { DeckItem { "TEST", int() }, DeckItem { "TEST" , int() } };
    BOOST_CHECK_THROW( DeckRecord( std::move( items ) ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(get_byIndex_returnsItem) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_NO_THROW(deckRecord.getItem(0U));
}

BOOST_AUTO_TEST_CASE(get_indexoutofbounds_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW(deckRecord.getItem(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(get_byName_returnsItem) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    deckRecord.getItem("TEST");
}

BOOST_AUTO_TEST_CASE(get_byNameNonExisting_throws) {
    DeckRecord deckRecord;
    deckRecord.addItem( DeckItem { "TEST", int() } );
    BOOST_CHECK_THROW(deckRecord.getItem("INVALID"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(StringsWithSpaceOK) {
    auto itemString = std::make_shared< ParserStringItem >(std::string("STRINGITEM1"));
    ParserRecord record1;
    RawRecord rawRecord( " ' VALUE ' " );
    ParseContext parseContext;
    MessageContainer msgContainer;
    record1.addItem( itemString );


    const auto deckRecord = record1.parse( parseContext , msgContainer, rawRecord );
    BOOST_CHECK_EQUAL(" VALUE " , deckRecord.getItem(0).get< std::string >(0));
}
