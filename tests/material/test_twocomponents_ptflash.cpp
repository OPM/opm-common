// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA.

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
 * \brief Isothermal (P-T) baseline tests on synthetic fixtures: the PTFlash
 *        behaviour that the P-H (isenthalpic) flash builds on.
 *
 * Cases are data-driven: a FlashCase struct (flashTestFixtures.hpp) carries the
 * full input state of the fluid + run configuration + expected phase outcome;
 * runFlash() executes it and returns a FlashOutcome: a plain-double
 * FlashResult summary plus the final fluid state. Assertions live here, the
 * driver stays Boost-free in the header.
 */
#include "config.h"

#define BOOST_TEST_MODULE TwoComponentsPtFlash
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include "flashTestFixtures.hpp"

#include <array>
#include <cmath>
#include <string>
#include <vector>

using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
using Opm::FlashTest::ExpectedPhase;
using Opm::FlashTest::FlashCase;
using Opm::FlashTest::FlashResult;
using Opm::FlashTest::runFlash;
using Opm::FlashTest::f1Pressure;
using Opm::FlashTest::f1Temperature;
using Opm::FlashTest::f1Z;
using Opm::FlashTest::f2Pressure;
using Opm::FlashTest::f2Temperature;
using Opm::FlashTest::f2Z;

// F1: binary C1/nC10 (the workhorse fixture)
using FluidSystemF1 = Opm::FlashTest::TwoComponentFluidSystem<Scalar>;
constexpr int numComponentsF1 = FluidSystemF1::numComponents;
using EvaluationF1 = Opm::FlashTest::FlashEvaluation<FluidSystemF1>;

// F2: ternary CO2/C1/nC10 on the generic three-component system
using FluidSystemF2 = Opm::FlashTest::F2FluidSystem<Scalar>;
constexpr int numComponentsF2 = FluidSystemF2::numComponents;
using EvaluationF2 = Opm::FlashTest::FlashEvaluation<FluidSystemF2>;

// The F2 components are static: register them once per process.
struct F2Components
{
    F2Components() { Opm::FlashTest::registerF2Components<Scalar>(); }
};

BOOST_GLOBAL_FIXTURE(F2Components);

namespace {

constexpr double INVARIANT_TOLERANCE = 1.e-8;       // absolute, on mass balance / normalization
constexpr double PARITY_TOLERANCE = 1.e-6;          // absolute, between solver methods
constexpr double FUGACITY_TOLERANCE = 1.e-6;        // relative; flash converges the fugacity ratio to 1e-8
constexpr double SINGLE_PHASE_L_TOLERANCE = 1.e-6;  // on the Li-label L in {0,1}

// assert the case's declared phase expectation against the flash result
template <int numComponents>
void checkExpectedPhase(const FlashCase<numComponents>& testCase,
                        const FlashResult<numComponents>& result)
{
    switch (testCase.expected) {
    case ExpectedPhase::two_phase:
        BOOST_CHECK_MESSAGE(!result.single_phase && result.L > 0. && result.L < 1.,
                            testCase.name << ": expected two-phase, got single_phase = "
                                          << result.single_phase << ", L = " << result.L);
        break;
    case ExpectedPhase::single_liquid:
        BOOST_CHECK_MESSAGE(result.single_phase,
                            testCase.name << ": expected single phase, got L = " << result.L);
        // note: BOOST_CHECK_CLOSE tolerance is in percent
        BOOST_CHECK_CLOSE(result.L, 1., SINGLE_PHASE_L_TOLERANCE);
        break;
    case ExpectedPhase::single_vapor:
        BOOST_CHECK_MESSAGE(result.single_phase,
                            testCase.name << ": expected single phase, got L = " << result.L);
        BOOST_CHECK_SMALL(result.L, SINGLE_PHASE_L_TOLERANCE);
        break;
    case ExpectedPhase::any:
        break;
    }
}

// two-phase physical invariants: composition normalization, component mass
// balance against the feed, and equal fugacity across the phases.
// Note: fluid_state.K() is an input seed to PTFlash::solve (the caller-set
// Wilson estimate) and is not updated to the converged K on this state, so it
// must not be checked against y/x here. The true equilibrium condition is
// equal fugacity, evaluated below through the same ParameterCache +
// fugacityCoefficient path PTFlash itself uses.
template <class FluidSystem, class FluidState>
void checkTwoPhaseInvariants(const FluidState& fluid_state,
                             const std::array<double, FluidSystem::numComponents>& z,
                             const EOSType eos_type)
{
    constexpr int numComponents = FluidSystem::numComponents;

    const double L = Opm::getValue(fluid_state.L());
    BOOST_CHECK_MESSAGE(L > 0. && L < 1.,
                        "expected a two-phase split, got L = " << L);

    // The ParameterCache holds one phase's EoS state at a time, so the oil
    // fugacity coefficients must be captured before updatePhase(gas); do not
    // interleave the phases in a refactor.
    using ParamCache = typename FluidSystem::template ParameterCache<typename FluidState::ValueType>;
    ParamCache paramCache(eos_type);
    paramCache.updatePhase(fluid_state, FluidSystem::oilPhaseIdx);
    std::array<double, numComponents> phi_oil;
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        phi_oil[compIdx] = Opm::getValue(
            FluidSystem::fugacityCoefficient(fluid_state, paramCache, FluidSystem::oilPhaseIdx, compIdx));
    }
    paramCache.updatePhase(fluid_state, FluidSystem::gasPhaseIdx);

