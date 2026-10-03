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
 * \brief The flash Newton residual used to hold the raw fugacity differences,
 *        which are of the order of the pressure.  Against the absolute
 *        tolerance of 1e-8, their round-off floor made convergence a matter of
 *        luck.  For the lean gases below, Newton took anything from three to a
 *        thousand iterations from one pressure to the next, and at some
 *        pressures it stalled on both attempts, so the whole flash failed.
 *        Scaling the fugacity rows by the pressure makes the criterion
 *        dimensionless.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashNewtonResidualScale
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <array>
#include <cmath>

namespace {

using Scalar = double;
constexpr int numComponents = 3;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, numComponents + 1>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem>;
using EOSType = Opm::CompositionalConfig::EOSType;
using Composition = std::array<Scalar, numComponents>;

// Methane, propane and eicosane from their pure-component data.
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        FluidSystem::addComponent(CompParam{"C1", 16.043, 190.56, 45.99e5, 0.0986, 0.0115});
        FluidSystem::addComponent(CompParam{"C3", 44.097, 369.83, 42.48e5, 0.2000, 0.1523});
        FluidSystem::addComponent(CompParam{"C20", 282.55, 768.0, 11.6e5, 1.17, 0.907});
    }
};

// Lean gases whose trace of eicosane drops out as a small liquid fraction.
constexpr Composition gas{0.98, 0.0195, 0.0005};
constexpr Composition leanerGas{0.99, 0.0098, 0.0002};

FluidState makeColdState(const Scalar pressure, const Scalar temperature, const Composition& z)
{
    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < 2; ++phaseIdx) {
        fs.setPressure(phaseIdx, pressure);
    }
    fs.setTemperature(temperature);
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, z[compIdx]);
    }
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
    }
    // No previous split, as at initialisation: the flash starts with stability analysis.
    fs.setLvalue(-1.0);
    return fs;
}

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(ColdFlashOfALeanGasConverges)
{
    struct Sweep { Scalar temperature; Composition z; int pMinBar; int pMaxBar; };
    // The leaner gas is two-phase up to its dew point at about 158 bar.
    for (const auto& sweep : {Sweep{323.15, gas, 100, 200}, Sweep{373.15, leanerGas, 90, 157}}) {
        for (int pBar = sweep.pMinBar; pBar <= sweep.pMaxBar; ++pBar) {
            BOOST_TEST_CONTEXT("T = " << sweep.temperature << " K, p = " << pBar << " bar") {
                auto fs = makeColdState(pBar * 1.0e5, sweep.temperature, sweep.z);
                bool singlePhase = true;
                BOOST_REQUIRE_NO_THROW(singlePhase
                                       = PtFlash::solve(fs, Opm::PTFlashMethod::SsiNewton, 1e-8,
                                                        EOSType::PR));
                BOOST_CHECK(!singlePhase);
                const Scalar L = Opm::getValue(fs.L());
                BOOST_CHECK(L > 0.0 && L < 2.0e-3);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(SplitIsInEquilibrium)
{
    // Plain substitution does not converge here within its iteration budget
    // either, so check the split against the equations it solves.
    auto fs = makeColdState(160.0e5, 323.15, gas);
    BOOST_REQUIRE_NO_THROW(PtFlash::solve(fs, Opm::PTFlashMethod::SsiNewton, 1e-8, EOSType::PR));

    typename FluidSystem::template ParameterCache<Evaluation> paramCache(EOSType::PR);
    paramCache.updatePhase(fs, FluidSystem::oilPhaseIdx);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    const Scalar L = Opm::getValue(fs.L());
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        BOOST_TEST_CONTEXT("component " << compIdx) {
            const Scalar x = Opm::getValue(fs.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
            const Scalar y = Opm::getValue(fs.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
            BOOST_CHECK_SMALL(L * x + (1.0 - L) * y - gas[compIdx], 1e-10);

            const Scalar phiL = Opm::getValue(FluidSystem::fugacityCoefficient(
                fs, paramCache, FluidSystem::oilPhaseIdx, compIdx));
            const Scalar phiV = Opm::getValue(FluidSystem::fugacityCoefficient(
                fs, paramCache, FluidSystem::gasPhaseIdx, compIdx));
            BOOST_CHECK_SMALL(x * phiL / (y * phiV) - 1.0, 1e-8);
        }
    }
}
