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
 * \brief The derivatives that PTFlash::solve() gives a two-phase split,
 *        against fourth-order central differences of the scalar flash.
 *
 *        The oil and gas pressures are separate inputs: a capillary pressure
 *        makes their values differ, and their derivatives need not agree.
 *        Each pressure is checked on its own and with both moving together,
 *        at equal phase pressures and with the gas pressure above the oil
 *        pressure.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashPressureDerivatives
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace {

using Scalar = double;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;
using PTFlashMethod = Opm::PTFlashMethod;
using IsothermalFlash = Opm::PTFlash<Scalar, FluidSystem>;
using ThermalFlash = Opm::PTFlash<Scalar, FluidSystem, true>;

constexpr int numComponents = FluidSystem::numComponents;
constexpr auto oilPhaseIdx = FluidSystem::oilPhaseIdx;
constexpr auto gasPhaseIdx = FluidSystem::gasPhaseIdx;
constexpr std::array<std::string_view, 2 * numComponents + 1> unknownNames{
    "x0", "x1", "x2", "y0", "y1", "y2", "L"};
using Unknowns = std::array<Scalar, unknownNames.size()>;
constexpr Scalar flashTolerance = 1e-10;

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

// A CO2, methane and decane mixture well inside its two-phase region.
constexpr auto z = std::array {Scalar(0.76524), Scalar(0.16825), Scalar(0.06651)};
constexpr Scalar baseTemperature = 394.46;
constexpr Scalar baseOilPressure = 135.42e5;

// A change of the oil pressure, the gas pressure and the temperature, and the
// step of its central difference: large against the flash tolerance, and still
// small for the truncation error of a fourth-order difference.
struct Direction
{
    std::string_view name;
    Scalar oilPressure;
    Scalar gasPressure;
    Scalar temperature;
    Scalar step;
};

// Equilibrium ratios and liquid fraction of a split.
struct Split
{
    std::array<Scalar, numComponents> K;
    Scalar L;
};

// A state that starts from a previous split, as in the next time step, so the
// flash solves the split at the phase pressures without a stability test.
template <class Value>
Opm::CompositionalFluidState<Value, FluidSystem> makeState(const Value& oilPressure,
                                                           const Value& gasPressure,
                                                           const Value& temperature,
                                                           const Split& start)
{
    Opm::CompositionalFluidState<Value, FluidSystem> fs;
    fs.setPressure(oilPhaseIdx, oilPressure);
    fs.setPressure(gasPhaseIdx, gasPressure);
    fs.setTemperature(temperature);
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, z[compIdx]);
        fs.setKvalue(compIdx, start.K[compIdx]);
    }
    fs.setLvalue(start.L);
    return fs;
}

// The split at equal phase pressures, from a cold start.
Split equalPressureSplit()
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    for (const auto phaseIdx : {oilPhaseIdx, gasPhaseIdx}) {
        fs.setPressure(phaseIdx, baseOilPressure);
    }
    fs.setTemperature(baseTemperature);
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(compIdx, z[compIdx]);
    }
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
    }
    fs.setLvalue(-1.0);
    BOOST_REQUIRE(!IsothermalFlash::flash_solve_scalar(
        fs, PTFlashMethod::SsiNewton, flashTolerance));
    Split split{};
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        split.K[compIdx]
            = fs.moleFraction(gasPhaseIdx, compIdx) / fs.moleFraction(oilPhaseIdx, compIdx);
    }
    split.L = fs.L();
    return split;
}

// x, y and L of the scalar flash, moved by a multiple of a direction.
template <class Flash>
Unknowns scalarFlash(const Scalar gasPressure,
                     const PTFlashMethod method,
                     const Split& start,
                     const Direction& direction,
                     const Scalar step)
{
    auto fs = makeState<Scalar>(baseOilPressure + step * direction.oilPressure,
                                gasPressure + step * direction.gasPressure,
                                baseTemperature + step * direction.temperature,
                                start);
    BOOST_REQUIRE(!Flash::flash_solve_scalar(fs, method, flashTolerance));
    Unknowns unknowns{};
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        unknowns[compIdx] = fs.moleFraction(oilPhaseIdx, compIdx);
        unknowns[compIdx + numComponents] = fs.moleFraction(gasPhaseIdx, compIdx);
    }
    unknowns.back() = fs.L();
    return unknowns;
}