    double sum_x = 0., sum_y = 0.;
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        const double x_i = Opm::getValue(fluid_state.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
        const double y_i = Opm::getValue(fluid_state.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
        sum_x += x_i;
        sum_y += y_i;

        // mass balance: z_i = L*x_i + (1 - L)*y_i
        BOOST_CHECK_SMALL(L * x_i + (1. - L) * y_i - z[compIdx], INVARIANT_TOLERANCE);

        // equal fugacity: x_i*phi_i^oil = y_i*phi_i^gas (pressure cancels)
        const double phi_gas_i = Opm::getValue(
            FluidSystem::fugacityCoefficient(fluid_state, paramCache, FluidSystem::gasPhaseIdx, compIdx));
        const double f_oil = x_i * phi_oil[compIdx];
        const double f_gas = y_i * phi_gas_i;
        BOOST_CHECK_MESSAGE(std::abs(f_oil / f_gas - 1.) < FUGACITY_TOLERANCE,
                            "component " << compIdx << ": fugacity mismatch, f_oil = "
                                         << f_oil << ", f_gas = " << f_gas);
    }
    BOOST_CHECK_SMALL(sum_x - 1., INVARIANT_TOLERANCE);
    BOOST_CHECK_SMALL(sum_y - 1., INVARIANT_TOLERANCE);
}

} // anonymous namespace

// Two-phase anchor: the split puts the light component in the vapor.
BOOST_AUTO_TEST_CASE(TwoPhaseF1)
{
    FlashCase<numComponentsF1> testCase{"two-phase F1", f1Pressure, f1Temperature, f1Z};
    testCase.expected = ExpectedPhase::two_phase;

    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    const auto& r = outcome.summary;

    checkExpectedPhase(testCase, r);
    // the light component (C1) concentrates in the vapor; the feed sits in between
    BOOST_CHECK_MESSAGE(r.x[0] < testCase.z[0] && testCase.z[0] < r.y[0],
                        "expected x_C1 < z_C1 < y_C1, got x = " << r.x[0] << ", y = " << r.y[0]);
}

// Baseline pressure, low temperature -> single-phase, labeled liquid (L = 1).
// Like the vapor case below, the label follows from Li's pressure-blind
// correlation: T = 200 K < Tc_est = 558.24 K for this feed.
BOOST_AUTO_TEST_CASE(SinglePhaseLiquidF1)
{
    FlashCase<numComponentsF1> testCase{"single-phase liquid F1", f1Pressure, 200., f1Z};
    testCase.expected = ExpectedPhase::single_liquid;

    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    checkExpectedPhase(testCase, outcome.summary);
}

// Low pressure / high temperature -> single-phase vapor (L = 0).
// 600 K is above Li's temperature Tc_est = sum(Vc_i*Tc_i*z_i)/sum(Vc_i*z_i)
// = 558.24 K of the equimolar C1/nC10 feed, and unambiguously vapor at 1 bar.
BOOST_AUTO_TEST_CASE(SinglePhaseVaporF1)
{
    FlashCase<numComponentsF1> testCase{"single-phase vapor F1", 1e5, 600., f1Z};
    testCase.expected = ExpectedPhase::single_vapor;

    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    checkExpectedPhase(testCase, outcome.summary);
}

