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
 * \brief Test-local fixtures for the compositional flash test suite:
 *        a binary C1/nC10 fluid system (F1), the F2 and H1 systems on the
 *        generic fluid system, the canonical operating points, and a
 *        case-struct driver (FlashCase -> runFlash -> FlashOutcome) that
 *        applies the full PTFlash input contract.
 *
 * This header is deliberately Boost-free: it holds data and physics only;
 * assertions live in the test files. Shared, Boost-free helpers go here.
 */
#ifndef OPM_FLASH_TEST_FIXTURES_HPP
#define OPM_FLASH_TEST_FIXTURES_HPP

#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/C1.hpp>

#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <array>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace Opm {
namespace FlashTest {

/*!
 * \brief A two phase two component fluid system with components
 *        methane (C1) and n-decane (nC10), the "F1" synthetic fixture.
 *
 * Test-local: lives in the FlashTest namespace on purpose; it is a fixture,
 * not installable API.
 */
template<class Scalar>
class TwoComponentFluidSystem
        : public Opm::BaseFluidSystem<Scalar, TwoComponentFluidSystem<Scalar> > {
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

    //! C1/nC10 Peng-Robinson binary interaction parameter. Pinned fixture
    //! value within the range of published C1/nC10 PR coefficients (roughly
    //! 0.04-0.05, source- and temperature-dependent); the two-phase baselines
    //! built on it were cross-validated against an independent PR
    //! implementation (CoolProp) using this same value. The unequal-index
    //! shortcut in interactionCoefficient() is valid only because the system
    //! is binary.
    static constexpr Scalar bipC1nC10 = 0.0411;

    template <class ValueType>
    using ParameterCache = PTFlashParameterCache<ValueType, TwoComponentFluidSystem<Scalar>>;
    using ViscosityModel = ViscosityModels<Scalar, TwoComponentFluidSystem<Scalar>>;
    using CubicEOS = ::Opm::CubicEOS<Scalar, TwoComponentFluidSystem<Scalar>>;

    static bool phaseIsActive(unsigned phaseIdx)
    {
        return phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx;
    }

    //! \copydoc BaseFluidSystem::acentricFactor
    static Scalar acentricFactor(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::acentricFactor();
        case Comp1Idx: return Comp1::acentricFactor();
        default: throw std::runtime_error("Illegal component index for acentricFactor");
        }
    }

    //! \copydoc BaseFluidSystem::criticalTemperature
    static Scalar criticalTemperature(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalTemperature();
        case Comp1Idx: return Comp1::criticalTemperature();
        default: throw std::runtime_error("Illegal component index for criticalTemperature");
        }
    }

    //! \copydoc BaseFluidSystem::criticalPressure
    static Scalar criticalPressure(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalPressure();
        case Comp1Idx: return Comp1::criticalPressure();
        default: throw std::runtime_error("Illegal component index for criticalPressure");
        }
    }

    //! \copydoc BaseFluidSystem::criticalVolume
    static Scalar criticalVolume(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::criticalVolume();
        case Comp1Idx: return Comp1::criticalVolume();
        default: throw std::runtime_error("Illegal component index for criticalVolume");
        }
    }

    //! \copydoc BaseFluidSystem::molarMass
    static Scalar molarMass(unsigned compIdx)
    {
        switch (compIdx) {
        case Comp0Idx: return Comp0::molarMass();
        case Comp1Idx: return Comp1::molarMass();
        default: throw std::runtime_error("Illegal component index for molarMass");
        }
    }

    //! \copydoc BaseFluidSystem::interactionCoefficient
    static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx)
    {
        if (comp1Idx != comp2Idx)
            return bipC1nC10;
        return 0.0;
    }

    //! \copydoc BaseFluidSystem::phaseName
    static std::string_view phaseName(unsigned phaseIdx)
    {
        static const std::string_view name[] = {"o",   // oleic phase
                                                "g"};  // gas phase

        assert(phaseIdx < 2);
        return name[phaseIdx];
    }

    //! \copydoc BaseFluidSystem::componentName
    static std::string_view componentName(unsigned compIdx)
    {
        static const std::string_view name[] = {
                Comp0::name(),
                Comp1::name(),
        };

        assert(compIdx < 2);
        return name[compIdx];
    }

    //! \copydoc BaseFluidSystem::density
    template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
    static LhsEval density(const FluidState& fluidState,
                           const ParameterCache<ParamCacheEval>& paramCache,
                           unsigned phaseIdx)
    {
        assert(phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx);
        return decay<LhsEval>(fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx));
    }

    //! \copydoc BaseFluidSystem::viscosity
    template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
    static LhsEval viscosity(const FluidState& fluidState,
                             const ParameterCache<ParamCacheEval>& paramCache,
                             unsigned phaseIdx)
    {
        // Use LBC method to calculate viscosity
        return decay<LhsEval>(ViscosityModel::LBC(fluidState, paramCache, phaseIdx));
    }

    //! \copydoc BaseFluidSystem::fugacityCoefficient
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

    //! \copydoc BaseFluidSystem::isCompressible
    static bool isCompressible([[maybe_unused]] unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);

        return true;
    }

    //! \copydoc BaseFluidSystem::isIdealMixture
    static bool isIdealMixture([[maybe_unused]] unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);

        return false;
    }

    //! \copydoc BaseFluidSystem::isLiquid
    static bool isLiquid(unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);

        return (phaseIdx == 0);
    }

    //! \copydoc BaseFluidSystem::isIdealGas
    static bool isIdealGas(unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);

        return (phaseIdx == 1);
    }
};

