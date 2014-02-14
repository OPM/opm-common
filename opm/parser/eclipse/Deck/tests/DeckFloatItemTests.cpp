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
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckFloatItem.hpp>

#include <opm/parser/eclipse/Units/Dimension.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(InitializeFloat) {
    BOOST_REQUIRE_NO_THROW(DeckFloatItem deckFloatItem("TEST"));
}

BOOST_AUTO_TEST_CASE(GetFloatAtIndex_NoData_ExceptionThrown) {
    const DeckFloatItem deckFloatItem("TEST");
    BOOST_CHECK_THROW(deckFloatItem.getRawFloat(0), std::out_of_range);
}


BOOST_AUTO_TEST_CASE(PushBackFloat_VectorPushed_ElementsCorrect) {
    DeckFloatItem deckFloatItem("TEST");
    std::deque<float> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    deckFloatItem.push_back(pushThese);
    BOOST_CHECK_EQUAL(13, deckFloatItem.getRawFloat(0));
    BOOST_CHECK_EQUAL(33, deckFloatItem.getRawFloat(1));
}


BOOST_AUTO_TEST_CASE(PushBackFloat_subVectorPushed_ElementsCorrect) {
    DeckFloatItem deckFloatItem("TEST");
    std::deque<float> pushThese;
    pushThese.push_back(13);
    pushThese.push_back(33);
    pushThese.push_back(47);
    deckFloatItem.push_back(pushThese , 2);
    BOOST_CHECK_EQUAL(13 , deckFloatItem.getRawFloat(0));
    BOOST_CHECK_EQUAL(33 , deckFloatItem.getRawFloat(1));
    BOOST_CHECK_EQUAL( 2U , deckFloatItem.size());
}



BOOST_AUTO_TEST_CASE(sizeFloat_correct) {
    DeckFloatItem deckFloatItem("TEST");

    BOOST_CHECK_EQUAL( 0U , deckFloatItem.size());
    deckFloatItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 1U , deckFloatItem.size());

    deckFloatItem.push_back( 100 );
    deckFloatItem.push_back( 100 );
    BOOST_CHECK_EQUAL( 3U , deckFloatItem.size());
}



BOOST_AUTO_TEST_CASE(DefaultApplied) {
    DeckFloatItem deckFloatItem("TEST");
    BOOST_CHECK_EQUAL( false , deckFloatItem.defaultApplied() );
    deckFloatItem.push_backDefault( 1 );
    BOOST_CHECK_EQUAL( true , deckFloatItem.defaultApplied() );
}



BOOST_AUTO_TEST_CASE(PushBackMultiple) {
    DeckFloatItem item("HEI");
    item.push_backMultiple(10.22 , 100 );
    BOOST_CHECK_EQUAL( 100U , item.size() );
    for (size_t i=0; i < 100; i++)
        BOOST_CHECK_EQUAL(10.22F , item.getRawFloat(i));
}



BOOST_AUTO_TEST_CASE(PushBackDimension) {
    DeckFloatItem item("HEI");
    std::shared_ptr<Dimension> activeDimension(new Dimension("Length" , 100));
    std::shared_ptr<Dimension> defaultDimension(new Dimension("Length" , 10));

    item.push_backDimension( activeDimension , defaultDimension);
}

BOOST_AUTO_TEST_CASE(PushBackDimensionInvalidType) {
    DeckIntItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 100));
    BOOST_CHECK_THROW( item.push_backDimension( dim , dim ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(GetSIWithoutDimensionThrows) {
    DeckFloatItem item("HEI");
    item.push_backMultiple(10.22 , 100 );

    BOOST_CHECK_THROW( item.getSIFloat(0) , std::invalid_argument );
    BOOST_CHECK_THROW( item.getSIFloatData( ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(GetSISingleDimensionCorrect) {
    DeckFloatItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 100));

    item.push_backMultiple(1 , 100 );
    item.push_backDimension( dim , dim );

    BOOST_CHECK_EQUAL( 1   , item.getRawFloat(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIFloat(0) );
}


BOOST_AUTO_TEST_CASE(GetSISingleDefault) {
    DeckFloatItem item("HEI");
    std::shared_ptr<Dimension> dim(new Dimension("Length" , 1));
    std::shared_ptr<Dimension> defaultDim(new Dimension("Length" , 100));

    item.push_backDefault(1 );
    item.push_backDimension( dim , defaultDim );

    BOOST_CHECK_EQUAL( 1   , item.getRawFloat(0) );
    BOOST_CHECK_EQUAL( 100 , item.getSIFloat(0) );
}


BOOST_AUTO_TEST_CASE(GetSIMultipleDim) {
    DeckFloatItem item("HEI");
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
        BOOST_CHECK_EQUAL( 2   , item.getSIFloat(i) );
        BOOST_CHECK_EQUAL( 4   , item.getSIFloat(i+ 1) );
        BOOST_CHECK_EQUAL( 8   , item.getSIFloat(i+2) );
        BOOST_CHECK_EQUAL(16   , item.getSIFloat(i+3) );
    }
}




