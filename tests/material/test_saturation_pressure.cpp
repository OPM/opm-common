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
 * \brief Tests for the SaturationPressure constraint solver.
 *
 * The reference values are taken from a reference simulator run of a 1D
 * vertical compositional equilibration case with three components (CO2,
 * methane and n-decane, Peng-Robinson, zero binary interaction coefficients)
 * at a constant reservoir temperature of 100 degC.  The bubble-point
 * pressures are the PSAT values reported in the restart file for the
 * single-phase oil cells, and the gas-oil contact values are taken from the
 * equilibration report in the PRT file.
 */
#include "config.h"

#define BOOST_TEST_MODULE SaturationPressure
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/SaturationPressure.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

using Scalar = double;
constexpr int numComponents = 3;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using SatP = Opm::SaturationPressure<Scalar, FluidSystem>;
using CompVec = typename SatP::CompVec;

constexpr auto eosType = Opm::CompositionalConfig::EOSType::PR;

// The constant reservoir temperature (RTEMP) of the test case, 100 degC.
constexpr Scalar temperature = 373.15;

// The fluid system is initialized once with the component properties of the
// test deck (TCRIT, PCRIT, ACF, MW, VCRIT).  Only the critical properties and
// the acentric factors enter the fugacity coefficients, but the full set is
// provided for completeness.
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        FluidSystem::addComponent(CompParam{"CO2", 44.0, 304.128, 73.773e5, 0.09412, 0.22394});
        FluidSystem::addComponent(CompParam{"C1", 16.04, 190.564, 45.992e5, 0.09863, 0.01142});
        FluidSystem::addComponent(CompParam{"C10", 142.28, 617.7, 21.03e5, 0.60980, 0.4884});
    }
};

// How far a (known phase, incipient phase) pair at \p press is from being a
// saturation point: the largest relative fugacity imbalance over the
// components, how far the incipient composition is from summing to one, and
// how far it is from the known composition (a vanishing distance is the
// trivial solution, which satisfies the other two at any pressure).
struct EquilibriumResidual
{
    Scalar fugacity{};
    Scalar closure{};
    Scalar distance{};
};

EquilibriumResidual equilibriumResidual(const CompVec& known,
                                        const unsigned knownPhaseIdx,
                                        const CompVec& incipient,
                                        const unsigned incipientPhaseIdx,
                                        const Scalar press)
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, press);
    fs.setPressure(FluidSystem::gasPhaseIdx, press);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(knownPhaseIdx, c, known[c]);
        fs.setMoleFraction(incipientPhaseIdx, c, incipient[c]);
    }

    typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, FluidSystem::oilPhaseIdx);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    EquilibriumResidual res;
    Scalar sum = 0.0;
    for (int c = 0; c < numComponents; ++c) {
        const Scalar phiL =
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::oilPhaseIdx, c);
        const Scalar phiV =
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::gasPhaseIdx, c);
        const Scalar fL = fs.moleFraction(FluidSystem::oilPhaseIdx, c) * phiL;
        const Scalar fV = fs.moleFraction(FluidSystem::gasPhaseIdx, c) * phiV;
        const Scalar scale = std::max({std::abs(fL), std::abs(fV), Scalar{1.0e-12}});
        res.fugacity = std::max(res.fugacity, std::abs(fL - fV) / scale);

        sum += incipient[c];
        res.distance += std::abs(incipient[c] - known[c]);
    }
    res.closure = std::abs(sum - 1.0);
    return res;
}

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(BubblePressureOilZone)
{
    // The single-phase oil cells of the test case.  The mixtures are binary
    // methane/decane (the CO2 fraction is zero); the expected bubble-point
    // pressures are the reference simulator PSAT values in bar.
    const std::array<std::pair<Scalar, Scalar>, 10> refValues{{
        {0.49, 156.29472},
        {0.47, 147.92114},
        {0.45, 139.75455},
        {0.43, 131.79112},
        {0.41, 124.02647},
        {0.39, 116.45577},
        {0.37, 109.07387},
        {0.35, 101.87540},
        {0.33, 94.85495},
        {0.31, 88.00698},
    }};

    for (const auto& [zMethane, expectedBar] : refValues) {
        const CompVec liquid{0.0, zMethane, 1.0 - zMethane};
        Scalar press = 0.0;
        CompVec vapor{};

        const bool converged =
            SatP::bubblePressure(liquid, temperature, eosType, press, vapor);

        BOOST_REQUIRE_MESSAGE(converged,
                              "bubble-point iteration must converge for z_C1 = " << zMethane);
        // The restart file stores PSAT in single precision; 1e-3 percent
        // (1e-5 relative) is well above that quantization.
        BOOST_CHECK_CLOSE(press / 1.0e5, expectedBar, 1.0e-3);
    }
}

