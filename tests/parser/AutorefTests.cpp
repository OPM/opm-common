/*
 Copyright (C) 2025 Equinor
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

#define BOOST_TEST_MODULE AutorefTests

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/AutoRefinement.hpp>
#include <opm/input/eclipse/EclipseState/Grid/AutoRefManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/readKeywordAutoRef.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <filesystem>

using namespace Opm;


BOOST_AUTO_TEST_CASE(ReadAutoref) {
    
    const std::string deck_string = R"(
RUNSPEC

AUTOREF
3 3 1 0. /

DIMENS
 10 10 10 /

GRID

DX
1000*1 /
DY
1000*1 /
DZ
1000*1 /
TOPS
100*1 /

PROPS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);

    const auto& autoref_keyword = deck["AUTOREF"][0];

    Opm::AutoRefManager autoRefManager{};

    Opm::readKeywordAutoRef(autoref_keyword.getRecord(0), autoRefManager);
    const auto autoRef = autoRefManager.getAutoRef();

    BOOST_CHECK_EQUAL( autoRef.NX(), 3);
    BOOST_CHECK_EQUAL( autoRef.NY(), 3);
    BOOST_CHECK_EQUAL( autoRef.NZ(), 1);
    BOOST_CHECK_EQUAL( autoRef.OPTION_TRANS_MULT(), 0.);
}

BOOST_AUTO_TEST_CASE(ThrowEvenRefinementFactor) {

    const std::string deck_string = R"(
RUNSPEC

DIMENS
1 1 1 /

AUTOREF
3 2 1 0. /

GRID

DX
1*1 /
DY
1*1 /
DZ
1*1 /
TOPS
1*0 /

PROPS

SOLUTION

SCHEDULE
)";
    
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);

    const auto& autoref_keyword = deck["AUTOREF"][0];

    Opm::AutoRefManager autoRefManager{};

    BOOST_CHECK_THROW(Opm::readKeywordAutoRef(autoref_keyword.getRecord(0), autoRefManager), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ThrowNonDefaultOptionTransMult) {

    const std::string deck_string = R"(
RUNSPEC

DIMENS
1 1 1 /

AUTOREF
3 5 7 1 /

GRID

DX
1*1 /
DY
1*1 /
DZ
1*1 /
TOPS
1*0 /

PROPS

SOLUTION

SCHEDULE
)";
    
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);

    const auto& autoref_keyword = deck["AUTOREF"][0];

    Opm::AutoRefManager autoRefManager{};

    BOOST_CHECK_THROW(Opm::readKeywordAutoRef(autoref_keyword.getRecord(0), autoRefManager), std::invalid_argument);
}
