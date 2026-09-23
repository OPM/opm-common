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
 * \brief The isothermal flash's iteration can diverge on deeply
 *        supercritical feeds (an equimolar N2/methane mixture at 100 bar
 *        and 400 K, with the standard N2/C1 interaction coefficient, is the
 *        recorded case): the composition iterates leave [0, 1] far enough
 *        that the cubic-EoS mixing parameters become non-finite. Before the
 *        finite guard this hit an assert() — process ABORT in debug builds
 *        and SILENT propagation of non-finite values in release builds. The
 *        guard turns both into a catchable NumericalProblem, which is the
 *        failure mode every caller of the flash already handles.
 */
#include "config.h"

#define BOOST_TEST_MODULE CubicEosFiniteGuard
#include <boost/test/unit_test.hpp>

#include <opm/common/Exceptions.hpp>

#include <opm/material/common/PolynomialUtils.hpp>
#include <opm/material/components/C1.hpp>
#include <opm/material/components/N2.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm {

//! Two-phase, two-component (N2/C1) test fluid system: the smallest fixed
//! mixture that still exhibits the recorded divergence, with the standard
//! N2/methane Peng-Robinson binary interaction coefficient.
template<class Scalar>
class N2C1TestFluidSystem
        : public Opm::BaseFluidSystem<Scalar, N2C1TestFluidSystem<Scalar> > {
public:
    static constexpr int numPhases = 2;
    static constexpr int numComponents = 2;
    static constexpr int numMisciblePhases = 2;
    static constexpr int numMiscibleComponents = 2;
    static constexpr bool waterEnabled = false;
    static constexpr int oilPhaseIdx = 0;
    static constexpr int gasPhaseIdx = 1;
    static constexpr int waterPhaseIdx = -1;

    static constexpr int Comp0Idx = 0;
    static constexpr int Comp1Idx = 1;

    using Comp0 = N2<Scalar>;
    using Comp1 = C1<Scalar>;

    template <class ValueType>
    using ParameterCache = PTFlashParameterCache<ValueType, N2C1TestFluidSystem<Scalar>>;
    using ViscosityModel = ViscosityModels<Scalar, N2C1TestFluidSystem<Scalar>>;
    using CubicEOS = ::Opm::CubicEOS<Scalar, N2C1TestFluidSystem<Scalar>>;

    static bool phaseIsActive(unsigned phaseIdx)
    { return phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx; }

    static Scalar acentricFactor(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::acentricFactor();
        case Comp1Idx: return Comp1::acentricFactor();
        default: throw std::runtime_error("Illegal component index for acentricFactor");
        }
    }

    static Scalar criticalTemperature(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalTemperature();
        case Comp1Idx: return Comp1::criticalTemperature();
        default: throw std::runtime_error("Illegal component index for criticalTemperature");
        }
    }

    static Scalar criticalPressure(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalPressure();
        case Comp1Idx: return Comp1::criticalPressure();
        default: throw std::runtime_error("Illegal component index for criticalPressure");
        }
    }

    static Scalar criticalVolume(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalVolume();
        case Comp1Idx: return Comp1::criticalVolume();
        default: throw std::runtime_error("Illegal component index for criticalVolume");
        }
    }

    static Scalar molarMass(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::molarMass();
        case Comp1Idx: return Comp1::molarMass();
        default: throw std::runtime_error("Illegal component index for molarMass");
        }
    }

    //! standard PR literature value for the N2/methane pair
    static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx)
    { return (comp1Idx != comp2Idx) ? 0.0291 : 0.0; }

    static std::string_view phaseName(unsigned phaseIdx)
    {
        static const std::string_view name[] = {"o", "g"};
        assert(phaseIdx < 2);
        return name[phaseIdx];
    }

    static std::string_view componentName(unsigned compIdx)
    {
        static const std::string_view name[] = {Comp0::name(), Comp1::name()};
        assert(compIdx < 2);
        return name[compIdx];
    }

    template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
    static LhsEval density(const FluidState& fluidState,
                           const ParameterCache<ParamCacheEval>& paramCache,
                           unsigned phaseIdx)
    {
        assert(phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx);
        return decay<LhsEval>(fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx));
    }

    template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
    static LhsEval viscosity(const FluidState& fluidState,
                             const ParameterCache<ParamCacheEval>& paramCache,
                             unsigned phaseIdx)
    { return decay<LhsEval>(ViscosityModel::LBC(fluidState, paramCache, phaseIdx)); }

    template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
    static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                       const ParameterCache<ParamCacheEval>& paramCache,
                                       unsigned phaseIdx,
                                       unsigned compIdx)
    {
        assert(phaseIdx < numPhases);
        assert(compIdx < numComponents);
        return decay<LhsEval>(CubicEOS::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx));
    }

    static bool isCompressible([[maybe_unused]] unsigned phaseIdx)
    { return true; }

    static bool isIdealMixture([[maybe_unused]] unsigned phaseIdx)
    { return false; }

    static bool isLiquid(unsigned phaseIdx)
    { return (phaseIdx == 0); }
};

} // namespace Opm

