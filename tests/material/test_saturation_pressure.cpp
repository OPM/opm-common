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
 * The reference values come from a 1D vertical compositional equilibration
 * case with three components (CO2, methane and n-decane, Peng-Robinson, zero
 * binary interaction coefficients) at a constant reservoir temperature of
 * 100 degC.  The bubble-point pressures are the PSAT values its restart file
 * reports for the single-phase oil cells, and the gas-oil contact values come
 * from its equilibration report.
 */
#include "config.h"

#define BOOST_TEST_MODULE SaturationPressure
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/SaturationPressure.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>
#include <tuple>
#include <utility>

namespace {

using Scalar = double;
constexpr int numComponents = 3;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using SatP = Opm::SaturationPressure<Scalar, FluidSystem>;
using CompVec = SatP::CompVec;

using FloatFluidSystem = Opm::GenericOilGasWaterFluidSystem<float, numComponents, false>;
using FloatSatP = Opm::SaturationPressure<float, FloatFluidSystem>;

constexpr auto eosType = Opm::CompositionalConfig::EOSType::PR;

// The constant reservoir temperature (RTEMP) of the test case, 100 degC.
constexpr Scalar temperature = 373.15;

// Initialize the component properties from the test deck once for all tests.
struct Fixture
{
    Fixture()
    {
        using CompParam = FluidSystem::ComponentParam;
        FluidSystem::init();
        FluidSystem::addComponent(CompParam{"CO2", 44.0, 304.128, 73.773e5, 0.09412, 0.22394});
        FluidSystem::addComponent(CompParam{"C1", 16.04, 190.564, 45.992e5, 0.09863, 0.01142});
        FluidSystem::addComponent(CompParam{"C10", 142.28, 617.7, 21.03e5, 0.60980, 0.4884});

        using FloatCompParam = FloatFluidSystem::ComponentParam;
        FloatFluidSystem::init();
        FloatFluidSystem::addComponent(
            FloatCompParam{"CO2", 44.0, 304.128, 73.773e5, 0.09412, 0.22394});
        FloatFluidSystem::addComponent(
            FloatCompParam{"C1", 16.04, 190.564, 45.992e5, 0.09863, 0.01142});
        FloatFluidSystem::addComponent(
            FloatCompParam{"C10", 142.28, 617.7, 21.03e5, 0.60980, 0.4884});
    }
};

// Equilibrium checks and composition distance. A zero distance can indicate a
// trivial solution; pure-fluid saturation also requires checking the EOS roots.
struct EquilibriumResidual
{
    Scalar fugacity{};
    Scalar closure{};
    Scalar distance{};
};

template <class FS>
EquilibriumResidual equilibriumResidualFor(
    const std::array<typename FS::Scalar, FS::numComponents>& known,
    const unsigned knownPhaseIdx,
    const std::array<typename FS::Scalar, FS::numComponents>& incipient,
    const unsigned incipientPhaseIdx,
    const typename FS::Scalar press,
    const typename FS::Scalar temp)
{
    using Value = typename FS::Scalar;
    constexpr int numComp = FS::numComponents;
    Opm::CompositionalFluidState<Value, FS> fs;
    fs.setTemperature(temp);
    fs.setPressure(FS::oilPhaseIdx, press);
    fs.setPressure(FS::gasPhaseIdx, press);
    for (int c = 0; c < numComp; ++c) {
        fs.setMoleFraction(knownPhaseIdx, c, known[c]);
        fs.setMoleFraction(incipientPhaseIdx, c, incipient[c]);
    }

    typename FS::template ParameterCache<Value> paramCache(eosType);
    paramCache.updatePhase(fs, FS::oilPhaseIdx);
    paramCache.updatePhase(fs, FS::gasPhaseIdx);

    EquilibriumResidual res;
    Value sum = 0.0;
    for (int c = 0; c < numComp; ++c) {
        const Value phiL =
            FS::fugacityCoefficient(fs, paramCache, FS::oilPhaseIdx, c);
        const Value phiV =
            FS::fugacityCoefficient(fs, paramCache, FS::gasPhaseIdx, c);
        const Value fL = fs.moleFraction(FS::oilPhaseIdx, c) * phiL;
        const Value fV = fs.moleFraction(FS::gasPhaseIdx, c) * phiV;
        const Value scale = std::max({std::abs(fL), std::abs(fV), Value{1.0e-12}});
        res.fugacity = std::max(res.fugacity,
                                static_cast<Scalar>(std::abs(fL - fV) / scale));

        sum += incipient[c];
        res.distance += std::abs(incipient[c] - known[c]);
    }
    res.closure = std::abs(sum - 1.0);
    return res;
}

EquilibriumResidual equilibriumResidual(const CompVec& known,
                                        const unsigned knownPhaseIdx,
                                        const CompVec& incipient,
                                        const unsigned incipientPhaseIdx,
                                        const Scalar press)
{
    return equilibriumResidualFor<FluidSystem>(known, knownPhaseIdx, incipient,
                                               incipientPhaseIdx, press, temperature);
}

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(BubblePressureOilZone)
{
    // The single-phase oil cells of the test case.  The mixtures are binary
    // methane/decane (the CO2 fraction is zero); the expected bubble-point
    // pressures are the reference PSAT values in bar.
    constexpr std::array<std::pair<Scalar, Scalar>, 10> refValues{{
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

BOOST_AUTO_TEST_CASE(CompositionNeedNotSumToOne)
{
    // Callers assemble z in floating point -- from a flash, or from a restart
    // file's single-precision mole fractions -- so it need not sum exactly to
    // one.  The boundary residual is measured against one, so without
    // rescaling a composition summing to 1 + d stops converging as soon as
    // |d| exceeds that tolerance.
    constexpr Scalar zMethane = 0.47;
    constexpr Scalar referenceBar = 147.92114;

    for (const Scalar excess : {Scalar{-1.0e-4}, Scalar{-3.0e-8}, Scalar{0.0},
                                Scalar{3.0e-8}, Scalar{1.0e-4}}) {
        const CompVec liquid{0.0, zMethane, 1.0 - zMethane + excess};

        Scalar press = 0.0;
        CompVec vapor{};
        BOOST_REQUIRE_MESSAGE(SatP::bubblePressure(liquid, temperature, eosType, press, vapor),
                              "bubble point must converge for a composition summing to "
                                  << 1.0 + excess);

        // Rescaling moves the mixture itself, so the bubble point moves with
        // it: 1e-4 of excess is worth about 0.02 bar here.
        BOOST_CHECK_CLOSE(press / 1.0e5, referenceBar, 3.0e-2);

        // The incipient vapour is normalised whatever the input was.
        Scalar sum = 0.0;
        for (int c = 0; c < numComponents; ++c) {
            sum += vapor[c];
        }
        BOOST_CHECK_CLOSE(sum, 1.0, 1.0e-6);
    }

    // The case that arises in practice: 0.47 and 0.53 sum to 1 - 3e-8 once
    // they have been through single precision.
    const CompVec rounded{0.0, static_cast<Scalar>(0.47f), static_cast<Scalar>(0.53f)};
    Scalar pRounded = 0.0;
    CompVec roundedVapor{};
    BOOST_REQUIRE(SatP::bubblePressure(rounded, temperature, eosType, pRounded, roundedVapor));
    BOOST_CHECK_CLOSE(pRounded / 1.0e5, referenceBar, 1.0e-3);

    // The dew search rescales the same way.
    Scalar pBubble = 0.0;
    CompVec vapour{};
    BOOST_REQUIRE(SatP::bubblePressure({0.0, 0.40, 0.60}, temperature, eosType,
                                       pBubble, vapour));
    vapour.back() += 3.0e-8;

    Scalar pDew = -1.0;
    CompVec incipient{};
    BOOST_REQUIRE_MESSAGE(SatP::dewPressure(vapour, temperature, eosType, pDew, incipient),
                          "dew point must converge for a vapour that does not sum to one");
    BOOST_CHECK_CLOSE(pDew, pBubble, 1.0e-3);
}

BOOST_AUTO_TEST_CASE(SaturationPressureAtGasOilContact)
{
    // At the gas-oil contact the liquid composition from ZMFVD is
    // (0, 0.5, 0.5), and the reference equilibration sets the contact pressure
    // to a saturation pressure of 160.56010 bar.  The incipient vapour becomes
    // the gas-cap composition, reported as ZMF = (0, 0.987784, 0.012216) in
    // the restart file.
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

BOOST_AUTO_TEST_CASE(BubbleDewRoundTrip)
{
    // The vapour that appears at the bubble point of a liquid is, at that same
    // temperature and pressure, a vapour at its own dew point with the liquid
    // as the incipient phase.  The dew-point search must therefore return the
    // bubble-point pressure it was handed and recover the liquid composition.
    //
    // These cases exercise the upper-dew branch selected by dewPressure().
    for (const Scalar zMethane : {Scalar{0.30}, Scalar{0.35}, Scalar{0.40}, Scalar{0.45}}) {
        const CompVec liquid{0.0, zMethane, 1.0 - zMethane};

        Scalar pBubble = 0.0;
        CompVec vapor{};
        BOOST_REQUIRE_MESSAGE(SatP::bubblePressure(liquid, temperature, eosType, pBubble, vapor),
                              "bubble point must converge for z_C1 = " << zMethane);

        Scalar pDew = -1.0;
        CompVec incipient{};
        BOOST_REQUIRE_MESSAGE(SatP::dewPressure(vapor, temperature, eosType, pDew, incipient),
                              "dew point must converge for the vapour of z_C1 = " << zMethane);

        BOOST_CHECK_CLOSE(pDew, pBubble, 1.0e-6);
        for (int c = 0; c < numComponents; ++c) {
            BOOST_CHECK_SMALL(std::abs(incipient[c] - liquid[c]), 1.0e-6);
        }
    }
}

BOOST_AUTO_TEST_CASE(SinglePrecisionBubbleDewRoundTrip)
{
    constexpr FloatSatP::CompVec liquid{0.0f, 0.4f, 0.6f};
    float pBubble = -1.0f;
    FloatSatP::CompVec vapor{};
    BOOST_REQUIRE(FloatSatP::bubblePressure(liquid, 373.15f, eosType, pBubble, vapor));

    Scalar pBubbleDouble = -1.0;
    CompVec vaporDouble{};
    BOOST_REQUIRE(SatP::bubblePressure({0.0, 0.4, 0.6}, temperature, eosType,
                                       pBubbleDouble, vaporDouble));
    BOOST_CHECK_CLOSE(static_cast<Scalar>(pBubble), pBubbleDouble, 1.0e-3);

    float pDew = -1.0f;
    FloatSatP::CompVec recovered{};
    BOOST_REQUIRE(FloatSatP::dewPressure(vapor, 373.15f, eosType, pDew, recovered));
    BOOST_CHECK_CLOSE(pDew, pBubble, 1.0e-3);
    for (int c = 0; c < numComponents; ++c) {
        BOOST_CHECK_SMALL(recovered[c] - liquid[c], 2.0e-5f);
    }
    const auto res = equilibriumResidualFor<FloatFluidSystem>(
        vapor, FloatFluidSystem::gasPhaseIdx,
        recovered, FloatFluidSystem::oilPhaseIdx, pDew, 373.15f);
    BOOST_CHECK_SMALL(res.closure, 2.0e-5);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-3);
}

BOOST_AUTO_TEST_CASE(DewPressureLowerBranch)
{
    // These mixtures have an ordinary envelope ending at a bubble boundary.
    // Reject unstable upper-branch stationary points near 155.6 and 314.0 bar,
    // respectively, and verify the lower dew point through fugacity equality.
    for (const Scalar yMethane : {Scalar{0.70}, Scalar{0.85}}) {
        const CompVec vapor{0.0, yMethane, 1.0 - yMethane};
        Scalar press = 0.0;
        CompVec liquid{};

        BOOST_REQUIRE_MESSAGE(SatP::dewPressure(vapor, temperature, eosType, press, liquid),
                              "dew point must converge for y_C1 = " << yMethane);
        BOOST_CHECK_GT(press, 0.0);
        BOOST_CHECK_LT(press, 5.0e5);

        const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                             liquid, FluidSystem::oilPhaseIdx, press);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-6);
        // The incipient liquid is nearly pure decane: a different phase.
        BOOST_CHECK_GT(res.distance, 1.0);
        BOOST_CHECK_GT(liquid[2], 0.99);
    }
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
    // For these methane-rich mixtures, successive substitution can converge to
    // trivial K == 1 states at high pressure. Any returned root must have a
    // distinct incipient-phase composition.
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
            // On failure, the pressure output must remain unchanged.
            BOOST_CHECK_LT(press, 0.0);
        }
    }
}