BOOST_AUTO_TEST_CASE(SaturationPressureAtGasOilContact)
{
    // At the gas-oil contact the liquid composition from ZMFVD is
    // (0, 0.5, 0.5) and the reference simulator reports "Pressure at gas-oil
    // contact set to saturation pressure of 160.56010" bar.  The incipient
    // vapour becomes the gas-cap composition, reported as
    // ZMF = (0, 0.987784, 0.012216) in the restart file.
    const CompVec liquid{0.0, 0.5, 0.5};
    Scalar press = 0.0;
    CompVec vapor{};

    const bool converged =
        SatP::bubblePressure(liquid, temperature, eosType, press, vapor);

    BOOST_REQUIRE(converged);
    BOOST_CHECK_CLOSE(press / 1.0e5, 160.56010, 1.0e-3);

    BOOST_CHECK_SMALL(vapor[0], 1.0e-10);
    BOOST_CHECK_CLOSE(vapor[1], 0.987784, 1.0e-2);
    BOOST_CHECK_CLOSE(vapor[2], 0.012216, 1.0e-1);
}

BOOST_AUTO_TEST_CASE(DewPressureGasCap)
{
    // Thermodynamic consistency at the contact: the gas cap is a retrograde
    // condensate, and its upper (retrograde) dew-point pressure must recover
    // the contact pressure, with the incipient liquid recovering the ZMFVD
    // composition at the contact.  The vapour composition is only known to
    // single precision, which limits the achievable agreement; the tolerances
    // reflect that.
    const CompVec vapor{0.0, 0.987784, 0.012216};
    Scalar press = 0.0;
    CompVec liquid{};

    const bool converged =
        SatP::dewPressure(vapor, temperature, eosType, press, liquid);

    BOOST_REQUIRE(converged);
    BOOST_CHECK_CLOSE(press / 1.0e5, 160.56010, 1.0e-2);

    BOOST_CHECK_SMALL(liquid[0], 1.0e-10);
    BOOST_CHECK_CLOSE(liquid[1], 0.5, 0.1);
    BOOST_CHECK_CLOSE(liquid[2], 0.5, 0.1);

    // The pair must also satisfy the equilibrium conditions in its own right.
    const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                         liquid, FluidSystem::oilPhaseIdx, press);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-6);
    BOOST_CHECK_GT(res.distance, 1.0e-3);
}

BOOST_AUTO_TEST_CASE(DewPressureLowerBranch)
{
    // A methane-rich mixture that is lean enough to have no retrograde dew
    // point at this temperature, so dewPressure() only succeeds through the
    // lower-branch fallback.  The assertion is the equilibrium condition
    // itself rather than a previously recorded pressure, so the test states
    // what a dew point is instead of what this solver happened to return.
    const CompVec vapor{0.0, 0.90, 0.10};
    Scalar press = 0.0;
    CompVec liquid{};

    BOOST_REQUIRE(SatP::dewPressure(vapor, temperature, eosType, press, liquid));
    // A genuinely lower-branch pressure: the retrograde region of comparable
    // mixtures sits above 100 bar, the lower dew point of this one near 1 bar.
    BOOST_CHECK_GT(press, 0.0);
    BOOST_CHECK_LT(press, 50.0e5);

    const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                         liquid, FluidSystem::oilPhaseIdx, press);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-6);

    // The incipient liquid must be a genuinely different phase, and being the
    // lower branch it is the heavy one: richer in decane than the vapour.
    BOOST_CHECK_GT(res.distance, 1.0e-3);
    BOOST_CHECK_GT(liquid[2], vapor[2]);
}

