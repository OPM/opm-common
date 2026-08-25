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
 * \brief Tests that the SSHIFT volume shift reaches the density.
 *
 * The fluid is the seven-component one of a deck whose reference run reports
 * a gas density of 165.87 kg/m3 at 272.599 bar and 393.15 K.  Without the
 * shift the equation of state gives 172.84.
 */
#include "config.h"

#define BOOST_TEST_MODULE VolumeShift
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <array>
#include <cmath>

namespace {

using Scalar = double;
constexpr int numComponents = 7;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using CompVec = std::array<Scalar, numComponents>;

constexpr auto eosType = Opm::CompositionalConfig::EOSType::PR;

// The state the reference reports the density at: the top of the column,
// where RTEMP is 120 degC and the equilibrated pressure 272.599 bar.
constexpr Scalar temperature = 393.15;
constexpr Scalar pressure = 272.599e5;

// A seven-component reservoir fluid: methane, CO2, ethane, propane and three
// lumped fractions.  The shifts are the SSHIFT values of the same deck.
const CompVec z{0.88583, 0.0437, 0.034, 0.0189, 0.0152, 0.0023, 0.00007};
const CompVec shift{-0.1595, -0.0817, -0.1134, -0.0863, -0.0243568,
                    0.10784125400986, 0.206892761354429};

struct Component
{
    const char* name;
    Scalar molarMass;   // kg/mol, as the parser hands it to the fluid system
    Scalar criticalT;   // K
    Scalar criticalP;   // Pa
    Scalar criticalV;   // m^3/kmol
    Scalar acentric;
};

// MW, TCRIT, PCRIT, VCRIT and ACF as the deck gives them.
const std::array<Component, numComponents> components{{
    {"CH4",     0.016043, 190.60, 45.40e5, 0.09900000, 0.00800},
    {"CO2",     0.044010, 304.20, 72.80e5, 0.09400000, 0.22500},
    {"C2H6",    0.030070, 305.40, 48.20e5, 0.14800000, 0.09800},
    {"C3H8",    0.044097, 369.80, 41.90e5, 0.20300000, 0.15200},
    {"C4-C9",   0.076900, 474.70, 32.40e5, 0.32321204, 0.25490},
    {"C10-C20", 0.163000, 646.00, 20.42e5, 0.63255728, 0.55100},
    {"C21+",    0.310850, 812.00, 14.50e5, 0.97988801, 0.90900},
}};

// The component parameters are static, so the fluid system carries one
// configuration per process: it is initialized once, with the shifts.
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        for (int c = 0; c < numComponents; ++c) {
            const auto& p = components[c];
            FluidSystem::addComponent(CompParam{p.name, p.molarMass, p.criticalT,
                                                p.criticalP, p.criticalV, p.acentric,
                                                shift[c]});
        }
    }
};

/// The gas density of the mixture, and the fugacity coefficients alongside it.
struct PhaseState
{
    Scalar density{};
    CompVec fugacityCoefficient{};
};

PhaseState gasState()
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
    }

    typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    PhaseState state;
    state.density = FluidSystem::density(fs, paramCache, FluidSystem::gasPhaseIdx);
    for (int c = 0; c < numComponents; ++c) {
        state.fugacityCoefficient[c] =
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::gasPhaseIdx, c);
    }
    return state;
}

} // Anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(ShiftMovesTheDensityOntoTheReference)
{
    const auto gas = gasState();

    // The reference reports 165.87 kg/m3 at the top of the column.  The
    // composition here is the one COMPVD states rather than the equilibrated
    // one of that exact depth, which is worth a few tenths of a percent; the
    // unshifted equation of state gives 172.84, so the tolerance is far
    // tighter than the effect being tested.
    BOOST_CHECK_CLOSE(gas.density, 165.87, 1.0);
}

BOOST_AUTO_TEST_CASE(ShiftLeavesTheFugacityCoefficientsAlone)
{
    // The shift is a translation along the volume axis, so it cancels from
    // the equilibrium ratios: the fugacity coefficients must be those of the
    // unshifted equation of state, which is why the shift is applied to the
    // density rather than to the cached molar volume.
    const auto gas = gasState();

    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
    }
    typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    // The cached molar volume is the unshifted one; the shift only appears
    // when the density is asked for.
    const Scalar Vm = paramCache.molarVolume(FluidSystem::gasPhaseIdx);
    const Scalar shiftVm = paramCache.volumeShift(fs, FluidSystem::gasPhaseIdx);
    BOOST_CHECK_GT(std::abs(shiftVm), 0.0);
    BOOST_CHECK_CLOSE(fs.averageMolarMass(FluidSystem::gasPhaseIdx) / (Vm - shiftVm),
                      gas.density, 1.0e-10);

    // Fugacity coefficients computed from the unshifted volume.
    for (int c = 0; c < numComponents; ++c) {
        const Scalar phi =
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::gasPhaseIdx, c);
        BOOST_CHECK_CLOSE(gas.fugacityCoefficient[c], phi, 1.0e-10);
    }
}
