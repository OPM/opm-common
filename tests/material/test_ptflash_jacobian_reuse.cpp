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
 * \brief The derivatives of a two-phase flash come from the implicit function
 *        theorem, which needs the Jacobian of the composition residuals in
 *        (x, y, L).  When Newton finishes the flash it has just assembled that
 *        Jacobian at the converged state, so the derivative pass takes it
 *        from there instead of assembling it again.
 *
 *        The tests check that the reused Jacobian is exactly the one the
 *        derivative pass would assemble, so the derivatives do not change by
 *        a single bit, and that a Jacobian from any other state is not used:
 *        after the phases are swapped into the other slots, or after the state
 *        moved by one unit in the last place.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashJacobianReuse
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/common/Exceptions.hpp>

#include <array>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Scalar = double;
constexpr int numComponents = 3;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
// Pressure and the first two overall mole fractions, as in the simulator.
using Evaluation = Opm::DenseAd::Evaluation<Scalar, numComponents>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using ScalarFluidState = Opm::CompositionalFluidState<Scalar, FluidSystem>;
using PTFlashMethod = Opm::PTFlashMethod;
using Composition = std::array<Scalar, numComponents>;

constexpr auto oilPhaseIdx = FluidSystem::oilPhaseIdx;
constexpr auto gasPhaseIdx = FluidSystem::gasPhaseIdx;
constexpr Scalar flashTolerance = 1e-8;

// Exposes the two stages that solve() chains: the scalar flash and the
// derivative pass.
class ExposedPtFlash : public Opm::PTFlash<Scalar, FluidSystem>
{
    using Base = Opm::PTFlash<Scalar, FluidSystem>;

public:
    using Base::NewtonJacobian;
    using Base::compositionJacobian_;
    using Base::flashScalar_;
    using Base::jacobianState_;
    using Base::updateDerivatives_;
};
using OptionalJacobian = std::optional<ExposedPtFlash::NewtonJacobian>;

// CO2, methane and decane with the data of the simple compositional decks.
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        FluidSystem::addComponent(CompParam{"CO2", 44.0, 304.128, 73.773e5, 0.09412, 0.22394});
        FluidSystem::addComponent(CompParam{"C1", 16.04, 190.564, 45.992e5, 0.09863, 0.01142});
        FluidSystem::addComponent(CompParam{"C10", 142.28, 617.7, 21.03e5, 0.6098, 0.4884});
    }
};

// A cell state whose values carry derivatives in pressure and in the first two
// overall mole fractions, with the last one dependent.
FluidState makeState(const Scalar pressure, const Scalar temperature, const Composition& z)
{
    FluidState fs;
    const auto p = Evaluation::createVariable(pressure, 0);
    fs.setPressure(oilPhaseIdx, p);
    fs.setPressure(gasPhaseIdx, p);
    fs.setTemperature(temperature);
    Evaluation last = 1.0;
    for (unsigned compIdx = 0; compIdx + 1 < numComponents; ++compIdx) {
        const auto zc = Evaluation::createVariable(z[compIdx], compIdx + 1);
        fs.setMoleFraction(compIdx, zc);
        last -= zc;
    }
    fs.setMoleFraction(numComponents - 1, last);
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setKvalue(compIdx, fs.wilsonK_(compIdx, /*eosRegionIdx=*/0));
    }
    // No previous split: the flash starts with stability analysis.
    fs.setLvalue(-1.0);
    return fs;
}

// The value-only copy that solve() hands to the scalar flash.
ScalarFluidState scalarCopy(const FluidState& fs)
{
    ScalarFluidState scalar;
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        scalar.setKvalue(compIdx, Opm::getValue(fs.K(compIdx)));
        scalar.setMoleFraction(compIdx, Opm::getValue(fs.moleFraction(compIdx)));
    }
    scalar.setLvalue(Opm::getValue(fs.L()));
    scalar.setPressure(oilPhaseIdx, Opm::getValue(fs.pressure(oilPhaseIdx)));
    scalar.setPressure(gasPhaseIdx, Opm::getValue(fs.pressure(gasPhaseIdx)));
    scalar.setTemperature(Opm::getValue(fs.temperature(0)));
    return scalar;
}