BOOST_AUTO_TEST_CASE(NearCriticalBubblePoints)
{
    // Liquids approaching the critical composition of the mixture.  The
    // successive substitution contracts ever more slowly there and the
    // saturation sum flattens against one, which defeated a fixed-point
    // pressure update within its iteration budget.  The reference values are
    // this implementation's converged results, cross-checked against a scan
    // of the saturation sum over pressure at fixed composition.
    constexpr std::array<std::pair<Scalar, Scalar>, 3> cases{{
        {0.80, 306.1289597e5},
        {0.82, 314.7255712e5},
        {0.834, 320.0963847e5},
    }};
    for (const auto& [xMethane, pExpected] : cases) {
        const CompVec liquid{0.0, xMethane, 1.0 - xMethane};
        Scalar press = 0.0;
        CompVec vapor{};

        BOOST_REQUIRE_MESSAGE(SatP::bubblePressure(liquid, temperature, eosType, press, vapor),
                              "bubble point must converge for x_C1 = " << xMethane);
        BOOST_CHECK_CLOSE(press, pExpected, 1.0e-4);

        const auto res = equilibriumResidual(liquid, FluidSystem::oilPhaseIdx,
                                             vapor, FluidSystem::gasPhaseIdx, press);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
        BOOST_CHECK_GT(res.distance, 1.0e-2);
    }
}

