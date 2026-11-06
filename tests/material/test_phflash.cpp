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

#include <dune/common/fmatrix.hh>

#include "flashTestFixtures.hpp"

#include <algorithm>
#include <array>
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

// A lean F1 feed at atmospheric pressure. The isothermal flash fails for it
// at every temperature from 447 to 549 K, where the mixture is a vapour (460 K,
// the hot end of the default interval, is one of them). The lean-feed tests
// take their flash failures from this behaviour.
constexpr std::array<double, 2> leanZ = {0.1, 0.9};
constexpr double atmosphericPressure = 1e5; // [Pa]

// the mixture enthalpy of F1 flashed at (pressure, temperature)
double enthalpyF1(const double pressure, const double temperature, const EnthalpyModel model,
                  const std::array<double, 2>& z = f1Z)
{
    FlashCase<numComponentsF1> probe{"enthalpy probe", pressure, temperature, z};
    const auto outcome = runFlash<FluidSystemF1, EvaluationF1>(probe);
    return EnthalpyF1::mixtureEnthalpy(outcome.state, Opm::FlashTest::f1CpTable(),
                                       EOSType::PR, model);
}

template <class Flash>
double recoverTemperatureF1(const double pressure, const double hSpec,
                            const typename Flash::Config& cfg,
                            const std::array<double, 2>& z = f1Z)
{
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(pressure, f1Temperature, z);
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

// An enthalpy provider that cannot evaluate its calls firstFailure to
// lastFailure (counted from 1). The two ends of the interval are calls 1 and
// 2, so call 3 is the first interior trial.
struct EnthalpyFailingCallsF1
{
    static inline int calls = 0;
    static inline int firstFailure = 0;
    static inline int lastFailure = -1;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        ++calls;
        if (calls >= firstFailure && calls <= lastFailure)
            throw std::runtime_error("no enthalpy at this trial");
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

// An enthalpy provider with a 5 kJ/mol step above 350 K: no temperature has
// an enthalpy inside the step.
struct EnthalpyStepAt350KF1
{
    static constexpr double stepTemperature = 350.;
    static constexpr double step = 5000.; // [J/mol]

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        const double h = EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
        return Opm::getValue(fs.temperature(0)) > stepTemperature ? h + step : h;
    }
};

// The built-in enthalpy as a replacement provider: the search then flashes
// at every trial, also with the caloric model.
struct PassThroughEnthalpyF1
{
    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

// An enthalpy provider that cannot evaluate from 447 to 549 K, whatever the
// isothermal flash does. H(T) [J/mol] is T - 447 below that range and T - 549
// above it: the two branches meet at zero, so that the false-position trials
// of a bracket around the range land in the range.
struct EnthalpyWithFailingRangeF1
{
    static constexpr double lowest = 447.;
    static constexpr double highest = 549.;

    static double enthalpy(const double T)
    {
        return T < lowest ? T - lowest : T - highest;
    }

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>&,
                                  const EOSType&,
                                  const EnthalpyModel)
    {
        const double T = Opm::getValue(fs.temperature(0));
        if (T >= lowest && T <= highest) {
            throw std::runtime_error("no enthalpy in this range");
        }
        return enthalpy(T);
    }
};

// An enthalpy provider H(T) = T [J/mol] that cannot evaluate below 365 K, so
// that a root can lie on the boundary of the evaluable range.
struct LinearEnthalpyFrom365KF1
{
    static constexpr double limit = 365.;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>&,
                                  const EOSType&,
                                  const EnthalpyModel)
    {
        const double T = Opm::getValue(fs.temperature(0));
        if (T < limit)
            throw std::runtime_error("no enthalpy below 365 K");
        return T;
    }
};

// An enthalpy provider whose call badCall (counted from 1) returns NaN or
// throws Dune::FMatrixError, which is not a std::runtime_error.
struct EnthalpyBadCallF1
{
    enum class Kind { notFinite, matrixError };

