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

#include <opm/parser/eclipse/Units/Dimension.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(InitializeDouble) {
    BOOST_REQUIRE_NO_THROW(DeckDoubleItem deckDoubleItem("TEST"));
}

BOOST_AUTO_TEST_CASE(GetDoubleAtIndex_NoData_ExceptionThrown) {
    DeckDoubleItem deckDoubleItem("TEST");

    BOOST_CHECK_THROW(deckDoubleItem.getRawDouble(0), std::out_of_range);
    deckDoubleItem.push_back(1.89);
    BOOST_CHECK_THROW(deckDoubleItem.getRawDouble(1), std::out_of_range);
}



BOOST_AUTO_TEST_CASE(PushBackDouble_VectorPushed_ElementsCorrect) {
    DeckDoubleItem deckDoubleItem("TEST");
    std::deque<double> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    deckDoubleItem.push_back(pushThese);
    BOOST_CHECK_EQUAL(13, deckDoubleItem.getRawDouble(0));
    BOOST_CHECK_EQUAL(33, deckDoubleItem.getRawDouble(1));
}


BOOST_AUTO_TEST_CASE(PushBackDouble_subVectorPushed_ElementsCorrect) {
    DeckDoubleItem deckDoubleItem("TEST");
    std::deque<double> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    pushThese.push_back(47);
    deckDoubleItem.push_back(pushThese , 2);
    BOOST_CHECK_EQUAL(13 , deckDoubleItem.getRawDouble(0));
    BOOST_CHECK_EQUAL(33 , deckDoubleItem.getRawDouble(1));
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



BOOST_AUTO_TEST_CASE(SetInDeck) {
    DeckDoubleItem deckDoubleItem("TEST");
    BOOST_CHECK_EQUAL( false , deckDoubleItem.wasSetInDeck(0) );
    deckDoubleItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( false , deckDoubleItem.wasSetInDeck(0) );
    deckDoubleItem.push_back( 10 );
    BOOST_CHECK_EQUAL( true , deckDoubleItem.wasSetInDeck(0) );
    deckDoubleItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( true , deckDoubleItem.wasSetInDeck(0) );
}



BOOST_AUTO_TEST_CASE(PushBackMultiple) {
    DeckDoubleItem item("HEI");
    item.push_backMultiple(10.22 , 100 );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL(10.22 , item.getRawDouble(i));
}



BOOST_AUTO_TEST_CASE(PushBackDimension) {
    DeckDoubleItem item("HEI");
    std::shared_ptr<Dimension> activeDimension(new Dimension("Length" , 100));
    std::shared_ptr<Dimension> defaultDimension(new Dimension("Length" , 10));

    item.push_back(1.234);
    item.push_backDimension( activeDimension , defaultDimension);

    item.push_backDefault(5.678);
    item.push_backDimension( activeDimension , defaultDimension);
}

BOOST_AUTO_TEST_CASE(PushBackDimensionInvalidType) {
    DeckIntItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 100));
    BOOST_CHECK_THROW( item.push_backDimension( dim , dim ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(GetSIWithoutDimensionThrows) {
    DeckDoubleItem item("HEI");
    item.push_backMultiple(10.22 , 100 );
    
    BOOST_CHECK_THROW( item.getSIDouble(0) , std::invalid_argument );
    BOOST_CHECK_THROW( item.getSIDoubleData( ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(GetSISingleDimensionCorrect) {
    DeckDoubleItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 100));

    item.push_backMultiple(1 , 100 );
    item.push_backDimension( dim , dim );

    BOOST_CHECK_EQUAL( 1   , item.getRawDouble(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIDouble(0) );
}


BOOST_AUTO_TEST_CASE(GetSISingleDefault) {
    DeckDoubleItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 1));
    std::shared_ptr<Dimension> defaultDim(new Dimension("Length" , 100));

    item.push_backDefault(1 );
    item.push_backDimension( dim , defaultDim );

    BOOST_CHECK_EQUAL( 1   , item.getRawDouble(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIDouble(0) );
}


BOOST_AUTO_TEST_CASE(GetSIMultipleDim) {
    DeckDoubleItem item("HEI");
    std::shared_ptr<Dimension> dim1(new Dimension("Length" , 2));
    std::shared_ptr<Dimension> dim2(new Dimension("Length" , 4));
    std::shared_ptr<Dimension> dim3(new Dimension("Length" , 8));
    std::shared_ptr<Dimension> dim4(new Dimension("Length" ,16));
    std::shared_ptr<Dimension> defaultDim(new Dimension("Length" , 100));

    item.push_backMultiple( 1 , 16 );
    item.push_backDimension( dim1 , defaultDim );
    item.push_backDimension( dim2 , defaultDim );
    item.push_backDimension( dim3 , defaultDim );
    item.push_backDimension( dim4 , defaultDim );

    for (size_t i=0; i < 16; i+= 4) {
        BOOST_CHECK_EQUAL( 2   , item.getSIDouble(i) );        
        BOOST_CHECK_EQUAL( 4   , item.getSIDouble(i+ 1) );        
        BOOST_CHECK_EQUAL( 8   , item.getSIDouble(i+2) );        
        BOOST_CHECK_EQUAL(16   , item.getSIDouble(i+3) );        
    }
}



