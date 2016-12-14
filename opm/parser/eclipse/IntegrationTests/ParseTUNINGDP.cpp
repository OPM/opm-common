/*
  Copyright 2016 Statoil ASA.

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

#include <string>

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE( parse_TUNINGDP ) {
    const std::string input = R"(
TUNINGDP
/

TUNINGDP
    1.0
/


TUNINGDP
    1.0 2.0
/

TUNINGDP
    1.0 2.0 3.0
/

TUNINGDP
    1.0 2.0 3.0 4.0
/

)";
    Parser().parseString( input );
}