BOOST_AUTO_TEST_CASE(RetrogradeDewPointOfBubblePointVapour)
{
    // The vapour in equilibrium with a decane-rich liquid at its bubble point
    // is a lean gas with two dew points: the lower one at the bubble pressure
    // it came from, and a retrograde one above it.  dewPressure() must report
    // the retrograde point.  It is approached from the single-phase side,
    // where the saturation sum rises towards one so slowly that a fixed-point
    // pressure update ran out of iterations short of it.
    const CompVec liquid{0.0, 0.17, 0.83};
    Scalar pBubble = 0.0;
    CompVec vapor{};
    BOOST_REQUIRE(SatP::bubblePressure(liquid, temperature, eosType, pBubble, vapor));
    BOOST_CHECK_CLOSE(pBubble, 44.4490922e5, 1.0e-4);

    Scalar pDew = 0.0;
    CompVec incipient{};
    BOOST_REQUIRE(SatP::dewPressure(vapor, temperature, eosType, pDew, incipient));
    BOOST_CHECK_CLOSE(pDew, 57.6521308e5, 1.0e-4);
    BOOST_CHECK_GT(pDew, pBubble);

    const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                         incipient, FluidSystem::oilPhaseIdx, pDew);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
    BOOST_CHECK_GT(res.distance, 1.0e-2);
}

