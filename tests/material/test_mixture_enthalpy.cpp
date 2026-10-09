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
 * CaloricData pins the ideal-gas heat-capacity polynomials on the component
 * classes against external reference values; CaloricModel and DepartureModel
 * test the mixture enthalpy the compositional path evaluates on top of them.
 */
#include "config.h"

#define BOOST_TEST_MODULE MixtureEnthalpy
#include <boost/test/unit_test.hpp>

#include <opm/material/Constants.hpp>
#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include "flashTestFixtures.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

using Scalar = double;
using Opm::FlashTest::FlashCase;
using Opm::FlashTest::runFlash;
using Opm::FlashTest::f1Pressure;
using Opm::FlashTest::f1Z;
using Opm::FlashTest::h1Pressure;
using Opm::FlashTest::h1Z;

// F1: binary C1/nC10
using FluidSystemF1 = Opm::FlashTest::TwoComponentFluidSystem<Scalar>;
constexpr int numComponentsF1 = FluidSystemF1::numComponents;
using EvaluationF1 = Opm::FlashTest::FlashEvaluation<FluidSystemF1>;
using EnthalpyF1 = Opm::MixtureEnthalpy<Scalar, FluidSystemF1>;

// H1: binary C1/nC20 with volume shifts, on the generic fluid system
using FluidSystemH1 = Opm::FlashTest::H1FluidSystem<Scalar>;
constexpr int numComponentsH1 = FluidSystemH1::numComponents;
using EvaluationH1 = Opm::FlashTest::FlashEvaluation<FluidSystemH1>;
using EnthalpyH1 = Opm::MixtureEnthalpy<Scalar, FluidSystemH1>;

// The H1 components are static: register them once per process.
struct H1Components
{
    H1Components() { Opm::FlashTest::registerH1Components<Scalar>(); }
};

BOOST_GLOBAL_FIXTURE(H1Components);

