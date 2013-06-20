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


#define BOOST_TEST_MODULE DeckItemTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(InitializeDouble) {
    BOOST_REQUIRE_NO_THROW(DeckDoubleItem deckDoubleItem("TEST"));
}

BOOST_AUTO_TEST_CASE(GetDoubleAtIndex_NoData_ExceptionThrown) {
    const DeckDoubleItem deckDoubleItem("TEST");
    BOOST_CHECK_THROW(deckDoubleItem.getDouble(0), std::out_of_range);
}



BOOST_AUTO_TEST_CASE(PushBackDouble_VectorPushed_ElementsCorrect) {
    DeckDoubleItem deckDoubleItem("TEST");
    std::vector<double> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    deckDoubleItem.push_back(pushThese);
    BOOST_CHECK_EQUAL(13, deckDoubleItem.getDouble(0));
    BOOST_CHECK_EQUAL(33, deckDoubleItem.getDouble(1));
}


BOOST_AUTO_TEST_CASE(PushBackDouble_subVectorPushed_ElementsCorrect) {
    DeckDoubleItem deckDoubleItem("TEST");
    std::vector<double> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    pushThese.push_back(47);
    deckDoubleItem.push_back(pushThese , 2);
    BOOST_CHECK_EQUAL(13 , deckDoubleItem.getDouble(0));
    BOOST_CHECK_EQUAL(33 , deckDoubleItem.getDouble(1));
    BOOST_CHECK_EQUAL( 2U , deckDoubleItem.size());
}



BOOST_AUTO_TEST_CASE(sizeDouble_correct) {
    DeckDoubleItem deckDoubleItem("TEST");
    
    BOOST_CHECK_EQUAL( 0U , deckDoubleItem.size());
    deckDoubleItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 1U , deckDoubleItem.size());
    
    deckDoubleItem.push_back( 100 );
    deckDoubleItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 3U , deckDoubleItem.size());
}