BOOST_AUTO_TEST_CASE(NearCriticalRetrogradeDewPoint)
{
    // A vapour just richer in methane than the critical composition.  Its
    // retrograde dew point lies within a bar or two of the critical pressure
    // (the bubble curve peaks at 331.2 bar near x_C1 = 0.88), where the two
    // phases differ in composition by a few percent only.  The point is
    // accepted once the stability certificate converges, which the
    // accelerated substitution makes possible this close to the critical
    // point.
    const CompVec vapor{0.0, 0.90, 0.10};
    Scalar press = 0.0;
    CompVec liquid{};
    BOOST_REQUIRE(SatP::dewPressure(vapor, temperature, eosType, press, liquid));
    BOOST_CHECK_GT(press, 300.0e5);
    BOOST_CHECK_LT(press, 340.0e5);

    const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                         liquid, FluidSystem::oilPhaseIdx, press);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
    BOOST_CHECK_GT(res.distance, 1.0e-2);
}

BOOST_AUTO_TEST_CASE(RetrogradeDewPointWithBinaryInteraction)
{
    // Seven-component gas from a reference equilibration run at 120 degC.
    // Exercise nonzero binary interaction coefficients and their parsed
    // lower-triangle ordering. The retrograde dew point is about 120 bar
    // below the reservoir pressure, with distinct phase compositions.
    using FluidSystem7 = Opm::GenericOilGasWaterFluidSystem<Scalar, 7, false>;
    using SatP7 = Opm::SaturationPressure<Scalar, FluidSystem7>;

    const auto deck = Opm::Parser{}.parseString(R"(
RUNSPEC
METRIC
DIMENS
 1 1 1 /
COMPS
7 /
TABDIMS
 1 /
OIL
GAS
GRID
DXV
 1 /
DYV
 1 /
DZV
 1 /
DEPTHZ
 4*2500 /
PROPS
EOS
 PR /
CNAMES
 CH4 CO2 C2H6 C3H8 C4-C9 C10-C20 C21+ /
TCRIT
 190.6 304.2 305.4 369.8 474.7 646.0 812.0 /
PCRIT
 45.40 72.80 48.20 41.90 32.40 20.42 14.50 /
VCRIT
 0.099 0.094 0.148 0.203 0.32321204 0.63255728 0.97988801 /
MW
 16.043 44.010 30.070 44.097 76.900 163.00 310.85 /
ACF
 0.008 0.225 0.098 0.152 0.2549 0.551 0.909 /
BIC
 1.0500000E-01
 2.6890022E-03 1.3000000E-01
 8.5370405E-03 1.2500000E-01 1.6620489E-03
 2.2915937E-02 0.0000000E+00 1.0088696E-02 3.5952572E-03
 5.4875390E-02 0.0000000E+00 3.4227722E-02 2.1174720E-02 7.4706799E-03
 8.1972182E-02 0.0000000E+00 5.6906187E-02 4.0015457E-02 2.0180685E-02 3.1846386E-03 /
SOLUTION
SCHEDULE
END
)");

    const auto eclState = Opm::EclipseState{ deck };
    const auto schedule = Opm::Schedule{ deck, eclState };
    FluidSystem7::init();
    BOOST_REQUIRE_NO_THROW(FluidSystem7::initFromState(eclState, schedule));

    // The composition of the gas zone in the reference data.
    constexpr SatP7::CompVec vapor{0.885830, 0.043700, 0.034000, 0.018900,
                                  0.015200, 0.002300, 0.000070};
    constexpr Scalar temp = 393.15;         // RTEMP of the reference case, 120 degC
    constexpr Scalar pReference = 151.7060; // its reported PSAT, in bar
    constexpr Scalar pReservoir = 272.6396; // and the cell pressure there

    Scalar press = 0.0;
    SatP7::CompVec liquid{};
    BOOST_REQUIRE(SatP7::dewPressure(vapor, temp, eosType, press, liquid));

    // The restart file stores PSAT in single precision; 1e-3 percent is well
    // above that quantization.
    BOOST_CHECK_CLOSE(press / 1.0e5, pReference, 1.0e-3);

    // The retrograde branch, not a point next to the reservoir pressure.
    BOOST_CHECK_LT(press / 1.0e5, 0.7 * pReservoir);

    // The incipient phase is a genuine liquid: heavy where the gas is light.
    BOOST_CHECK_GT(liquid[6], 100.0 * vapor[6]);
    BOOST_CHECK_LT(liquid[0], 0.5 * vapor[0]);

    const auto res = equilibriumResidualFor<FluidSystem7>(
        vapor, FluidSystem7::gasPhaseIdx, liquid, FluidSystem7::oilPhaseIdx, press, temp);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
    BOOST_CHECK_GT(res.distance, 1.0);
}

