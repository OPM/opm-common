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

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <dune/common/fmatrix.hh>

#include "flashTestFixtures.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

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

// makeConfigF1 with the Newton stages before the bracket search
PhFlashF1::Config newtonConfigF1(const EnthalpyModel model)
{
    auto cfg = makeConfigF1(model);
    cfg.newtonStart = true;
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

// The search with and without the Newton stage. The tests of the bracket
// search describe it without; with it, the same case is solved by the Newton
// stage or met by the bracket search after the Newton stage hands over.
constexpr std::array<bool, 2> newtonStarts = {false, true};

template <class Flash>
double recoverTemperatureF1(const double pressure, const double hSpec,
                            const typename Flash::Config& cfg,
                            const std::array<double, 2>& z = f1Z,
                            const double startTemperature = f1Temperature)
{
    auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(pressure, startTemperature, z);
    const bool ok = Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR);
    BOOST_REQUIRE_MESSAGE(ok, "PHFlash found no solution for hSpec = " << hSpec);
    return Opm::getValue(fs.temperature(0));
}

double recoverTemperatureF1(const double pressure, const double hSpec, const EnthalpyModel model)
{
    return recoverTemperatureF1<PhFlashF1>(pressure, hSpec, makeConfigF1(model));
}

// The replacement enthalpy providers below declare newtonStage, except the two
// written for the caller's state.

// An enthalpy provider that records the temperatures it is evaluated at.
struct RecordingEnthalpyF1
{
    static constexpr bool newtonStage = true;
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
    static constexpr bool newtonStage = true;
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
// lastFailure (counted from 1). In the bracket search alone, the two ends of
// the interval are calls 1 and 2, so call 3 is the first interior trial.
struct EnthalpyFailingCallsF1
{
    static constexpr bool newtonStage = true;
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
    static constexpr bool newtonStage = true;
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
// at every trial, also with the caloric model. It counts its calls.
struct PassThroughEnthalpyF1
{
    static constexpr bool newtonStage = true;
    static inline int calls = 0;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        ++calls;
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

// An enthalpy provider written for one fluid-state type only, the caller's.
// It counts its calls.
struct CallerStateEnthalpyF1
{
    using FluidState = Opm::CompositionalFluidState<EvaluationF1, FluidSystemF1>;

    static inline int calls = 0;

    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        ++calls;
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model);
    }
};

// A caller's fluid-state type with a member of its own.
struct CallerStateF1 : Opm::CompositionalFluidState<EvaluationF1, FluidSystemF1>
{
    using Base = Opm::CompositionalFluidState<EvaluationF1, FluidSystemF1>;

    explicit CallerStateF1(const Base& fs)
        : Base(fs)
    {}

    double enthalpyShift = 0.0; // [J/mol]
};

// An enthalpy provider that takes any fluid-state type, but whose body reads
// the member of CallerStateF1. It counts its calls.
struct GenericCallerStateEnthalpyF1
{
    static inline int calls = 0;

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>& cpTable,
                                  const EOSType& eosType,
                                  const EnthalpyModel model)
    {
        ++calls;
        return EnthalpyF1::mixtureEnthalpy(fs, cpTable, eosType, model) + fs.enthalpyShift;
    }
};

// An enthalpy provider H(T) = 1e-6 T^4 [J/mol], convex, so that a Newton step
// from below the root overshoots it. It cannot evaluate from 400.005 to
// 400.05 K, just above the root at 400 K, and records the temperatures it is
// evaluated at.
struct QuarticEnthalpyF1
{
    static constexpr bool newtonStage = true;
    static constexpr double failingFrom = 400.005;
    static constexpr double failingTo = 400.05;
    static inline double lowest = std::numeric_limits<double>::max();
    static inline double highest = std::numeric_limits<double>::lowest();

    static double enthalpy(const double T)
    {
        return 1e-6 * T * T * T * T;
    }

    template <class FluidState>
    static Scalar mixtureEnthalpy(const FluidState& fs,
                                  const Opm::CpTable<Scalar, numComponentsF1>&,
                                  const EOSType&,
                                  const EnthalpyModel)
    {
        const double T = Opm::getValue(fs.temperature(0));
        lowest = std::min(lowest, T);
        highest = std::max(highest, T);
        if (T > failingFrom && T < failingTo) {
            throw std::runtime_error("no enthalpy in this range");
        }
        return enthalpy(T);
    }
};

