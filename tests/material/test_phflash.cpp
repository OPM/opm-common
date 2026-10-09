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
 * \brief Tests for the isenthalpic (P-H) flash.
 *
 * The round-trip tests flash at a known temperature, compute the mixture
 * enthalpy of the result, and hand that enthalpy to PHFlash, which has to
 * recover the known temperature. The target enthalpy comes from the same
 * model that is inverted, so an error in the model itself would cancel; the
 * model is tested on its own in test_mixture_enthalpy.cpp. This file tests
 * the inversion: the search interval, its limits, and its failure modes.
 */
#include "config.h"

#define BOOST_TEST_MODULE PHFlash
#include <boost/test/unit_test.hpp>

#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>
#include <opm/material/constraintsolvers/PHFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include "flashTestFixtures.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
using Opm::EnthalpyModel;
using Opm::PTFlashMethod;
using Opm::FlashTest::FlashCase;
using Opm::FlashTest::makeInitialState;
using Opm::FlashTest::runFlash;
using Opm::FlashTest::f1Pressure;
using Opm::FlashTest::f1Temperature;
using Opm::FlashTest::f1Z;
using Opm::FlashTest::f2Pressure;
using Opm::FlashTest::f2Z;

// F1: binary C1/nC10
using FluidSystemF1 = Opm::FlashTest::TwoComponentFluidSystem<Scalar>;
constexpr int numComponentsF1 = FluidSystemF1::numComponents;
using EvaluationF1 = Opm::FlashTest::FlashEvaluation<FluidSystemF1>;
using EnthalpyF1 = Opm::MixtureEnthalpy<Scalar, FluidSystemF1>;
using PhFlashF1 = Opm::PHFlash<Scalar, FluidSystemF1>;

// F2: ternary CO2/C1/nC10 on the generic fluid system
using FluidSystemF2 = Opm::FlashTest::F2FluidSystem<Scalar>;
constexpr int numComponentsF2 = FluidSystemF2::numComponents;
using EvaluationF2 = Opm::FlashTest::FlashEvaluation<FluidSystemF2>;
using EnthalpyF2 = Opm::MixtureEnthalpy<Scalar, FluidSystemF2>;
using PhFlashF2 = Opm::PHFlash<Scalar, FluidSystemF2>;

// The F2 components are static: register them once per process.
struct F2Components
{
    F2Components() { Opm::FlashTest::registerF2Components<Scalar>(); }
};

BOOST_GLOBAL_FIXTURE(F2Components);