BOOST_AUTO_TEST_CASE(RetrogradeDewPointOfANearCriticalGas)
{
    // Near criticality a flat trial total does not establish stationarity.
    // Finish the stability trial and return the upper point near 331.4 bar,
    // rather than falling through to the lower point near 0.961 bar.
    const CompVec vapor{0.0, 0.895, 0.105};
    Scalar press = -123.0;
    CompVec liquid{};

    BOOST_REQUIRE(SatP::dewPressure(vapor, temperature, eosType, press, liquid));
    BOOST_CHECK_CLOSE(press / 1.0e5, 331.3565338, 1.0e-3);

    const auto res = equilibriumResidual(vapor, FluidSystem::gasPhaseIdx,
                                         liquid, FluidSystem::oilPhaseIdx, press);
    BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
    BOOST_CHECK_SMALL(res.closure, 1.0e-10);
    BOOST_CHECK_GT(res.distance, 1.0e-3);
}

BOOST_AUTO_TEST_CASE(NarrowRetrogradeEnvelopeKeepsTheUpperDewPoint)
{
    // CO2/methane at 200 K has two closely spaced dew points for these gas
    // compositions. Generate each gas from its equilibrium liquid at the
    // upper boundary, independently of the dew search. The old scan either
    // failed or returned the lower dew point: for x_CO2 = 0.068 it returned
    // 51.20128 instead of 51.50684 bar. At x_CO2 = 0.070 the interval is only
    // about 0.056 bar wide, much less than a 10% pressure step.
    constexpr Scalar temp = 200.0;
    for (const Scalar xCO2 : {Scalar{0.068}, Scalar{0.070}, Scalar{0.065}}) {
        const CompVec liquid{xCO2, 1.0 - xCO2, 0.0};
        CompVec vapor{};
        Scalar pBubble{};
        BOOST_REQUIRE(SatP::bubblePressure(liquid, temp, eosType, pBubble, vapor));

        Scalar pDew = -123.0;
        CompVec recovered{};
        BOOST_REQUIRE_MESSAGE(SatP::dewPressure(vapor, temp, eosType, pDew, recovered),
                              "upper dew point must converge for x_CO2 = " << xCO2);
        BOOST_CHECK_CLOSE(pDew, pBubble, 1.0e-4);
        for (int c = 0; c < numComponents; ++c) {
            BOOST_CHECK_SMALL(recovered[c] - liquid[c], 1.0e-6);
        }
        const auto res = equilibriumResidualFor<FluidSystem>(
            vapor, FluidSystem::gasPhaseIdx, recovered, FluidSystem::oilPhaseIdx, pDew, temp);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
        BOOST_CHECK_GT(res.distance, 1.0e-2);
    }
}