    static inline int calls = 0;
    static inline int badCall = 0;
    static inline Kind kind = Kind::notFinite;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        if (++calls == badCall) {
            if (kind == Kind::matrixError)
                DUNE_THROW(Dune::FMatrixError, "singular matrix at this trial");
            return std::numeric_limits<Scalar>::quiet_NaN();
        }
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
    // Default interval 270-460 K: the hot end cannot be evaluated; the search
    // toward it from the cold end brackets the root at 300 K.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    BOOST_REQUIRE_GT(cfg.tempMax, EnthalpyUpTo420KF1::limit);

    constexpr double Tstar = 300.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    const double T = recoverTemperatureF1<FlashUpTo420K>(f1Pressure, hSpec, cfg);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(RootBeyondEvaluableRangeReturnsFalse)
{
    // The root at 440 K lies inside the interval asked for, but above 420 K,
    // where the enthalpy cannot be evaluated: the search locates the 420 K
    // boundary without a sign change below it.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    const double hSpec = enthalpyF1(f1Pressure, 440., EnthalpyModel::caloric);

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_CASE(NoEvaluableTemperatureReturnsFalse)
{
    // The whole interval is above the limit: no temperature to start from.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMin = 500.;
    cfg.tempMax = 600.;
    const double hSpec = enthalpyF1(f1Pressure, 550., EnthalpyModel::caloric);

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

BOOST_AUTO_TEST_CASE(RecoversRootNextToFailingEndLeanFeed)
{
    // Lean feed at 1 bar, default interval 270-460 K: the isothermal flash
    // does not converge at 460 K. The root at 440 K has to be found with both
    // models, as it is with the interval ending at 445 K.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    constexpr double Tstar = 440.;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        cfg.model = model;
        const double hSpec = enthalpyF1(atmosphericPressure, Tstar, model, leanZ);
        const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec, cfg, leanZ);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "model " << static_cast<int>(model) << ": expected " << Tstar
                            << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(RecoversRootBeyondFailedMidpoint)
{
    // 90 % C1 at 300 bar, 250-600 K: the isothermal flash fails at the hot end,
    // 600 K, and at the first midpoint toward it, 425 K, but converges at the
    // root, 450 K. The search has to look beyond the failed midpoint.
    constexpr std::array<double, 2> richZ = {0.9, 0.1};
    constexpr double pressure = 300e5; // [Pa]
    constexpr double Tstar = 450.;
    const auto cfg = makeConfigF1(EnthalpyModel::eos_departure);
    const double hSpec = enthalpyF1(pressure, Tstar, EnthalpyModel::eos_departure, richZ);
    const double T = recoverTemperatureF1<PhFlashF1>(pressure, hSpec, cfg, richZ);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

using FlashFrom365K = Opm::PHFlash<Scalar, FluidSystemF1, LinearEnthalpyFrom365KF1>;

BOOST_AUTO_TEST_CASE(BoundaryRootOnTrialIsAccepted)
{
    // H(T) = T, which cannot be evaluated below 365 K, and hSpec = 365: the
    // root is on the boundary. From 365 K it is the cold end itself; from
    // 270 K, which cannot be evaluated, the first trial toward the cold end
    // lands on it exactly.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMax = 460.;
    for (const double tempMin : {365., 270.}) {
        cfg.tempMin = tempMin;
        const double T = recoverTemperatureF1<FlashFrom365K>(f1Pressure,
                                                             LinearEnthalpyFrom365KF1::limit, cfg);
        BOOST_CHECK_SMALL(T - LinearEnthalpyFrom365KF1::limit, cfg.temperatureTolerance);
    }
}

BOOST_AUTO_TEST_CASE(ApproachedBoundaryRootIsAccepted)
{
    // 270-470 K: no trial lands on 365 K. The search approaches the boundary
    // from above without a sign change, and the secant through the last two
    // trials places the root within the temperature tolerance.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMin = 270.;
    cfg.tempMax = 470.;
    const double T = recoverTemperatureF1<FlashFrom365K>(f1Pressure,
                                                         LinearEnthalpyFrom365KF1::limit, cfg);
    BOOST_CHECK_SMALL(T - LinearEnthalpyFrom365KF1::limit, cfg.temperatureTolerance);
}

BOOST_AUTO_TEST_CASE(TargetBelowEvaluableRangeReturnsFalse)
{
    // hSpec just below 365 J/mol: the root lies below 365 K, where the
    // enthalpy cannot be evaluated. The last evaluated temperature, next to
    // 365 K, is not accepted, although its residual is within the enthalpy
    // tolerance.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMin = 270.;
    cfg.tempMax = 460.;
    for (const double hSpec : {364.5, 364.999}) {
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
        BOOST_CHECK_MESSAGE(!FlashFrom365K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                  ptTolerance, EOSType::PR),
                            "hSpec = " << hSpec << " accepted at "
                            << Opm::getValue(fs.temperature(0)) << " K");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(InteriorFailures)

using FlashFailingCalls = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyFailingCallsF1>;

BOOST_AUTO_TEST_CASE(RecoversRootPastFailingTrialLeanFeed)
{
    // Lean feed at 1 bar, 250-600 K: both ends and 560 K flash, but the
    // isothermal flash fails at every temperature from 447 to 549 K. With the
    // built-in caloric model the search runs no flash; the only one is at the
    // root, 560 K, which has to be found.
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 560.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec, cfg, leanZ);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(FlashingSearchRecoversRootLeanFeed)
{
    // As above, with a replacement provider: the search flashes at every
    // trial, meets the failing flashes from 447 to 549 K, and has to replace
    // those trials to find the root at 560 K.
    using FlashingSearch = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 560.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    const double T = recoverTemperatureF1<FlashingSearch>(atmosphericPressure, hSpec, cfg, leanZ);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(RecoversRootBesideFailedTrialsLeanFeed)
{
    // Lean feed at 1 bar, 250-600 K: for the root at 443 K the bracket narrows
    // to [425, 556.25] K, where the false-position trial and its replacements
    // all fall in the range from 447 to 549 K in which the isothermal flash
    // fails. The root can be evaluated and has to be found.
    const auto cfg = makeConfigF1(EnthalpyModel::eos_departure);
    constexpr double Tstar = 443.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::eos_departure, leanZ);
    const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec, cfg, leanZ);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(FlashingSearchRecoversRootBesideFailedTrialsLeanFeed)
{
    // As above on the hot side of the failing range: a search that flashes at
    // every trial, with the caloric model and the root at 549.5 K.
    using FlashingSearch = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 549.5;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    const double T = recoverTemperatureF1<FlashingSearch>(atmosphericPressure, hSpec, cfg, leanZ);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
}

using FlashWithFailingRange = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyWithFailingRangeF1>;

BOOST_AUTO_TEST_CASE(RecoversRootOnEitherSideOfFailingRange)
{
    // The same with a range that fails whatever the isothermal flash does: the
    // root is found on either side of it.
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    for (const double Tstar : {443., 552.}) {
        const double hSpec = EnthalpyWithFailingRangeF1::enthalpy(Tstar);
        const double T = recoverTemperatureF1<FlashWithFailingRange>(f1Pressure, hSpec, cfg);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "expected " << Tstar << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(FailedTrialIsReplaced)
{
    // The first interior trial cannot be evaluated; another temperature in the
    // bracket replaces it, and the root is found.
    EnthalpyFailingCallsF1::calls = 0;
    EnthalpyFailingCallsF1::firstFailure = 3;
    EnthalpyFailingCallsF1::lastFailure = 3;

    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    const double T = recoverTemperatureF1<FlashFailingCalls>(f1Pressure, hSpec,
                                                             makeConfigF1(EnthalpyModel::caloric));
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
    BOOST_CHECK_GT(EnthalpyFailingCallsF1::calls, 3);
}

BOOST_AUTO_TEST_CASE(NoEvaluableReplacementReturnsFalse)
{
    // No interior temperature can be evaluated: the bracket cannot be refined.
    EnthalpyFailingCallsF1::calls = 0;
    EnthalpyFailingCallsF1::firstFailure = 3;
    EnthalpyFailingCallsF1::lastFailure = std::numeric_limits<int>::max();

    const double hSpec = enthalpyF1(f1Pressure, 340., EnthalpyModel::caloric);
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!FlashFailingCalls::solve(fs, hSpec, makeConfigF1(EnthalpyModel::caloric),
                                          PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
}

using FlashBadCall = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyBadCallF1>;

// The number of enthalpy calls of a solve without a bad call; the last call
// is the check at the solution.
int callsWithoutBadCall(const double hSpec)
{
    EnthalpyBadCallF1::calls = 0;
    EnthalpyBadCallF1::badCall = 0;
    recoverTemperatureF1<FlashBadCall>(f1Pressure, hSpec, makeConfigF1(EnthalpyModel::caloric));
    return EnthalpyBadCallF1::calls;
}

// The root Tstar is found with a bad call at either end of the interval or
// at one of the first two interior trials.
void checkRootFoundPastBadCall(const double hSpec, const double Tstar)
{
    for (int badCall = 1; badCall <= 4; ++badCall) {
        EnthalpyBadCallF1::calls = 0;
        EnthalpyBadCallF1::badCall = badCall;
        const double T = recoverTemperatureF1<FlashBadCall>(f1Pressure, hSpec,
                                                            makeConfigF1(EnthalpyModel::caloric));
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "bad call " << badCall << ": expected " << Tstar
                            << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(NonFiniteResidualIsNotEvaluable)
{
    // A NaN enthalpy is treated like a trial that cannot be evaluated. Taken
    // as evaluated, NaN would pass every sign test as a positive residual.
    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    EnthalpyBadCallF1::kind = EnthalpyBadCallF1::Kind::notFinite;
    BOOST_REQUIRE_GT(callsWithoutBadCall(hSpec), 5);
    checkRootFoundPastBadCall(hSpec, Tstar);
}

BOOST_AUTO_TEST_CASE(MatrixErrorIsNotEvaluable)
{
    // Dune::FMatrixError, which the isothermal flash can throw for a singular
    // matrix, is not a std::runtime_error. During the search it is treated
    // like a trial that cannot be evaluated; at the check at the solution,
    // solve() returns false instead of throwing.
    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    EnthalpyBadCallF1::kind = EnthalpyBadCallF1::Kind::matrixError;
    const int finalCall = callsWithoutBadCall(hSpec);
    BOOST_REQUIRE_GT(finalCall, 5);
    checkRootFoundPastBadCall(hSpec, Tstar);

    EnthalpyBadCallF1::calls = 0;
    EnthalpyBadCallF1::badCall = finalCall;
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    bool solved = true;
    BOOST_CHECK_NO_THROW(solved = FlashBadCall::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                      ptTolerance, EOSType::PR));
    BOOST_CHECK(!solved);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(AcceptanceCheck)

BOOST_AUTO_TEST_CASE(EnthalpyStepReturnsFalse)
{
    // hSpec lies inside a 5 kJ/mol step of H(T) at 350 K. The bracket
    // converges onto the step, and the residual check at the converged
    // temperature reports that no temperature has this enthalpy.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyStepAt350KF1>;
    const auto cfg = makeConfigF1(EnthalpyModel::caloric);
    const double hSpec = enthalpyF1(f1Pressure, EnthalpyStepAt350KF1::stepTemperature,
                                    EnthalpyModel::caloric)
                       + 0.5 * EnthalpyStepAt350KF1::step;

    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
    BOOST_CHECK(!Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
    BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - EnthalpyStepAt350KF1::stepTemperature,
                      cfg.temperatureTolerance);
}

BOOST_AUTO_TEST_SUITE_END()
