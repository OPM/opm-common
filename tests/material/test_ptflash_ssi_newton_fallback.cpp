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
 * \brief In the "ssi+newton" flash method, Newton is a polish step on top of
 *        successive substitution — but its composition update can fail to
 *        converge on perfectly ordinary two-phase states that successive
 *        substitution handles without difficulty (an equimolar
 *        methane/n-decane mixture at 50 bar and 295 K, with the standard
 *        binary interaction coefficient, is the recorded case). Previously
 *        that failure threw away the whole solve, including the
 *        near-converged successive-substitution result it started from.
 *        With the switch-back, the solve falls back to plain successive
 *        substitution and finishes — and must produce the same answer the
 *        "ssi" method produces.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashSsiNewtonFallback
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace Opm {

//! Two-phase, two-component (C1/nC10) test fluid system with the standard
//! methane/n-decane Peng-Robinson binary interaction coefficient — the
//! configuration that exhibits the recorded Newton stall.
template<class Scalar>
class C1C10TestFluidSystem
        : public Opm::BaseFluidSystem<Scalar, C1C10TestFluidSystem<Scalar> > {
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

    using Comp0 = C1<Scalar>;
    using Comp1 = C10<Scalar>;

    template <class ValueType>
    using ParameterCache = PTFlashParameterCache<ValueType, C1C10TestFluidSystem<Scalar>>;
    using ViscosityModel = ViscosityModels<Scalar, C1C10TestFluidSystem<Scalar>>;
    using CubicEOS = ::Opm::CubicEOS<Scalar, C1C10TestFluidSystem<Scalar>>;

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

    //! standard PR literature value for the methane/n-decane pair
    static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx)
    { return (comp1Idx != comp2Idx) ? 0.0411 : 0.0; }

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

//! Test adapter whose AD fugacity equations have an exactly singular Jacobian,
//! while scalar successive substitution retains the physical EOS. The test
//! calls flash_2ph() directly; a full solve() would also use the AD branch when
//! reconstructing derivatives.
template<class Scalar>
class SingularJacobianTestFluidSystem : public C1C10TestFluidSystem<Scalar>
{
    using Parent = C1C10TestFluidSystem<Scalar>;

public:
    template<class ValueType>
    using ParameterCache = typename Parent::template ParameterCache<ValueType>;

    static inline int adFugacityCalls = 0;

    template<class FluidState,
             class LhsEval = typename FluidState::ValueType,
             class ParamCacheEval = LhsEval>
    static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                       const ParameterCache<ParamCacheEval>& paramCache,
                                       unsigned phaseIdx,
                                       unsigned compIdx)
    {
        if constexpr (!std::is_floating_point_v<LhsEval>) {
            // Constant coefficients make the two fugacity rows linearly
            // dependent with the composition-closure row.
            ++adFugacityCalls;
            return LhsEval(1.0);
        }
        else {
            return Parent::fugacityCoefficient(
                fluidState, paramCache, phaseIdx, compIdx);
        }
    }
};

//! Test adapter that makes coincident phase compositions satisfy the scalar
//! fugacity equations exactly, independently of the liquid fraction.
template<class Scalar>
class CoincidentPhasesTestFluidSystem : public C1C10TestFluidSystem<Scalar>
{
    using Parent = C1C10TestFluidSystem<Scalar>;

public:
    template<class ValueType>
    using ParameterCache = typename Parent::template ParameterCache<ValueType>;

    template<class FluidState,
             class LhsEval = typename FluidState::ValueType,
             class ParamCacheEval = LhsEval>
    static LhsEval fugacityCoefficient(const FluidState&,
                                       const ParameterCache<ParamCacheEval>&,
                                       unsigned,
                                       unsigned)
    {
        return LhsEval(1.0);
    }
};

} // namespace Opm

using Scalar = double;
using FluidSystem = Opm::C1C10TestFluidSystem<Scalar>;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem, true>;
using EOSType = Opm::CompositionalConfig::EOSType;
using PTFlashMethod = Opm::PTFlashMethod;

using SingularFluidSystem = Opm::SingularJacobianTestFluidSystem<Scalar>;
using SingularFluidState = Opm::CompositionalFluidState<Scalar, SingularFluidSystem>;
using SingularComponentVector = Dune::FieldVector<Scalar, SingularFluidSystem::numComponents>;

