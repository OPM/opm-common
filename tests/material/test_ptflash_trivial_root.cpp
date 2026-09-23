// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital, Mathematics and Cybernetics.

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
 * \brief Regression test for Newton converging to a non-physical trivial root.
 *
 * At \f$x = y\f$ the component balance \f$z - L x - (1 - L) y\f$ no longer
 * depends on \f$L\f$, so the Newton Jacobian is singular in that direction and
 * round-off can produce a small residual at an unbounded, non-physical value of
 * \f$L\f$. Newton can therefore report convergence instead of signaling failure.
 *
 * The state recorded here is a near-pure CO2 cell taken from the
 * co2_ptflash_ecfv model test. Before the guard was added, Newton returned
 * \f$L = -2.5\cdot10^{16}\f$ and a mole fraction of \f$-0.78\f$, causing a
 * non-finite mobility. Successive substitution instead converges to coincident
 * phases with an indeterminate phase fraction. The flash must reject Newton's
 * result and reclassify the coincident state through stability analysis.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashTrivialRoot
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <dune/common/fvector.hh>

#include <cmath>
#include <limits>
#include <string_view>

using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
using PTFlashMethod = Opm::PTFlashMethod;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;

namespace {

template <class Component>
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

// The component parameters are static, so they are registered once per process.
struct Fixture
{
    Fixture()
    {
        FluidSystem::init();
        registerComponent<Opm::SimpleCO2<Scalar>>();
        registerComponent<Opm::C1<Scalar>>();
        registerComponent<Opm::C10<Scalar>>();
    }
};

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

namespace {

constexpr int numComponents = FluidSystem::numComponents;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, numComponents + 1>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem>;
using ScalarFluidState = Opm::CompositionalFluidState<Scalar, FluidSystem>;
using ScalarComponentVector = Dune::FieldVector<Scalar, numComponents>;

class ExposedPtFlash : public PtFlash
{
public:
    using PtFlash::flash_2ph;
    using PtFlash::phasesCoincide_;
};

constexpr Scalar flashTolerance = 1.e-8;

// The positive incoming liquid fraction marks this as a warm-started two-phase
// state, so the flash skips stability analysis and uses the stored K values.
FluidState makeRecordedState()
{
    constexpr Scalar pressure = 14981770.14236617;
    constexpr Scalar temperature = 423.25;
    constexpr Scalar z[numComponents] = {0.9900000005514261,
                                         0.009000003165640235,
                                         0.0009999962829336465};
    constexpr Scalar K[numComponents] = {6.260223748149762,
                                         12.126317087860864,
                                         0.0071260520516577735};

    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        fs.setPressure(phaseIdx, pressure);
    }
    fs.setTemperature(temperature);
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, z[compIdx]);
        fs.setKvalue(compIdx, K[compIdx]);
    }
    fs.setLvalue(0.2252754135811573);
    return fs;
}

FluidState makeDifferentiatedRecordedState()
{
    auto fs = makeRecordedState();
    const auto pressure = Opm::getValue(fs.pressure(0));
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        fs.setPressure(phaseIdx, Evaluation::createVariable(pressure, 0));
    }
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        const auto composition = Opm::getValue(fs.moleFraction(compIdx));
        fs.setMoleFraction(
            compIdx, Evaluation::createVariable(composition, compIdx + 1));
    }
    return fs;
}

FluidState makePhaseTransitionState(const Scalar pressure, const Scalar liquid_fraction)
{
    constexpr Scalar temperature = 350.0;
    constexpr Scalar z[numComponents] = {0.5, 0.3, 0.2};
    constexpr Scalar K[numComponents] = {2.0, 3.0, 0.05};

    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        fs.setPressure(phaseIdx, pressure);
    }
    fs.setTemperature(temperature);
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, z[compIdx]);
        fs.setKvalue(compIdx, K[compIdx]);
    }
    fs.setLvalue(liquid_fraction);
    return fs;
}

bool isMoleFractionLike(const Evaluation& value)
{
    const Scalar scalar_value = Opm::getValue(value);
    return std::isfinite(scalar_value) && scalar_value >= -flashTolerance
        && scalar_value <= 1. + flashTolerance;
}

} // anonymous namespace

