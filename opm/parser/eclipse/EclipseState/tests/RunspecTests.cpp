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

#define BOOST_TEST_MODULE RunspecTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(PhaseFromString) {
    BOOST_CHECK_THROW( get_phase("XXX") , std::invalid_argument );
    BOOST_CHECK_THROW( get_phase("WATE") , std::invalid_argument );
    BOOST_CHECK_THROW( get_phase("OI") , std::invalid_argument );
    BOOST_CHECK_THROW( get_phase("OILL") , std::invalid_argument );

    BOOST_CHECK_EQUAL( Phase::OIL  , get_phase("OIL") );
    BOOST_CHECK_EQUAL( Phase::WATER, get_phase("WATER") );
    BOOST_CHECK_EQUAL( Phase::WATER, get_phase("WAT") );
    BOOST_CHECK_EQUAL( Phase::GAS  , get_phase("GAS") );
}

BOOST_AUTO_TEST_CASE(TwoPhase) {
    const std::string input = R"(
    RUNSPEC
    OIL
    WATER
    )";

    Parser parser;
    ParseContext parseContext;

    auto deck = parser.parseString(input, parseContext);

    Runspec runspec( deck );
    const auto& phases = runspec.phases();
    BOOST_CHECK_EQUAL( 2, phases.size() );
    BOOST_CHECK(  phases.active( Phase::OIL ) );
    BOOST_CHECK( !phases.active( Phase::GAS ) );
    BOOST_CHECK(  phases.active( Phase::WATER ) );
}

BOOST_AUTO_TEST_CASE(ThreePhase) {
    const std::string input = R"(
    RUNSPEC
    OIL
    GAS
    WATER
    )";

    Parser parser;
    ParseContext parseContext;

    auto deck = parser.parseString(input, parseContext);

    Runspec runspec( deck );
    const auto& phases = runspec.phases();
    BOOST_CHECK_EQUAL( 3, phases.size() );
    BOOST_CHECK( phases.active( Phase::OIL ) );
    BOOST_CHECK( phases.active( Phase::GAS ) );
    BOOST_CHECK( phases.active( Phase::WATER ) );

}


BOOST_AUTO_TEST_CASE(TABDIMS) {
    const std::string input = R"(
    RUNSPEC
    TABDIMS
      1 * 3 * 5 * /
    OIL
    GAS
    WATER
    )";

    Parser parser;
    ParseContext parseContext;

    auto deck = parser.parseString(input, parseContext);

    Runspec runspec( deck );
    const auto& tabdims = runspec.tabdims();
    BOOST_CHECK_EQUAL( tabdims.getNumSatTables( ) , 1 );
    BOOST_CHECK_EQUAL( tabdims.getNumPVTTables( ) , 1 );
    BOOST_CHECK_EQUAL( tabdims.getNumSatNodes( ) , 3 );
    BOOST_CHECK_EQUAL( tabdims.getNumPressureNodes( ) , 20 );
    BOOST_CHECK_EQUAL( tabdims.getNumFIPRegions( ) , 5 );
    BOOST_CHECK_EQUAL( tabdims.getNumRSNodes( ) , 20 );
}