// A lean feed (10 % C1) at 1 bar and 500 K has two cubic roots, and the vapour
// root has the lower Gibbs energy: the mixture is a vapour, and the liquid
// root is not the stable one.
BOOST_AUTO_TEST_CASE(LeanFeedStableRootIsVapourF1)
{
    constexpr std::array<double, 2> leanZ = {0.1, 0.9};
    auto fs = Opm::FlashTest::makeInitialState<FluidSystemF1, EvaluationF1>(1e5, 500., leanZ);
    for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx) {
        fs.setMoleFraction(FluidSystemF1::oilPhaseIdx, compIdx, leanZ[compIdx]);
        fs.setMoleFraction(FluidSystemF1::gasPhaseIdx, compIdx, leanZ[compIdx]);
    }

    using ParamCache = typename FluidSystemF1::template ParameterCache<EvaluationF1>;
    ParamCache paramCache(EOSType::PR);
    std::array<double, 2> molarVolume{};
    std::array<double, 2> sumZLnPhi{};
    for (const auto phaseIdx : {FluidSystemF1::oilPhaseIdx, FluidSystemF1::gasPhaseIdx}) {
        paramCache.updatePhase(fs, phaseIdx);
        const unsigned rootIdx = phaseIdx == FluidSystemF1::oilPhaseIdx ? 0 : 1;
        molarVolume[rootIdx] = Opm::getValue(paramCache.molarVolume(phaseIdx));
        for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx) {
            sumZLnPhi[rootIdx] += leanZ[compIdx] * std::log(Opm::getValue(
                FluidSystemF1::fugacityCoefficient(fs, paramCache, phaseIdx, compIdx)));
        }
    }

    BOOST_TEST_MESSAGE("lean feed, 1 bar, 500 K: sum z ln(phi) = " << sumZLnPhi[0]
                       << " on the liquid root, " << sumZLnPhi[1] << " on the vapour root");
    BOOST_CHECK_LT(molarVolume[0], 0.5 * molarVolume[1]); // two distinct roots
    BOOST_CHECK_LT(sumZLnPhi[1], sumZLnPhi[0] - 1.);
}

// The lean feed at 1 bar is a vapour from about 447 K, while its cubic keeps a
// liquid root up to about 549 K. The flash has to classify it as a single-phase
// vapour there, with every method.
BOOST_AUTO_TEST_CASE(LeanVapourIsSinglePhaseF1)
{
    constexpr std::array<double, 2> leanZ = {0.1, 0.9};
    for (const auto method : {Opm::PTFlashMethod::Ssi, Opm::PTFlashMethod::Newton,
                              Opm::PTFlashMethod::SsiNewton}) {
        for (const double T : {450., 500., 540.}) {
            FlashCase<numComponentsF1> testCase{"lean vapour F1", 1e5, T, leanZ};
            testCase.method = method;
            testCase.expected = ExpectedPhase::single_vapor;
            BOOST_TEST_CONTEXT("T = " << T << " K, method " << Opm::ptFlashMethodToString(method)) {
                FlashResult<numComponentsF1> result{};
                BOOST_CHECK_NO_THROW(
                    (result = runFlash<FluidSystemF1, EvaluationF1>(testCase).summary));
                checkExpectedPhase(testCase, result);
            }
        }
    }
}

// The equimolar feed at 1 bar is a gas below Li's temperature too: its cubic
// has a single root, about a hundred times the pseudo-critical volume
// sum(z_i*Vc_i) of the feed. The flash labels it vapour with every method.
BOOST_AUTO_TEST_CASE(SingleRootGasIsVapourF1)
{
    for (const auto method : {Opm::PTFlashMethod::Ssi, Opm::PTFlashMethod::Newton,
                              Opm::PTFlashMethod::SsiNewton}) {
        for (const double T : {450., 500., 540.}) {
            FlashCase<numComponentsF1> testCase{"single-root gas F1", 1e5, T, f1Z};
            testCase.method = method;
            testCase.expected = ExpectedPhase::single_vapor;
            BOOST_TEST_CONTEXT("T = " << T << " K, method " << Opm::ptFlashMethodToString(method)) {
                FlashResult<numComponentsF1> result{};
                BOOST_CHECK_NO_THROW(
                    (result = runFlash<FluidSystemF1, EvaluationF1>(testCase).summary));
                checkExpectedPhase(testCase, result);
            }
        }
    }
}