// A warm start must reach the same phase classification as a cold stability test.
BOOST_AUTO_TEST_CASE(ColdAndWarmHybridClassifySinglePhase)
{
    auto cold = makeRecordedState();
    cold.setLvalue(-1.0);
    BOOST_REQUIRE(PtFlash::solve(cold, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    auto warm = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(warm, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    BOOST_CHECK_SMALL(Opm::getValue(cold.L()), flashTolerance);
    BOOST_CHECK_SMALL(Opm::getValue(warm.L()), flashTolerance);
}

// Reclassification must avoid reconstructing the singular two-phase derivatives.
BOOST_AUTO_TEST_CASE(WarmHybridReturnsSinglePhaseDerivatives)
{
    auto fs = makeDifferentiatedRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    BOOST_CHECK(isMoleFractionLike(fs.L()));
    BOOST_CHECK_SMALL(Opm::getValue(fs.L()), flashTolerance);
    for (int derivativeIdx = 0; derivativeIdx < numComponents + 1; ++derivativeIdx) {
        BOOST_CHECK_SMALL(fs.L().derivative(derivativeIdx), flashTolerance);
    }
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            BOOST_CHECK(isMoleFractionLike(fs.moleFraction(phaseIdx, compIdx)));
            BOOST_CHECK_SMALL(
                Opm::getValue(fs.moleFraction(phaseIdx, compIdx)
                              - fs.moleFraction(compIdx)),
                flashTolerance);
        }
    }
}

// Both composition methods must reclassify the coincident SSI result.
BOOST_AUTO_TEST_CASE(WarmSsiAndHybridClassifySinglePhase)
{
    auto fs_ssi = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs_ssi, PTFlashMethod::Ssi, flashTolerance, EOSType::PR));

    auto fs_hybrid = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs_hybrid, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    BOOST_CHECK_SMALL(std::abs(Opm::getValue(fs_hybrid.L()) - Opm::getValue(fs_ssi.L())),
                      flashTolerance);
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            BOOST_CHECK_SMALL(
                std::abs(Opm::getValue(fs_hybrid.moleFraction(phaseIdx, compIdx))
                         - Opm::getValue(fs_ssi.moleFraction(phaseIdx, compIdx))),
                flashTolerance);
        }
    }
}

// A rejected warm-started Newton split must be reclassified instead of escaping.
BOOST_AUTO_TEST_CASE(NewtonReassessesNonPhysicalRoot)
{
    auto fs = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs, PTFlashMethod::Newton, flashTolerance, EOSType::PR));
    BOOST_CHECK_SMALL(Opm::getValue(fs.L()), flashTolerance);
}

// Invalid ratios from a diverged warm start must enter the existing stability
// recovery instead of escaping the flash.
BOOST_AUTO_TEST_CASE(NonFiniteRatiosTriggerStabilityRecovery)
{
    auto fs = makeRecordedState();
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setKvalue(compIdx, std::numeric_limits<Scalar>::infinity());
    }

    BOOST_REQUIRE(PtFlash::solve(fs, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));
    BOOST_CHECK_SMALL(Opm::getValue(fs.L()), flashTolerance);
}

// At 970 bar this feed's cubic has two roots below the covolume. A floored
// liquid root clamped every fugacity coefficient, and substitution converged
// on that clamp as a single vapour.
BOOST_AUTO_TEST_CASE(DenseLiquidKeepsLiquidLabel)
{
    for (const auto method :
         {PTFlashMethod::Newton, PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        BOOST_TEST_CONTEXT("method " << static_cast<int>(method))
        {
            constexpr Scalar z[numComponents] = {0.1, 0.6, 0.3};
            FluidState fs;
            for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
                fs.setPressure(phaseIdx, 970.e5);
            }
            fs.setTemperature(400.0);
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                fs.setMoleFraction(compIdx, z[compIdx]);
                fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
            }
            fs.setLvalue(-1.0);

            BOOST_REQUIRE(PtFlash::solve(fs, method, flashTolerance, EOSType::PR));
            BOOST_CHECK_EQUAL(Opm::getValue(fs.L()), 1.0);
        }
    }
}

// Out-of-range negative-flash and coincident SSI results both indicate that a
// warm-started cell has crossed into the single-phase region.
BOOST_AUTO_TEST_CASE(WarmSsiReassessesOutOfRangeSplits)
{
    constexpr Scalar pressures[] = {180.e5, 220.e5};
    for (const Scalar pressure : pressures) {
        auto cold = makePhaseTransitionState(pressure, -1.0);
        BOOST_REQUIRE(PtFlash::solve(cold, PTFlashMethod::Ssi, flashTolerance, EOSType::PR));

        auto warm = makePhaseTransitionState(pressure, 0.5);
        BOOST_REQUIRE(PtFlash::solve(warm, PTFlashMethod::Ssi, flashTolerance, EOSType::PR));

        BOOST_CHECK_SMALL(Opm::getValue(cold.L()) - 1.0, flashTolerance);
        BOOST_CHECK_SMALL(Opm::getValue(warm.L()) - 1.0, flashTolerance);
    }
}

