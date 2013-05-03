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

#include "opm/parser/eclipse/Parser/ItemSize.hpp"
#include "opm/parser/eclipse/Parser/ParserEnums.hpp"


using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize) {
  BOOST_REQUIRE_NO_THROW( ItemSize itemSize );
}



BOOST_AUTO_TEST_CASE(Default) {
  ItemSize itemSize;
  BOOST_REQUIRE_EQUAL( itemSize.sizeType() , UNSPECIFIED );
  BOOST_REQUIRE_THROW( itemSize.sizeValue() , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(Fixed) {
  ItemSize itemSize(100);
  BOOST_REQUIRE_EQUAL( itemSize.sizeType() , ITEM_FIXED );
  BOOST_REQUIRE_EQUAL( itemSize.sizeValue() , 100U );
}



BOOST_AUTO_TEST_CASE(Fixed2) {
  ItemSize itemSize(ITEM_FIXED , 100U);
  BOOST_REQUIRE_EQUAL( itemSize.sizeType() , ITEM_FIXED );
  BOOST_REQUIRE_EQUAL( itemSize.sizeValue() , 100U );
}



BOOST_AUTO_TEST_CASE(Box) {
  ItemSize itemSize(ITEM_BOX);
  BOOST_REQUIRE_EQUAL( itemSize.sizeType() , ITEM_BOX );
  BOOST_REQUIRE_THROW( itemSize.sizeValue() , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(Boost) {
  ItemSizeConstPtr itemSize( new ItemSize( ITEM_BOX ));
  BOOST_REQUIRE_EQUAL( itemSize->sizeType() , ITEM_BOX );
  BOOST_REQUIRE_THROW( itemSize->sizeValue() , std::invalid_argument );
}
