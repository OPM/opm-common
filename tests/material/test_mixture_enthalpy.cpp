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
 * \brief Tests for the compositional caloric data and enthalpy machinery.
 *
 * CaloricData pins the per-component identity cards (the ideal-gas
 * heat-capacity polynomials on the component classes) against external
 * reference values; CaloricModel and DepartureModel test the mixture
 * enthalpy the compositional path evaluates on top of them.
 */
#include "config.h"

#define BOOST_TEST_MODULE MixtureEnthalpy
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/IdealGasCaloricData.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include "flashTestFixtures.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

using Scalar = double;
using Opm::FlashTest::FlashCase;
using Opm::FlashTest::runFlash;
using Opm::FlashTest::f1Pressure;
using Opm::FlashTest::f1Z;

// F1: binary C1/nC10
using FluidSystemF1 = Opm::FlashTest::TwoComponentFluidSystem<Scalar>;
constexpr int numComponentsF1 = FluidSystemF1::numComponents;
using EvaluationF1 = Opm::FlashTest::FlashEvaluation<FluidSystemF1>;
using EnthalpyF1 = Opm::MixtureEnthalpy<Scalar, FluidSystemF1>;

namespace {

const Scalar T0 = Opm::IdealGasCaloricData<Scalar>::referenceTemperature();

// unchecked probe helper: flash F1 at (P, T) and return the mixture enthalpy
// of the flashed state. Deliberately assertion-free — the calling test owns
// its expectations; do not bolt checks in here.
double mixtureEnthalpyAt(const double pressure, const double temperature)
{
    FlashCase<numComponentsF1> testCase{"enthalpy probe", pressure, temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    return Opm::getValue(EnthalpyF1::mixtureEnthalpy(outcome.state, Opm::FlashTest::f1CpTable(), T0));
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(CaloricData)

// The caloric equations (ComponentCp), species-blind: enthalpy is the exact
// integral of the heat-capacity polynomial, zero at the reference datum.
BOOST_AUTO_TEST_CASE(EnthalpyIntegralMatchesHeatCapacity)
{
    const Opm::ComponentCp<double> cp = Opm::C1<double>::idealGasHeatCapacityPolynomial();
    const double T0 = 298.15;

    // h(T0) = 0 by construction
    BOOST_CHECK_EQUAL(cp.enthalpyIntegral(T0, T0), 0.0);

    // dh/dT == cp(T), checked against a central finite difference
    const double T = 400.;
    const double dT = 1e-3;
    const double dhdT = (cp.enthalpyIntegral(T + dT, T0) - cp.enthalpyIntegral(T - dT, T0)) / (2. * dT);
    BOOST_CHECK_CLOSE(dhdT, cp.heatCapacity(T), 1e-6); // [%]
}

// The identity cards against EXTERNAL reference values — the one check no
// amount of internal consistency can substitute: a self-consistent test
// manufactures its expectation from the same coefficients it verifies, so a
// corrupt row passes it. The pinned values are the reference-EoS ideal-gas
// heat capacities (Setzmann & Wagner 1991 methane; Lemmon & Span 2006
// n-decane; Span & Wagner 1996 CO2), tabulated at four temperatures spanning
// the fit window.
BOOST_AUTO_TEST_CASE(CardsMatchReferenceIdealGasCp)
{
    constexpr std::array<double, 4> temps{298.15, 350., 450., 600.};
    const struct {
        const char* name;
        Opm::ComponentCp<double> card;
        std::array<double, 4> cpRef; // [J/(mol K)] at temps[]
    } pins[] = {
        {"methane", Opm::C1<double>::idealGasHeatCapacityPolynomial(),
         {35.7085, 37.9628, 43.5052, 52.4919}},
        {"decane", Opm::C10<double>::idealGasHeatCapacityPolynomial(),
         {233.0250, 266.2081, 328.2699, 405.7329}},
        {"carbonDioxide", Opm::SimpleCO2<double>::idealGasHeatCapacityPolynomial(),
         {37.1408, 39.3941, 43.0700, 47.3303}},
    };

    for (const auto& pin : pins) {
        for (std::size_t i = 0; i < temps.size(); ++i) {
            BOOST_TEST_CONTEXT(pin.name << " at T = " << temps[i]) {
                // 1% relative: an order looser than the fit residuals (max
                // 0.3%), an order tighter than the defect class it guards
                BOOST_CHECK_CLOSE(pin.card.heatCapacity(temps[i]),
                                  pin.cpRef[i], 1.0); // [%]
            }
        }
    }
}

// The triple points added on the identity cards, against the reference-EoS
// fluid values (papers cited on the classes).
BOOST_AUTO_TEST_CASE(TriplePointsMatchReferenceFluids)
{
    BOOST_CHECK_CLOSE(Opm::C1<double>::tripleTemperature(), 90.6941, 1e-6);
    BOOST_CHECK_CLOSE(Opm::C1<double>::triplePressure(), 11696.06, 0.01);
    BOOST_CHECK_CLOSE(Opm::C10<double>::tripleTemperature(), 243.5, 1e-6);
    BOOST_CHECK_CLOSE(Opm::C10<double>::triplePressure(), 1.4042, 0.02);
}

BOOST_AUTO_TEST_SUITE_END() // CaloricData

BOOST_AUTO_TEST_SUITE(CaloricModel)

// Enthalpy is strictly increasing in temperature (Cp > 0). The sweep
// deliberately crosses phase-regime boundaries: with the caloric model H is
// monotone irrespective of how the split changes along the way.
BOOST_AUTO_TEST_CASE(MonotoneInTemperature)
{
    const std::array<double, 4> temperatures = {250., 300., 350., 400.};

    double previous = mixtureEnthalpyAt(f1Pressure, temperatures[0]);
    for (std::size_t i = 1; i < temperatures.size(); ++i) {
        const double current = mixtureEnthalpyAt(f1Pressure, temperatures[i]);
        BOOST_CHECK_MESSAGE(current > previous,
                            "H not monotone: H(" << temperatures[i] << ") = " << current
                                                 << " <= H(" << temperatures[i-1] << ") = " << previous);
        previous = current;
    }
}

// The reference datum: H(T0) = 0, regardless of the phase split
BOOST_AUTO_TEST_CASE(ReferenceDatum)
{
    const double H0 = mixtureEnthalpyAt(f1Pressure, T0);
    BOOST_CHECK_SMALL(H0, 1e-12);
}

// Analytic mixtureCp matches a central finite difference of the mixture
// enthalpy. Note the physics of WHY they may be compared at all: the FD probe
// re-flashes at T±h (a total derivative, split re-equilibrated) while
// mixtureCp is a frozen-split partial derivative — they agree only because
// the caloric H is flash-independent. Do NOT reuse this check unchanged for a
// departure-mode enthalpy, where the two derivatives legitimately differ.
BOOST_AUTO_TEST_CASE(CpVersusFiniteDifference)
{
    constexpr double T = 300.;
    constexpr double h = 1e-3; // [K]

    FlashCase<numComponentsF1> testCase{"cp probe", f1Pressure, T, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    const double cpAnalytic = Opm::getValue(
        EnthalpyF1::mixtureCp(outcome.state, Opm::FlashTest::f1CpTable()));

    const double cpFD = (mixtureEnthalpyAt(f1Pressure, T + h)
                         - mixtureEnthalpyAt(f1Pressure, T - h)) / (2.*h);

    BOOST_CHECK_CLOSE(cpAnalytic, cpFD, 1e-6); // [%]
}

// Phase-decomposed mixture enthalpy equals the feed-direct sum
// sum_i z_i*h_i(T). With the caloric model the phase split cancels exactly by
// material balance — this pins the L/x/y weighting (indexing) of the
// implementation and documents that caloric H is flash-independent.
BOOST_AUTO_TEST_CASE(PhaseDecompositionMatchesFeedSum)
{
    constexpr double T = 340.;

    FlashCase<numComponentsF1> testCase{"decomposition probe", f1Pressure, T, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    // the cancellation is only non-trivially tested on a genuine two-phase split
    BOOST_REQUIRE(!outcome.summary.single_phase);
    BOOST_REQUIRE(outcome.summary.L > 0. && outcome.summary.L < 1.);

    const auto cpTable = Opm::FlashTest::f1CpTable();
    const double viaPhases = Opm::getValue(EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable, T0));

    double viaFeed = 0.;
    for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
        viaFeed += f1Z[compIdx] * cpTable[compIdx].enthalpyIntegral(T, T0);

    BOOST_CHECK_CLOSE(viaPhases, viaFeed, 1e-9); // [%]
}

// The caloric coefficients live on the component classes (the species'
// identity card, next to its EoS constants); the IdealGasCaloricData presets
// are delegating wrappers over them. Pin the delegation coefficient-wise so
// the two surfaces can never drift apart.
BOOST_AUTO_TEST_CASE(DelegationMatchesCards)
{
    using Caloric = Opm::IdealGasCaloricData<double>;

    const auto checkSame = [](const char* name,
                              const Opm::ComponentCp<double>& wrapper,
                              const Opm::ComponentCp<double>& owner) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_CHECK_EQUAL(wrapper.c0, owner.c0);
            BOOST_CHECK_EQUAL(wrapper.c1, owner.c1);
            BOOST_CHECK_EQUAL(wrapper.c2, owner.c2);
            BOOST_CHECK_EQUAL(wrapper.c3, owner.c3);
        }
    };
    checkSame("methane", Caloric::methane(),
              Opm::C1<double>::idealGasHeatCapacityPolynomial());
    checkSame("decane", Caloric::decane(),
              Opm::C10<double>::idealGasHeatCapacityPolynomial());
    checkSame("carbonDioxide", Caloric::carbonDioxide(),
              Opm::SimpleCO2<double>::idealGasHeatCapacityPolynomial());
}

BOOST_AUTO_TEST_SUITE_END() // CaloricModel

// ────────────────────────────────────────────────────────────────────────────
// EoS-consistent departure (residual) enthalpy:
// H_res = -R*T^2 * sum_i w_i * dln(phi_i)/dT per phase, with the temperature
// derivative of the fugacity coefficient supplied by densead AD through the
// fluid system's own fugacityCoefficient. This is the model under which the
// enthalpy genuinely depends on the flash result (the caloric cancellation
// tested above no longer holds).
// ────────────────────────────────────────────────────────────────────────────
BOOST_AUTO_TEST_SUITE(DepartureModel)

namespace {
using EOSType = Opm::CompositionalConfig::EOSType;
}

// The AD temperature-derivative of ln(phi) matches a central finite
// difference of ln(phi(T)) evaluated at fixed pressure and frozen phase
// composition — per phase, per component.
BOOST_AUTO_TEST_CASE(AdDerivativeMatchesFiniteDifference)
{
    FlashCase<numComponentsF1> testCase{"AD probe", f1Pressure,
                                        Opm::FlashTest::f1Temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    BOOST_REQUIRE(!outcome.summary.single_phase);

    constexpr double h = 1e-3; // FD step [K]
    for (unsigned phaseIdx : {static_cast<unsigned>(FluidSystemF1::oilPhaseIdx),
                              static_cast<unsigned>(FluidSystemF1::gasPhaseIdx)}) {
        // freeze this phase's composition from the flashed state
        std::array<double, numComponentsF1> w;
        for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
            w[compIdx] = Opm::getValue(outcome.state.moleFraction(phaseIdx, compIdx));

        // independent, scalar evaluation path for ln(phi(T)) at fixed (P, w)
        auto lnPhi = [&](const double T, const int compIdx) {
            Opm::CompositionalFluidState<double, FluidSystemF1> fs;
            fs.setTemperature(T);
            fs.setPressure(FluidSystemF1::oilPhaseIdx, f1Pressure);
            fs.setPressure(FluidSystemF1::gasPhaseIdx, f1Pressure);
            for (int i = 0; i < numComponentsF1; ++i)
                fs.setMoleFraction(phaseIdx, i, w[i]);
            FluidSystemF1::ParameterCache<double> paramCache(EOSType::PR);
            paramCache.updatePhase(fs, phaseIdx);
            return std::log(FluidSystemF1::fugacityCoefficient(fs, paramCache, phaseIdx, compIdx));
        };

        const double T = Opm::FlashTest::f1Temperature;
        for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx) {
            const double ad = EnthalpyF1::phaseDLnPhiDT(outcome.state, phaseIdx, compIdx, EOSType::PR);
            const double fd = (lnPhi(T + h, compIdx) - lnPhi(T - h, compIdx)) / (2.*h);
            BOOST_CHECK_CLOSE(ad, fd, 1e-3); // [%]
        }
    }
}

// Ideal-gas limit: the gas-phase residual vanishes as P -> 0
BOOST_AUTO_TEST_CASE(IdealGasLimit)
{
    FlashCase<numComponentsF1> lowP{"near-vacuum probe", 1e3, Opm::FlashTest::f1Temperature, f1Z};
    FlashCase<numComponentsF1> anchor{"anchor probe", f1Pressure, Opm::FlashTest::f1Temperature, f1Z};

    const auto lowOutcome = runFlash<FluidSystemF1, EvaluationF1>(lowP);
    const auto anchorOutcome = runFlash<FluidSystemF1, EvaluationF1>(anchor);

    const double resLow = EnthalpyF1::phaseResidualEnthalpy(
        lowOutcome.state, FluidSystemF1::gasPhaseIdx, EOSType::PR);
    const double resAnchor = EnthalpyF1::phaseResidualEnthalpy(
        anchorOutcome.state, FluidSystemF1::gasPhaseIdx, EOSType::PR);

    BOOST_CHECK_LT(std::abs(resLow), 10.);   // [J/mol] — near-ideal at 10 mbar
    BOOST_CHECK_LT(std::abs(resLow), 0.05 * std::abs(resAnchor));
}

// With the departure term the enthalpy couples to the split: the L-weighted
// phase decomposition still holds, but the caloric feed-sum identity breaks,
// and the liquid residual is attractive (vaporization-enthalpy scale).
BOOST_AUTO_TEST_CASE(DepartureCouplesToSplit)
{
    FlashCase<numComponentsF1> testCase{"departure anchor", f1Pressure,
                                        Opm::FlashTest::f1Temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    BOOST_REQUIRE(!outcome.summary.single_phase);
    BOOST_REQUIRE(outcome.summary.L > 0. && outcome.summary.L < 1.);

    const auto cpTable = Opm::FlashTest::f1CpTable();

    // (a) decomposition consistency of the model-switch overload
    const double viaMixture = Opm::getValue(EnthalpyF1::mixtureEnthalpy(
        outcome.state, cpTable, T0, EOSType::PR, Opm::EnthalpyModel::eos_departure));
    const double L = outcome.summary.L;
    const double hOil = Opm::getValue(EnthalpyF1::phaseEnthalpy(
                            outcome.state, FluidSystemF1::oilPhaseIdx, cpTable, T0))
        + EnthalpyF1::phaseResidualEnthalpy(outcome.state, FluidSystemF1::oilPhaseIdx, EOSType::PR);
    const double hGas = Opm::getValue(EnthalpyF1::phaseEnthalpy(
                            outcome.state, FluidSystemF1::gasPhaseIdx, cpTable, T0))
        + EnthalpyF1::phaseResidualEnthalpy(outcome.state, FluidSystemF1::gasPhaseIdx, EOSType::PR);
    BOOST_CHECK_CLOSE(viaMixture, L*hOil + (1. - L)*hGas, 1e-9); // [%]

    // (b) the caloric cancellation is broken: departure H differs from the
    // feed-direct ideal sum by far more than any tolerance
    double viaFeedIdeal = 0.;
    for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
        viaFeedIdeal += f1Z[compIdx] * cpTable[compIdx].enthalpyIntegral(
            Opm::FlashTest::f1Temperature, T0);
    BOOST_CHECK_GT(std::abs(viaMixture - viaFeedIdeal), 100.); // [J/mol]

    // (c) the liquid's residual is negative (attractive interactions)
    BOOST_CHECK_LT(EnthalpyF1::phaseResidualEnthalpy(
        outcome.state, FluidSystemF1::oilPhaseIdx, EOSType::PR), 0.);
}

// The caloric seam is untouched: the model-switch overload with
// EnthalpyModel::caloric reproduces the original caloric overload exactly.
BOOST_AUTO_TEST_CASE(CaloricSeamUnchanged)
{
    FlashCase<numComponentsF1> testCase{"seam probe", f1Pressure,
                                        Opm::FlashTest::f1Temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    const auto cpTable = Opm::FlashTest::f1CpTable();

    const double viaCaloric = Opm::getValue(
        EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable, T0));
    const double viaSwitch = Opm::getValue(EnthalpyF1::mixtureEnthalpy(
        outcome.state, cpTable, T0, EOSType::PR, Opm::EnthalpyModel::caloric));
    BOOST_CHECK_CLOSE(viaCaloric, viaSwitch, 1e-12); // [%]
}

// the shared enthalpy-model parser: round-trips and loud failure
BOOST_AUTO_TEST_CASE(EnthalpyModelStrings)
{
    BOOST_CHECK(Opm::enthalpyModelFromString("caloric") == Opm::EnthalpyModel::caloric);
    BOOST_CHECK(Opm::enthalpyModelFromString("eos_departure") == Opm::EnthalpyModel::eos_departure);
    BOOST_CHECK_EQUAL(Opm::enthalpyModelToString(Opm::EnthalpyModel::caloric), "caloric");
    BOOST_CHECK_EQUAL(Opm::enthalpyModelToString(Opm::EnthalpyModel::eos_departure), "eos_departure");
    BOOST_CHECK_THROW(Opm::enthalpyModelFromString("nonsense"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END() // DepartureModel
