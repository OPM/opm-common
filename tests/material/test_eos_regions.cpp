// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \brief Tests the reservoir EOS regions of the generic compositional fluid
 *        system.
 *
 * A deck with two EOS regions is compared against a deck holding the second
 * region alone. The latter initializes a fluid system with its own static
 * data, so EOS region 2 of the former must give the same results.
 */
#include "config.h"

#define BOOST_TEST_MODULE EosRegions
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace
{

using Scalar = double;
constexpr int numComponents = 3;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
// Enabling water gives the reference its own static component data.
using ReferenceSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, true>;
using EOSType = Opm::CompositionalConfig::EOSType;
using CompVec = std::array<Scalar, numComponents>;

// The records of one EOS region for CO2, C1 and C10.
struct Region
{
    std::string_view eos;
    std::string_view mw;
    std::string_view tcrit;
    std::string_view pcrit;
    std::string_view vcrit;
    std::string_view acf;
    std::string_view bic;
    std::string_view sshift;
    std::string_view omegaa;
    std::string_view omegab;
};

constexpr Region regionOne {"PR",
                            "44.01 16.04 142.28",
                            "304.13 190.56 617.7",
                            "73.77 45.99 21.03",
                            "0.0941 0.0986 0.6098",
                            "0.2239 0.0114 0.4884",
                            "0.10 0.0 0.05",
                            "0.0 0.0 0.0",
                            "0.4570 0.4575 0.4580",
                            "0.0777 0.0778 0.0779"};

// Differs from region one in everything but the molecular weights.
constexpr Region regionTwo {"SRK",
                            "44.01 16.04 142.28",
                            "300.0 195.0 600.0",
                            "75.0 46.5 22.0",
                            "0.0950 0.0990 0.6200",
                            "0.2300 0.0120 0.4900",
                            "0.20 0.15 0.10",
                            "-0.05 0.02 0.10",
                            "0.4270 0.4275 0.4280",
                            "0.0865 0.0866 0.0867"};

// The keywords taking one record per EOS region, and the record of each.
constexpr std::array<std::pair<std::string_view, std::string_view Region::*>, 10> regionKeywords {{
    {"EOS", &Region::eos},
    {"MW", &Region::mw},
    {"TCRIT", &Region::tcrit},
    {"PCRIT", &Region::pcrit},
    {"VCRIT", &Region::vcrit},
    {"ACF", &Region::acf},
    {"BIC", &Region::bic},
    {"SSHIFT", &Region::sshift},
    {"OMEGAA", &Region::omegaa},
    {"OMEGAB", &Region::omegab},
}};

std::string
deckString(const std::vector<Region>& regions)
{
    auto deck = fmt::format(R"(
RUNSPEC
METRIC
TABDIMS
 8* {} /
OIL
GAS
DIMENS
 1 1 1 /
COMPS
 3 /
GRID
DX
 10 /
DY
 10 /
DZ
 10 /
TOPS
 1000 /
PORO
 0.2 /
PERMX
 100 /
PERMY
 100 /
PERMZ
 100 /
PROPS
CNAMES
 CO2
 C1
 C10
/
)",
                            regions.size());
    for (const auto& [keyword, records] : regionKeywords) {
        deck += fmt::format("{}\n", keyword);
        for (const auto& region : regions) {
            deck += fmt::format(" {} /\n", region.*records);
        }
    }
    return deck;
}

template <class System>
void
initFromDeck(const std::vector<Region>& regions)
{
    const auto deck = Opm::Parser {}.parseString(deckString(regions));
    const Opm::EclipseState eclState(deck);
    const Opm::Schedule schedule(deck, eclState, std::make_shared<Opm::Python>());
    System::initFromState(eclState, schedule);
}

struct Fixture
{
    Fixture()
    {
        initFromDeck<FluidSystem>({regionOne, regionTwo});
        initFromDeck<ReferenceSystem>({regionTwo});
    }
};

