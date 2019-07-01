/*
  Copyright 2019 SINTEF Digital, Mathematics and Cybernetics.
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

#define BOOST_TEST_MODULE FoamTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/FoamConfig.hpp>


using namespace Opm;


static Deck createDeck() {
const char *deckData =
"RUNSPEC\n"
"\n"
"DIMENS\n"
" 10 10 10 /\n"
"TABDIMS\n"
"3 /\n"
"GRID\n"
"DX\n"
"1000*0.25 /\n"
"DY\n"
"1000*0.25 /\n"
"DZ\n"
"1000*0.25 /\n"
"TOPS\n"
"100*0.25 /\n"
"FAULTS \n"
"  'F1'  1  1  1  4   1  4  'X' / \n"
"  'F2'  5  5  1  4   1  4  'X-' / \n"
"/\n"
"MULTFLT \n"
"  'F1' 0.50 / \n"
"  'F2' 0.50 / \n"
"/\n"
"EDIT\n"
"MULTFLT /\n"
"  'F2' 0.25 / \n"
"/\n"
"WATER\n"
"\n"
"OIL\n"
"\n"
"GAS\n"
"\n"
"FOAM\n"
"\n"
"TITLE\n"
"The title\n"
"\n"
"START\n"
"8 MAR 1998 /\n"
"\n"
"PROPS\n"
"FOAMFSC\n"
"1 2 0.3 /\n"
"4 5 /\n"
"6 /\n"
"\n"
"REGIONS\n"
"SWAT\n"
"1000*1 /\n"
"SATNUM\n"
"1000*2 /\n"
"\n";

    Parser parser;
    return parser.parseString( deckData );
}


BOOST_AUTO_TEST_CASE(FoamConfigTest) {
    auto deck = createDeck();
    EclipseState state(deck);
    const FoamConfig& fc = state.getInitConfig().getFoamConfig();
    BOOST_REQUIRE_EQUAL(fc.size(), 3);
    BOOST_CHECK_EQUAL(fc.getRecord(0).referenceSurfactantConcentration(), 1.0);
    BOOST_CHECK_EQUAL(fc.getRecord(0).exponent(), 2.0);
    BOOST_CHECK_EQUAL(fc.getRecord(0).minimumSurfactantConcentration(), 0.3);
    BOOST_CHECK_EQUAL(fc.getRecord(1).referenceSurfactantConcentration(), 4.0);
    BOOST_CHECK_EQUAL(fc.getRecord(1).exponent(), 5.0);
    BOOST_CHECK_EQUAL(fc.getRecord(1).minimumSurfactantConcentration(), 1e-20); // Defaulted.
    BOOST_CHECK_EQUAL(fc.getRecord(2).referenceSurfactantConcentration(), 6.0);
    BOOST_CHECK_EQUAL(fc.getRecord(2).exponent(), 1.0); // Defaulted.
    BOOST_CHECK_EQUAL(fc.getRecord(2).minimumSurfactantConcentration(), 1e-20); // Defaulted.
}