// The derivative pass of solve() on a copy of the cell state, with or without
// the Jacobian of a Newton solve.
FluidState withDerivatives(const ScalarFluidState& scalar,
                           FluidState fs,
                           const OptionalJacobian& newtonJacobian)
{
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setMoleFraction(oilPhaseIdx, compIdx, scalar.moleFraction(oilPhaseIdx, compIdx));
        fs.setMoleFraction(gasPhaseIdx, compIdx, scalar.moleFraction(gasPhaseIdx, compIdx));
    }
    ExposedPtFlash::updateDerivatives_(scalar, fs, /*eosRegionIdx=*/0, false, newtonJacobian);
    return fs;
}

// Bitwise equality of value and derivatives.
bool identical(const Evaluation& a, const Evaluation& b)
{
    if (a.value() != b.value()) {
        return false;
    }
    for (int i = 0; i < Evaluation::numVars; ++i) {
        if (a.derivative(i) != b.derivative(i)) {
            return false;
        }
    }
    return true;
}

bool sameFlashDerivatives(const FluidState& a, const FluidState& b)
{
    bool same = identical(a.L(), b.L());
    for (const auto phaseIdx : {oilPhaseIdx, gasPhaseIdx}) {
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            same = same
                && identical(a.moleFraction(phaseIdx, compIdx), b.moleFraction(phaseIdx, compIdx));
        }
    }
    return same;
}

// The two-phase states below, from the initial gas of the decks to CO2-rich
// mixtures behind an injection front, at the decks' 150 C.
constexpr Scalar temperature = 423.15;
constexpr std::array<Composition, 4> compositions{
    Composition{0.1, 0.8, 0.1},
    Composition{0.3, 0.5, 0.2},
    Composition{0.6, 0.3, 0.1},
    Composition{0.2, 0.4, 0.4},
};
constexpr std::array<Scalar, 5> pressures{40.0e5, 55.0e5, 80.0e5, 120.0e5, 160.0e5};

struct Flash
{
    FluidState state;
    ScalarFluidState scalar;
    OptionalJacobian newtonJacobian;
    bool singlePhase = true;
};

// The scalar stage of solve(), keeping the Newton Jacobian.  Plain Newton has
// no substitution to fall back on and fails on a few of the states below, with
// or without the reuse; it skips those.  The other methods must not fail.
std::optional<Flash> flash(const FluidState& state, const PTFlashMethod method)
{
    Flash result{state, scalarCopy(state), std::nullopt, true};
    try {
        auto [singlePhase, newtonJacobian]
            = ExposedPtFlash::flashScalar_(result.scalar, method, flashTolerance, /*eosRegionIdx=*/0, 0);
        result.singlePhase = singlePhase;
        result.newtonJacobian = std::move(newtonJacobian);
    } catch (const Opm::NumericalProblem&) {
        if (method != PTFlashMethod::Newton) {
            throw;
        }
        return std::nullopt;
    }
    return result;
}