namespace {

constexpr double ptTolerance = 1.e-8;        // isothermal flash, fugacity-ratio residual
constexpr double roundTripTolerance = 1.e-3; // [K] on the recovered temperature

PhFlashF1::Config makeConfigF1(const EnthalpyModel model)
{
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.model = model;
    cfg.tempMin = 250.;
    cfg.tempMax = 600.;
    return cfg;
}

// the mixture enthalpy of F1 flashed at (pressure, temperature)
double enthalpyF1(const double pressure, const double temperature, const EnthalpyModel model)
{
    FlashCase<numComponentsF1> probe{"enthalpy probe", pressure, temperature, f1Z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(probe);
    return EnthalpyF1::mixtureEnthalpy(outcome.state, Opm::FlashTest::f1CpTable(),
                                       EOSType::PR, model);
}

template <class Flash>
double recoverTemperatureF1(const double pressure, const double hSpec,
                            const typename Flash::Config& cfg)
{
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(pressure, f1Temperature, f1Z);
    const bool ok = Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR);
    BOOST_REQUIRE_MESSAGE(ok, "PHFlash found no solution for hSpec = " << hSpec);
    return Opm::getValue(fs.temperature(0));
}

double recoverTemperatureF1(const double pressure, const double hSpec, const EnthalpyModel model)
{
    return recoverTemperatureF1<PhFlashF1>(pressure, hSpec, makeConfigF1(model));
}

// An enthalpy provider that records the temperatures it is evaluated at.
struct RecordingEnthalpyF1
{
    static inline double lowest = std::numeric_limits<double>::max();
    static inline double highest = std::numeric_limits<double>::lowest();

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        const double T = Opm::getValue(fs.temperature(0));
        lowest = std::min(lowest, T);
        highest = std::max(highest, T);
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

// An enthalpy provider that cannot evaluate above 420 K, as the isothermal
// flash cannot for some feeds at the hot end of an interval.
struct EnthalpyUpTo420KF1
{
    static constexpr double limit = 420.;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        if (Opm::getValue(fs.temperature(0)) > limit)
            throw std::runtime_error("no enthalpy above 420 K");
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(RoundTrip)

BOOST_AUTO_TEST_CASE(CaloricF1)
{
    for (const double Tstar : {260., 300., 340., 380.}) {
        const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
        const double T = recoverTemperatureF1(f1Pressure, hSpec, EnthalpyModel::caloric);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "caloric: expected " << Tstar << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(DepartureF1)
{
    for (const double Tstar : {260., 300., 340., 380.}) {
        const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::eos_departure);
        const double T = recoverTemperatureF1(f1Pressure, hSpec, EnthalpyModel::eos_departure);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "eos_departure: expected " << Tstar << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(DenseSweepF1)
{
    constexpr double Tlo = 255.;
    constexpr double Thi = 390.;
    constexpr int numPoints = 25;
    for (int i = 0; i < numPoints; ++i) {
        const double Tstar = Tlo + i * (Thi - Tlo) / (numPoints - 1);
        for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
            const double hSpec = enthalpyF1(f1Pressure, Tstar, model);
            const double T = recoverTemperatureF1(f1Pressure, hSpec, model);
            BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                                "expected " << Tstar << " K, recovered " << T << " K");
        }
    }
}

BOOST_AUTO_TEST_CASE(TernaryF2)
{
    PhFlashF2::Config cfg;
    cfg.cpTable = Opm::FlashTest::f2CpTable();
    cfg.tempMin = 250.;
    cfg.tempMax = 500.;
    constexpr double Tstar = 300.;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        cfg.model = model;
        FlashCase<numComponentsF2> probe{"F2 enthalpy probe", f2Pressure, Tstar, f2Z};
        const auto outcome = runFlash<FluidSystemF2, EvaluationF2>(probe);
        const double hSpec = EnthalpyF2::mixtureEnthalpy(outcome.state, cfg.cpTable,
                                                         EOSType::PR, model);

        auto fs = makeInitialState<FluidSystemF2, EvaluationF2>(f2Pressure, 350., f2Z);
        const bool ok = PhFlashF2::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                         ptTolerance, EOSType::PR);
        BOOST_REQUIRE(ok);
        BOOST_CHECK_MESSAGE(std::abs(Opm::getValue(fs.temperature(0)) - Tstar) < roundTripTolerance,
                            "F2 round trip failed for model " << static_cast<int>(model));
    }
}

BOOST_AUTO_TEST_CASE(JouleThomsonSign)
{
    // Expanding from 50 bar to 10 bar at constant enthalpy: the caloric model
    // has no pressure dependence, the eos_departure model has to cool.
    constexpr double pLow = 10e5; // [Pa]

    const double hCaloric = enthalpyF1(f1Pressure, f1Temperature, EnthalpyModel::caloric);
    const double tCaloric = recoverTemperatureF1(pLow, hCaloric, EnthalpyModel::caloric);
    BOOST_CHECK_SMALL(tCaloric - f1Temperature, 1e-5);

    const double hDeparture = enthalpyF1(f1Pressure, f1Temperature, EnthalpyModel::eos_departure);
    const double tDeparture = recoverTemperatureF1(pLow, hDeparture, EnthalpyModel::eos_departure);
    BOOST_CHECK_MESSAGE(tDeparture < f1Temperature,
                        "expected cooling, got " << tDeparture << " K at " << pLow << " Pa");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(SearchInterval)

BOOST_AUTO_TEST_CASE(StaysInsideCpFitRange)
{
    // The interval asked for is wider than the 250-600 K fit range of the
    // C1 and nC10 polynomials; no trial may leave the fit range.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, RecordingEnthalpyF1>;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.tempMin = 150.;
    cfg.tempMax = 900.;

    constexpr double Tstar = 300.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    const double T = recoverTemperatureF1<Flash>(f1Pressure, hSpec, cfg);

    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
    BOOST_CHECK_GE(RecordingEnthalpyF1::lowest, 250.);
    BOOST_CHECK_LE(RecordingEnthalpyF1::highest, 600.);
}

BOOST_AUTO_TEST_CASE(OutsideCpFitRangeReturnsFalse)
{
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.tempMin = 650.;
    cfg.tempMax = 800.;
    const double hSpec = enthalpyF1(f1Pressure, 300., EnthalpyModel::caloric);

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!PhFlashF1::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_CASE(UnpopulatedCpTableReturnsFalse)
{
    PhFlashF1::Config cfg; // all-zero cp table: H(T) = 0 everywhere
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!PhFlashF1::solve(fs, 0.0, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_CASE(EnthalpyAboveRangeReturnsFalse)
{
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    const double hAboveMax = enthalpyF1(f1Pressure, cfg.tempMax, EnthalpyModel::caloric) + 1e6;

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!PhFlashF1::solve(fs, hAboveMax, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(BracketAdaptation)

using FlashUpTo420K = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyUpTo420KF1>;

BOOST_AUTO_TEST_CASE(RecoversInteriorRoot)
{
    // Default interval 270-460 K: the hot end cannot be evaluated and moves
    // inward to 412.5 K; the root at 300 K is still found.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    BOOST_REQUIRE_GT(cfg.tempMax, EnthalpyUpTo420KF1::limit);

    constexpr double Tstar = 300.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    const double T = recoverTemperatureF1<FlashUpTo420K>(f1Pressure, hSpec, cfg);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(RootBeyondAdaptedEndReturnsFalse)
{
    // The root at 440 K lies inside the interval asked for, but beyond the
    // adapted hot end (412.5 K), where the enthalpy cannot be evaluated.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    const double hSpec = enthalpyF1(f1Pressure, 440., EnthalpyModel::caloric);

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_CASE(NoEvaluableEndReturnsFalse)
{
    // Both ends above the limit: the adaptation cannot find an end to start from.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMin = 500.;
    cfg.tempMax = 600.;
    const double hSpec = enthalpyF1(f1Pressure, 550., EnthalpyModel::caloric);

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_SUITE_END()