BOOST_AUTO_TEST_CASE(OrdinaryDewPointAfterFollowingTheBubbleBoundary)
{
    // Here the upper dew search and the direct bubble search both struggle.
    // Continuation must follow the vapour-like trial to the bubble boundary
    // before returning the lower dew point, including the near-critical
    // stability check for the ternary mixture.
    for (const auto& [temp, vapor, expectedBar] :
         {std::tuple{440.0, CompVec{0.0, 0.8, 0.2}, 4.587166726},
          std::tuple{570.0, CompVec{0.25, 0.3, 0.45}, 38.68411340}}) {
        Scalar press = -123.0;
        CompVec liquid{};
        BOOST_REQUIRE(SatP::dewPressure(vapor, temp, eosType, press, liquid));
        BOOST_CHECK_CLOSE(press / 1.0e5, expectedBar, 1.0e-4);
        const auto res = equilibriumResidualFor<FluidSystem>(
            vapor, FluidSystem::gasPhaseIdx, liquid, FluidSystem::oilPhaseIdx, press, temp);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
        BOOST_CHECK_GT(res.distance, 0.1);
    }
}

BOOST_AUTO_TEST_CASE(AcceleratedDewSearchKeepsFiniteIterates)
{
    // Guard against K underflow producing a NaN pressure and an EOS exception.
    // An independent stability scan locates the first mixture's envelope at
    // 18.44153--22.72437 bar; the other four remain single-phase over 0.5--600 bar.
    struct Case
    {
        Scalar temp;
        CompVec vapor;
        Scalar expectedBar;  // zero when the independent scan found no dew point
    };
    constexpr std::array<Case, 5> cases{{
        {600.0, {0.05, 0.0, 0.95}, 18.44153},
        {600.0, {0.35, 0.0, 0.65}, 0.0},
        {610.0, {0.20, 0.0, 0.80}, 0.0},
        {610.0, {0.25, 0.0, 0.75}, 0.0},
        {610.0, {0.30, 0.0, 0.70}, 0.0},
    }};
    for (const auto& [temp, vapor, expectedBar] : cases) {
        constexpr Scalar sentinel = -123.0;
        Scalar press = sentinel;
        CompVec liquid{};
        bool converged = false;
        BOOST_REQUIRE_NO_THROW(
            converged = SatP::dewPressure(vapor, temp, eosType, press, liquid));

        if (expectedBar == 0.0) {
            BOOST_CHECK_MESSAGE(!converged,
                                "single-phase mixture reported a dew point at "
                                << press / 1.0e5 << " bar");
            BOOST_CHECK_EQUAL(press, sentinel);
            continue;
        }

        BOOST_REQUIRE_MESSAGE(converged,
                              "dew point must be found for z_CO2 = " << vapor[0]);
        BOOST_CHECK_CLOSE(press / 1.0e5, expectedBar, 1.0e-3);

        const auto res = equilibriumResidualFor<FluidSystem>(
            vapor, FluidSystem::gasPhaseIdx, liquid, FluidSystem::oilPhaseIdx, press, temp);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_GT(res.distance, 1.0e-3);
    }
}