// An out-of-range negative-flash result identifies which side of the phase
// boundary the mixture occupies; Li's approximate label must not reverse it.
BOOST_AUTO_TEST_CASE(NegativeFlashPreservesVapourDirection)
{
    for (const auto method : {PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        auto fs = makePhaseTransitionState(1.e5, 0.5);
        fs.setTemperature(400.0);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
        }

        BOOST_REQUIRE(PtFlash::solve(fs, method, flashTolerance, EOSType::PR));
        BOOST_CHECK_SMALL(Opm::getValue(fs.L()), flashTolerance);
    }
}

// A uniform K vector produces identical normalised phase compositions and must
// exercise the initial Newton guard directly.
BOOST_AUTO_TEST_CASE(NewtonRejectsCoincidentInitialPhases)
{
    constexpr Scalar pressure = 14981770.14236617;
    constexpr Scalar temperature = 423.25;
    constexpr Scalar composition[numComponents] = {0.99, 0.009, 0.001};

    ScalarFluidState fs;
    ScalarComponentVector z;
    ScalarComponentVector K;
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        fs.setPressure(phaseIdx, pressure);
        fs.setSaturation(phaseIdx, 0.5);
    }
    fs.setTemperature(temperature);
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        z[compIdx] = composition[compIdx];
        K[compIdx] = 4.0;
    }
    Scalar L = 0.5;

    const auto reports_initial_trivial_solution = [](const Opm::NumericalProblem& error) {
        return std::string_view{error.what()}.find("started from the trivial solution")
            != std::string_view::npos;
    };
    BOOST_CHECK_EXCEPTION(ExposedPtFlash::flash_2ph(
                              z, PTFlashMethod::Newton, K, L, fs, flashTolerance, EOSType::PR),
                          Opm::NumericalProblem,
                          reports_initial_trivial_solution);
}

// An absent component has x = y = 0 and therefore an undefined K. It must not
// make a genuine split between the remaining components look trivial or create
// a 0/0 fugacity ratio in the default hybrid method.
BOOST_AUTO_TEST_CASE(HybridAllowsAbsentComponent)
{
    auto fs = makeRecordedState();
    fs.setMoleFraction(0, 0.0);
    fs.setMoleFraction(1, 0.5);
    fs.setMoleFraction(2, 0.5);
    BOOST_REQUIRE(!PtFlash::solve(fs, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    BOOST_CHECK_GT(Opm::getValue(fs.L()), 0.0);
    BOOST_CHECK_LT(Opm::getValue(fs.L()), 1.0);
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        BOOST_CHECK_SMALL(Opm::getValue(fs.moleFraction(phaseIdx, 0)), flashTolerance);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            BOOST_CHECK(isMoleFractionLike(fs.moleFraction(phaseIdx, compIdx)));
        }
    }
}

