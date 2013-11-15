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


#define BOOST_TEST_MODULE ParserEnumTests
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(TestCompletionStateEnum2String) {
    BOOST_CHECK_EQUAL( "AUTO" , CompletionStateEnum2String(AUTO));
    BOOST_CHECK_EQUAL( "OPEN" , CompletionStateEnum2String(OPEN));
    BOOST_CHECK_EQUAL( "SHUT" , CompletionStateEnum2String(SHUT));
}


BOOST_AUTO_TEST_CASE(TestCompletionStateEnumFromString) {
    BOOST_CHECK_THROW( CompletionStateEnumFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( AUTO , CompletionStateEnumFromString("AUTO"));
    BOOST_CHECK_EQUAL( SHUT , CompletionStateEnumFromString("SHUT"));
    BOOST_CHECK_EQUAL( OPEN , CompletionStateEnumFromString("OPEN"));
}



BOOST_AUTO_TEST_CASE(TestCompletionStateEnumLoop) {
    BOOST_CHECK_EQUAL( AUTO    , CompletionStateEnumFromString( CompletionStateEnum2String( AUTO ) ));
    BOOST_CHECK_EQUAL( SHUT , CompletionStateEnumFromString( CompletionStateEnum2String( SHUT ) ));
    BOOST_CHECK_EQUAL( OPEN , CompletionStateEnumFromString( CompletionStateEnum2String( OPEN ) ));

    BOOST_CHECK_EQUAL( "AUTO"    , CompletionStateEnum2String(CompletionStateEnumFromString(  "AUTO" ) ));
    BOOST_CHECK_EQUAL( "OPEN" , CompletionStateEnum2String(CompletionStateEnumFromString(  "OPEN" ) ));
    BOOST_CHECK_EQUAL( "SHUT" , CompletionStateEnum2String(CompletionStateEnumFromString(  "SHUT" ) ));
}

/*****************************************************************/


BOOST_AUTO_TEST_CASE(TestPhaseEnum2String) {
    BOOST_CHECK_EQUAL( "OIL" , PhaseEnum2String(OIL));
    BOOST_CHECK_EQUAL( "GAS" , PhaseEnum2String(GAS));
    BOOST_CHECK_EQUAL( "WATER" , PhaseEnum2String(WATER));
}


BOOST_AUTO_TEST_CASE(TestPhaseEnumFromString) {
    BOOST_CHECK_THROW( PhaseEnumFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( OIL , PhaseEnumFromString("OIL"));
    BOOST_CHECK_EQUAL( WATER , PhaseEnumFromString("WATER"));
    BOOST_CHECK_EQUAL( GAS , PhaseEnumFromString("GAS"));
}



BOOST_AUTO_TEST_CASE(TestPhaseEnumLoop) {
    BOOST_CHECK_EQUAL( OIL    , PhaseEnumFromString( PhaseEnum2String( OIL ) ));
    BOOST_CHECK_EQUAL( WATER , PhaseEnumFromString( PhaseEnum2String( WATER ) ));
    BOOST_CHECK_EQUAL( GAS , PhaseEnumFromString( PhaseEnum2String( GAS ) ));

    BOOST_CHECK_EQUAL( "OIL"    , PhaseEnum2String(PhaseEnumFromString(  "OIL" ) ));
    BOOST_CHECK_EQUAL( "GAS" , PhaseEnum2String(PhaseEnumFromString(  "GAS" ) ));
    BOOST_CHECK_EQUAL( "WATER" , PhaseEnum2String(PhaseEnumFromString(  "WATER" ) ));
}



BOOST_AUTO_TEST_CASE(TestPhaseEnumMask) {
    BOOST_CHECK_EQUAL( 0 , OIL & GAS );
    BOOST_CHECK_EQUAL( 0 , OIL & WATER );
    BOOST_CHECK_EQUAL( 0 , WATER & GAS );
}