// An enthalpy provider that cannot evaluate from 447 to 549 K, whatever the
// isothermal flash does. H(T) [J/mol] is T - 447 below that range and T - 549
// above it: the two branches meet at zero, so that the false-position trials
// of a bracket around the range land in the range.
struct EnthalpyWithFailingRangeF1
{
    static constexpr bool newtonStage = true;
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
    static constexpr bool newtonStage = true;
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
    static constexpr bool newtonStage = true;
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

// A state flashed at (pressure, temperature): the phase split a simulator
// carries into the next isenthalpic flash.
template <class FluidSystem, class Evaluation>
Opm::CompositionalFluidState<Evaluation, FluidSystem>
carriedState(const double pressure, const double temperature,
             const std::array<double, FluidSystem::numComponents>& z)
{
    FlashCase<FluidSystem::numComponents> probe{"carried state", pressure, temperature, z};
    return runFlash<FluidSystem, Evaluation>(probe).state;
}

PhFlashF2::Config departureConfigF2()
{
    PhFlashF2::Config cfg;
    cfg.cpTable = Opm::FlashTest::f2CpTable();
    cfg.model = EnthalpyModel::eos_departure;
    cfg.tempMin = 250.;
    cfg.tempMax = 500.;
    cfg.newtonStart = true;
    return cfg;
}

// the eos_departure mixture enthalpy of F2 flashed at (pressure, temperature)
double enthalpyF2(const double pressure, const double temperature,
                  const std::array<double, 3>& z = f2Z)
{
    return EnthalpyF2::mixtureEnthalpy(carriedState<FluidSystemF2, EvaluationF2>(pressure,
                                                                                temperature, z),
                                       Opm::FlashTest::f2CpTable(), EOSType::PR,
                                       EnthalpyModel::eos_departure);
}

// The OpmLog debug messages issued while it lives.
class DebugMessages
{
public:
    DebugMessages()
    {
        Opm::OpmLog::addBackend(name,
                                std::make_shared<Opm::StreamLog>(stream_,
                                                                 Opm::Log::MessageType::Debug));
    }

    ~DebugMessages()
    {
        Opm::OpmLog::removeBackend(name);
    }

    bool contain(const std::string& text) const
    {
        return stream_.str().find(text) != std::string::npos;
    }

private:
    static constexpr const char* name = "PHFlashTest";
    std::ostringstream stream_;
};

// Solve from fs; true if the solution came from its phase split.
template <class Flash, class FluidState>
bool solvedFromSplit(FluidState& fs, const double hSpec, const typename Flash::Config& cfg)
{
    DebugMessages messages;
    BOOST_REQUIRE(Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance, EOSType::PR, 1));
    return messages.contain("solved from the incoming phase split");
}

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
            auto cfg = makeConfigF1(model);
            for (const bool newtonStart : newtonStarts) {
                cfg.newtonStart = newtonStart;
                const double T = recoverTemperatureF1<PhFlashF1>(f1Pressure, hSpec, cfg);
                BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                                    "newtonStart " << newtonStart << ": expected " << Tstar
                                    << " K, recovered " << T << " K");
            }
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
    // The Newton stage starts from inside the interval, from below it and
    // from above it.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, RecordingEnthalpyF1>;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.tempMin = 150.;
    cfg.tempMax = 900.;

    constexpr double Tstar = 300.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    for (const bool newtonStart : newtonStarts) {
        for (const double start : {f1Temperature, 100., 900.}) {
            BOOST_TEST_CONTEXT("newtonStart " << newtonStart << ", start " << start << " K") {
                cfg.newtonStart = newtonStart;
                RecordingEnthalpyF1::lowest = std::numeric_limits<double>::max();
                RecordingEnthalpyF1::highest = std::numeric_limits<double>::lowest();
                const double T = recoverTemperatureF1<Flash>(f1Pressure, hSpec, cfg, f1Z, start);

                BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
                BOOST_CHECK_GE(RecordingEnthalpyF1::lowest, 250.);
                BOOST_CHECK_LE(RecordingEnthalpyF1::highest, 600.);
            }
        }
    }
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
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    const double hAboveMax = enthalpyF1(f1Pressure, cfg.tempMax, EnthalpyModel::caloric) + 1e6;

    for (const bool newtonStart : newtonStarts) {
        cfg.newtonStart = newtonStart;
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
        BOOST_CHECK_MESSAGE(!PhFlashF1::solve(fs, hAboveMax, cfg, PTFlashMethod::Ssi,
                                              ptTolerance, EOSType::PR),
                            "newtonStart " << newtonStart);
    }
}