// An endpoint of the estimated K values is not a thermodynamic phase test.
// These warm estimates start at an endpoint or reach one during substitution,
// and collapse onto the trivial solution; the warm-start stability retry must
// still recover the equilibrium a stability-seeded flash finds.
BOOST_AUTO_TEST_CASE(EndpointEstimatesRecoverTwoPhaseEquilibrium)
{
    for (const auto method :
         {PTFlashMethod::Newton, PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        auto reference = makeRecordedState();
        reference.setMoleFraction(0, 0.0);
        reference.setMoleFraction(1, 0.5);
        reference.setMoleFraction(2, 0.5);
        BOOST_CHECK(!PtFlash::solve(reference, method, flashTolerance, EOSType::PR));

        for (const Scalar heavy_k : {0.8, 0.4}) {
            BOOST_TEST_CONTEXT("method " << static_cast<int>(method) << ", heavy K " << heavy_k)
            {
                auto warm = makeRecordedState();
                warm.setMoleFraction(0, 0.0);
                warm.setMoleFraction(1, 0.5);
                warm.setMoleFraction(2, 0.5);
                warm.setKvalue(0, 1.0);
                warm.setKvalue(1, 2.0);
                warm.setKvalue(2, heavy_k);

                BOOST_CHECK(!PtFlash::solve(warm, method, flashTolerance, EOSType::PR));
                BOOST_CHECK_SMALL(Opm::getValue(warm.L() - reference.L()), 1.e-7);
                for (unsigned phase = 0; phase < FluidSystem::numPhases; ++phase) {
                    for (int comp = 0; comp < numComponents; ++comp) {
                        BOOST_CHECK_SMALL(Opm::getValue(warm.moleFraction(phase, comp)
                                                        - reference.moleFraction(phase, comp)),
                                          1.e-7);
                    }
                }
            }
        }
    }
}

// Newton has no split to refine from an endpoint estimate, so the Newton method
// substitutes there first. Stale Wilson estimates from 5 bar give an endpoint
// at 39 and 41 bar, where Newton started from it hits a singular Jacobian.
BOOST_AUTO_TEST_CASE(NewtonSubstitutesFromEndpointEstimate)
{
    constexpr Scalar temperature = 470.0;
    constexpr Scalar z[numComponents] = {0.1, 0.6, 0.3};
    const auto makeWarmState = [&z, temperature](const Scalar pressure) {
        FluidState stale;
        FluidState fs;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            stale.setPressure(phaseIdx, 5.e5);
            fs.setPressure(phaseIdx, pressure);
        }
        stale.setTemperature(temperature);
        fs.setTemperature(temperature);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            fs.setMoleFraction(compIdx, z[compIdx]);
            fs.setKvalue(compIdx, stale.wilsonK_(compIdx));
        }
        fs.setLvalue(0.29);
        return fs;
    };

    for (const Scalar pressure : {39.e5, 41.e5}) {
        BOOST_TEST_CONTEXT("pressure " << pressure)
        {
            auto ssi = makeWarmState(pressure);
            auto newton = makeWarmState(pressure);
            BOOST_CHECK(!PtFlash::solve(ssi, PTFlashMethod::Ssi, flashTolerance, EOSType::PR));
            BOOST_CHECK(
                !PtFlash::solve(newton, PTFlashMethod::Newton, flashTolerance, EOSType::PR));
            BOOST_CHECK_SMALL(Opm::getValue(newton.L() - ssi.L()), 1.e-7);
        }
    }
}

// Newton can reach the trivial root again after the stability retry. This
// state is two-phase, so Newton may fail on it but must not report one phase.
BOOST_AUTO_TEST_CASE(RetryRejectsNewtonTrivialRoot)
{
    constexpr Scalar z[numComponents]
        = {0.64496973076836983, 0.035832069958223252, 0.31919819927340687};
    constexpr Scalar K[numComponents]
        = {1.3358649522511163, 1.4550115090329607, 0.57519955699825431};
    const auto makeWarmState = [&z, &K]() {
        FluidState fs;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            fs.setPressure(phaseIdx, 12763009.333978247);
        }
        fs.setTemperature(519.46257482288365);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            fs.setMoleFraction(compIdx, z[compIdx]);
            fs.setKvalue(compIdx, K[compIdx]);
        }
        fs.setLvalue(0.13865234839438245);
        return fs;
    };

    auto hybrid = makeWarmState();
    BOOST_REQUIRE(!PtFlash::solve(hybrid, PTFlashMethod::SsiNewton, flashTolerance, EOSType::PR));

    auto newton = makeWarmState();
    try {
        BOOST_CHECK(!PtFlash::solve(newton, PTFlashMethod::Newton, flashTolerance, EOSType::PR));
    } catch (const Opm::NumericalProblem&) {
        // Failing is acceptable here; a single-phase answer is not.
    }
}

// Warm ratios near one can make substitution converge to the split with its
// phases in the opposite slots; the oil slot must still hold the heavier phase.
BOOST_AUTO_TEST_CASE(NearUnitWarmStartKeepsHeavierPhaseAsOil)
{
    constexpr Scalar z[numComponents] = {0.76524, 0.16825, 0.06651};
    constexpr Scalar nearUnitK[numComponents] = {0.99, 1.01, 1.02};
    const auto makeState = [&z](const Scalar liquid_fraction) {
        FluidState fs;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            fs.setPressure(phaseIdx, 135.42e5);
        }
        fs.setTemperature(394.46);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            fs.setMoleFraction(compIdx, z[compIdx]);
            fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
        }
        fs.setLvalue(liquid_fraction);
        return fs;
    };

    auto reference = makeState(-1.0);
    BOOST_REQUIRE(!PtFlash::solve(reference, PTFlashMethod::Ssi, flashTolerance, EOSType::PR));

    for (const auto method :
         {PTFlashMethod::Newton, PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        BOOST_TEST_CONTEXT("method " << static_cast<int>(method))
        {
            auto warm = makeState(0.5);
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                warm.setKvalue(compIdx, nearUnitK[compIdx]);
            }
            BOOST_CHECK(!PtFlash::solve(warm, method, flashTolerance, EOSType::PR));
            BOOST_CHECK_SMALL(Opm::getValue(warm.L() - reference.L()), 1.e-7);
            for (unsigned phase = 0; phase < FluidSystem::numPhases; ++phase) {
                for (int comp = 0; comp < numComponents; ++comp) {
                    BOOST_CHECK_SMALL(Opm::getValue(warm.moleFraction(phase, comp)
                                                    - reference.moleFraction(phase, comp)),
                                      1.e-7);
                }
            }
        }
    }
}