using Scalar = double;
using FluidSystem = Opm::N2C1TestFluidSystem<Scalar>;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem, true>;
using EOSType = Opm::CompositionalConfig::EOSType;
using PTFlashMethod = Opm::PTFlashMethod;

namespace {

FluidState makeState(const Scalar p, const Scalar t)
{
    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx)
        fs.setPressure(phaseIdx, p);
    fs.setTemperature(t);
    fs.setMoleFraction(0, 0.5);
    fs.setMoleFraction(1, 0.5);
    for (int compIdx = 0; compIdx < 2; ++compIdx)
        fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
    fs.setLvalue(-1.0);
    return fs;
}

// Coefficients of the Peng-Robinson cubic in Z, highest power first.
template <class Value>
std::array<Value, 4>
pengRobinsonCubic(const Value& A, const Value& B)
{
    const Scalar m1 = 1 + std::sqrt(2.0);
    const Scalar m2 = 1 - std::sqrt(2.0);
    return {Value(1.0),
            (m1 + m2 - 1) * B - 1,
            A + m1 * m2 * B * B - (m1 + m2) * B * (B + 1),
            -A * B - m1 * m2 * B * B * (B + 1)};
}

// Whether every root cubicRoots() returns is finite, in value and
// derivatives, and solves the cubic to within rounding.
template <class Value>
bool
cubicRootsAreSound(const Value& A, const Value& B)
{
    const auto c = pengRobinsonCubic(A, B);
    Value Z[3] = {0.0, 0.0, 0.0};
    const unsigned numRoots = Opm::cubicRoots(Z, c[0], c[1], c[2], c[3]);
    for (unsigned rootIdx = 0; rootIdx < numRoots; ++rootIdx) {
        const Scalar z = Opm::getValue(Z[rootIdx]);
        if (!std::isfinite(z)) {
            return false;
        }
        if constexpr (!std::is_same_v<Value, Scalar>) {
            for (int varIdx = 0; varIdx < Z[rootIdx].size(); ++varIdx) {
                if (!std::isfinite(Z[rootIdx].derivative(varIdx))) {
                    return false;
                }
            }
        }
        Scalar residual = 0.0;
        Scalar scale = 0.0;
        for (const auto& coefficient : c) {
            residual = residual * z + Opm::getValue(coefficient);
            scale = scale * std::abs(z) + std::abs(Opm::getValue(coefficient));
        }
        if (std::abs(residual) > 1e-8 * scale) {
            return false;
        }
    }
    return numRoots == 1 || numRoots == 3;
}

} // anonymous namespace

// the recorded divergence case must report as a catchable exception —
// never as an abort (debug) or a silent non-finite state (release).
// Probed via the pure "newton" method: in "ssi+newton" the switch-back
// catches this exception internally and finishes with successive
// substitution (see test_ptflash_ssi_newton_fallback.cpp).
BOOST_AUTO_TEST_CASE(NewtonDivergenceThrowsCatchable)
{
    auto fs = makeState(100e5, 400.0);
    BOOST_CHECK_THROW(
        PtFlash::solve(fs, PTFlashMethod::Newton, 1e-8, EOSType::PR),
        Opm::NumericalProblem);
}