BOOST_AUTO_TEST_CASE(DewPointReachedFromTheBubbleBoundary)
{
    // Both dew scans miss these narrow CO2/decane envelopes near criticality.
    // Trace down from the bubble boundary to find the dew point. Independent
    // stability scans locate the envelopes at 24.26--26.86 and 22.40--23.64 bar.
    struct Case
    {
        Scalar temp;
        CompVec vapor;
        Scalar expectedBar;
    };
    constexpr std::array<Case, 2> cases{{
        {612.0, {0.08, 0.0, 0.92}, 24.2564},
        {614.0, {0.04, 0.0, 0.96}, 22.3950},
    }};
    for (const auto& [temp, vapor, expectedBar] : cases) {
        Scalar pDew = -123.0;
        CompVec liquid{};
        BOOST_REQUIRE_MESSAGE(SatP::dewPressure(vapor, temp, eosType, pDew, liquid),
                              "dew point must be found for z_CO2 = " << vapor[0]);
        BOOST_CHECK_CLOSE(pDew / 1.0e5, expectedBar, 1.0e-3);

        // It is the lower boundary of an ordinary envelope: the bubble point
        // lies above it.
        Scalar pBubble = 0.0;
        CompVec bubbleVapor{};
        BOOST_REQUIRE(SatP::bubblePressure(vapor, temp, eosType, pBubble, bubbleVapor));
        BOOST_CHECK_LT(pDew, pBubble);

        const auto res = equilibriumResidualFor<FluidSystem>(
            vapor, FluidSystem::gasPhaseIdx, liquid, FluidSystem::oilPhaseIdx, pDew, temp);
        BOOST_CHECK_SMALL(res.fugacity, 1.0e-8);
        BOOST_CHECK_SMALL(res.closure, 1.0e-10);
        BOOST_CHECK_GT(res.distance, 1.0e-3);
        // The incipient liquid is the heavier phase.
        BOOST_CHECK_GT(liquid[2], vapor[2]);
    }
}

