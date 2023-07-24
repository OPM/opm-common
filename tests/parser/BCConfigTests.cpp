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

#define BOOST_TEST_MODULE BCConfigTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/BCConfig.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Schedule/BCProp.hpp>

#include <string>

namespace {

Opm::Deck createDeck(const std::string& input)
{
    Opm::Parser parser;
    auto deck = parser.parseString(input);
    return deck;
}

}

BOOST_AUTO_TEST_CASE(GasRateFree)
{
    const std::string input = R"(
RUNSPEC

DIMENS
  10 10 3 /
OIL
GAS
WATER
LAB
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
BCCON
  1 1 1 2* 5 10 X- /
  2 20 20 4* X /
/
SCHEDULE
BCPROP
 1 RATE GAS -0.01 /
 2 FREE /
/
)";

    auto deck = createDeck(input);
    Opm::BCConfig config(deck);
    const auto& kw = deck.get<Opm::ParserKeywords::BCPROP>();
    Opm::BCProp prop;
    for (const auto& record : kw.back()) {
        prop.updateBCProp(record);
    }

    BOOST_CHECK_EQUAL(config.size(), 2U);
    const auto& c1 = *config.begin();
    BOOST_CHECK_EQUAL(c1.index, 1);
    BOOST_CHECK_EQUAL(c1.i1, 0);
    BOOST_CHECK_EQUAL(c1.i2, 0);
    BOOST_CHECK_EQUAL(c1.j1, 0);
    BOOST_CHECK_EQUAL(c1.j2, 9);
    BOOST_CHECK_EQUAL(c1.k1, 4);
    BOOST_CHECK_EQUAL(c1.k2, 9);
    BOOST_CHECK_EQUAL(c1.dir, Opm::FaceDir::XMinus);

    const auto& c2 = *(config.begin()+1);
    BOOST_CHECK_EQUAL(c2.index, 2);
    BOOST_CHECK_EQUAL(c2.i1, 19);
    BOOST_CHECK_EQUAL(c2.i2, 19);
    BOOST_CHECK_EQUAL(c2.j1, 0);
    BOOST_CHECK_EQUAL(c2.j2, 9);
    BOOST_CHECK_EQUAL(c2.k1, 0);
    BOOST_CHECK_EQUAL(c2.k2, 2);
    BOOST_CHECK_EQUAL(c2.dir, Opm::FaceDir::XPlus);

    BOOST_CHECK_EQUAL(prop.size(), 2U);
    BOOST_CHECK(prop[1].bctype == Opm::BCType::RATE);
    BOOST_CHECK(prop[1].component == Opm::BCComponent::GAS);
    BOOST_CHECK_EQUAL(prop[1].rate,
                      deck.getActiveUnitSystem().to_si("Mass/Time*Length*Length", -0.01));

    BOOST_CHECK(prop[2].bctype == Opm::BCType::FREE);
    BOOST_CHECK(prop[2].component == Opm::BCComponent::NONE);
    BOOST_CHECK_EQUAL(prop[2].rate, 0.0);
}

BOOST_AUTO_TEST_CASE(DirichletThermal)
{
    const std::string input = R"(
RUNSPEC

DIMENS
  10 10 3 /
OIL
GAS
WATER
LAB
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
BCCON
  1 1* * 1 5 1 10 Y- /
  2 20 20 4* Y /
/
SCHEDULE
BCPROP
 1 THERMAL WATER -0.05 * 50.0 /
 2 DIRICHLET OIL -0.01 279.0 /
/
)";

    auto deck = createDeck(input);
    Opm::BCConfig config(deck);
    const auto& kw = deck.get<Opm::ParserKeywords::BCPROP>();
    Opm::BCProp prop;
    for (const auto& record : kw.back()) {
        prop.updateBCProp(record);
    }

    BOOST_CHECK_EQUAL(config.size(), 2U);
    const auto& c1 = *config.begin();
    BOOST_CHECK_EQUAL(c1.index, 1);
    BOOST_CHECK_EQUAL(c1.i1, 0);
    BOOST_CHECK_EQUAL(c1.i2, 9);
    BOOST_CHECK_EQUAL(c1.j1, 0);
    BOOST_CHECK_EQUAL(c1.j2, 4);
    BOOST_CHECK_EQUAL(c1.k1, 0);
    BOOST_CHECK_EQUAL(c1.k2, 9);
    BOOST_CHECK_EQUAL(c1.dir, Opm::FaceDir::YMinus);

    const auto& c2 = *(config.begin()+1);
    BOOST_CHECK_EQUAL(c2.index, 2);
    BOOST_CHECK_EQUAL(c2.i1, 19);
    BOOST_CHECK_EQUAL(c2.i2, 19);
    BOOST_CHECK_EQUAL(c2.j1, 0);
    BOOST_CHECK_EQUAL(c2.j2, 9);
    BOOST_CHECK_EQUAL(c2.k1, 0);
    BOOST_CHECK_EQUAL(c2.k2, 2);
    BOOST_CHECK_EQUAL(c2.dir, Opm::FaceDir::YPlus);
    BOOST_CHECK_EQUAL(prop.size(), 2U);

    BOOST_CHECK(prop[1].bctype == Opm::BCType::THERMAL);
    BOOST_CHECK(prop[1].component == Opm::BCComponent::WATER);
    using measure = Opm::UnitSystem::measure;
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si("Mass/Time*Length*Length", -0.05),
                      prop[1].rate);
    BOOST_CHECK(!prop[1].pressure.has_value());
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::temperature, 50.0),
                      *prop[1].temperature);

    BOOST_CHECK(prop[2].bctype == Opm::BCType::DIRICHLET);
    BOOST_CHECK(prop[2].component == Opm::BCComponent::OIL);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si("Mass/Time*Length*Length", -0.01),
                      prop[2].rate);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::pressure, 279.0),
                      *prop[2].pressure);
}

