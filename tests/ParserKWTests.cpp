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

#include "opm/parser/eclipse/ParserKW.hpp"


using namespace Opm;


BOOST_AUTO_TEST_CASE(Initialize) {
  BOOST_REQUIRE_NO_THROW( ParserKW parserKW );
  
  ParserKW parserKW;
  BOOST_REQUIRE_EQUAL( parserKW.getName() , "");
}


BOOST_AUTO_TEST_CASE(NamedInit) {
  std::string keyword("KEYWORD");
  ParserKW parserKW( keyword );
  BOOST_REQUIRE_EQUAL( parserKW.getName( ) , keyword );
}


BOOST_AUTO_TEST_CASE(NameTooLong) {
  std::string keyword("KEYWORDTOOLONG");

  BOOST_REQUIRE_THROW( ParserKW parserKW( keyword ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(MixedCase) {
  std::string keyword("KeyWord");

  BOOST_REQUIRE_THROW( ParserKW parserKW( keyword ) , std::invalid_argument );
}