BOOST_AUTO_TEST_CASE(BracketSearchNeedsNoIncomingTemperature)
{
    // By default the bracket search alone runs, and takes only the pressure
    // and the composition of the incoming state; its temperature is never set
    // here.
    struct UnsetTemperatureState : Opm::CompositionalFluidState<EvaluationF1, FluidSystemF1>
    {
        using Base = Opm::CompositionalFluidState<EvaluationF1, FluidSystemF1>;

        const EvaluationF1& temperature(const unsigned phaseIdx) const
        {
            readBeforeSet = readBeforeSet || !set;
            return Base::temperature(phaseIdx);
        }
        void setTemperature(const EvaluationF1& value)
        {
            set = true;
            Base::setTemperature(value);
        }

        bool set = false;
        mutable bool readBeforeSet = false;
    };

    constexpr double Tstar = 300.;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        BOOST_TEST_CONTEXT("model " << static_cast<int>(model)) {
            const auto cfg = makeConfigF1(model);
            UnsetTemperatureState fs;
            fs.setPressure(FluidSystemF1::oilPhaseIdx, EvaluationF1(f1Pressure));
            fs.setPressure(FluidSystemF1::gasPhaseIdx, EvaluationF1(f1Pressure));
            for (int compIdx = 0; compIdx < numComponentsF1; ++compIdx) {
                fs.setMoleFraction(compIdx, EvaluationF1(f1Z[compIdx]));
            }

            BOOST_REQUIRE(PhFlashF1::solve(fs, enthalpyF1(f1Pressure, Tstar, model), cfg,
                                           PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
            BOOST_CHECK(!fs.readBeforeSet);
            BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
        }
    }
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
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<FlashUpTo420K>(f1Pressure, hSpec, cfg);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(RootBeyondEvaluableRangeReturnsFalse)
{
    // The root at 440 K lies inside the interval asked for, but above 420 K,
    // where the enthalpy cannot be evaluated: the search locates the 420 K
    // boundary without a sign change below it.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    const double hSpec = enthalpyF1(f1Pressure, 440., EnthalpyModel::caloric);

    for (const bool newtonStart : newtonStarts) {
        cfg.newtonStart = newtonStart;
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
        BOOST_CHECK_MESSAGE(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                  ptTolerance, EOSType::PR),
                            "newtonStart " << newtonStart);
    }
}

BOOST_AUTO_TEST_CASE(NoEvaluableTemperatureReturnsFalse)
{
    // The whole interval is above the limit: no temperature to start from.
    PhFlashF1::Config cfg;
    cfg.cpTable = Opm::FlashTest::f1CpTable();
    cfg.tempMin = 500.;
    cfg.tempMax = 600.;
    const double hSpec = enthalpyF1(f1Pressure, 550., EnthalpyModel::caloric);

    for (const bool newtonStart : newtonStarts) {
        cfg.newtonStart = newtonStart;
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
        BOOST_CHECK_MESSAGE(!FlashUpTo420K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                  ptTolerance, EOSType::PR),
                            "newtonStart " << newtonStart);
    }
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
        for (const bool newtonStart : newtonStarts) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec,
                                                             cfg, leanZ);
            BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                                "model " << static_cast<int>(model) << ", newtonStart "
                                << newtonStart << ": expected " << Tstar
                                << " K, recovered " << T << " K");
        }
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
    auto cfg = makeConfigF1(EnthalpyModel::eos_departure);
    const double hSpec = enthalpyF1(pressure, Tstar, EnthalpyModel::eos_departure, richZ);
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<PhFlashF1>(pressure, hSpec, cfg, richZ);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
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
    for (const bool newtonStart : newtonStarts) {
        for (const double tempMin : {365., 270.}) {
            BOOST_TEST_CONTEXT("newtonStart " << newtonStart << ", tempMin " << tempMin) {
                cfg.newtonStart = newtonStart;
                cfg.tempMin = tempMin;
                const double T = recoverTemperatureF1<FlashFrom365K>(
                    f1Pressure, LinearEnthalpyFrom365KF1::limit, cfg);
                BOOST_CHECK_SMALL(T - LinearEnthalpyFrom365KF1::limit, cfg.temperatureTolerance);
            }
        }
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
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<FlashFrom365K>(
                f1Pressure, LinearEnthalpyFrom365KF1::limit, cfg);
            BOOST_CHECK_SMALL(T - LinearEnthalpyFrom365KF1::limit, cfg.temperatureTolerance);
        }
    }
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
    for (const bool newtonStart : newtonStarts) {
        cfg.newtonStart = newtonStart;
        for (const double hSpec : {364.5, 364.999}) {
            auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
            BOOST_CHECK_MESSAGE(!FlashFrom365K::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                      ptTolerance, EOSType::PR),
                                "newtonStart " << newtonStart << ": hSpec = " << hSpec
                                << " accepted at " << Opm::getValue(fs.temperature(0)) << " K");
        }
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
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 560.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec,
                                                             cfg, leanZ);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(FlashingSearchRecoversRootLeanFeed)
{
    // As above, with a replacement provider: the search flashes at every
    // trial, meets the failing flashes from 447 to 549 K, and has to replace
    // those trials to find the root at 560 K. From 300 K the Newton stage
    // walks into that range and hands over to the bracket search.
    using FlashingSearch = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 560.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<FlashingSearch>(atmosphericPressure, hSpec,
                                                                  cfg, leanZ);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(RecoversRootBesideFailedTrialsLeanFeed)
{
    // Lean feed at 1 bar, 250-600 K: for the root at 443 K the bracket narrows
    // to [425, 556.25] K, where the false-position trial and its replacements
    // all fall in the range from 447 to 549 K in which the isothermal flash
    // fails. The root can be evaluated and has to be found.
    auto cfg = makeConfigF1(EnthalpyModel::eos_departure);
    constexpr double Tstar = 443.;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::eos_departure, leanZ);
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<PhFlashF1>(atmosphericPressure, hSpec,
                                                             cfg, leanZ);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(FlashingSearchRecoversRootBesideFailedTrialsLeanFeed)
{
    // As above on the hot side of the failing range: a search that flashes at
    // every trial, with the caloric model and the root at 549.5 K.
    using FlashingSearch = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    constexpr double Tstar = 549.5;
    const double hSpec = enthalpyF1(atmosphericPressure, Tstar, EnthalpyModel::caloric, leanZ);
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            const double T = recoverTemperatureF1<FlashingSearch>(atmosphericPressure, hSpec,
                                                                  cfg, leanZ);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

using FlashWithFailingRange = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyWithFailingRangeF1>;

BOOST_AUTO_TEST_CASE(RecoversRootOnEitherSideOfFailingRange)
{
    // The same with a range that fails whatever the isothermal flash does: the
    // root is found on either side of it.
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    for (const bool newtonStart : newtonStarts) {
        cfg.newtonStart = newtonStart;
        for (const double Tstar : {443., 552.}) {
            const double hSpec = EnthalpyWithFailingRangeF1::enthalpy(Tstar);
            const double T = recoverTemperatureF1<FlashWithFailingRange>(f1Pressure, hSpec, cfg);
            BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                                "newtonStart " << newtonStart << ": expected " << Tstar
                                << " K, recovered " << T << " K");
        }
    }
}

BOOST_AUTO_TEST_CASE(FailedTrialIsReplaced)
{
    // The first interior trial of the bracket search cannot be evaluated;
    // another temperature in the bracket replaces it, and the root is found.
    EnthalpyFailingCallsF1::calls = 0;
    EnthalpyFailingCallsF1::firstFailure = 3;
    EnthalpyFailingCallsF1::lastFailure = 3;

    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.newtonStart = false;
    const double T = recoverTemperatureF1<FlashFailingCalls>(f1Pressure, hSpec, cfg);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
    BOOST_CHECK_GT(EnthalpyFailingCallsF1::calls, 3);
}

BOOST_AUTO_TEST_CASE(NoEvaluableReplacementReturnsFalse)
{
    // No interior temperature can be evaluated: the bracket cannot be refined.
    // With the Newton stage, its first trial and the first enthalpy of its
    // slope can be evaluated, and nothing after them.
    const double hSpec = enthalpyF1(f1Pressure, 340., EnthalpyModel::caloric);
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    for (const bool newtonStart : newtonStarts) {
        EnthalpyFailingCallsF1::calls = 0;
        EnthalpyFailingCallsF1::firstFailure = 3;
        EnthalpyFailingCallsF1::lastFailure = std::numeric_limits<int>::max();
        cfg.newtonStart = newtonStart;
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
        BOOST_CHECK_MESSAGE(!FlashFailingCalls::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                      ptTolerance, EOSType::PR),
                            "newtonStart " << newtonStart);
    }
}