// Nearly pure n-decane at 1 bar and 300 K is a liquid. Its cubic also has a
// vapour root, but the liquid root is the stable one, and the label stays
// liquid with every method.
BOOST_AUTO_TEST_CASE(StableLiquidKeepsLiquidLabelF1)
{
    constexpr std::array<double, 2> decaneZ = {0.001, 0.999};
    for (const auto method : {Opm::PTFlashMethod::Ssi, Opm::PTFlashMethod::Newton,
                              Opm::PTFlashMethod::SsiNewton}) {
        FlashCase<numComponentsF1> testCase{"nearly pure n-decane F1", 1e5, 300., decaneZ};
        testCase.method = method;
        testCase.expected = ExpectedPhase::single_liquid;
        BOOST_TEST_CONTEXT("method " << Opm::ptFlashMethodToString(method)) {
            FlashResult<numComponentsF1> result{};
            BOOST_CHECK_NO_THROW(
                (result = runFlash<FluidSystemF1, EvaluationF1>(testCase).summary));
            checkExpectedPhase(testCase, result);
        }
    }
}

// Two-phase split invariants: normalization, mass balance, equal fugacity
BOOST_AUTO_TEST_CASE(TwoPhaseInvariantsF1)
{
    FlashCase<numComponentsF1> testCase{"two-phase invariants F1", f1Pressure, f1Temperature, f1Z};
    testCase.expected = ExpectedPhase::two_phase;

    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    checkExpectedPhase(testCase, outcome.summary);
    checkTwoPhaseInvariants<FluidSystemF1>(outcome.state, testCase.z, testCase.eos_type);
}

// ssi / newton / ssi+newton converge to the same solution
BOOST_AUTO_TEST_CASE(MethodParityF1)
{
    const std::vector<Opm::PTFlashMethod> methods {Opm::PTFlashMethod::Ssi,
                                                   Opm::PTFlashMethod::Newton,
                                                   Opm::PTFlashMethod::SsiNewton};

    std::vector<FlashResult<numComponentsF1>> results;
    for (const auto& method : methods) {
        FlashCase<numComponentsF1> testCase{"parity F1 ["
                                            + std::string{Opm::ptFlashMethodToString(method)} + "]",
                                            f1Pressure, f1Temperature, f1Z};
        testCase.method = method;
        testCase.expected = ExpectedPhase::two_phase;

        const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
        checkExpectedPhase(testCase, outcome.summary);
        results.push_back(outcome.summary);
    }

    for (std::size_t m = 1; m < methods.size(); ++m) {
        BOOST_CHECK_MESSAGE(std::abs(results[m].L - results[0].L) < PARITY_TOLERANCE,
                            methods[m] << " vs " << methods[0] << ": L differs: "
                                       << results[m].L << " vs " << results[0].L);
        for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx) {
            BOOST_CHECK_SMALL(results[m].x[compIdx] - results[0].x[compIdx], PARITY_TOLERANCE);
            BOOST_CHECK_SMALL(results[m].y[compIdx] - results[0].y[compIdx], PARITY_TOLERANCE);
        }
    }
}

// Ternary (F2) split satisfies the same invariants at the fixture's canonical
// two-phase point; z ordering follows the F2 registration order
// (Comp0 = CO2, Comp1 = C1, Comp2 = nC10).
BOOST_AUTO_TEST_CASE(TernaryInvariantsF2)
{
    FlashCase<numComponentsF2> testCase{"two-phase invariants F2", f2Pressure, f2Temperature, f2Z};
    testCase.expected = ExpectedPhase::two_phase;

    const auto outcome = runFlash<FluidSystemF2, EvaluationF2>(testCase);
    checkExpectedPhase(testCase, outcome.summary);
    checkTwoPhaseInvariants<FluidSystemF2>(outcome.state, testCase.z, testCase.eos_type);
}
