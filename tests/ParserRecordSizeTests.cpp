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

#include "opm/parser/eclipse/Parser/ParserRecordSize.hpp"


using namespace Opm;


BOOST_AUTO_TEST_CASE(Initialize) {
  BOOST_REQUIRE_NO_THROW( ParserRecordSize recordSize );
}


BOOST_AUTO_TEST_CASE(DynamicSize) {
  ParserRecordSize recordSize;
  BOOST_REQUIRE_THROW( recordSize.recordSize() , std::logic_error );
}


BOOST_AUTO_TEST_CASE(FixedSize) {
  BOOST_REQUIRE_NO_THROW(  ParserRecordSize recordSize(100) );
  ParserRecordSize recordSize(100);
  
  BOOST_REQUIRE_EQUAL( recordSize.recordSize() , (size_t) 100 );
}