/*!
 * \brief The Evaluation type the flash test suite runs PTFlash with.
 *
 * Arity = numComponents + 1, bound to the AD variable layout used by
 * makeInitialState below: slot 0 = pressure, slot 1 = temperature,
 * slots 2.. = the first numComponents-1 overall mole fractions (the last
 * mole fraction closes the sum to 1). Deriving the size anywhere else
 * invites the arity and the slot assignment to drift apart.
 */
template <class FluidSystem>
using FlashEvaluation = Opm::DenseAd::Evaluation<double, FluidSystem::numComponents + 1>;

// Canonical operating points.
// Shared anchors so the physical baselines cannot drift between test stages.
// Tests remain free to define local probe points; these are the documented
// reference conditions of the fixtures themselves.

//! F1 (C1/nC10) two-phase anchor: 50 bar, 300 K, equimolar feed, well inside
//! the two-phase window at this pressure.
inline constexpr double f1Pressure = 50e5;      // [Pa]
inline constexpr double f1Temperature = 300.;   // [K]
inline constexpr std::array<double, 2> f1Z = {0.5, 0.5};

/*!
 * \brief The F2 fluid system: CO2, C1 (methane) and nC10 (decane) on the
 *        generic three-component system, as in test_threecomponents_ptflash.
 *        Its components are static: a test that uses F2 calls
 *        registerF2Components() once per process, before the first flash.
 */
template <class Scalar>
using F2FluidSystem = GenericOilGasWaterFluidSystem<Scalar, 3, false>;

template <class FluidSystem, class Component>
void registerComponent()
{
    using CompParam = typename FluidSystem::ComponentParam;
    FluidSystem::addComponent(CompParam{Component::name(),
                                        Component::molarMass(),
                                        Component::criticalTemperature(),
                                        Component::criticalPressure(),
                                        Component::criticalVolume(),
                                        Component::acentricFactor()});
}

//! Register the F2 components in the order CO2, C1, nC10.
template <class Scalar>
void registerF2Components()
{
    using FS = F2FluidSystem<Scalar>;
    FS::init();
    registerComponent<FS, SimpleCO2<Scalar>>();
    registerComponent<FS, C1<Scalar>>();
    registerComponent<FS, C10<Scalar>>();
}

//! F2 ternary anchor: the known two-phase point of the existing
//! three-component PTFlash test. Component order of the F2 fluid system:
//! Comp0 = CO2, Comp1 = C1 (methane), Comp2 = nC10 (decane).
inline constexpr double f2Pressure = 10e5;      // [Pa]
inline constexpr double f2Temperature = 300.;   // [K]
inline constexpr std::array<double, 3> f2Z = {0.5, 0.3, 0.2};

/*!
 * \brief The cp table for the F1 fixture, in the fluid system's component
 *        order (asserted at compile time).
 */
inline CpTable<double, 2> f1CpTable()
{
    using FS = TwoComponentFluidSystem<double>;
    static_assert(std::is_same_v<typename FS::Comp0, C1<double>>,
                  "F1 cp table assumes Comp0 = C1 (methane)");
    static_assert(std::is_same_v<typename FS::Comp1, C10<double>>,
                  "F1 cp table assumes Comp1 = nC10 (decane)");
    return {C1<double>::idealGasHeatCapacityPolynomial(),
            C10<double>::idealGasHeatCapacityPolynomial()};
}

