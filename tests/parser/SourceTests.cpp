/*
 Copyright (C) 2023 Equinor
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

#define BOOST_TEST_MODULE SourceTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Schedule/SourceProp.hpp>

#include <string>

namespace {

Opm::Deck createDeck(const std::string& input)
{
    Opm::Parser parser;
    auto deck = parser.parseString(input);
    return deck;
}

}

BOOST_AUTO_TEST_CASE(Source)
{
    const std::string input = R"(
RUNSPEC

DIMENS
  10 10 3 /
OIL
GAS
WATER
START
  1 'JAN' 2015 /
GRID
DX
  300*1000 /
DY
  300*1000 /
DZ
  300*1000 /
TOPS
  100*8325 /

SCHEDULE

SOURCE
 1 1 1 GAS -0.01 /
 1 1 1 WATER -0.01 /
/

DATES             -- 1
 10  'JUN'  2007 /
/

SOURCE
 1 1 1 GAS -0.0 /
 1 1 2 WATER -0.02 /
/

)";

    auto deck = createDeck(input);
    const auto& kw = deck.get<Opm::ParserKeywords::SOURCE>();
    Opm::SourceProp prop;
    for (const auto& record : kw[0]) {
        prop.updateSourceProp(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 2U);
    const auto& c1 = *prop.begin();
    BOOST_CHECK_EQUAL(c1.i, 0);
    BOOST_CHECK_EQUAL(c1.j, 0);
    BOOST_CHECK_EQUAL(c1.k, 0);
    BOOST_CHECK(c1.component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c1.rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", -0.01));

    double rate2 = prop.rate({{0,0,0},Opm::SourceComponent::WATER});
    BOOST_CHECK_EQUAL(rate2,
                      deck.getActiveUnitSystem().to_si("Mass/Time", -0.01));


    for (const auto& record : kw[1]) {
        prop.updateSourceProp(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 3U);
    const auto& c21 = *prop.begin();
    BOOST_CHECK_EQUAL(c21.i, 0);
    BOOST_CHECK_EQUAL(c21.j, 0);
    BOOST_CHECK_EQUAL(c21.k, 0);
    BOOST_CHECK(c21.component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c21.rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", -0.00));

    double rate22 = prop.rate({{0,0,0},Opm::SourceComponent::WATER});
    BOOST_CHECK_EQUAL(rate22,
                      deck.getActiveUnitSystem().to_si("Mass/Time", -0.01));

    double rate23 = prop.rate({{0,0,1},Opm::SourceComponent::WATER});
    BOOST_CHECK_EQUAL(rate23,
                      deck.getActiveUnitSystem().to_si("Mass/Time", -0.02));
    
}
