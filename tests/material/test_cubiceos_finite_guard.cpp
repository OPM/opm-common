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
 * \brief Non-finite cubic-EoS mixing parameters are reported as a
 *        catchable NumericalProblem, the failure mode every caller of the
 *        flash already handles, instead of an assert() (an abort in debug
 *        builds, silent non-finite values in release builds).
 *
 * The recorded case is an equimolar N2/methane mixture at 100 bar and 400 K,
 * with the standard N2/C1 interaction coefficient. Its cubic has one physical
 * root. Measured against the collapsed liquid root, the stability test drives
 * the composition iterates out of [0, 1] until the mixing parameters are no
 * longer finite; measured against the physical root, the mixture is a single
 * gas phase.
 */
#include "config.h"

#define BOOST_TEST_MODULE CubicEosFiniteGuard
#include <boost/test/unit_test.hpp>

#include <opm/common/Exceptions.hpp>

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

#include <limits>
#include <stdexcept>
#include <string_view>

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

} // anonymous namespace

// A phase composition with a non-finite mole fraction gives non-finite
// mixing parameters, which are reported as a catchable NumericalProblem.
BOOST_AUTO_TEST_CASE(NonFiniteMixingParametersThrowCatchable)
{
    auto fs = makeState(100e5, 400.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, std::numeric_limits<Scalar>::quiet_NaN());
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 1, 0.5);
    FluidSystem::ParameterCache<Evaluation> paramCache(EOSType::PR);
    BOOST_CHECK_THROW(paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx), Opm::NumericalProblem);
}

// The recorded case flashes to a single gas phase with every method.
BOOST_AUTO_TEST_CASE(RecordedCaseIsSinglePhaseGas)
{
    for (const auto method :
         {PTFlashMethod::Ssi, PTFlashMethod::Newton, PTFlashMethod::SsiNewton}) {
        BOOST_TEST_CONTEXT("method " << Opm::ptFlashMethodToString(method)) {
            auto fs = makeState(100e5, 400.0);
            bool singlePhase = false;
            BOOST_CHECK_NO_THROW(singlePhase = PtFlash::solve(fs, method, 1e-8, EOSType::PR));
            BOOST_CHECK(singlePhase);
            BOOST_CHECK_SMALL(Opm::getValue(fs.L()), 1e-12);
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