using CoincidentFluidSystem = Opm::CoincidentPhasesTestFluidSystem<Scalar>;
using CoincidentFluidState = Opm::CompositionalFluidState<Scalar, CoincidentFluidSystem>;
using CoincidentComponentVector = Dune::FieldVector<Scalar, CoincidentFluidSystem::numComponents>;

class SingularPtFlash : public Opm::PTFlash<Scalar, SingularFluidSystem>
{
public:
    using Opm::PTFlash<Scalar, SingularFluidSystem>::flash_2ph;
};

class CoincidentPtFlash : public Opm::PTFlash<Scalar, CoincidentFluidSystem>
{
public:
    using Opm::PTFlash<Scalar, CoincidentFluidSystem>::flash_2ph;
};

namespace {

FluidState makeState(const Scalar p, const Scalar t, const Scalar first_component = 0.5)
{
    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx)
        fs.setPressure(phaseIdx, p);
    fs.setTemperature(t);
    fs.setMoleFraction(0, first_component);
    fs.setMoleFraction(1, 1.0 - first_component);
    for (int compIdx = 0; compIdx < 2; ++compIdx)
        fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
    fs.setLvalue(-1.0);
    return fs;
}

SingularFluidState makeSingularState()
{
    SingularFluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < SingularFluidSystem::numPhases; ++phaseIdx) {
        // Preserve the deliberately exact row dependence even when the
        // compiler contracts floating-point operations.
        fs.setPressure(phaseIdx, 4194304.0); // 2^22 Pa
        fs.setSaturation(phaseIdx, 0.5);
    }
    fs.setTemperature(295.0);
    for (int compIdx = 0; compIdx < SingularFluidSystem::numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, 0.5);
    }
    return fs;
}

SingularComponentVector makeOverallComposition()
{
    SingularComponentVector z;
    z[0] = 0.5;
    z[1] = 0.5;
    return z;
}

SingularComponentVector makeInitialK()
{
    const auto fs = makeSingularState();
    SingularComponentVector K;
    for (int compIdx = 0; compIdx < SingularFluidSystem::numComponents; ++compIdx) {
        K[compIdx] = fs.wilsonK_(compIdx);
    }
    return K;
}

} // anonymous namespace

// the recorded stall state must now SOLVE under ssi+newton (previously the
// Newton polish threw away the whole solve, successive-substitution progress
// included)
BOOST_AUTO_TEST_CASE(FallbackConvergesWhereNewtonStalled)
{
    auto fs = makeState(50e5, 295.0);
    BOOST_CHECK_NO_THROW(PtFlash::solve(fs, PTFlashMethod::SsiNewton, 1e-8, EOSType::PR));
}

// and the fallback answer is THE answer: it must match the plain "ssi"
// method's result on the same state — same split, same compositions
BOOST_AUTO_TEST_CASE(FallbackMatchesSsi)
{
    auto fsSsi = makeState(50e5, 295.0);
    PtFlash::solve(fsSsi, PTFlashMethod::Ssi, 1e-8, EOSType::PR);

    auto fsFallback = makeState(50e5, 295.0);
    PtFlash::solve(fsFallback, PTFlashMethod::SsiNewton, 1e-8, EOSType::PR);

    BOOST_CHECK_SMALL(std::abs(Opm::getValue(fsFallback.L()) - Opm::getValue(fsSsi.L())), 1e-8);
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        for (int compIdx = 0; compIdx < 2; ++compIdx) {
            BOOST_CHECK_SMALL(std::abs(
                Opm::getValue(fsFallback.moleFraction(phaseIdx, compIdx)) -
                Opm::getValue(fsSsi.moleFraction(phaseIdx, compIdx))), 1e-8);
        }
    }
}

// benign states keep converging under ssi+newton, ideally via Newton itself —
// the switch-back must not change where nothing goes wrong
BOOST_AUTO_TEST_CASE(BenignStateUnaffected)
{
    auto fs = makeState(50e5, 300.0);
    BOOST_CHECK_NO_THROW(PtFlash::solve(fs, PTFlashMethod::SsiNewton, 1e-8, EOSType::PR));
}