// the default method's honest failure mode on the same state is unchanged
// (successive substitution reports its own non-convergence as a throw)
BOOST_AUTO_TEST_CASE(SsiStillFailsLoudlyNotFatally)
{
    auto fs = makeState(100e5, 400.0);
    BOOST_CHECK_THROW(
        PtFlash::solve(fs, PTFlashMethod::Ssi, 1e-8, EOSType::PR),
        std::runtime_error);
}

// Where one and three real roots meet, rounding put the acos or acosh
// argument outside its domain and cubicRoots() returned NaN.
BOOST_AUTO_TEST_CASE(CubicRootsSoundWhereRootCountChanges)
{
    using Eval = Opm::DenseAd::Evaluation<Scalar, 1>;
    std::vector<std::pair<Scalar, Scalar>> unsound;
    const auto check = [&unsound](const Scalar A, const Scalar B) {
        if (!cubicRootsAreSound(A, B) || !cubicRootsAreSound(Eval(A), Eval::createVariable(B, 0))) {
            unsound.emplace_back(A, B);
        }
    };

    check(0.85500000000000009, 0.43664024846283928);
    const auto numRoots = [](const Scalar A, const Scalar B) {
        const auto c = pengRobinsonCubic(A, B);
        Scalar Z[3];
        return Opm::cubicRoots(Z, c[0], c[1], c[2], c[3]);
    };
    for (int i = 0; i < 200; ++i) {
        const Scalar A = 0.05 + 0.05 * i;
        Scalar lower = 1e-4;
        Scalar upper = 3.0;
        if (numRoots(A, lower) == numRoots(A, upper)) {
            continue;
        }
        for (int iteration = 0; iteration < 100; ++iteration) {
            const Scalar middle = (lower + upper) / 2;
            (numRoots(A, middle) == numRoots(A, lower) ? lower : upper) = middle;
        }
        for (int step = -50; step <= 50; ++step) {
            check(A, lower * (1 + step * 1e-15));
        }
    }

    const auto first = unsound.empty() ? std::pair<Scalar, Scalar> {} : unsound.front();
    BOOST_CHECK_MESSAGE(unsound.empty(),
                        unsound.size() << " cubics with an unsound root, the first at A = "
                                       << first.first << ", B = " << first.second);
}

// Without a linear term the depressed cubic t^3 + q = 0 has the one real root
// -cbrt(q). cubicRoots() threw std::runtime_error there, which the flash does
// not recover from.
BOOST_AUTO_TEST_CASE(CubicRootsWithoutLinearTerm)
{
    // x^3 - 3x^2 + 3x + d = (x - 1)^3 + 1 + d has the single real root 0 at
    // d = 0, where dx/dd = -1/3.
    Scalar Z[3] = {};
    BOOST_REQUIRE_EQUAL(Opm::cubicRoots(Z, 1.0, -3.0, 3.0, 0.0), 1u);
    BOOST_CHECK_SMALL(Z[0], 1e-14);

    using Eval = Opm::DenseAd::Evaluation<Scalar, 1>;
    Eval ZAd[3];
    BOOST_REQUIRE_EQUAL(
        Opm::cubicRoots(ZAd, Eval(1.0), Eval(-3.0), Eval(3.0), Eval::createVariable(0.0, 0)), 1u);
    BOOST_CHECK_SMALL(ZAd[0].value(), 1e-14);
    BOOST_CHECK_CLOSE(ZAd[0].derivative(0), -1.0 / 3.0, 1e-10);
}

// an ordinary state keeps flashing under both methods — the guard costs
// nothing where nothing goes wrong
BOOST_AUTO_TEST_CASE(BenignStateUnaffected)
{
    for (const auto method : {PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        auto fs = makeState(100e5, 300.0);
        BOOST_CHECK_NO_THROW(PtFlash::solve(fs, method, 1e-8, EOSType::PR));
    }
}
