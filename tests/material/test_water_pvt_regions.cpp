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
 * \brief Tests that the generic compositional fluid system takes the water
 *        properties of the PVT region its parameter cache selects.
 *
 * The expected values follow from PVTW: with X = cw (p - pref), the water
 * density is rho_ref (1 + X + X^2/2) / Bw_ref, and without viscosibility the
 * viscosity is the reference viscosity at every pressure.
 */
#include "config.h"

#define BOOST_TEST_MODULE WaterPvtRegions
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <memory>

namespace {

using Scalar = double;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, true>;
using EOSType = Opm::CompositionalConfig::EOSType;

constexpr auto deck = R"(
RUNSPEC
METRIC
TABDIMS
 1* 2 /
OIL
GAS
WATER
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
EOS
 PR /
MW
 44.01 16.04 142.28 /
TCRIT
 304.13 190.56 617.7 /
PCRIT
 73.77 45.99 21.03 /
VCRIT
 0.0941 0.0986 0.6098 /
ACF
 0.2239 0.0114 0.4884 /
BIC
 0 0 0 /
PVTW
 75.0 1.03 4.0E-5 0.3 0.0 /
 75.0 1.01 2.5E-5 0.9 0.0 /
DENSITY
 800.0 1000.0 1.0 /
 800.0 1150.0 1.0 /
)";

const Opm::TableManager& testTables()
{
    static const auto parsed = Opm::Parser{}.parseString(deck);
    static const Opm::TableManager tables(parsed);
    return tables;
}

struct Fixture
{
    Fixture()
    {
        const auto parsed = Opm::Parser{}.parseString(deck);
        const Opm::EclipseState eclState(parsed);
        const Opm::Schedule schedule(parsed, eclState, std::make_shared<Opm::Python>());
        FluidSystem::initFromState(eclState, schedule);
    }
};

constexpr Scalar pressure = 150.0e5;
constexpr Scalar temperature = 350.0;

Opm::CompositionalFluidState<Scalar, FluidSystem> waterState()
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    for (unsigned phase = 0; phase < FluidSystem::numPhases; ++phase) {
        fs.setPressure(phase, pressure);
    }
    return fs;
}

Scalar expectedDensity(const Opm::PVTWRecord& pvtw, const Scalar surfaceDensity)
{
    const Scalar x = pvtw.compressibility * (pressure - pvtw.reference_pressure);
    return surfaceDensity * (1.0 + x + 0.5 * x * x) / pvtw.volume_factor;
}

} // Anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(WaterPropertiesFollowTheSelectedPvtRegion)
{
    const auto fs = waterState();
    const auto& tables = testTables();
    const auto& pvtw = tables.getPvtwTable();
    const auto& density = tables.getDensityTable();
    BOOST_REQUIRE_EQUAL(pvtw.size(), 2u);
    BOOST_REQUIRE_EQUAL(density.size(), pvtw.size());
    BOOST_REQUIRE(pvtw[0].viscosity != pvtw[1].viscosity);
    BOOST_REQUIRE(density[0].water != density[1].water);

    for (unsigned regionIdx = 0; regionIdx < pvtw.size(); ++regionIdx) {
        BOOST_TEST_CONTEXT("PVT region " << regionIdx + 1) {
            typename FluidSystem::template ParameterCache<Scalar> paramCache(EOSType::PR);
            paramCache.setRegionIndex(regionIdx);
            BOOST_CHECK_CLOSE(FluidSystem::density(fs, paramCache, FluidSystem::waterPhaseIdx),
                              expectedDensity(pvtw[regionIdx], density[regionIdx].water),
                              1.0e-10);
            BOOST_CHECK_CLOSE(FluidSystem::viscosity(fs, paramCache, FluidSystem::waterPhaseIdx),
                              pvtw[regionIdx].viscosity, 1.0e-10);
        }
    }
}

BOOST_AUTO_TEST_CASE(FirstPvtRegionIsTheDefault)
{
    const auto fs = waterState();
    const auto& tables = testTables();
    BOOST_REQUIRE(!tables.getPvtwTable().empty());
    BOOST_REQUIRE(!tables.getDensityTable().empty());
    const typename FluidSystem::template ParameterCache<Scalar> paramCache(EOSType::PR);
    BOOST_CHECK_EQUAL(paramCache.regionIndex(), 0u);
    BOOST_CHECK_CLOSE(FluidSystem::density(fs, paramCache, FluidSystem::waterPhaseIdx),
                      expectedDensity(tables.getPvtwTable()[0],
                                      tables.getDensityTable()[0].water),
                      1.0e-10);
}