BOOST_AUTO_TEST_CASE(PropertyProbeOverEosVariantsAndStates)
{
    // A deterministic sweep of the whole input domain.  It asserts the three
    // properties that hold for every input rather than any particular
    // pressure: the call does not throw, a successful result is a usable
    // saturation pressure with a normalised incipient composition, and a
    // failed one leaves both outputs exactly as the caller left them.
    constexpr std::array eosVariants{
        Opm::CompositionalConfig::EOSType::PR,
        Opm::CompositionalConfig::EOSType::PRCORR,
        Opm::CompositionalConfig::EOSType::RK,
        Opm::CompositionalConfig::EOSType::SRK,
    };

    // Below, around and well above the critical temperatures of the mixture.
    constexpr std::array temperatures{
        Scalar{195.0}, Scalar{279.0}, Scalar{363.0},
        Scalar{447.0}, Scalar{531.0}, Scalar{615.0},
    };

    // Pure components, binaries and a zero-free interior, plus a reproducible
    // spread of ternaries from a fixed seed.
    std::vector<CompVec> mixtures{
        {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
        {0.5, 0.5, 0.0}, {0.5, 0.0, 0.5}, {0.0, 0.5, 0.5},
        {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0},
    };
    std::mt19937 gen{20260910};
    std::uniform_real_distribution<Scalar> unit{0.0, 1.0};
    while (mixtures.size() < 50) {
        CompVec z{unit(gen), unit(gen), unit(gen)};
        const Scalar sum = z[0] + z[1] + z[2];
        if (sum <= 0.0) {
            continue;
        }
        for (auto& zi : z) {
            zi /= sum;
        }
        mixtures.push_back(z);
    }

    // A sentinel no search would produce, so any write is visible.
    constexpr Scalar pressSentinel = -12345.0;
    const CompVec compSentinel{-1.0, -2.0, -3.0};

    std::size_t calls = 0, converged = 0;
    for (const auto eos : eosVariants) {
        for (const Scalar temp : temperatures) {
            for (const auto& z : mixtures) {
                for (const bool bubble : {true, false}) {
                    Scalar press = pressSentinel;
                    CompVec incipient = compSentinel;

                    bool ok = false;
                    BOOST_REQUIRE_NO_THROW(
                        ok = bubble ? SatP::bubblePressure(z, temp, eos, press, incipient)
                                    : SatP::dewPressure(z, temp, eos, press, incipient));
                    ++calls;

                    if (!ok) {
                        // The failure contract: nothing was written.
                        BOOST_CHECK_EQUAL(press, pressSentinel);
                        BOOST_CHECK(incipient == compSentinel);
                        continue;
                    }

                    ++converged;
                    BOOST_CHECK(std::isfinite(press));
                    BOOST_CHECK_GT(press, 0.0);

                    Scalar sum = 0.0;
                    for (const Scalar yi : incipient) {
                        BOOST_CHECK(std::isfinite(yi));
                        BOOST_CHECK_GE(yi, 0.0);
                        sum += yi;
                    }
                    BOOST_CHECK_CLOSE(sum, 1.0, 1.0e-6);
                }
            }
        }
    }

    BOOST_CHECK_EQUAL(calls, eosVariants.size() * temperatures.size() * mixtures.size() * 2);
    // The probe is worthless if nothing converges; it is a coverage floor, not
    // a physical claim.
    BOOST_CHECK_GT(converged, calls / 10);
}