/*!
 * \brief The cp table for the F2 fixture, in the component order that
 *        registerF2Components() registers: CO2, C1 (methane), nC10 (decane).
 */
inline CpTable<double, 3> f2CpTable()
{
    return {SimpleCO2<double>::idealGasHeatCapacityPolynomial(),
            C1<double>::idealGasHeatCapacityPolynomial(),
            C10<double>::idealGasHeatCapacityPolynomial()};
}

/*!
 * \brief n-Eicosane (nC20), the heavy component of the H1 fixture.
 *
 * Its acentric factor is above 0.49, where PRCORR differs from PR. In the
 * C1-rich liquid of H1 at 250 K its fugacity coefficient is below the range
 * CubicEOS limits fugacity coefficients to.
 */
template <class Scalar>
struct NC20 {
    static std::string_view name() { return "C20"; }
    static Scalar molarMass() { return 0.28255; }          // [kg/mol]
    static Scalar criticalTemperature() { return 768.0; }  // [K]
    static Scalar criticalPressure() { return 10.7e5; }    // [Pa]
    static Scalar criticalVolume() { return 1.19; }        // [m^3/kmol]
    static Scalar acentricFactor() { return 0.907; }
};

/*!
 * \brief The H1 fluid system: C1 and nC20 on the generic two-component
 *        system, with a volume shift on both components. Its components are
 *        static: a test that uses H1 calls registerH1Components() once per
 *        process, before the first flash.
 */
template <class Scalar>
using H1FluidSystem = GenericOilGasWaterFluidSystem<Scalar, 2, false>;

//! Volume shifts (SSHIFT) of the H1 components, in registration order.
inline constexpr std::array<double, 2> h1VolumeShift = {-0.15, 0.10};

//! Register the H1 components in the order C1, nC20.
template <class Scalar>
void registerH1Components()
{
    using FS = H1FluidSystem<Scalar>;
    using CompParam = typename FS::ComponentParam;
    FS::init();
    FS::addComponent(CompParam{C1<Scalar>::name(),
                               C1<Scalar>::molarMass(),
                               C1<Scalar>::criticalTemperature(),
                               C1<Scalar>::criticalPressure(),
                               C1<Scalar>::criticalVolume(),
                               C1<Scalar>::acentricFactor(),
                               h1VolumeShift[0]});
    FS::addComponent(CompParam{NC20<Scalar>::name(),
                               NC20<Scalar>::molarMass(),
                               NC20<Scalar>::criticalTemperature(),
                               NC20<Scalar>::criticalPressure(),
                               NC20<Scalar>::criticalVolume(),
                               NC20<Scalar>::acentricFactor(),
                               h1VolumeShift[1]});
}

//! H1 anchor: 50 bar, equimolar feed; two phases from 250 K to 300 K.
inline constexpr double h1Pressure = 50e5;      // [Pa]
inline constexpr std::array<double, 2> h1Z = {0.5, 0.5};

/*!
 * \brief Build a fluid state ready for PTFlash::solve, applying the full input
 *        contract: phase pressures, global mole fractions, temperature, and the
 *        caller-side Wilson K-value + L seed.
 *
 * AD variable layout (see FlashEvaluation): pressure = slot 0, temperature =
 * slot 1, first numComponents-1 overall mole fractions = slots 2.. (the last
 * mole fraction closes the sum to 1).
 */
template <class FluidSystem, class Evaluation>
Opm::CompositionalFluidState<Evaluation, FluidSystem>
makeInitialState(double pressure, double temperature,
                 const std::array<double, FluidSystem::numComponents>& z)
{
    constexpr int numComponents = FluidSystem::numComponents;

    const Evaluation p_init = Evaluation::createVariable(pressure, 0);
    const Evaluation T_init = Evaluation::createVariable(temperature, 1);

    Opm::CompositionalFluidState<Evaluation, FluidSystem> fluid_state;
    fluid_state.setPressure(FluidSystem::oilPhaseIdx, p_init);
    fluid_state.setPressure(FluidSystem::gasPhaseIdx, p_init);

    Evaluation sum = 0.;
    for (int compIdx = 0; compIdx < numComponents - 1; ++compIdx) {
        const Evaluation zi = Evaluation::createVariable(z[compIdx], 2 + compIdx);
        fluid_state.setMoleFraction(compIdx, zi);
        sum += zi;
    }
    fluid_state.setMoleFraction(numComponents - 1, 1. - sum);

    fluid_state.setTemperature(T_init);

    // PTFlash input contract: the caller seeds the initial K-values (Wilson)
    // and the liquid fraction L before calling solve.
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
        fluid_state.setKvalue(compIdx, Ktmp);
    }
    const Evaluation Ltmp = 1.;
    fluid_state.setLvalue(Ltmp);

    return fluid_state;
}