using FlashBadCall = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyBadCallF1>;

// The number of enthalpy calls of a solve without a bad call; the last call
// is the check at the solution.
int callsWithoutBadCall(const double hSpec, const bool newtonStart)
{
    EnthalpyBadCallF1::calls = 0;
    EnthalpyBadCallF1::badCall = 0;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.newtonStart = newtonStart;
    recoverTemperatureF1<FlashBadCall>(f1Pressure, hSpec, cfg);
    return EnthalpyBadCallF1::calls;
}

// The root Tstar is found with a bad call at any of the first four calls.
// In the bracket search alone they are the ends of the interval and the first
// two interior trials. With the Newton stage they are its first trial, the
// two enthalpies of the slope there, and its second trial.
void checkRootFoundPastBadCall(const double hSpec, const double Tstar, const bool newtonStart)
{
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    cfg.newtonStart = newtonStart;
    for (int badCall = 1; badCall <= 4; ++badCall) {
        EnthalpyBadCallF1::calls = 0;
        EnthalpyBadCallF1::badCall = badCall;
        const double T = recoverTemperatureF1<FlashBadCall>(f1Pressure, hSpec, cfg);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "newtonStart " << newtonStart << ", bad call " << badCall
                            << ": expected " << Tstar << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(NonFiniteResidualIsNotEvaluable)
{
    // A NaN enthalpy is treated like a trial that cannot be evaluated. Taken
    // as evaluated, NaN would pass every sign test as a positive residual.
    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    EnthalpyBadCallF1::kind = EnthalpyBadCallF1::Kind::notFinite;
    for (const bool newtonStart : newtonStarts) {
        BOOST_REQUIRE_GT(callsWithoutBadCall(hSpec, newtonStart), 5);
        checkRootFoundPastBadCall(hSpec, Tstar, newtonStart);
    }
}

BOOST_AUTO_TEST_CASE(MatrixErrorIsNotEvaluable)
{
    // Dune::FMatrixError, which the isothermal flash can throw for a singular
    // matrix, is not a std::runtime_error. During the search it is treated
    // like a trial that cannot be evaluated. At the check at the solution of
    // the bracket search, solve() returns false instead of throwing; at that
    // of the Newton stage, the bracket search takes over.
    constexpr double Tstar = 340.;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    EnthalpyBadCallF1::kind = EnthalpyBadCallF1::Kind::matrixError;
    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            const int finalCall = callsWithoutBadCall(hSpec, newtonStart);
            BOOST_REQUIRE_GT(finalCall, 5);
            checkRootFoundPastBadCall(hSpec, Tstar, newtonStart);

            EnthalpyBadCallF1::calls = 0;
            EnthalpyBadCallF1::badCall = finalCall;
            auto cfg = makeConfigF1(EnthalpyModel::caloric);
            cfg.newtonStart = newtonStart;
            auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
            bool solved = true;
            BOOST_CHECK_NO_THROW(solved = FlashBadCall::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                                              ptTolerance, EOSType::PR));
            BOOST_CHECK_EQUAL(solved, newtonStart);
            if (solved) {
                BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(AcceptanceCheck)

BOOST_AUTO_TEST_CASE(EnthalpyStepReturnsFalse)
{
    // hSpec lies inside a 5 kJ/mol step of H(T) at 350 K. The bracket
    // converges onto the step, and the residual check at the converged
    // temperature reports that no temperature has this enthalpy.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyStepAt350KF1>;
    auto cfg = makeConfigF1(EnthalpyModel::caloric);
    const double hSpec = enthalpyF1(f1Pressure, EnthalpyStepAt350KF1::stepTemperature,
                                    EnthalpyModel::caloric)
                       + 0.5 * EnthalpyStepAt350KF1::step;

    for (const bool newtonStart : newtonStarts) {
        BOOST_TEST_CONTEXT("newtonStart " << newtonStart) {
            cfg.newtonStart = newtonStart;
            auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, f1Temperature, f1Z);
            BOOST_CHECK(!Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi,
                                      ptTolerance, EOSType::PR));
            BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0))
                              - EnthalpyStepAt350KF1::stepTemperature,
                              cfg.temperatureTolerance);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(NewtonStage)

BOOST_AUTO_TEST_CASE(StartAtRootTakesOneTrial)
{
    // Started at the root, the search evaluates the enthalpy four times: at
    // its one trial, at the two states of the slope there, and at the check
    // at the solution.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        for (const double Tstar : {300., 340.}) {
            BOOST_TEST_CONTEXT("model " << static_cast<int>(model) << ", " << Tstar << " K") {
                const double hSpec = enthalpyF1(f1Pressure, Tstar, model);
                PassThroughEnthalpyF1::calls = 0;
                const double T = recoverTemperatureF1<Flash>(f1Pressure, hSpec,
                                                             newtonConfigF1(model), f1Z, Tstar);
                BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
                BOOST_CHECK_EQUAL(PassThroughEnthalpyF1::calls, 4);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(RecoversRootFromAnyStart)
{
    // Starts near and far on either side of the root, outside the interval,
    // and not finite, with the built-in provider and with a flash at every
    // trial.
    using FlashingSearch = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        const auto cfg = newtonConfigF1(model);
        for (const double Tstar : {280., 340., 400.}) {
            const double hSpec = enthalpyF1(f1Pressure, Tstar, model);
            for (const double start : {Tstar - 50., Tstar - 5., Tstar - 1., Tstar + 1.,
                                       Tstar + 5., Tstar + 50., 100., 900., nan}) {
                BOOST_TEST_CONTEXT("model " << static_cast<int>(model) << ", " << Tstar
                                   << " K from " << start << " K") {
                    BOOST_CHECK_SMALL(recoverTemperatureF1<PhFlashF1>(f1Pressure, hSpec, cfg,
                                                                      f1Z, start) - Tstar,
                                      roundTripTolerance);
                    BOOST_CHECK_SMALL(recoverTemperatureF1<FlashingSearch>(f1Pressure, hSpec, cfg,
                                                                           f1Z, start) - Tstar,
                                      roundTripTolerance);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(StartThatCannotBeEvaluatedHandsOver)
{
    // The start lies in the range that cannot be evaluated: the Newton stage
    // stops at its first trial, and the bracket search over the interval finds
    // the root on either side of the range.
    using FlashWithFailingRange = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyWithFailingRangeF1>;
    const auto cfg = newtonConfigF1(EnthalpyModel::caloric);
    for (const double Tstar : {443., 552.}) {
        const double hSpec = EnthalpyWithFailingRangeF1::enthalpy(Tstar);
        const double T = recoverTemperatureF1<FlashWithFailingRange>(f1Pressure, hSpec, cfg,
                                                                     f1Z, 500.);
        BOOST_CHECK_MESSAGE(std::abs(T - Tstar) < roundTripTolerance,
                            "expected " << Tstar << " K, recovered " << T << " K");
    }
}

BOOST_AUTO_TEST_CASE(BracketOfNewtonStageIsKept)
{
    // H(T) = 1e-6 T^4 from 380 K: the first Newton step overshoots the root at
    // 400 K to about 401.6 K, and the second lands at about 400.01 K, which
    // cannot be evaluated. The bracket search goes on from [380, 401.6] K and
    // never evaluates the ends of the interval.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, QuarticEnthalpyF1>;
    constexpr double Tstar = 400.;
    QuarticEnthalpyF1::lowest = std::numeric_limits<double>::max();
    QuarticEnthalpyF1::highest = std::numeric_limits<double>::lowest();
    const double T = recoverTemperatureF1<Flash>(f1Pressure, QuarticEnthalpyF1::enthalpy(Tstar),
                                                 newtonConfigF1(EnthalpyModel::caloric), f1Z, 380.);
    BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
    BOOST_CHECK_GT(QuarticEnthalpyF1::lowest, 379.99);
    BOOST_CHECK_LT(QuarticEnthalpyF1::highest, 401.7);
}

BOOST_AUTO_TEST_CASE(SolutionMeetsTightEnthalpyTolerance)
{
    // An enthalpy tolerance far below slope * temperatureTolerance: the
    // solution after the last Newton step meets it.
    constexpr double Tstar = 360.;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        BOOST_TEST_CONTEXT("model " << static_cast<int>(model)) {
            auto cfg = newtonConfigF1(model);
            cfg.enthalpyTolerance = 1e-6;
            const double hSpec = enthalpyF1(f1Pressure, Tstar, model);
            const double T = recoverTemperatureF1<PhFlashF1>(f1Pressure, hSpec, cfg, f1Z, 325.);
            BOOST_CHECK_SMALL(T - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(UnconfirmedSolutionHandsOver)
{
    // Started 0.5 mK below the 5 kJ/mol step of H(T) at 350 K, the slope
    // spans the step and the Newton step is within the temperature tolerance,
    // 4.5 mK above the root. The flash there does not confirm it, and the
    // bracket search finds the root.
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, EnthalpyStepAt350KF1>;
    constexpr double Tstar = EnthalpyStepAt350KF1::stepTemperature - 5e-3;
    auto cfg = newtonConfigF1(EnthalpyModel::caloric);
    cfg.enthalpyTolerance = 0.1;
    const double hSpec = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
    const double T = recoverTemperatureF1<Flash>(f1Pressure, hSpec, cfg, f1Z,
                                                 EnthalpyStepAt350KF1::stepTemperature - 5e-4);
    BOOST_CHECK_SMALL(T - Tstar, 1e-5);
}

// A provider that does not declare newtonStage is evaluated on the caller's
// state only: the bracket search alone runs, with or without newtonStart.
// Started at the root, the Newton stage would evaluate it four times.
template <class Provider, class CallerState>
void checkBracketSearchAlone()
{
    using Flash = Opm::PHFlash<Scalar, FluidSystemF1, Provider>;
    constexpr double Tstar = 340.;
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        BOOST_TEST_CONTEXT("model " << static_cast<int>(model)) {
            const double hSpec = enthalpyF1(f1Pressure, Tstar, model);
            std::array<int, 2> calls{};
            for (const bool newtonStart : newtonStarts) {
                auto cfg = makeConfigF1(model);
                cfg.newtonStart = newtonStart;
                CallerState fs(makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, Tstar,
                                                                             f1Z));
                Provider::calls = 0;
                BOOST_REQUIRE(Flash::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance,
                                           EOSType::PR));
                BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
                calls[newtonStart] = Provider::calls;
            }
            BOOST_CHECK_GT(calls[false], 4);
            BOOST_CHECK_EQUAL(calls[true], calls[false]);
        }
    }
}

BOOST_AUTO_TEST_CASE(CallerStateProviderGetsBracketSearch)
{
    checkBracketSearchAlone<CallerStateEnthalpyF1, CallerStateEnthalpyF1::FluidState>();
    // A template, which a check of the call alone would let into the Newton
    // stage, where its body does not compile.
    checkBracketSearchAlone<GenericCallerStateEnthalpyF1, CallerStateF1>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(IncomingSplit)

BOOST_AUTO_TEST_CASE(TwoPhaseSplitF2)
{
    // Two-phase states carried from near and far: the split and the
    // temperature are solved for together, to the state that the bracket
    // search alone finds.
    constexpr double pressure = 50e5;
    const auto cfg = departureConfigF2();
    auto bracketOnly = cfg;
    bracketOnly.newtonStart = false;
    for (const double Tstar : {300., 380.}) {
        const double hSpec = enthalpyF2(pressure, Tstar);
        auto reference = makeInitialState<FluidSystemF2, EvaluationF2>(pressure, Tstar, f2Z);
        BOOST_REQUIRE(PhFlashF2::solve(reference, hSpec, bracketOnly, PTFlashMethod::Ssi,
                                       ptTolerance, EOSType::PR));
        for (const double start : {Tstar, Tstar - 20., Tstar - 5., Tstar - 1., Tstar + 1.,
                                   Tstar + 5., Tstar + 20.}) {
            BOOST_TEST_CONTEXT(Tstar << " K from " << start << " K") {
                auto fs = carriedState<FluidSystemF2, EvaluationF2>(pressure, start, f2Z);
                const double L0 = Opm::getValue(fs.L());
                BOOST_REQUIRE(L0 > 0. && L0 < 1.);
                BOOST_CHECK(solvedFromSplit<PhFlashF2>(fs, hSpec, cfg));
                BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
                BOOST_CHECK_SMALL(Opm::getValue(fs.L()) - Opm::getValue(reference.L()), 1e-6);
                for (int compIdx = 0; compIdx < numComponentsF2; ++compIdx) {
                    for (const auto phaseIdx : {FluidSystemF2::oilPhaseIdx,
                                                FluidSystemF2::gasPhaseIdx}) {
                        const double w = Opm::getValue(fs.moleFraction(phaseIdx, compIdx));
                        const double ref = Opm::getValue(reference.moleFraction(phaseIdx, compIdx));
                        BOOST_CHECK_SMALL(w - ref, 1e-6);
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(SinglePhaseStatesF1)
{
    // A liquid (the equimolar feed at 200 bar) and a vapour (a methane-rich
    // feed at 10 bar), carried within the single-phase region.
    struct Case
    {
        double pressure;
        std::array<double, 2> z;
        double Tstar;
        double label;
    };
    const auto cfg = newtonConfigF1(EnthalpyModel::eos_departure);
    for (const auto& [pressure, z, Tstar, label] : {Case{200e5, f1Z, 350., 1.},
                                                    Case{10e5, {0.97, 0.03}, 430., 0.}}) {
        const double hSpec = enthalpyF1(pressure, Tstar, EnthalpyModel::eos_departure, z);
        for (const double start : {Tstar, Tstar - 15., Tstar + 15.}) {
            BOOST_TEST_CONTEXT(pressure << " Pa, " << Tstar << " K from " << start << " K") {
                auto fs = carriedState<FluidSystemF1, EvaluationF1>(pressure, start, z);
                BOOST_REQUIRE_EQUAL(Opm::getValue(fs.L()), label);
                BOOST_CHECK(solvedFromSplit<PhFlashF1>(fs, hSpec, cfg));
                BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
                BOOST_CHECK_EQUAL(Opm::getValue(fs.L()), label);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PhaseCountChangesF2)
{
    // At 100 bar the F2 feed is a liquid up to its bubble point between 290
    // and 300 K. From the split at 300 K the combined iteration converges to
    // a liquid fraction above one, and the single-phase iteration goes on
    // from there. From the liquid at 280 K the single-phase solution lies
    // above the bubble point, and the stage runs again from the split the
    // flash there finds.
    constexpr double pressure = 100e5;
    const auto cfg = departureConfigF2();
    for (const auto& [Tstar, start] : {std::pair{280., 300.}, std::pair{300., 280.}}) {
        BOOST_TEST_CONTEXT(Tstar << " K from " << start << " K") {
            const double hSpec = enthalpyF2(pressure, Tstar);
            auto fs = carriedState<FluidSystemF2, EvaluationF2>(pressure, start, f2Z);
            const double L0 = Opm::getValue(fs.L());
            BOOST_REQUIRE((L0 == 1.) == (Tstar > start));
            BOOST_CHECK(solvedFromSplit<PhFlashF2>(fs, hSpec, cfg));
            BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
            const double L = Opm::getValue(fs.L());
            BOOST_CHECK((L == 1.) == (Tstar < start));
        }
    }
}

BOOST_AUTO_TEST_CASE(SplitEndsInVapourF1)
{
    // At 10 bar the methane-rich feed is a vapour above its dew point between
    // 395 and 400 K. From the split at 390 K the combined iteration converges
    // to a liquid fraction below zero, and the single-phase iteration goes on
    // from there.
    constexpr double pressure = 10e5;
    constexpr std::array<double, 2> z = {0.97, 0.03};
    constexpr double Tstar = 420.;
    const double hSpec = enthalpyF1(pressure, Tstar, EnthalpyModel::eos_departure, z);
    auto fs = carriedState<FluidSystemF1, EvaluationF1>(pressure, 390., z);
    const double L0 = Opm::getValue(fs.L());
    BOOST_REQUIRE(L0 > 0. && L0 < 1.);
    BOOST_CHECK(solvedFromSplit<PhFlashF1>(fs, hSpec,
                                           newtonConfigF1(EnthalpyModel::eos_departure)));
    BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
    BOOST_CHECK_EQUAL(Opm::getValue(fs.L()), 0.);
}

BOOST_AUTO_TEST_CASE(ChangedFeedF2)
{
    // The overall composition has changed since the state was flashed, as
    // between two iterations of a simulator: the split is solved for the new
    // feed.
    constexpr double pressure = 50e5;
    constexpr double Tstar = 320.;
    constexpr std::array<double, 3> z = {0.45, 0.33, 0.22};
    const double hSpec = enthalpyF2(pressure, Tstar, z);
    auto fs = carriedState<FluidSystemF2, EvaluationF2>(pressure, 325., f2Z);
    for (int compIdx = 0; compIdx < numComponentsF2; ++compIdx) {
        fs.setMoleFraction(compIdx, EvaluationF2(z[compIdx]));
    }
    BOOST_CHECK(solvedFromSplit<PhFlashF2>(fs, hSpec, departureConfigF2()));
    BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
}

BOOST_AUTO_TEST_CASE(StartOutsideIntervalF2)
{
    // Splits carried from below and above the search interval, and one whose
    // temperature is not finite: the stage starts from the nearer end of the
    // interval, or from its midpoint, with the carried split.
    constexpr double pressure = 50e5;
    constexpr double Tstar = 300.;
    auto cfg = departureConfigF2();
    cfg.tempMin = 290.;
    cfg.tempMax = 320.;
    const double hSpec = enthalpyF2(pressure, Tstar);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const auto& [flashedAt, incoming] :
         {std::pair{280., 280.}, std::pair{330., 330.}, std::pair{310., nan}}) {
        BOOST_TEST_CONTEXT("split from " << flashedAt << " K, incoming " << incoming << " K") {
            auto fs = carriedState<FluidSystemF2, EvaluationF2>(pressure, flashedAt, f2Z);
            fs.setTemperature(EvaluationF2(incoming));
            const double L0 = Opm::getValue(fs.L());
            BOOST_REQUIRE(L0 > 0. && L0 < 1.);
            BOOST_CHECK(solvedFromSplit<PhFlashF2>(fs, hSpec, cfg));
            BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(TightEnthalpyToleranceF2)
{
    // At this tolerance the flash at the solution of the combined iteration
    // does not confirm it: the split it solves differs slightly from that of
    // the flash. The later stages meet the tolerance.
    constexpr double pressure = 50e5;
    constexpr double Tstar = 300.;
    auto cfg = departureConfigF2();
    cfg.enthalpyTolerance = 1e-6;
    const double hSpec = enthalpyF2(pressure, Tstar);
    for (const double start : {Tstar - 20., Tstar - 1., Tstar + 1., Tstar + 20.}) {
        BOOST_TEST_CONTEXT("from " << start << " K") {
            auto fs = carriedState<FluidSystemF2, EvaluationF2>(pressure, start, f2Z);
            BOOST_REQUIRE(PhFlashF2::solve(fs, hSpec, cfg, PTFlashMethod::Ssi, ptTolerance,
                                           EOSType::PR));
            const double residual
                = EnthalpyF2::mixtureEnthalpy(fs, cfg.cpTable, EOSType::PR, cfg.model) - hSpec;
            BOOST_CHECK_SMALL(residual, cfg.enthalpyTolerance);
            BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
        }
    }
}

BOOST_AUTO_TEST_CASE(SplitIsNotUsed)
{
    constexpr double Tstar = 300.;
    const double hDeparture = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::eos_departure);

    // makeInitialState leaves L = 1 with zero phase compositions, which is
    // not a flashed state.
    {
        DebugMessages messages;
        auto fs = makeInitialState<FluidSystemF1, EvaluationF1>(f1Pressure, 310., f1Z);
        BOOST_REQUIRE(PhFlashF1::solve(fs, hDeparture, newtonConfigF1(EnthalpyModel::eos_departure),
                                       PTFlashMethod::Ssi, ptTolerance, EOSType::PR, 1));
        BOOST_CHECK(!messages.contain("incoming phase split"));
        BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
    }

    // The caloric model searches without a flash anyway.
    {
        DebugMessages messages;
        const double hCaloric = enthalpyF1(f1Pressure, Tstar, EnthalpyModel::caloric);
        auto fs = carriedState<FluidSystemF1, EvaluationF1>(f1Pressure, 310., f1Z);
        BOOST_REQUIRE(PhFlashF1::solve(fs, hCaloric, newtonConfigF1(EnthalpyModel::caloric),
                                       PTFlashMethod::Ssi, ptTolerance, EOSType::PR, 1));
        BOOST_CHECK(!messages.contain("incoming phase split"));
        BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
    }

    // A replacement provider defines its own enthalpy: from a split carried
    // at the root, the Newton stage on T evaluates it four times, as from a
    // state that was not flashed.
    {
        using Flash = Opm::PHFlash<Scalar, FluidSystemF1, PassThroughEnthalpyF1>;
        auto fs = carriedState<FluidSystemF1, EvaluationF1>(f1Pressure, Tstar, f1Z);
        PassThroughEnthalpyF1::calls = 0;
        BOOST_REQUIRE(Flash::solve(fs, hDeparture, newtonConfigF1(EnthalpyModel::eos_departure),
                                   PTFlashMethod::Ssi, ptTolerance, EOSType::PR));
        BOOST_CHECK_EQUAL(PassThroughEnthalpyF1::calls, 4);
        BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
    }

    // By default the bracket search alone runs.
    {
        DebugMessages messages;
        auto fs = carriedState<FluidSystemF1, EvaluationF1>(f1Pressure, 310., f1Z);
        BOOST_REQUIRE(PhFlashF1::solve(fs, hDeparture, makeConfigF1(EnthalpyModel::eos_departure),
                                       PTFlashMethod::Ssi, ptTolerance, EOSType::PR, 1));
        BOOST_CHECK(!messages.contain("incoming phase split"));
        BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - Tstar, roundTripTolerance);
    }

    // A split without one of the components: the later stages solve it.
    {
        DebugMessages messages;
        constexpr std::array<double, 3> z = {0., 0.6, 0.4};
        constexpr double TstarF2 = 330.;
        auto fs = carriedState<FluidSystemF2, EvaluationF2>(50e5, 325., z);
        const double L0 = Opm::getValue(fs.L());
        BOOST_REQUIRE(L0 > 0. && L0 < 1.);
        BOOST_REQUIRE(PhFlashF2::solve(fs, enthalpyF2(50e5, TstarF2, z), departureConfigF2(),
                                       PTFlashMethod::Ssi, ptTolerance, EOSType::PR, 1));
        BOOST_CHECK(!messages.contain("incoming phase split"));
        BOOST_CHECK_SMALL(Opm::getValue(fs.temperature(0)) - TstarF2, roundTripTolerance);
    }
}

BOOST_AUTO_TEST_SUITE_END()
