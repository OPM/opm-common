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
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/common/Exceptions.hpp>

#include <array>
#include <cmath>

namespace {

using Scalar = double;
constexpr int numComponents = 7;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
// A distinct instantiation, so it carries its own static component parameters:
// the same fluid with every shift set to zero.  Comparing against it is what
// makes the fugacity test below independent rather than self-referential.
using UnshiftedSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, true>;
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

        using UnshiftedParam = typename UnshiftedSystem::ComponentParam;
        UnshiftedSystem::init();
        for (int c = 0; c < numComponents; ++c) {
            const auto& p = components[c];
            UnshiftedSystem::addComponent(UnshiftedParam{p.name, p.molarMass, p.criticalT,
                                                         p.criticalP, p.criticalV,
                                                         p.acentric, 0.0});
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

BOOST_AUTO_TEST_CASE(ShiftLeavesTheEquilibriumRatiosAlone)
{
    // The invariant of a volume translation is the equilibrium ratio, not the
    // fugacity coefficient itself.  Peneloux scales the fugacity of component
    // c by exp(-c_c p / (R T)), a factor set by the component and the state
    // but not by the phase, so it cancels from K_c = phi_c^liquid /
    // phi_c^vapour and leaves the phase split untouched.  That ratio is what
    // this checks, against a separate fluid system holding the same
    // components with zero shifts.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    Opm::CompositionalFluidState<Scalar, UnshiftedSystem> fsRef;
    fs.setTemperature(temperature);
    fsRef.setTemperature(temperature);
    for (int ph : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
        fs.setPressure(ph, pressure);
        fsRef.setPressure(ph, pressure);
    }
    for (int c = 0; c < numComponents; ++c) {
        for (int ph : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
            fs.setMoleFraction(ph, c, z[c]);
            fsRef.setMoleFraction(ph, c, z[c]);
        }
    }

    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    typename UnshiftedSystem::template ParameterCache<Scalar> pcRef(eosType);
    for (int ph : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
        pc.updatePhase(fs, ph);
        pcRef.updatePhase(fsRef, ph);
    }

    // The shift is real, and the reference system carries none of it.
    BOOST_CHECK_GT(std::abs(pc.volumeShift(fs, FluidSystem::gasPhaseIdx)), 0.0);
    BOOST_CHECK_SMALL(pcRef.volumeShift(fsRef, UnshiftedSystem::gasPhaseIdx), 1.0e-30);

    for (int c = 0; c < numComponents; ++c) {
        const Scalar K =
            FluidSystem::fugacityCoefficient(fs, pc, FluidSystem::oilPhaseIdx, c) /
            FluidSystem::fugacityCoefficient(fs, pc, FluidSystem::gasPhaseIdx, c);
        const Scalar KRef =
            UnshiftedSystem::fugacityCoefficient(fsRef, pcRef, UnshiftedSystem::oilPhaseIdx, c) /
            UnshiftedSystem::fugacityCoefficient(fsRef, pcRef, UnshiftedSystem::gasPhaseIdx, c);
        BOOST_CHECK_CLOSE(K, KRef, 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(ShiftDoesNotReachTheCachedVolume)
{
    // The equilibrium ratios above survive because the coefficients are built
    // from the unshifted root.  Pinning that here keeps a future change from
    // folding the shift into the cache, where it would corrupt the
    // two-parameter expression the coefficients use rather than translate it.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
    }
    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, FluidSystem::gasPhaseIdx);

    const Scalar Vm = pc.molarVolume(FluidSystem::gasPhaseIdx);
    const Scalar shift = pc.volumeShift(fs, FluidSystem::gasPhaseIdx);
    BOOST_CHECK_CLOSE(pc.correctedMolarVolume(fs, FluidSystem::gasPhaseIdx),
                      Vm - shift, 1.0e-10);
    // The density follows the corrected volume, not the cached one.
    BOOST_CHECK_CLOSE(FluidSystem::density(fs, pc, FluidSystem::gasPhaseIdx),
                      fs.averageMolarMass(FluidSystem::gasPhaseIdx) / (Vm - shift),
                      1.0e-10);
}

BOOST_AUTO_TEST_CASE(ShiftMovesTheLiquidDensityToo)
{
    // The liquid phase is where the two-parameter equation of state is worst
    // and the shift matters most, so it gets its own check.  The expected
    // shift is rebuilt here from the deck's SSHIFT and b_c = Omega_b R Tc/pc
    // rather than read back from the parameter cache, so the whole chain
    // (SSHIFT -> b_c -> corrected volume -> density) is verified independently.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
    }
    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, FluidSystem::oilPhaseIdx);

    constexpr Scalar OmegaB = 0.0777960739;   // Peng-Robinson
    constexpr Scalar R = 8.31446261815324;
    Scalar expectedShift = 0.0;
    for (int c = 0; c < numComponents; ++c) {
        const auto& p = components[c];
        const Scalar b = OmegaB * R * p.criticalT / p.criticalP;
        expectedShift += z[c] * shift[c] * b;
    }
    // The constants here are written out independently of the library's, so
    // the agreement is limited by their last digits rather than by round-off;
    // the effect under test is a ten percent change in density.
    BOOST_CHECK_CLOSE(pc.volumeShift(fs, FluidSystem::oilPhaseIdx), expectedShift, 1.0e-3);

    const Scalar Vm = pc.molarVolume(FluidSystem::oilPhaseIdx);
    const Scalar rho = FluidSystem::density(fs, pc, FluidSystem::oilPhaseIdx);
    BOOST_CHECK_CLOSE(rho,
                      fs.averageMolarMass(FluidSystem::oilPhaseIdx) / (Vm - expectedShift),
                      1.0e-4);
    // The shifts are mostly negative, so the correction enlarges the volume
    // and the shifted liquid is the lighter of the two.
    BOOST_CHECK_LT(rho, fs.averageMolarMass(FluidSystem::oilPhaseIdx) / Vm);
}

// A three-component system of its own, so parsing a deck into it cannot
// disturb the seven-component one the tests above share.
using DeckSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;

BOOST_AUTO_TEST_CASE(ParsedShiftReachesTheFluidSystem)
{
    // The tests above hand the shifts to addComponent() directly, which leaves
    // the parsing path unproven.  This one goes through the deck: SSHIFT is
    // read into the compositional configuration and initFromState() must carry
    // it onto the component parameters.
    const auto deck = Opm::Parser{}.parseString(R"(
RUNSPEC
METRIC
DIMENS
 1 1 1 /
COMPS
3 /
TABDIMS
 1 /
OIL
GAS
WATER
GRID
DXV
 1 /
DYV
 1 /
DZV
 1 /
DEPTHZ
 4*2000 /
PROPS
CNAMES
 C1 C10 CO2 /
TCRIT
 190.6 617.7 304.2 /
PCRIT
 46.0 21.1 73.8 /
VCRIT
 0.0990 0.6240 0.0940 /
MW
 16.043 142.285 44.010 /
ACF
 0.008 0.4885 0.225 /
SSHIFT
 -0.1595 0.10784 -0.0817 /
SOLUTION
SCHEDULE
END
)");

    const auto eclState = Opm::EclipseState{ deck };
    const auto schedule = Opm::Schedule{ deck, eclState };

    BOOST_REQUIRE_NO_THROW(DeckSystem::initFromState(eclState, schedule));

    const std::array<Scalar, 3> expected{-0.1595, 0.10784, -0.0817};
    for (unsigned c = 0; c < 3; ++c) {
        BOOST_CHECK_CLOSE(DeckSystem::volumeShift(c), expected[c], 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(AnOverlargeShiftIsRejectedRatherThanReturned)
{
    // SSHIFT is unconstrained input.  A shift bigger than the molar volume
    // makes the corrected volume non-positive, where the density would come
    // back negative or infinite instead of failing.
    using BigShift = Opm::GenericOilGasWaterFluidSystem<Scalar, 1, false>;
    using CompParam = typename BigShift::ComponentParam;
    BigShift::init();
    BigShift::addComponent(CompParam{"C1", 0.016043, 190.60, 45.40e5, 0.099, 0.008,
                                     /*volume_shift=*/1.0e3});

    Opm::CompositionalFluidState<Scalar, BigShift> fs;
    fs.setTemperature(Scalar{400});
    fs.setPressure(BigShift::oilPhaseIdx, Scalar{100e5});
    fs.setPressure(BigShift::gasPhaseIdx, Scalar{100e5});
    fs.setMoleFraction(BigShift::gasPhaseIdx, 0, Scalar{1});
    fs.setMoleFraction(BigShift::oilPhaseIdx, 0, Scalar{1});

    typename BigShift::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, BigShift::gasPhaseIdx);

    BOOST_CHECK_THROW(BigShift::density(fs, pc, BigShift::gasPhaseIdx),
                      Opm::NumericalProblem);
}