/*!
 * \brief What phase state a flash case is expected to produce.
 *
 * single_liquid / single_vapor refer to the Li-correlation label PTFlash
 * assigns to single-phase states (pressure-blind: liquid iff T < Tc_est).
 */
enum class ExpectedPhase { any, two_phase, single_liquid, single_vapor };

/*!
 * \brief A complete flash test case: the full input state of the fluid plus
 *        the run configuration and the expected phase outcome.
 *
 * Aggregate-initializable: FlashCase<2> c{"name", 50e5, 300., {0.5, 0.5}};
 * (trailing members take their defaults).
 */
template <int numComponents>
struct FlashCase {
    std::string name;                          // shown in failure messages
    double pressure;                           // [Pa], both phases
    double temperature;                        // [K]
    std::array<double, numComponents> z;       // overall mole fractions
    PTFlashMethod method = PTFlashMethod::Ssi; // Ssi / Newton / SsiNewton
    Opm::CompositionalConfig::EOSType eos_type = Opm::CompositionalConfig::EOSType::PR;
    double tolerance = 1.e-8;                  // PTFlash convergence tolerance on the
                                               // fugacity-ratio residual (equal-fugacity mismatch)
    int verbosity = 0;
    ExpectedPhase expected = ExpectedPhase::any;
};

/*!
 * \brief The state of the fluid after the flash, as plain doubles.
 *
 * x/y/K are meaningful only when !single_phase: in single-phase states both
 * phase compositions degenerate to the feed and K carries no information.
 */
template <int numComponents>
struct FlashResult {
    bool single_phase;                         // PTFlash::solve return value
    double L;                                  // liquid mole fraction
    std::array<double, numComponents> x;       // liquid composition
    std::array<double, numComponents> y;       // vapor composition
    std::array<double, numComponents> K;       // converged equilibrium ratio y/x
    std::array<double, numComponents> K_wilson; // the input Wilson seed (fs.K() keeps this)
};

/*!
 * \brief Full outcome of running a FlashCase: the plain-double summary plus
 *        the final consistent fluid state (needed e.g. for fugacity checks).
 */
template <class FluidSystem, class Evaluation>
struct FlashOutcome {
    FlashResult<FluidSystem::numComponents> summary;
    Opm::CompositionalFluidState<Evaluation, FluidSystem> state;
};

/*!
 * \brief Run one FlashCase through PTFlash and collect the outcome.
 */
template <class FluidSystem, class Evaluation>
FlashOutcome<FluidSystem, Evaluation>
runFlash(const FlashCase<FluidSystem::numComponents>& testCase)
{
    constexpr int numComponents = FluidSystem::numComponents;
    using Flash = Opm::PTFlash<double, FluidSystem, true>;

    FlashOutcome<FluidSystem, Evaluation> outcome;
    outcome.state = makeInitialState<FluidSystem, Evaluation>(
        testCase.pressure, testCase.temperature, testCase.z);

    // record the Wilson seed: fluid_state.K() is an input to solve and still
    // holds this seed afterwards; the converged ratio is y/x
    for (int compIdx = 0; compIdx < numComponents; ++compIdx)
        outcome.summary.K_wilson[compIdx] = Opm::getValue(outcome.state.K(compIdx));

    outcome.summary.single_phase = Flash::solve(outcome.state, testCase.method,
                                                testCase.tolerance, testCase.eos_type,
                                                testCase.verbosity);

    outcome.summary.L = Opm::getValue(outcome.state.L());
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        const double x_i = Opm::getValue(outcome.state.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
        const double y_i = Opm::getValue(outcome.state.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
        outcome.summary.x[compIdx] = x_i;
        outcome.summary.y[compIdx] = y_i;
        outcome.summary.K[compIdx] = y_i / x_i;
    }
    return outcome;
}

} // namespace FlashTest
} // namespace Opm

#endif // OPM_FLASH_TEST_FIXTURES_HPP
