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
 * \brief The cubic EoS must not feed floored or non-finite values into the
 *        flash. For an equimolar N2/methane mixture at 100 bar and 400 K,
 *        with the standard N2/C1 interaction coefficient, the two smaller
 *        roots of the cubic lie below the covolume. Taking one as the liquid
 *        root floored the molar volume and clamped every fugacity
 *        coefficient, which drove the flash iterates out of [0, 1] until the
 *        mixing parameters became non-finite. The finite guard turns such a
 *        composition into a catchable NumericalProblem instead of an assert()
 *        abort (debug) or silent non-finite values (release).
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
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm {

//! Two-phase, two-component (N2/C1) test fluid system: the smallest fixed
//! mixture that exhibits the recorded roots below the covolume, with the
//! standard N2/methane Peng-Robinson binary interaction coefficient.
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

// The reduced EoS state reported in issue #5385. Its rounded A and B still
// give three real PR roots, but both smaller roots lie below B. The full
// four-component fluid system is immaterial to this cubic-root selection.
struct ReportedEosState
{
    using ValueType = Scalar;
    Scalar pressure(unsigned) const
    {
        return 12000 * 6894.757293168; // 12000 psia
    }
    Scalar temperature(unsigned) const
    {
        return (237.0 - 32.0) * 5.0 / 9.0 + 273.15;
    }
};

struct ReportedEosParams
{
    Scalar A(unsigned) const
    {
        return 3.3021;
    }
    Scalar B(unsigned) const
    {
        return 1.0525;
    }
    Scalar m1(unsigned) const
    {
        return 1 + std::sqrt(2.0);
    }
    Scalar m2(unsigned) const
    {
        return 1 - std::sqrt(2.0);
    }
};

// A NaN coefficient gives NaN roots on every platform.
struct NonFiniteRootParams : ReportedEosParams
{
    Scalar A(unsigned) const
    {
        return std::numeric_limits<Scalar>::quiet_NaN();
    }
};

// The reported state of issue #5385 with the floored liquid volume it once
// got, Z = 0.0026 below B = 1.05.
template <class Value>
struct FlooredVolumeState
{
    using ValueType = Value;
    Value pressure(unsigned) const
    {
        return ReportedEosState {}.pressure(0);
    }
    Value temperature(unsigned) const
    {
        return ReportedEosState {}.temperature(0);
    }
    Value moleFraction(unsigned, unsigned compIdx) const
    {
        return compIdx == 0 ? 1.0 : 0.0;
    }
};

struct FlooredVolumeParams : ReportedEosParams
{
    Scalar molarVolume(unsigned) const
    {
        return 1e-7;
    }
    Scalar Bi(unsigned, unsigned) const
    {
        return B(0);
    }
    Scalar aCache(unsigned, unsigned, unsigned) const
    {
        return A(0);
    }
};

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

// A diverged composition update must reach the caller as a catchable
// exception, never as an abort (debug) or a non-finite state (release).
BOOST_AUTO_TEST_CASE(NonFiniteCompositionThrowsCatchable)
{
    auto fs = makeState(100e5, 400.0);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 0, std::numeric_limits<Scalar>::quiet_NaN());
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 1, 0.5);
    FluidSystem::ParameterCache<Evaluation> paramCache(EOSType::PR);
    BOOST_CHECK_THROW(paramCache.updatePhase(fs, FluidSystem::oilPhaseIdx), Opm::NumericalProblem);
}

// A NaN root must throw rather than abort on an assertion (AD) or hide
// behind the floored molar volume (plain scalars).
BOOST_AUTO_TEST_CASE(NonFiniteRootThrowsCatchable)
{
    NonFiniteRootParams params;
    const auto adState = makeState(100e5, 400.0);
    const ReportedEosState scalarState;
    for (const bool isGasPhase : {false, true}) {
        BOOST_CHECK_THROW(FluidSystem::CubicEOS::computeMolarVolume(
                              adState, params, FluidSystem::oilPhaseIdx, isGasPhase),
                          Opm::NumericalProblem);
        BOOST_CHECK_THROW(FluidSystem::CubicEOS::computeMolarVolume(
                              scalarState, params, FluidSystem::oilPhaseIdx, isGasPhase),
                          Opm::NumericalProblem);
    }
}

// Below the covolume ln(Z - B) is NaN. That must throw rather than become
// the clamp (plain scalars) or pass on as NaN (AD).
BOOST_AUTO_TEST_CASE(FugacityBelowCovolumeThrowsCatchable)
{
    const FlooredVolumeParams params;
    BOOST_CHECK_THROW(FluidSystem::CubicEOS::computeFugacityCoefficient(
                          FlooredVolumeState<Scalar> {}, params, FluidSystem::oilPhaseIdx, 0),
                      Opm::NumericalProblem);
    BOOST_CHECK_THROW(FluidSystem::CubicEOS::computeFugacityCoefficient(
                          FlooredVolumeState<Evaluation> {}, params, FluidSystem::oilPhaseIdx, 0),
                      Opm::NumericalProblem);
}

// Both smaller roots lie below the covolume here, so the liquid shares the
// vapour root rather than a floored volume with clamped fugacities.
BOOST_AUTO_TEST_CASE(LiquidRootLiesAboveCovolume)
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        fs.setPressure(phaseIdx, 100e5);
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            fs.setMoleFraction(phaseIdx, compIdx, 0.5);
        }
    }
    fs.setTemperature(400.0);

    FluidSystem::ParameterCache<Scalar> paramCache(EOSType::PR);
    paramCache.updatePhase(fs, FluidSystem::oilPhaseIdx);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);
    BOOST_CHECK_CLOSE(paramCache.molarVolume(FluidSystem::oilPhaseIdx),
                      paramCache.molarVolume(FluidSystem::gasPhaseIdx),
                      1e-10);
    for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
        BOOST_CHECK_CLOSE(
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::oilPhaseIdx, compIdx),
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::gasPhaseIdx, compIdx),
            1e-10);
    }
}

// Issue #5385: the high-pressure PR cubic has two inadmissible roots. Both
// phase labels must use the sole root above the covolume.
BOOST_AUTO_TEST_CASE(ReportedHighPressureRoots)
{
    ReportedEosState fs;
    ReportedEosParams params;
    const auto liquidVolume
        = FluidSystem::CubicEOS::computeMolarVolume(fs, params, FluidSystem::oilPhaseIdx, false);
    const auto vapourVolume
        = FluidSystem::CubicEOS::computeMolarVolume(fs, params, FluidSystem::gasPhaseIdx, true);
    const auto Z = fs.pressure(FluidSystem::oilPhaseIdx) * liquidVolume
        / (Opm::Constants<Scalar>::R * fs.temperature(FluidSystem::oilPhaseIdx));

    BOOST_CHECK_GT(Z, params.B(FluidSystem::oilPhaseIdx));
    BOOST_CHECK_CLOSE_FRACTION(Z, 1.66193933, 1e-7);
    BOOST_CHECK_CLOSE_FRACTION(liquidVolume, vapourVolume, 1e-12);
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

// The supercritical feed is a single vapour under every flash method.
BOOST_AUTO_TEST_CASE(SupercriticalStateIsSingleVapour)
{
    for (const auto method :
         {PTFlashMethod::Newton, PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        BOOST_TEST_CONTEXT("method " << static_cast<int>(method))
        {
            auto fs = makeState(100e5, 400.0);
            BOOST_REQUIRE(PtFlash::solve(fs, method, 1e-8, EOSType::PR));
            BOOST_CHECK_SMALL(Opm::getValue(fs.L()), 1e-8);
        }
    }
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