BOOST_AUTO_TEST_CASE(Mech)
{
    const std::string input = R"(
RUNSPEC

DIMENS
  10 10 3 /
OIL
GAS
WATER
LAB
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
BCCON
  1 1* * 1 5 1 10 Y- /
  2 20 20 4* Y /
  3 10 15 1 10 4 7 Z /
/
SCHEDULE
BCPROP
 1 NONE * * * * FIXED 1 0 0 1.0 * * 2.0 * * /
 2 NONE * * * * FIXED 0 1 0 * 3.0 * /
 3 NONE * * * * FREE 0 0 1 * * 4.0 * * 5.0 /
/
)";

    auto deck = createDeck(input);
    Opm::BCConfig config(deck);
    const auto& kw = deck.get<Opm::ParserKeywords::BCPROP>();
    Opm::BCProp prop;
    for (const auto& record : kw.back()) {
        prop.updateBCProp(record);
    }

    BOOST_CHECK_EQUAL(config.size(), 3U);

    BOOST_CHECK_EQUAL(prop.size(), 3U);
    BOOST_CHECK(prop[1].bctype == Opm::BCType::NONE);
    BOOST_CHECK(prop[1].component == Opm::BCComponent::NONE);
    using measure = Opm::UnitSystem::measure;
    BOOST_CHECK_EQUAL(0.0,
                      prop[1].rate);
    BOOST_CHECK(!prop[1].pressure.has_value());
    BOOST_CHECK(!prop[1].temperature.has_value());
    BOOST_CHECK(prop[1].bcmechtype == Opm::BCMECHType::FIXED);
    BOOST_CHECK(prop[1].mechbcvalue.has_value());
    BOOST_CHECK_EQUAL(prop[1].mechbcvalue->fixeddir[0], 1);
    BOOST_CHECK_EQUAL(prop[1].mechbcvalue->fixeddir[1], 0);
    BOOST_CHECK_EQUAL(prop[1].mechbcvalue->fixeddir[2], 0);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::pressure, 1.0),
                      prop[1].mechbcvalue->stress[0]);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->stress[1], 1e-12);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->stress[2], 1e-12);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->stress[3], 1e-12);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->stress[4], 1e-12);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->stress[5], 1e-12);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::length, 2.0),
                      prop[1].mechbcvalue->disp[0]);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->disp[1], 1e-12);
    BOOST_CHECK_SMALL(prop[1].mechbcvalue->disp[2], 1e-12);

    BOOST_CHECK(prop[2].bctype == Opm::BCType::NONE);
    BOOST_CHECK(prop[2].component == Opm::BCComponent::NONE);
    using measure = Opm::UnitSystem::measure;
    BOOST_CHECK_EQUAL(0.0,
                      prop[2].rate);
    BOOST_CHECK(!prop[2].pressure.has_value());
    BOOST_CHECK(!prop[2].temperature.has_value());
    BOOST_CHECK(prop[2].bcmechtype == Opm::BCMECHType::FIXED);
    BOOST_CHECK(prop[2].mechbcvalue.has_value());
    BOOST_CHECK_EQUAL(prop[2].mechbcvalue->fixeddir[0], 0);
    BOOST_CHECK_EQUAL(prop[2].mechbcvalue->fixeddir[1], 1);
    BOOST_CHECK_EQUAL(prop[2].mechbcvalue->fixeddir[2], 0);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->stress[0], 1e-12);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::pressure, 3.0),
                      prop[2].mechbcvalue->stress[1]);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->stress[2], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->stress[3], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->stress[4], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->stress[5], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->disp[0], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->disp[1], 1e-12);
    BOOST_CHECK_SMALL(prop[2].mechbcvalue->disp[2], 1e-12);

    BOOST_CHECK(prop[3].bctype == Opm::BCType::NONE);
    BOOST_CHECK(prop[3].component == Opm::BCComponent::NONE);
    using measure = Opm::UnitSystem::measure;
    BOOST_CHECK_EQUAL(0.0,
                      prop[3].rate);
    BOOST_CHECK(!prop[3].pressure.has_value());
    BOOST_CHECK(!prop[3].temperature.has_value());
    BOOST_CHECK(prop[3].bcmechtype == Opm::BCMECHType::FREE);
    BOOST_CHECK(prop[3].mechbcvalue.has_value());
    BOOST_CHECK_EQUAL(prop[3].mechbcvalue->fixeddir[0], 0);
    BOOST_CHECK_EQUAL(prop[3].mechbcvalue->fixeddir[1], 0);
    BOOST_CHECK_EQUAL(prop[3].mechbcvalue->fixeddir[2], 1);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->stress[0], 1e-12);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->stress[1], 1e-12);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::pressure, 4.0),
                      prop[3].mechbcvalue->stress[2]);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->stress[3], 1e-12);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->stress[4], 1e-12);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->stress[5], 1e-12);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->disp[0], 1e-12);
    BOOST_CHECK_SMALL(prop[3].mechbcvalue->disp[1], 1e-12);
    BOOST_CHECK_EQUAL(deck.getActiveUnitSystem().to_si(measure::length, 5.0),
                      prop[3].mechbcvalue->disp[2]);
}