// solve() with one derivative per direction, against a central difference
// along each direction.
template <class Flash, std::size_t numDirections>
void checkDerivatives(const Scalar gasPressure,
                      const PTFlashMethod method,
                      const std::array<Direction, numDirections>& directions)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, numDirections>;
    const Split start = equalPressureSplit();
    Evaluation oilPressure = baseOilPressure;
    Evaluation phaseGasPressure = gasPressure;
    Evaluation temperature = baseTemperature;
    for (std::size_t dirIdx = 0; dirIdx < numDirections; ++dirIdx) {
        oilPressure.setDerivative(dirIdx, directions[dirIdx].oilPressure);
        phaseGasPressure.setDerivative(dirIdx, directions[dirIdx].gasPressure);
        temperature.setDerivative(dirIdx, directions[dirIdx].temperature);
    }
    auto fs = makeState(oilPressure, phaseGasPressure, temperature, start);
    BOOST_REQUIRE(!Flash::solve(fs, method, flashTolerance));
    std::array<Evaluation, unknownNames.size()> unknowns;
    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
        unknowns[compIdx] = fs.moleFraction(oilPhaseIdx, compIdx);
        unknowns[compIdx + numComponents] = fs.moleFraction(gasPhaseIdx, compIdx);
    }
    unknowns.back() = fs.L();

    for (std::size_t dirIdx = 0; dirIdx < numDirections; ++dirIdx) {
        const auto& direction = directions[dirIdx];
        const auto flashAt = [&](const Scalar steps) {
            return scalarFlash<Flash>(gasPressure, method, start, direction,
                                      steps * direction.step);
        };
        const auto plus = flashAt(1);
        const auto minus = flashAt(-1);
        const auto plus2 = flashAt(2);
        const auto minus2 = flashAt(-2);
        Unknowns difference{};
        for (std::size_t idx = 0; idx < difference.size(); ++idx) {
            difference[idx] = (8 * (plus[idx] - minus[idx]) - (plus2[idx] - minus2[idx]))
                / (12 * direction.step);
        }
        const Scalar scale
            = std::abs(std::ranges::max(difference, {}, [](const Scalar d) { return std::abs(d); }));
        // A near-zero derivative is only resolved to the flash tolerance, so the
        // bound is relative to the largest one. The differences agree to a few
        // parts in ten million of it.
        for (std::size_t idx = 0; idx < difference.size(); ++idx) {
            BOOST_TEST_CONTEXT("d" << unknownNames[idx] << " along " << direction.name)
            {
                BOOST_CHECK_SMALL(unknowns[idx].derivative(dirIdx) - difference[idx],
                                  2e-6 * scale);
            }
        }
    }
}

// ssi+newton ends with a Newton solve, whose Jacobian the derivative pass
// reuses; ssi leaves the derivative pass to assemble it.
constexpr std::array methods{PTFlashMethod::Ssi, PTFlashMethod::SsiNewton};

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

// Each phase pressure on its own, and both together as when the phases share
// one pressure.
BOOST_AUTO_TEST_CASE(PressureDerivativesMatchCentralDifferences)
{
    constexpr std::array directions{
        Direction{"oil pressure", 1.0, 0.0, 0.0, 3e3},
        Direction{"gas pressure", 0.0, 1.0, 0.0, 3e3},
        Direction{"both pressures", 1.0, 1.0, 0.0, 3e3},
    };
    for (const Scalar capillaryPressure : {0.0, 1e5, 5e5}) {
        for (const auto method : methods) {
            BOOST_TEST_CONTEXT("method " << method << ", gas pressure " << capillaryPressure
                                         << " Pa above the oil pressure")
            {
                checkDerivatives<IsothermalFlash>(
                    baseOilPressure + capillaryPressure, method, directions);
            }
        }
    }
}

// The thermal flash has the temperature as a state variable after the pressures.
BOOST_AUTO_TEST_CASE(ThermalDerivativesMatchCentralDifferences)
{
    constexpr std::array directions{
        Direction{"oil pressure", 1.0, 0.0, 0.0, 3e3},
        Direction{"gas pressure", 0.0, 1.0, 0.0, 3e3},
        Direction{"temperature", 0.0, 0.0, 1.0, 1e-1},
    };
    for (const auto method : methods) {
        BOOST_TEST_CONTEXT("method " << method)
        {
            checkDerivatives<ThermalFlash>(baseOilPressure + 1e5, method, directions);
        }
    }
}