// A warm start from a converged split, as in the next Newton iteration of the
// simulator: a slightly higher pressure and a little more CO2.
FluidState warmStart(const Flash& previous)
{
    const Scalar pressure = Opm::getValue(previous.state.pressure(oilPhaseIdx)) + 2.0e5;
    Composition z{};
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        z[compIdx] = Opm::getValue(previous.state.moleFraction(compIdx));
    }
    z[0] += 0.01;
    z[1] -= 0.01;
    auto fs = makeState(pressure, temperature, z);
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        fs.setKvalue(compIdx, previous.scalar.K(compIdx));
    }
    fs.setLvalue(previous.scalar.L());
    return fs;
}

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(ReusedJacobianGivesTheSameDerivatives)
{
    using enum PTFlashMethod;
    for (const auto method : {Newton, SsiNewton, Ssi}) {
        int twoPhaseFlashes = 0;
        int reused = 0;
        for (const auto& z : compositions) {
            for (const Scalar pressure : pressures) {
                BOOST_TEST_CONTEXT(Opm::ptFlashMethodToString(method)
                                   << ", z = (" << z[0] << ", " << z[1] << ", " << z[2]
                                   << "), p = " << pressure / 1e5 << " bar") {
                    const auto cold = flash(makeState(pressure, temperature, z), method);
                    if (!cold || cold->singlePhase) {
                        continue;
                    }
                    std::vector results{*cold};
                    if (const auto warm = flash(warmStart(*cold), method)) {
                        results.push_back(*warm);
                    }
                    for (const auto& result : results) {
                        if (result.singlePhase) {
                            continue;
                        }
                        ++twoPhaseFlashes;
                        const bool reusable = result.newtonJacobian.has_value()
                            && result.newtonJacobian->state
                                == ExposedPtFlash::jacobianState_(result.scalar);
                        if (reusable) {
                            ++reused;
                            // The Newton Jacobian is the one the derivative
                            // pass would assemble, to the last bit.
                            const auto assembled
                                = ExposedPtFlash::compositionJacobian_(result.scalar, /*eosRegionIdx=*/0);
                            BOOST_CHECK(result.newtonJacobian->jacobian == assembled);
                        }
                        const auto reusing
                            = withDerivatives(result.scalar, result.state, result.newtonJacobian);
                        const auto assembling
                            = withDerivatives(result.scalar, result.state, std::nullopt);
                        BOOST_CHECK(sameFlashDerivatives(reusing, assembling));

                        // solve() takes the same path end to end.
                        auto solved = result.state;
                        ExposedPtFlash::solve(solved, method, flashTolerance, /*eosRegionIdx=*/0);
                        BOOST_CHECK(sameFlashDerivatives(solved, assembling));
                    }
                }
            }
        }
        BOOST_TEST_MESSAGE(Opm::ptFlashMethodToString(method)
                           << ": " << twoPhaseFlashes << " two-phase flashes, " << reused
                           << " with a reused Jacobian");
        BOOST_CHECK(twoPhaseFlashes > 0);
        if (method == PTFlashMethod::Ssi) {
            // Substitution assembles no Jacobian to reuse.
            BOOST_CHECK_EQUAL(reused, 0);
        } else {
            BOOST_CHECK(reused > 0);
        }
    }
}

BOOST_AUTO_TEST_CASE(JacobianOfAnotherStateIsNotReused)
{
    const auto result = flash(makeState(80.0e5, temperature, compositions[1]),
                              PTFlashMethod::Newton);
    BOOST_REQUIRE(result.has_value());
    const auto& converged = *result;
    BOOST_REQUIRE(!converged.singlePhase);
    BOOST_REQUIRE(converged.newtonJacobian.has_value());

    // States the flash may still move to after Newton: the phases in the other
    // slots, and the same split one unit in the last place higher in pressure.
    auto swapped = converged.scalar;
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        swapped.setMoleFraction(oilPhaseIdx, compIdx,
                                converged.scalar.moleFraction(gasPhaseIdx, compIdx));
        swapped.setMoleFraction(gasPhaseIdx, compIdx,
                                converged.scalar.moleFraction(oilPhaseIdx, compIdx));
    }
    swapped.setLvalue(1.0 - converged.scalar.L());
    auto shifted = converged.scalar;
    for (const auto phaseIdx : {oilPhaseIdx, gasPhaseIdx}) {
        const Scalar pressure = shifted.pressure(phaseIdx);
        shifted.setPressure(phaseIdx, std::nextafter(pressure, 2 * pressure));
    }

    using namespace std::string_view_literals;
    for (const auto& [name, moved] : {std::pair{"swapped phases"sv, swapped},
                                      std::pair{"shifted pressure"sv, shifted}}) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_CHECK(converged.newtonJacobian->state != ExposedPtFlash::jacobianState_(moved));
            const auto offered = withDerivatives(moved, converged.state, converged.newtonJacobian);
            const auto assembled = withDerivatives(moved, converged.state, std::nullopt);
            BOOST_CHECK(sameFlashDerivatives(offered, assembled));
        }
    }

    // The check above has teeth: the swapped state has another Jacobian, and
    // using the stale one there would change the derivatives.
    const auto swappedJacobian = ExposedPtFlash::compositionJacobian_(swapped, /*eosRegionIdx=*/0);
    BOOST_CHECK(swappedJacobian != converged.newtonJacobian->jacobian);
    const OptionalJacobian forged = ExposedPtFlash::NewtonJacobian{
        converged.newtonJacobian->jacobian, ExposedPtFlash::jacobianState_(swapped)};
    BOOST_CHECK(!sameFlashDerivatives(withDerivatives(swapped, converged.state, forged),
                                      withDerivatives(swapped, converged.state, std::nullopt)));
}