namespace {

using EOSType = Opm::CompositionalConfig::EOSType;

const Scalar T0 = Opm::ComponentCp<Scalar>::referenceTemperature();

// unchecked probe helper: flash F1 at (P, T) and return the mixture enthalpy
// of the flashed state. Deliberately assertion-free: the calling test owns
// its expectations.
double mixtureEnthalpyAt(const double pressure, const double temperature)
{
    FlashCase<numComponentsF1> testCase{"enthalpy probe", pressure, temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    return Opm::getValue(EnthalpyF1::mixtureEnthalpy(outcome.state, Opm::FlashTest::f1CpTable()));
}

// the composition of one phase of a flashed state, as plain doubles
template <class FluidSystem, class FluidState>
std::array<double, FluidSystem::numComponents>
phaseComposition(const FluidState& state, const unsigned phaseIdx)
{
    std::array<double, FluidSystem::numComponents> w;
    for (int compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx)
        w[compIdx] = Opm::getValue(state.moleFraction(phaseIdx, compIdx));
    return w;
}

// one phase at (T, p, w) as a plain-double state, with its parameter cache
// updated for the given equation of state
template <class FluidSystem>
struct PhaseProbe
{
    Opm::CompositionalFluidState<double, FluidSystem> fs;
    typename FluidSystem::template ParameterCache<double> paramCache;

    PhaseProbe(const double T, const double p,
               const std::array<double, FluidSystem::numComponents>& w,
               const unsigned phaseIdx, const EOSType eosType)
        : paramCache(eosType)
    {
        fs.setTemperature(T);
        fs.setPressure(FluidSystem::oilPhaseIdx, p);
        fs.setPressure(FluidSystem::gasPhaseIdx, p);
        for (int compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx)
            fs.setMoleFraction(phaseIdx, compIdx, w[compIdx]);
        paramCache.updatePhase(fs, phaseIdx);
    }
};

// -R T^2 sum_i w_i dln(phi_i)/dT at fixed (p, w), with the temperature
// derivative of lnPhi(T), an array of ln(phi_i), as a central finite difference
template <int numComponents, class LnPhi>
double residualByFiniteDifference(const double T,
                                  const std::array<double, numComponents>& w,
                                  const LnPhi& lnPhi)
{
    constexpr double h = 1e-3; // [K]
    const auto plus = lnPhi(T + h);
    const auto minus = lnPhi(T - h);
    double sum = 0.;
    for (int compIdx = 0; compIdx < numComponents; ++compIdx)
        sum += w[compIdx] * (plus[compIdx] - minus[compIdx]) / (2.*h);
    return -Opm::Constants<double>::R * T * T * sum;
}

// ln(phi_i) of every component from the fluid system's fugacityCoefficient
template <class FluidSystem>
std::array<double, FluidSystem::numComponents>
fluidSystemLnPhi(const double T, const double p,
                 const std::array<double, FluidSystem::numComponents>& w,
                 const unsigned phaseIdx, const EOSType eosType)
{
    PhaseProbe<FluidSystem> probe(T, p, w, phaseIdx, eosType);
    std::array<double, FluidSystem::numComponents> lnPhi;
    for (int compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
        lnPhi[compIdx] = std::log(FluidSystem::fugacityCoefficient(probe.fs, probe.paramCache,
                                                                   phaseIdx, compIdx));
    }
    return lnPhi;
}

// ln(phi_i) of every component of the cubic, evaluated here from its A, B,
// B_i, A_ij and Z, without the range CubicEOS limits phi_i to. The volume
// translation of the fluid system is not included.
template <class FluidSystem>
std::array<double, FluidSystem::numComponents>
cubicLnPhi(const PhaseProbe<FluidSystem>& probe,
           const std::array<double, FluidSystem::numComponents>& w,
           const unsigned phaseIdx)
{
    const auto& pc = probe.paramCache;
    const double T = probe.fs.temperature(phaseIdx);
    const double p = probe.fs.pressure(phaseIdx);
    const double A = pc.A(phaseIdx);
    const double B = pc.B(phaseIdx);
    const double m1 = pc.m1(phaseIdx);
    const double m2 = pc.m2(phaseIdx);
    const double Z = p * pc.molarVolume(phaseIdx) / (Opm::Constants<double>::R * T);

    std::array<double, FluidSystem::numComponents> lnPhi;
    for (int i = 0; i < FluidSystem::numComponents; ++i) {
        double sumA = 0.;
        for (int j = 0; j < FluidSystem::numComponents; ++j)
            sumA += pc.aCache(phaseIdx, i, j) * w[j];
        const double Bi = pc.Bi(phaseIdx, i);
        lnPhi[i] = Bi/B * (Z - 1.) - std::log(Z - B)
                 + A/((m1 - m2)*B) * std::log((Z + m2*B)/(Z + m1*B)) * (2.*sumA/A - Bi/B);
    }
    return lnPhi;
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(CaloricData)

// The caloric equations (ComponentCp), species-blind: enthalpy is the exact
// integral of the heat-capacity polynomial, zero at the reference datum.
BOOST_AUTO_TEST_CASE(EnthalpyIntegralMatchesHeatCapacity)
{
    const Opm::ComponentCp<double> cp = Opm::C1<double>::idealGasHeatCapacityPolynomial();

    // h(T0) = 0 by construction
    BOOST_CHECK_EQUAL(cp.enthalpyIntegral(T0), 0.0);

    // dh/dT == cp(T), checked against a central finite difference
    const double T = 400.;
    const double dT = 1e-3;
    const double dhdT = (cp.enthalpyIntegral(T + dT) - cp.enthalpyIntegral(T - dT)) / (2. * dT);
    BOOST_CHECK_CLOSE(dhdT, cp.heatCapacity(T), 1e-6); // [%]
}

// The cp polynomials against external reference values, the one check that
// internal consistency cannot replace: a self-consistent test derives its
// expectation from the same coefficients it verifies, so a corrupt row
// passes it. The pinned values are the ideal-gas heat capacities of the
// reference equations of state (Setzmann & Wagner 1991 methane; Lemmon &
// Span 2006 n-decane; Span & Wagner 1996 CO2) at four temperatures spanning
// the fit range.
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

// Each polynomial carries the temperature range of its fit. Inside it the
// heat capacity is positive and increasing; outside it the cubics turn over.
BOOST_AUTO_TEST_CASE(CardsCarryTheirFitRange)
{
    const struct {
        const char* name;
        Opm::ComponentCp<double> card;
    } cards[] = {
        {"methane", Opm::C1<double>::idealGasHeatCapacityPolynomial()},
        {"decane", Opm::C10<double>::idealGasHeatCapacityPolynomial()},
        {"carbonDioxide", Opm::SimpleCO2<double>::idealGasHeatCapacityPolynomial()},
    };

    for (const auto& c : cards) {
        BOOST_TEST_CONTEXT(c.name) {
            BOOST_CHECK_EQUAL(c.card.minTemperature, 250.);
            BOOST_CHECK_EQUAL(c.card.maxTemperature, 600.);

            double previous = c.card.heatCapacity(c.card.minTemperature);
            BOOST_CHECK_GT(previous, 0.);
            for (double T = c.card.minTemperature + 10.; T <= c.card.maxTemperature; T += 10.) {
                const double current = c.card.heatCapacity(T);
                BOOST_CHECK_MESSAGE(current > previous,
                                    "cp not increasing at T = " << T);
                previous = current;
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // CaloricData

BOOST_AUTO_TEST_SUITE(CaloricModel)

// Enthalpy is strictly increasing in temperature (cp > 0) over the common fit
// range of the cp polynomials. The sweep crosses phase-regime boundaries:
// with the caloric model H is monotone however the split changes on the way.
BOOST_AUTO_TEST_CASE(MonotoneInTemperature)
{
    const auto cpTable = Opm::FlashTest::f1CpTable();
    double Tmin = cpTable[0].minTemperature;
    double Tmax = cpTable[0].maxTemperature;
    for (const auto& card : cpTable) {
        Tmin = std::max(Tmin, card.minTemperature);
        Tmax = std::min(Tmax, card.maxTemperature);
    }

    double previous = mixtureEnthalpyAt(f1Pressure, Tmin);
    for (double T = Tmin + 50.; T <= Tmax; T += 50.) {
        const double current = mixtureEnthalpyAt(f1Pressure, T);
        BOOST_CHECK_MESSAGE(current > previous,
                            "H not monotone: H(" << T << ") = " << current
                                                 << " <= H(" << T - 50. << ") = " << previous);
        previous = current;
    }
}

// The datum under the caloric model: H(T0) = 0, regardless of the phase split
// (under eos_departure it is not zero; see DepartureDatumIsIdealGasAtT0)
BOOST_AUTO_TEST_CASE(ReferenceDatum)
{
    const double H0 = mixtureEnthalpyAt(f1Pressure, T0);
    BOOST_CHECK_SMALL(H0, 1e-12);
}

// Analytic mixtureCp matches a central finite difference of the mixture
// enthalpy. The two may be compared only because the caloric H does not
// depend on the flash: the finite difference re-flashes at T+-h (a total
// derivative, split re-equilibrated), while mixtureCp is a partial derivative
// at a frozen split. Under the departure model the two legitimately differ.
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
// material balance; this pins the L/x/y weighting (indexing) of the
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
    const double viaPhases = Opm::getValue(EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable));

    double viaFeed = 0.;
    for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
        viaFeed += f1Z[compIdx] * cpTable[compIdx].enthalpyIntegral(T);

    BOOST_CHECK_CLOSE(viaPhases, viaFeed, 1e-9); // [%]
}

BOOST_AUTO_TEST_SUITE_END() // CaloricModel

// EoS-consistent departure (residual) enthalpy:
// H_res = -R*T^2 * d/dT [sum_i w_i * ln(phi_i)] per phase. This is the model
// under which the enthalpy depends on the flash result (the caloric
// cancellation tested above no longer holds).
BOOST_AUTO_TEST_SUITE(DepartureModel)

// Where no fugacity coefficient is at the range limit of CubicEOS, the
// residual equals -R T^2 sum_i w_i dln(phi_i)/dT taken from the fluid
// system's own fugacityCoefficient (finite difference at fixed P and phase
// composition). F1 is checked for every cubic EoS; the flashed state is only
// a probe point there. H1 adds the volume shift, which enters ln(phi_i), and a
// component with an acentric factor above 0.49, where PRCORR differs from PR.
BOOST_AUTO_TEST_CASE(ResidualMatchesFugacityCoefficients)
{
    constexpr double T = Opm::FlashTest::f1Temperature;

    FlashCase<numComponentsF1> caseF1{"F1 probe", f1Pressure, T, f1Z};
    const auto outcomeF1 = runFlash<FluidSystemF1, EvaluationF1>(caseF1);
    BOOST_REQUIRE(!outcomeF1.summary.single_phase);

    for (const auto eosType : {EOSType::PR, EOSType::PRCORR,
                               EOSType::SRK, EOSType::RK}) {
        for (unsigned phaseIdx : {static_cast<unsigned>(FluidSystemF1::oilPhaseIdx),
                                  static_cast<unsigned>(FluidSystemF1::gasPhaseIdx)}) {
            BOOST_TEST_CONTEXT("F1, EoS " << Opm::CompositionalConfig::eosTypeToString(eosType)
                               << ", phase " << phaseIdx) {
                const auto w = phaseComposition<FluidSystemF1>(outcomeF1.state, phaseIdx);
                const double expected = residualByFiniteDifference<numComponentsF1>(T, w,
                    [&](const double t) {
                        return fluidSystemLnPhi<FluidSystemF1>(t, f1Pressure, w, phaseIdx, eosType);
                    });
                BOOST_CHECK_CLOSE(EnthalpyF1::phaseResidualEnthalpy(outcomeF1.state, phaseIdx, eosType),
                                  expected, 1e-4); // [%]
            }
        }
    }

    for (const auto eosType : {EOSType::PR, EOSType::PRCORR}) {
        FlashCase<numComponentsH1> caseH1{"H1 probe", h1Pressure, T, h1Z};
        caseH1.eos_type = eosType;
        const auto outcomeH1 = runFlash<FluidSystemH1, EvaluationH1>(caseH1);
        BOOST_REQUIRE(!outcomeH1.summary.single_phase);

        for (unsigned phaseIdx : {static_cast<unsigned>(FluidSystemH1::oilPhaseIdx),
                                  static_cast<unsigned>(FluidSystemH1::gasPhaseIdx)}) {
            BOOST_TEST_CONTEXT("H1, EoS " << Opm::CompositionalConfig::eosTypeToString(eosType)
                               << ", phase " << phaseIdx) {
                const auto w = phaseComposition<FluidSystemH1>(outcomeH1.state, phaseIdx);
                const double expected = residualByFiniteDifference<numComponentsH1>(T, w,
                    [&](const double t) {
                        return fluidSystemLnPhi<FluidSystemH1>(t, h1Pressure, w, phaseIdx, eosType);
                    });
                BOOST_CHECK_CLOSE(EnthalpyH1::phaseResidualEnthalpy(outcomeH1.state, phaseIdx, eosType),
                                  expected, 1e-4); // [%]
            }
        }
    }
}

// In the H1 liquid at 250 K the fugacity coefficient of nC20 is below the
// range CubicEOS limits it to, so the limited value has no temperature
// derivative. The residual must still match ln(phi_i) evaluated without the
// limit (with the volume translation of the fluid system). Through the
// limited coefficients it came out at a few kJ/mol instead of about -60 kJ/mol.
BOOST_AUTO_TEST_CASE(HeavyComponentBelowFugacityLimit)
{
    constexpr double T = 250.;
    constexpr unsigned oilPhaseIdx = FluidSystemH1::oilPhaseIdx;
    constexpr unsigned heavyIdx = 1;

    for (const auto eosType : {EOSType::PR, EOSType::PRCORR}) {
        BOOST_TEST_CONTEXT("EoS " << Opm::CompositionalConfig::eosTypeToString(eosType)) {
            FlashCase<numComponentsH1> testCase{"H1 heavy liquid", h1Pressure, T, h1Z};
            testCase.eos_type = eosType;
            const auto outcome = runFlash<FluidSystemH1, EvaluationH1>(testCase);
            BOOST_REQUIRE(!outcome.summary.single_phase);
            const auto x = phaseComposition<FluidSystemH1>(outcome.state, oilPhaseIdx);

            // the case is in the limited regime
            const PhaseProbe<FluidSystemH1> probe(T, h1Pressure, x, oilPhaseIdx, eosType);
            const double phiLimited = FluidSystemH1::CubicEOS::computeFugacityCoefficient(
                probe.fs, probe.paramCache, oilPhaseIdx, heavyIdx);
            const double phiCubic = std::exp(cubicLnPhi(probe, x, oilPhaseIdx)[heavyIdx]);
            BOOST_REQUIRE_LT(phiCubic, phiLimited);

            const double expected = residualByFiniteDifference<numComponentsH1>(T, x,
                [&](const double t) {
                    const PhaseProbe<FluidSystemH1> probeAtT(t, h1Pressure, x, oilPhaseIdx, eosType);
                    auto lnPhi = cubicLnPhi(probeAtT, x, oilPhaseIdx);
                    for (int compIdx = 0; compIdx < numComponentsH1; ++compIdx) {
                        lnPhi[compIdx] -= FluidSystemH1::volumeShift(compIdx)
                                          * probeAtT.paramCache.Bi(oilPhaseIdx, compIdx);
                    }
                    return lnPhi;
                });
            const double residual = EnthalpyH1::phaseResidualEnthalpy(outcome.state, oilPhaseIdx, eosType);
            BOOST_CHECK_CLOSE(residual, expected, 1e-4); // [%]
            BOOST_CHECK_LT(residual, -40e3);             // [J/mol]: vaporization-enthalpy scale
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

    BOOST_CHECK_LT(std::abs(resLow), 10.);   // [J/mol], near-ideal at 10 mbar
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
        outcome.state, cpTable, EOSType::PR, Opm::EnthalpyModel::eos_departure));
    const double L = outcome.summary.L;
    const double hOil = Opm::getValue(EnthalpyF1::phaseEnthalpy(
                            outcome.state, FluidSystemF1::oilPhaseIdx, cpTable))
        + EnthalpyF1::phaseResidualEnthalpy(outcome.state, FluidSystemF1::oilPhaseIdx, EOSType::PR);
    const double hGas = Opm::getValue(EnthalpyF1::phaseEnthalpy(
                            outcome.state, FluidSystemF1::gasPhaseIdx, cpTable))
        + EnthalpyF1::phaseResidualEnthalpy(outcome.state, FluidSystemF1::gasPhaseIdx, EOSType::PR);
    BOOST_CHECK_CLOSE(viaMixture, L*hOil + (1. - L)*hGas, 1e-9); // [%]

    // (b) the caloric cancellation is broken: departure H differs from the
    // feed-direct ideal sum by far more than any tolerance
    double viaFeedIdeal = 0.;
    for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
        viaFeedIdeal += f1Z[compIdx] * cpTable[compIdx].enthalpyIntegral(
            Opm::FlashTest::f1Temperature);
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
        EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable));
    const double viaSwitch = Opm::getValue(EnthalpyF1::mixtureEnthalpy(
        outcome.state, cpTable, EOSType::PR, Opm::EnthalpyModel::caloric));
    BOOST_CHECK_CLOSE(viaCaloric, viaSwitch, 1e-12); // [%]
}

// The datum is the ideal gas at T0, not the state at T0: with the departure
// model the enthalpy at T0 is the phase-weighted residual enthalpy of the
// flashed state (the caloric part is zero there at any pressure), not zero.
BOOST_AUTO_TEST_CASE(DepartureDatumIsIdealGasAtT0)
{
    FlashCase<numComponentsF1> testCase{"datum probe", f1Pressure, T0, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    const auto cpTable = Opm::FlashTest::f1CpTable();
    const double L = outcome.summary.L;

    const double H0 = EnthalpyF1::mixtureEnthalpy(
        outcome.state, cpTable, EOSType::PR, Opm::EnthalpyModel::eos_departure);
    double residual = 0.;
    if (L > 0.)
        residual += L * EnthalpyF1::phaseResidualEnthalpy(
            outcome.state, FluidSystemF1::oilPhaseIdx, EOSType::PR);
    if (L < 1.)
        residual += (1. - L) * EnthalpyF1::phaseResidualEnthalpy(
            outcome.state, FluidSystemF1::gasPhaseIdx, EOSType::PR);

    BOOST_CHECK_CLOSE(H0, residual, 1e-9); // [%]
    BOOST_CHECK_GT(std::abs(H0), 100.);    // [J/mol]: the residual is not part of the datum
}

// The model-switch overloads return a plain Scalar for any fluid-state value
// type: the departure part carries no derivatives, so an AD result would hold
// a derivative that looks complete but is not. The caloric-only overload keeps
// the state's value type, because its derivative is complete.
BOOST_AUTO_TEST_CASE(ModelSwitchReturnsValueOnly)
{
    FlashCase<numComponentsF1> testCase{"return-type probe", f1Pressure,
                                        Opm::FlashTest::f1Temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(testCase);
    const auto cpTable = Opm::FlashTest::f1CpTable();
    using ValueType = std::decay_t<decltype(outcome.state)>::ValueType;

    static_assert(!std::is_same_v<ValueType, Scalar>,
                  "the probe state must be AD-valued for this check to mean anything");
    static_assert(std::is_same_v<decltype(EnthalpyF1::mixtureEnthalpy(
                                     outcome.state, cpTable, EOSType::PR,
                                     Opm::EnthalpyModel::eos_departure)),
                                 Scalar>,
                  "the model-switch mixtureEnthalpy must return a plain Scalar");
    static_assert(std::is_same_v<decltype(EnthalpyF1::phaseEnthalpy(
                                     outcome.state, FluidSystemF1::oilPhaseIdx, cpTable,
                                     EOSType::PR, Opm::EnthalpyModel::eos_departure)),
                                 Scalar>,
                  "the model-switch phaseEnthalpy must return a plain Scalar");
    static_assert(std::is_same_v<decltype(EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable)),
                                 ValueType>,
                  "the caloric overload keeps the state's value type");

    // returning a plain Scalar does not change the value
    BOOST_CHECK_CLOSE(EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable, EOSType::PR,
                                                  Opm::EnthalpyModel::caloric),
                      Opm::getValue(EnthalpyF1::mixtureEnthalpy(outcome.state, cpTable)),
                      1e-12); // [%]
}

// A single-phase state takes its residual on the cubic root with the lower
// Gibbs energy, whatever its phase label. F1 with z_C1 = 0.01 at 10 mbar and
// 360 K is a vapour that Li's correlation labels liquid. PTFlash does not
// converge there, so the state is set up as PTFlash leaves a single-phase
// state: both phases hold the feed.
BOOST_AUTO_TEST_CASE(SinglePhaseUsesStableRoot)
{
    constexpr double T = 360.; // [K]
    constexpr double p = 1e3;  // [Pa]
    constexpr std::array<double, numComponentsF1> z = {0.01, 0.99};
    const auto cpTable = Opm::FlashTest::f1CpTable();

    Opm::CompositionalFluidState<double, FluidSystemF1> fs;
    fs.setTemperature(T);
    for (const unsigned phaseIdx : {static_cast<unsigned>(FluidSystemF1::oilPhaseIdx),
                                    static_cast<unsigned>(FluidSystemF1::gasPhaseIdx)}) {
        fs.setPressure(phaseIdx, p);
        for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx)
            fs.setMoleFraction(phaseIdx, compIdx, z[compIdx]);
    }

    // the state has three roots, and the two give very different residuals
    const double onLiquidRoot = EnthalpyF1::phaseResidualEnthalpy(
        fs, FluidSystemF1::oilPhaseIdx, EOSType::PR);
    const double onVapourRoot = EnthalpyF1::phaseResidualEnthalpy(
        fs, FluidSystemF1::gasPhaseIdx, EOSType::PR);
    BOOST_REQUIRE_LT(onLiquidRoot, -40e3);           // [J/mol]
    BOOST_REQUIRE_LT(std::abs(onVapourRoot), 100.);  // [J/mol]

    // labelled liquid (L = 1) and labelled vapour (L = 0): the vapour root either way
    for (const double L : {1., 0.}) {
        fs.setLvalue(L);
        const double hIdeal = Opm::getValue(EnthalpyF1::mixtureEnthalpy(fs, cpTable));
        const double h = EnthalpyF1::mixtureEnthalpy(fs, cpTable, EOSType::PR,
                                                     Opm::EnthalpyModel::eos_departure);
        BOOST_TEST_CONTEXT("L = " << L) {
            BOOST_CHECK_SMALL(h - hIdeal - onVapourRoot, 1e-6); // [J/mol]
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // DepartureModel