BOOST_AUTO_TEST_CASE(SupercriticalLiquidHasNoBubblePoint)
{
    // Pure methane is far above its critical temperature here, so no bubble
    // point exists at any pressure.  The solver must refuse and leave the
    // pressure output untouched.
    const CompVec liquid{0.0, 1.0, 0.0};
    Scalar press = -1.0;
    CompVec vapor{};

    BOOST_CHECK(!SatP::bubblePressure(liquid, temperature, eosType, press, vapor));
    BOOST_CHECK_LT(press, 0.0);
}

BOOST_AUTO_TEST_CASE(PureComponentSaturationPressure)
{
    // Pure decane is well below its critical temperature here, so it has a
    // genuine vapour pressure even though both phases necessarily share its
    // composition.  What tells this state from the trivial solution is that
    // the phases occupy distinct EOS roots, not that their compositions
    // differ; a solver that requires a composition difference refuses it.
    const CompVec liquid{0.0, 0.0, 1.0};
    Scalar pBubble = 0.0;
    CompVec vapor{};

    BOOST_REQUIRE(SatP::bubblePressure(liquid, temperature, eosType, pBubble, vapor));
    BOOST_CHECK_GT(pBubble, 1.0e2);
    BOOST_CHECK_LT(pBubble, 1.0e5);
    for (int c = 0; c < numComponents; ++c) {
        BOOST_CHECK_SMALL(std::abs(vapor[c] - liquid[c]), 1.0e-6);
    }

    // The equilibrium condition holds even though the compositions coincide.
    const auto res = equilibriumResidual(liquid, FluidSystem::oilPhaseIdx,
                                         vapor, FluidSystem::gasPhaseIdx, pBubble);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-6);
    BOOST_CHECK_SMALL(res.distance, 1.0e-6);

    // For a pure component the dew and bubble pressures are one and the same.
    Scalar pDew = 0.0;
    CompVec incipient{};
    BOOST_REQUIRE(SatP::dewPressure(liquid, temperature, eosType, pDew, incipient));
    BOOST_CHECK_CLOSE(pDew, pBubble, 1.0e-3);
}

BOOST_AUTO_TEST_CASE(TrivialSolutionIsNotReportedAsADewPoint)
{
    // Almost pure methane far above its critical temperature has no dew point
    // here.  The successive substitution can still settle onto K == 1 at an
    // arbitrarily high pressure; that trivial solution satisfies the
    // saturation condition numerically and must not be returned as an answer.
    for (const Scalar zMethane : {Scalar{0.999}, Scalar{0.9999}}) {
        const CompVec vapor{0.0, zMethane, 1.0 - zMethane};
        Scalar press = -1.0;
        CompVec liquid{};

        const bool converged =
            SatP::dewPressure(vapor, temperature, eosType, press, liquid);

        if (converged) {
            // If a root is reported at all it must be a real one, i.e. a
            // distinct incipient phase rather than a copy of the vapour.
            const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                                 liquid, FluidSystem::oilPhaseIdx, press);
            BOOST_CHECK_MESSAGE(res.distance > 1.0e-3,
                                "trivial solution reported as a dew point for z_C1 = "
                                << zMethane << " at " << press / 1.0e5 << " bar");
        }
        else {
            // The expected outcome is an outright refusal, which must leave
            // the pressure untouched rather than half-written.
            BOOST_CHECK_LT(press, 0.0);
        }
    }
}