// An absent component contributes no ratio, but the remaining equal phase
// compositions must still be recognised as coincident.
BOOST_AUTO_TEST_CASE(AbsentComponentDoesNotHideCoincidentPhases)
{
    ScalarFluidState fs;
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 0, 0.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, 0.0);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 1, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 1, 0.5);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 2, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 2, 0.5);

    BOOST_CHECK(ExposedPtFlash::phasesCoincide_(fs));
}

// Normalisation makes a single active component identical in both phases for
// every K, so it cannot by itself establish that a Newton state is trivial.
BOOST_AUTO_TEST_CASE(SingleActiveComponentIsNotCoincident)
{
    ScalarFluidState fs;
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 0, 1.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, 1.0);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 1, 0.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 1, 0.0);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 2, 0.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 2, 0.0);

    BOOST_CHECK(!ExposedPtFlash::phasesCoincide_(fs));
}

// A non-positive component must not hide coincidence among the components
// whose phase ratio remains defined. Bounds validation rejects the bad state.
BOOST_AUTO_TEST_CASE(NonPositiveComponentDoesNotHideCoincidentPhases)
{
    ScalarFluidState fs;
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 0, -1.e-9);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, 1.e-9);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 1, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 1, 0.5);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 2, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 2, 0.5);

    BOOST_CHECK(ExposedPtFlash::phasesCoincide_(fs));
}

// Skipping an invalid component must not hide a genuine split among the
// remaining components.
BOOST_AUTO_TEST_CASE(InvalidActiveCompositionIsNotCoincident)
{
    ScalarFluidState fs;
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 0, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, -0.5);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 1, 0.5);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 1, 1.5);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, 2, 0.0);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 2, 0.0);

    BOOST_CHECK(!ExposedPtFlash::phasesCoincide_(fs));
}

// A single component cannot split. The stability trials only compared its
// two EoS roots, returned K = 1, and the flash failed in Rachford-Rice.
BOOST_AUTO_TEST_CASE(SingleComponentIsSinglePhase)
{
    struct Case
    {
        int component;
        Scalar pressure;
        Scalar temperature;
        Scalar liquidFraction;
    };
    // CO2 saturates near 51 bar at 288.15 K, and decane boils near 447 K at
    // 1 bar. Methane is supercritical at 300 K, where Li's criterion decides.
    const Case cases[] = {{0, 40.e5, 288.15, 0.0},
                          {0, 50.e5, 288.15, 0.0},
                          {0, 60.e5, 288.15, 1.0},
                          {2, 1.e5, 288.15, 1.0},
                          {2, 1.e5, 470.0, 0.0},
                          {1, 100.e5, 300.0, 0.0}};
    for (const auto method :
         {PTFlashMethod::Newton, PTFlashMethod::Ssi, PTFlashMethod::SsiNewton}) {
        for (const auto& c : cases) {
            for (const Scalar initialLiquidFraction : {-1.0, 0.5}) {
                BOOST_TEST_CONTEXT("method " << static_cast<int>(method) << ", component "
                                             << c.component << " at " << c.pressure << " Pa and "
                                             << c.temperature
                                             << " K from L = " << initialLiquidFraction)
                {
                    FluidState fs;
                    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
                        fs.setPressure(phaseIdx, c.pressure);
                    }
                    fs.setTemperature(c.temperature);
                    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                        fs.setMoleFraction(compIdx, compIdx == c.component ? 1.0 : 0.0);
                        fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
                    }
                    fs.setLvalue(initialLiquidFraction);

                    BOOST_REQUIRE(PtFlash::solve(fs, method, flashTolerance, EOSType::PR));
                    BOOST_CHECK_EQUAL(Opm::getValue(fs.L()), c.liquidFraction);
                    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
                        BOOST_CHECK_EQUAL(Opm::getValue(fs.moleFraction(phaseIdx, c.component)),
                                          1.0);
                    }
                }
            }
        }
    }
}