/// Gas density and fugacity coefficients of one composition at 150 bar and 350 K.
template <class System>
std::pair<Scalar, CompVec>
gasProperties(const unsigned eosRegionIdx)
{
    Opm::CompositionalFluidState<Scalar, System> fs;
    fs.setTemperature(350.0);
    fs.setPressure(System::oilPhaseIdx, 150.0e5);
    fs.setPressure(System::gasPhaseIdx, 150.0e5);
    const CompVec y {0.3, 0.6, 0.1};
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(System::oilPhaseIdx, c, y[c]);
        fs.setMoleFraction(System::gasPhaseIdx, c, y[c]);
    }

    typename System::template ParameterCache<Scalar> paramCache(eosRegionIdx);
    paramCache.updatePhase(fs, System::gasPhaseIdx);

    CompVec phi {};
    for (int c = 0; c < numComponents; ++c) {
        phi[c] = System::fugacityCoefficient(fs, paramCache, System::gasPhaseIdx, c);
    }
    return {System::density(fs, paramCache, System::gasPhaseIdx), phi};
}

struct FlashResult
{
    Scalar L {};
    CompVec x {};
    CompVec y {};
};

/// Flash of one feed at 60 bar and 320 K from Wilson estimates.
template <class System>
FlashResult
flash(const unsigned eosRegionIdx)
{
    Opm::CompositionalFluidState<Scalar, System> fs;
    fs.setTemperature(320.0);
    fs.setPressure(System::oilPhaseIdx, 60.0e5);
    fs.setPressure(System::gasPhaseIdx, 60.0e5);
    const CompVec z {0.2, 0.4, 0.4};
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(c, z[c]);
        fs.setKvalue(c, fs.wilsonK_(c, eosRegionIdx));
    }
    fs.setLvalue(-1.0);

    Opm::PTFlash<Scalar, System>::flash_solve_scalar(fs, Opm::PTFlashMethod::SsiNewton, 1.0e-10,
                                                     eosRegionIdx);

    FlashResult result {fs.L(), {}, {}};
    for (int c = 0; c < numComponents; ++c) {
        result.x[c] = fs.moleFraction(System::oilPhaseIdx, c);
        result.y[c] = fs.moleFraction(System::gasPhaseIdx, c);
    }
    return result;
}

} // Anonymous namespace

BOOST_FIXTURE_TEST_CASE(CountsTheRegionsOfTheDeck, Fixture)
{
    BOOST_CHECK_EQUAL(FluidSystem::numEosRegions(), 2u);
    BOOST_CHECK_EQUAL(ReferenceSystem::numEosRegions(), 1u);
}

BOOST_FIXTURE_TEST_CASE(EachRegionHasItsEquationOfState, Fixture)
{
    BOOST_CHECK(FluidSystem::eosType(0) == EOSType::PR);
    BOOST_CHECK(FluidSystem::eosType(1) == EOSType::SRK);
    BOOST_CHECK(ReferenceSystem::eosType(0) == EOSType::SRK);

    // The OMEGAA and OMEGAB defaults of a deck belong to its equations of state.
    BOOST_CHECK_THROW(FluidSystem::setEosType(EOSType::RK), std::logic_error);
    BOOST_CHECK_THROW(ReferenceSystem::setEosType(EOSType::RK), std::logic_error);
    BOOST_CHECK(FluidSystem::eosType(0) == EOSType::PR);
    BOOST_CHECK(ReferenceSystem::eosType(0) == EOSType::SRK);
}

