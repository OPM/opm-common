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
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <dune/common/fvector.hh>

#include <cmath>
#include <stdexcept>
#include <string>

using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
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
    BOOST_REQUIRE(PtFlash::solve(cold, "ssi+newton", flashTolerance, EOSType::PR));

    auto warm = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(warm, "ssi+newton", flashTolerance, EOSType::PR));

    BOOST_CHECK_SMALL(Opm::getValue(cold.L()), flashTolerance);
    BOOST_CHECK_SMALL(Opm::getValue(warm.L()), flashTolerance);
}

// Reclassification must avoid reconstructing the singular two-phase derivatives.
BOOST_AUTO_TEST_CASE(WarmHybridReturnsSinglePhaseDerivatives)
{
    auto fs = makeDifferentiatedRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs, "ssi+newton", flashTolerance, EOSType::PR));

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
    BOOST_REQUIRE(PtFlash::solve(fs_ssi, "ssi", flashTolerance, EOSType::PR));

    auto fs_hybrid = makeRecordedState();
    BOOST_REQUIRE(PtFlash::solve(fs_hybrid, "ssi+newton", flashTolerance, EOSType::PR));

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
    BOOST_REQUIRE(PtFlash::solve(fs, "newton", flashTolerance, EOSType::PR));
    BOOST_CHECK_SMALL(Opm::getValue(fs.L()), flashTolerance);
}

// Out-of-range negative-flash and coincident SSI results both indicate that a
// warm-started cell has crossed into the single-phase region.
BOOST_AUTO_TEST_CASE(WarmSsiReassessesOutOfRangeSplits)
{
    constexpr Scalar pressures[] = {180.e5, 220.e5};
    for (const Scalar pressure : pressures) {
        auto cold = makePhaseTransitionState(pressure, -1.0);
        BOOST_REQUIRE(PtFlash::solve(cold, "ssi", flashTolerance, EOSType::PR));

        auto warm = makePhaseTransitionState(pressure, 0.5);
        BOOST_REQUIRE(PtFlash::solve(warm, "ssi", flashTolerance, EOSType::PR));

        BOOST_CHECK_SMALL(Opm::getValue(cold.L()) - 1.0, flashTolerance);
        BOOST_CHECK_SMALL(Opm::getValue(warm.L()) - 1.0, flashTolerance);
    }
}

// An out-of-range negative-flash result identifies which side of the phase
// boundary the mixture occupies; Li's approximate label must not reverse it.
BOOST_AUTO_TEST_CASE(NegativeFlashPreservesVapourDirection)
{
    for (const auto* method : {"ssi", "ssi+newton"}) {
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

    const auto reports_initial_trivial_solution = [](const std::runtime_error& error) {
        return std::string(error.what()).find("started from the trivial solution")
            != std::string::npos;
    };
    BOOST_CHECK_EXCEPTION(ExposedPtFlash::flash_2ph(
                              z, "newton", K, L, fs, flashTolerance, EOSType::PR),
                          std::runtime_error,
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
    BOOST_REQUIRE(!PtFlash::solve(fs, "ssi+newton", flashTolerance, EOSType::PR));

    BOOST_CHECK_GT(Opm::getValue(fs.L()), 0.0);
    BOOST_CHECK_LT(Opm::getValue(fs.L()), 1.0);
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        BOOST_CHECK_SMALL(Opm::getValue(fs.moleFraction(phaseIdx, 0)), flashTolerance);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            BOOST_CHECK(isMoleFractionLike(fs.moleFraction(phaseIdx, compIdx)));
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