// A finite negative-flash root is the single-phase criterion used by well
// flashes. It must not be rejected as a failed composition solve.
BOOST_AUTO_TEST_CASE(SurfaceNegativeFlashIsSingleLiquid)
{
    for (const Scalar initial_liquid_fraction : {-1.0, 0.5}) {
        auto fs = makeState(1.01325e5, 288.71, 1.e-3);
        fs.setLvalue(initial_liquid_fraction);

        BOOST_REQUIRE(PtFlash::solve(fs, PTFlashMethod::Ssi, 1.e-8, EOSType::PR));
        BOOST_CHECK_SMALL(Opm::getValue(fs.L()) - 1.0, 1.e-8);
    }
}

// A rejected stored split must use the non-trivial K estimate returned by the
// stability analysis and retry the composition solve once.
BOOST_AUTO_TEST_CASE(NewtonRecoveryRetriesFromStabilityEstimate)
{
    auto fs = makeState(50e5, 295.0);
    fs.setLvalue(0.5);
    fs.setKvalue(0, 4.0);
    fs.setKvalue(1, 4.0);

    BOOST_REQUIRE(!PtFlash::solve(fs, PTFlashMethod::Newton, 1.e-8, EOSType::PR));
    BOOST_CHECK_GT(Opm::getValue(fs.L()), 0.0);
    BOOST_CHECK_LT(Opm::getValue(fs.L()), 1.0);
}

// A singular FieldMatrix solve must trigger the hybrid SSI fallback rather
// than escape as an uncaught Dune::FMatrixError.
BOOST_AUTO_TEST_CASE(SingularJacobianFallsBackToSsi)
{
    const auto z = makeOverallComposition();

    auto newtonState = makeSingularState();
    auto newtonK = makeInitialK();
    Scalar newtonL = 0.5;
    BOOST_CHECK_THROW(SingularPtFlash::flash_2ph(z,
                                                 PTFlashMethod::Newton,
                                                 newtonK,
                                                 newtonL,
                                                 newtonState,
                                                 1.e-8,
                                                 EOSType::PR),
                      Dune::FMatrixError);

    auto ssiState = makeSingularState();
    auto ssiK = makeInitialK();
    Scalar ssiL = 0.5;
    SingularPtFlash::flash_2ph(z, PTFlashMethod::Ssi, ssiK, ssiL, ssiState, 1.e-8, EOSType::PR);

    auto hybridState = makeSingularState();
    auto hybridK = makeInitialK();
    Scalar hybridL = 0.5;
    SingularFluidSystem::adFugacityCalls = 0;
    BOOST_REQUIRE_NO_THROW(SingularPtFlash::flash_2ph(z,
                                                      PTFlashMethod::SsiNewton,
                                                      hybridK,
                                                      hybridL,
                                                      hybridState,
                                                      1.e-8,
                                                      EOSType::PR));
    BOOST_CHECK_GT(SingularFluidSystem::adFugacityCalls, 0);

    BOOST_CHECK_SMALL(std::abs(hybridL - ssiL), 1.e-8);
    for (unsigned phaseIdx = 0; phaseIdx < SingularFluidSystem::numPhases; ++phaseIdx) {
        for (int compIdx = 0; compIdx < SingularFluidSystem::numComponents; ++compIdx) {
            BOOST_CHECK_SMALL(
                std::abs(hybridState.moleFraction(phaseIdx, compIdx)
                         - ssiState.moleFraction(phaseIdx, compIdx)),
                1.e-8);
        }
    }
}

// The composition solver may return a finite negative-flash L outside [0, 1].
// Its caller uses that value to classify the mixture as single-phase.
BOOST_AUTO_TEST_CASE(NegativeFlashLiquidFractionIsNotACompositionFailure)
{
    for (const auto method : {PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        CoincidentFluidState fs;
        for (unsigned phaseIdx = 0;
             phaseIdx < CoincidentFluidSystem::numPhases;
             ++phaseIdx) {
            fs.setPressure(phaseIdx, 4194304.0);
            fs.setSaturation(phaseIdx, 0.5);
        }
        fs.setTemperature(295.0);

        CoincidentComponentVector z;
        CoincidentComponentVector K;
        z[0] = 0.5;
        z[1] = 0.5;
        K[0] = 4.0;
        K[1] = 4.0;

        Scalar L = 2.12;
        BOOST_REQUIRE_NO_THROW(
            CoincidentPtFlash::flash_2ph(z, method, K, L, fs, 1.e-8, EOSType::PR));
        BOOST_CHECK_SMALL(L - 2.12, 1.e-12);
    }
}