BOOST_FIXTURE_TEST_CASE(RegionMatchesADeckOfThatRegionAlone, Fixture)
{
    for (unsigned c = 0; c < numComponents; ++c) {
        BOOST_TEST_CONTEXT("component " << c)
        {
            BOOST_CHECK_EQUAL(FluidSystem::criticalTemperature(c, 1),
                              ReferenceSystem::criticalTemperature(c, 0));
            BOOST_CHECK_EQUAL(FluidSystem::criticalPressure(c, 1),
                              ReferenceSystem::criticalPressure(c, 0));
            BOOST_CHECK_EQUAL(FluidSystem::criticalVolume(c, 1),
                              ReferenceSystem::criticalVolume(c, 0));
            BOOST_CHECK_EQUAL(FluidSystem::acentricFactor(c, 1),
                              ReferenceSystem::acentricFactor(c, 0));
            BOOST_CHECK_EQUAL(FluidSystem::volumeShift(c, 1), ReferenceSystem::volumeShift(c, 0));
            BOOST_CHECK_EQUAL(FluidSystem::omegaA(c, 1).value(),
                              ReferenceSystem::omegaA(c, 0).value());
            BOOST_CHECK_EQUAL(FluidSystem::omegaB(c, 1).value(),
                              ReferenceSystem::omegaB(c, 0).value());
            for (unsigned other = 0; other < numComponents; ++other) {
                BOOST_CHECK_EQUAL(FluidSystem::interactionCoefficient(c, other, 1),
                                  ReferenceSystem::interactionCoefficient(c, other, 0));
            }
        }
    }

    const auto [density, phi] = gasProperties<FluidSystem>(1);
    const auto [refDensity, refPhi] = gasProperties<ReferenceSystem>(0);
    BOOST_CHECK_CLOSE(density, refDensity, 1.0e-10);
    for (int c = 0; c < numComponents; ++c) {
        BOOST_CHECK_CLOSE(phi[c], refPhi[c], 1.0e-10);
    }
}

BOOST_FIXTURE_TEST_CASE(RegionsKeepTheirOwnProperties, Fixture)
{
    // Region one differs from region two in every critical temperature.
    for (unsigned c = 0; c < numComponents; ++c) {
        BOOST_CHECK_NE(FluidSystem::criticalTemperature(c, 0),
                       FluidSystem::criticalTemperature(c, 1));
    }
    BOOST_CHECK_EQUAL(FluidSystem::interactionCoefficient(0, 1, 0), 0.10);
    BOOST_CHECK_EQUAL(FluidSystem::volumeShift(0, 0), 0.0);
    BOOST_CHECK_EQUAL(FluidSystem::omegaA(0, 0).value(), 0.4570);
    BOOST_CHECK_EQUAL(FluidSystem::omegaB(0, 0).value(), 0.0777);

    const auto densityOne = gasProperties<FluidSystem>(0).first;
    const auto densityTwo = gasProperties<FluidSystem>(1).first;
    BOOST_CHECK_GT(std::abs(densityOne - densityTwo), 1.0);
}

BOOST_FIXTURE_TEST_CASE(CacheKeepsItsRegion, Fixture)
{
    const FluidSystem::ParameterCache<Scalar> paramCache(1);
    BOOST_CHECK_EQUAL(paramCache.eosRegionIndex(), 1u);
    BOOST_CHECK_EQUAL(paramCache.pvtRegionIndex(), 0u);
}

BOOST_FIXTURE_TEST_CASE(FlashUsesTheRegionItIsGiven, Fixture)
{
    const auto result = flash<FluidSystem>(1);
    const auto reference = flash<ReferenceSystem>(0);
    BOOST_TEST_INFO("L = " << result.L);
    BOOST_CHECK(result.L > 0.0 && result.L < 1.0);
    BOOST_CHECK_CLOSE(result.L, reference.L, 1.0e-8);
    for (int c = 0; c < numComponents; ++c) {
        BOOST_TEST_CONTEXT("component " << c)
        {
            BOOST_CHECK_CLOSE(result.x[c], reference.x[c], 1.0e-8);
            BOOST_CHECK_CLOSE(result.y[c], reference.y[c], 1.0e-8);
        }
    }

    const auto regionOneResult = flash<FluidSystem>(0);
    BOOST_CHECK_GT(std::abs(regionOneResult.L - result.L), 1.0e-3);
}

BOOST_FIXTURE_TEST_CASE(UnknownRegionIsRejected, Fixture)
{
    using Cache = FluidSystem::ParameterCache<Scalar>;
    using ReferenceCache = ReferenceSystem::ParameterCache<Scalar>;
    BOOST_CHECK_THROW(Cache(2), std::out_of_range);
    BOOST_CHECK_THROW(ReferenceCache(1), std::out_of_range);
    BOOST_CHECK_NO_THROW(ReferenceCache(0));
}

BOOST_AUTO_TEST_CASE(RegionsMustShareTheMolecularWeights)
{
    auto heavier = regionTwo;
    heavier.mw = "44.01 16.04 150.0";
    BOOST_CHECK_THROW(initFromDeck<FluidSystem>({regionOne, heavier}), std::runtime_error);
}
