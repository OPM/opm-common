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
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <cmath>
#include <stdexcept>
#include <string_view>

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

} // namespace Opm

using Scalar = double;
using FluidSystem = Opm::C1C10TestFluidSystem<Scalar>;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem, true>;
using EOSType = Opm::CompositionalConfig::EOSType;

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

} // anonymous namespace

// the recorded stall state must now SOLVE under ssi+newton (previously the
// Newton polish threw away the whole solve, successive-substitution progress
// included)
BOOST_AUTO_TEST_CASE(FallbackConvergesWhereNewtonStalled)
{
    auto fs = makeState(50e5, 295.0);
    BOOST_CHECK_NO_THROW(PtFlash::solve(fs, "ssi+newton", 1e-8, EOSType::PR));
}

// and the fallback answer is THE answer: it must match the plain "ssi"
// method's result on the same state — same split, same compositions
BOOST_AUTO_TEST_CASE(FallbackMatchesSsi)
{
    auto fsSsi = makeState(50e5, 295.0);
    PtFlash::solve(fsSsi, "ssi", 1e-8, EOSType::PR);

    auto fsFallback = makeState(50e5, 295.0);
    PtFlash::solve(fsFallback, "ssi+newton", 1e-8, EOSType::PR);

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
    BOOST_CHECK_NO_THROW(PtFlash::solve(fs, "ssi+newton", 1e-8, EOSType::PR));
}
