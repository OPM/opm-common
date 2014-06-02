/*
  Copyright 2014 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE EclipseGridTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>

// forward declarations
Opm::DeckKeywordConstPtr createSATNUMKeyword();
Opm::DeckKeywordConstPtr createTABDIMSKeyword();

BOOST_AUTO_TEST_CASE(Empty) {
    Opm::GridProperty<int> gridProperties( 100 , "SATNUM" , 77);
    const std::vector<int>& data = gridProperties.getData();
    BOOST_CHECK_EQUAL( 100U , data.size());
    BOOST_CHECK_EQUAL( 100U , gridProperties.size());
    for (size_t i=0; i < data.size(); i++) {
        BOOST_CHECK_EQUAL( 77 , data[i] );
        BOOST_CHECK_EQUAL( 77 , gridProperties.iget( i ));
    }
}


BOOST_AUTO_TEST_CASE(EmptyDefault) {
    Opm::GridProperty<int> gridProperties( 100 , "SATNUM");
    const std::vector<int>& data = gridProperties.getData();
    BOOST_CHECK_EQUAL( 100U , data.size());
    for (size_t i=0; i < data.size(); i++)
        BOOST_CHECK_EQUAL( 0 , data[i] );
}

Opm::DeckKeywordConstPtr createSATNUMKeyword( ) {
    const char *deckData =
    "SATNUM \n"
    "  0 1 2 3 4 5 6 7 8 9 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData);
    return deck->getKeyword("SATNUM");
}

Opm::DeckKeywordConstPtr createTABDIMSKeyword( ) {
    const char *deckData =
    "TABDIMS\n"
    "  0 1 2 3 4 5 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData);
    return deck->getKeyword("TABDIMS");
}



BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_notData_Throws) {
    Opm::DeckKeywordConstPtr tabdimsKw = createTABDIMSKeyword(); 
    Opm::GridProperty<int> gridProperty( 6 , "TABDIMS" , 100);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( tabdimsKw ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_wrong_size_throws) {
    Opm::DeckKeywordConstPtr satnumKw = createSATNUMKeyword(); 
    Opm::GridProperty<int> gridProperty( 15 , "SATNUM",66);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( satnumKw ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(SetFromDeckKeyword) {
    Opm::DeckKeywordConstPtr satnumKw = createSATNUMKeyword(); 
    Opm::GridProperty<int> gridProperty( 10 , "SATNUM" , 99);
    gridProperty.loadFromDeckKeyword( satnumKw );
    const std::vector<int>& data = gridProperty.getData();
    for (size_t i=0; i < data.size(); i++)
        BOOST_CHECK_EQUAL( i , data[i] );
}
