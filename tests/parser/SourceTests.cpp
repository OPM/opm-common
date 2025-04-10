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
#include <opm/input/eclipse/Schedule/Source.hpp>

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
 1 1 1 GAS 0.01 /
 1 1 1 WATER 0.01 /
/

DATES             -- 1
 10  'JUN'  2007 /
/

SOURCE
 1 1 1 GAS 0.0 /
 1 1 2 WATER 0.02 /
/

)";

    auto deck = createDeck(input);
    const auto& kw = deck.get<Opm::ParserKeywords::SOURCE>();
    Opm::Source prop;
    for (const auto& record : kw[0]) {
        prop.updateSource(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 1U);  // num cells
    const auto& c1 = prop.begin();
    BOOST_CHECK_EQUAL(c1->second.size(), 2U);  // num entries for one cell
    BOOST_CHECK_EQUAL(c1->first[0], 0);
    BOOST_CHECK_EQUAL(c1->first[1], 0);
    BOOST_CHECK_EQUAL(c1->first[2], 0);
    BOOST_CHECK(c1->second[0].component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c1->second[0].rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    BOOST_CHECK(prop.hasSource({0,0,0}));
    double rate2 = prop.rate({0,0,0}, Opm::SourceComponent::WATER);
    BOOST_CHECK_EQUAL(rate2,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    for (const auto& record : kw[1]) {
        prop.updateSource(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 2U);  // num cells
    const auto& c21 = prop.begin();
    BOOST_CHECK_EQUAL(c1->second.size(), 2U);  // num entries for one cell
    BOOST_CHECK_EQUAL(c21->first[0], 0);
    BOOST_CHECK_EQUAL(c21->first[1], 0);
    BOOST_CHECK_EQUAL(c21->first[2], 0);
    BOOST_CHECK(c21->second[0].component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c21->second[0].rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.00));

    double rate22 = prop.rate({0,0,0}, Opm::SourceComponent::WATER);
    BOOST_CHECK_EQUAL(rate22,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    double rate23 = prop.rate({0,0,1}, Opm::SourceComponent::WATER);
    BOOST_CHECK_EQUAL(rate23,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.02));
}

BOOST_AUTO_TEST_CASE(SourceEnergy)
{
    const std::string input = R"(
RUNSPEC

DIMENS
  10 10 3 /
OIL
GAS
WATER
THERMAL
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
 1 1 1 GAS 0.01 1.0/
/

DATES             -- 1
 10  'JUN'  2007 /
/

SOURCE
 1 1 1 GAS 0.01 1.0/
 1 1 1 WATER 0.02 2.0/
/


DATES             -- 2
 11  'JUN'  2007 /
/

SOURCE
 1 1 1 GAS 0.01 1* 50/
 1 1 1 WATER 0.02 1* 100/
/

)";

    auto deck = createDeck(input);
    const auto& kw = deck.get<Opm::ParserKeywords::SOURCE>();
    Opm::Source prop;
    for (const auto& record : kw[0]) {
        prop.updateSource(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 1U);  // num cells
    const auto& c1 = prop.begin();
    BOOST_CHECK_EQUAL(c1->second.size(), 1U);  // num entries per cell
    BOOST_CHECK(c1->second[0].component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c1->second[0].rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    BOOST_CHECK_EQUAL(c1->second[0].hrate.value(),
                      deck.getActiveUnitSystem().to_si("Energy/Time", 1.0));

    double rate = prop.rate({0,0,0}, Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    auto hrate = prop.hrate({0,0,0}, Opm::SourceComponent::GAS);
    BOOST_CHECK(hrate);
    BOOST_CHECK_EQUAL(hrate.value(),
                      deck.getActiveUnitSystem().to_si("Energy/Time", 1.0));


    for (const auto& record : kw[1]) {
        prop.updateSource(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 1U);  // num cells
    const auto& c21 = prop.begin();
    BOOST_CHECK_EQUAL(c21->second.size(), 2U);  // num entries per cell
    BOOST_CHECK(c21->second[0].component == Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(c21->second[0].rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.01));

    BOOST_CHECK_EQUAL(c21->second[0].hrate.value(),
                      deck.getActiveUnitSystem().to_si("Energy/Time", 1.0));

    double rate21 = prop.rate({0,0,0}, Opm::SourceComponent::GAS);
    BOOST_CHECK_EQUAL(rate21, c21->second[0].rate);
    double rate22 = prop.rate({0,0,0}, Opm::SourceComponent::WATER);
    BOOST_CHECK_EQUAL(rate22,
                      deck.getActiveUnitSystem().to_si("Mass/Time", 0.02));
    auto hrate2 = prop.hrate({0,0,0}, Opm::SourceComponent::WATER);
    BOOST_CHECK(hrate2);
    BOOST_CHECK_EQUAL(hrate2.value(), deck.getActiveUnitSystem().to_si("Energy/Time", 2.0));

    for (const auto& record : kw[2]) {
        prop.updateSource(record);
    }

    BOOST_CHECK_EQUAL(prop.size(), 1U);  // num cells
    BOOST_CHECK(!prop.hrate({0,0,0}, Opm::SourceComponent::GAS));
    BOOST_CHECK(!prop.hrate({0,0,0}, Opm::SourceComponent::WATER));
    auto temp1 = prop.temperature({0,0,0}, Opm::SourceComponent::GAS);
    BOOST_CHECK(temp1);
    BOOST_CHECK_EQUAL(temp1.value(), 50 + 273.15);
    auto temp2 = prop.temperature({0,0,0}, Opm::SourceComponent::WATER);
    BOOST_CHECK(temp2);
    BOOST_CHECK_EQUAL(temp2.value(), 273.15 + 100);
}
