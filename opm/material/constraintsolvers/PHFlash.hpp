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
 * \brief Isenthalpic (pressure-enthalpy, "P-H") flash: given the pressure and
 *        overall composition carried by a fluid state and a specified molar
 *        enthalpy, find the temperature at which the flashed mixture has that
 *        enthalpy, and leave the state flashed at the solution.
 *
 * The isothermal flash (PTFlash) is reused unmodified: at each trial
 * temperature the mixture is flashed, and its molar enthalpy (MixtureEnthalpy,
 * caloric or eos_departure model) is compared with the specified value. The
 * temperature is bracketed and found by a root finder.
 *
 * Contract notes:
 * - The specified enthalpy is molar [J/mol] and has to be computed on the
 *   datum of MixtureEnthalpy (the ideal gas at
 *   ComponentCp::referenceTemperature()) with the same model. A datum or model
 *   mismatch shows as a systematically wrong temperature, not as a failure.
 * - The temperature search stays inside the fit range of every cp polynomial
 *   in the table (ComponentCp::minTemperature, maxTemperature).
 * - Values only: after solve() the derivatives of the fluid state (if any) are
 *   those of the final isothermal flash at the converged temperature, held
 *   fixed. The sensitivity of the solution temperature (dT/d{H,p,z}) is not
 *   propagated.
 * - With the caloric model the mixture enthalpy does not depend on the phase
 *   split (H = sum_i z_i h_i(T)) and increases strictly with T, so the root
 *   is unique. With the eos_departure model H(T) has slope kinks at bubble
 *   and dew crossings, and can jump where the single-phase label flips along
 *   the search. The root finder converges across kinks; a jump is reported as
 *   no solution (see PHFlash::solve).
 */
#ifndef OPM_PH_FLASH_HPP
#define OPM_PH_FLASH_HPP

#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/numeric/RootFinders.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include <fmt/format.h>

namespace Opm {

/*!
 * \brief Run configuration of the isenthalpic flash: the enthalpy model and
 *        the temperature search.
 *
 * The search interval is [tempMin, tempMax] intersected with the fit range of
 * every cp polynomial in cpTable. It has to contain the solution; a specified
 * enthalpy outside the attainable range makes solve() return false.
 *
 * Both ends of the interval are full isothermal flashes. At extreme
 * temperatures the stability test or the split can fail to converge (and
 * throw). An end whose flash throws is moved toward the other end until it
 * flashes (bracket adaptation). No root is lost by this: where the isothermal
 * flash does not converge, H(T) cannot be evaluated.
 */
template <class Scalar, int numComponents>
struct PHFlashConfig {
    //! Per-component ideal-gas heat-capacity polynomials, indexed like the
    //! fluid system's components. Has to be populated by the caller: solve()
    //! returns false for the default-constructed, all-zero table.
    CpTable<Scalar, numComponents> cpTable{};
    //! Search interval [K], further limited to the fit range of cpTable.
    Scalar tempMin = 270.0;
    Scalar tempMax = 460.0;
    //! Convergence tolerance of the root finder on the temperature [K].
    Scalar tolerance = 1e-6;
    //! Acceptance tolerance on the enthalpy residual |H(T) - hSpec| [J/mol],
    //! checked at the converged temperature. Where the eos_departure enthalpy
    //! jumps, the root finder converges its interval onto the jump although
    //! there is no root; this check reports that case as a failure.
    Scalar enthalpyTolerance = 1.0;
    int maxIterations = 100;
    EnthalpyModel model = EnthalpyModel::caloric;
};

/*!
 * \brief The isenthalpic (P-H) flash: solves H(p, T, z) = hSpec for the
 *        temperature by a bracketed root search over the isothermal flash,
 *        with the mixture-enthalpy model selected in the configuration.
 *
 * EnthalpyCalc provides the enthalpy. A replacement needs a static
 * mixtureEnthalpy(fluidState, cpTable, eosType, model) returning the molar
 * enthalpy [J/mol] of a flashed state, as MixtureEnthalpy does.
 */
template <class Scalar, class FluidSystem,
          class EnthalpyCalc = MixtureEnthalpy<Scalar, FluidSystem>>
struct PHFlash {
    static constexpr int numComponents = FluidSystem::numComponents;

    using EOSType = CompositionalConfig::EOSType;
    using Config = PHFlashConfig<Scalar, numComponents>;

    /*!
     * \brief Solve H(p, T, z) = hSpec for T and flash the state at the
     *        solution.
     *
     * \param fluidState carries the pressure and overall mole fractions on
     *        entry; on success it holds the flashed state at the solution
     *        temperature. The equilibrium-ratio and liquid-fraction seeds of
     *        the isothermal flash are set internally at every trial.
     * \param hSpec specified molar mixture enthalpy [J/mol]
     * \param cfg the enthalpy model and the temperature search
     * \param twoPhaseMethod iteration scheme of the isothermal flash
     * \param ptTolerance convergence tolerance of the isothermal flash
     * \param eosType cubic equation of state of the isothermal flash and of
     *        the eos_departure enthalpy
     * \param verbosity verbosity of the isothermal flash; at 1 or more the
     *        adapted search interval is logged
     * \return true when the root search converged and the final residual
     *         |H(T) - hSpec| is within cfg.enthalpyTolerance. false when no
     *         solution is found: an all-zero cp table, a search interval that
     *         is empty after limiting it to the cp fit range, hSpec outside
     *         the enthalpy range of the (adapted) interval, an end that does
     *         not flash after adaptation, a failed isothermal flash at an
     *         interior trial, or a final residual above the tolerance. On
     *         false, the state is left at the last trial and is not valid.
     *
     * \note Only std::runtime_error from the isothermal flash or the enthalpy
     *       is treated as "cannot evaluate here"; std::logic_error propagates.
     */
    template <class FluidState>
    static bool solve(FluidState& fluidState,
                      const Scalar hSpec,
                      const Config& cfg,
                      const PTFlashMethod twoPhaseMethod,
                      const Scalar ptTolerance,
                      const EOSType& eosType,
                      const int verbosity = 0)
    {
        using Flash = PTFlash<Scalar, FluidSystem, true>;
        using ValueType = typename FluidState::ValueType;

        // An all-zero table makes H(T) identically zero, which would let
        // hSpec = 0 "succeed" at an arbitrary end of the interval.
        Scalar cpMagnitude = 0.0;
        for (const auto& cp : cfg.cpTable) {
            cpMagnitude += std::abs(cp.c0) + std::abs(cp.c1)
                         + std::abs(cp.c2) + std::abs(cp.c3);
        }
        if (!(cpMagnitude > 0.0))
            return false;

        // Search only where every cp polynomial is inside its fit range.
        Scalar tLo = cfg.tempMin;
        Scalar tHi = cfg.tempMax;
        for (const auto& cp : cfg.cpTable) {
            tLo = std::max(tLo, cp.minTemperature);
            tHi = std::min(tHi, cp.maxTemperature);
        }
        if (!(tLo < tHi))
            return false;

        // Warm start between nearby trials: the equilibrium ratios of a
        // converged two-phase trial seed the next isothermal flash better than
        // the Wilson estimate. Two rules keep this sound:
        //
        //   1. Only K is reused; L is always reset to -1. PTFlash runs its
        //      stability test only when L is outside (0, 1), and the test
        //      starts from the incoming K. A reused interior L would skip the
        //      stability test, and near a phase boundary a stale split can
        //      converge to the trivial root x = y = z without throwing. Every
        //      split is therefore checked for stability at its own (T, p, z).
        //
        //   2. The seed is reused only within maxWarmStep of the temperature
        //      that produced it. With rule 1 this is a performance choice,
        //      not a correctness guard. The two ends of the interval are
        //      evaluated back to back and therefore always start cold.
        //
        // Only a two-phase split with strictly positive x and y is cached
        // (single-phase ratios carry no information, and a zero mole fraction
        // would cache K = 0). The cached ratios are y/x of the converged
        // state; fluidState.K() is an input of the flash, not a result. A
        // warm-started flash that fails drops the seed and retries cold.
        bool haveWarmSeed = false;
        std::array<Scalar, numComponents> warmK{};
        Scalar warmT = 0.0;
        const Scalar maxWarmStep = std::min(Scalar(0.1) * (tHi - tLo), Scalar(15.0));

        auto seedCold = [&]() {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                fluidState.setKvalue(compIdx, fluidState.wilsonK_(compIdx));
            fluidState.setLvalue(ValueType(-1.0));
        };

        // r(T) = H(p, T, z) - hSpec. Evaluating it flashes the state at T.
        auto residual = [&](const Scalar T) -> Scalar {
            fluidState.setTemperature(ValueType(T));

            bool solved = false;
            if (haveWarmSeed && std::abs(T - warmT) <= maxWarmStep) {
                for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                    fluidState.setKvalue(compIdx, ValueType(warmK[compIdx]));
                fluidState.setLvalue(ValueType(-1.0)); // rule 1
                try {
                    Flash::solve(fluidState, twoPhaseMethod, ptTolerance, eosType, verbosity);
                    solved = true;
                }
                catch (const std::runtime_error&) {
                    haveWarmSeed = false;
                }
            }
            if (!solved) {
                seedCold();
                Flash::solve(fluidState, twoPhaseMethod, ptTolerance, eosType, verbosity);
            }

            // cache the converged split for the next trial
            const Scalar L = getValue(fluidState.L());
            if (L > 0.0 && L < 1.0) {
                haveWarmSeed = true;
                warmT = T;
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    const Scalar x = getValue(
                        fluidState.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
                    const Scalar y = getValue(
                        fluidState.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
                    if (!(x > 0.0) || !(y > 0.0)) {
                        haveWarmSeed = false;
                        break;
                    }
                    warmK[compIdx] = y / x;
                }
            }
            else {
                haveWarmSeed = false;
            }

            const Scalar h = getValue(EnthalpyCalc::mixtureEnthalpy(
                fluidState, cfg.cpTable, eosType, cfg.model));
            return h - hSpec;
        };

        // The root finder works in double. Its first two evaluations are the
        // two ends, which the bracket check below has already computed; the
        // cached values save two isothermal flashes.
        double rLoCached = 0.0;
        double rHiCached = 0.0;
        bool endsCached = false;
        auto residualAsDouble = [&](const double T) -> double {
            if (endsCached) {
                if (T == static_cast<double>(tLo))
                    return rLoCached;
                if (T == static_cast<double>(tHi))
                    return rHiCached;
            }
            return static_cast<double>(residual(static_cast<Scalar>(T)));
        };

        try {
            // Evaluate one end, moving it a quarter of the remaining span
            // toward the other end each time its isothermal flash throws.
            constexpr int maxAdaptations = 8;
            auto evaluateEnd = [&](Scalar& tEnd, const Scalar tOther, Scalar& rEnd) {
                for (int attempt = 0; attempt < maxAdaptations; ++attempt) {
                    try {
                        rEnd = residual(tEnd);
                        return true;
                    }
                    catch (const std::runtime_error&) {
                        tEnd += Scalar(0.25) * (tOther - tEnd);
                    }
                }
                return false;
            };

            const Scalar tLoInitial = tLo;
            const Scalar tHiInitial = tHi;
            Scalar rLo = 0.0;
            Scalar rHi = 0.0;
            if (!evaluateEnd(tLo, tHi, rLo) || !evaluateEnd(tHi, tLo, rHi))
                return false;
            if (!(tLo < tHi))
                return false;
            if (verbosity >= 1 && (tLo != tLoInitial || tHi != tHiInitial)) {
                OpmLog::debug(fmt::format("PHFlash: search interval adapted to [{}, {}] K, "
                                          "the isothermal flash did not converge at an end",
                                          tLo, tHi));
            }
            if (rLo * rHi > 0.0)
                return false; // hSpec is not attained on the interval
            rLoCached = static_cast<double>(rLo);
            rHiCached = static_cast<double>(rHi);
            endsCached = true;

            int iterationsUsed = 0;
            const double temperature =
                RegulaFalsi<ThrowOnError>::solve(residualAsDouble,
                                                 static_cast<double>(tLo),
                                                 static_cast<double>(tHi),
                                                 cfg.maxIterations,
                                                 static_cast<double>(cfg.tolerance),
                                                 iterationsUsed);

            // Flash the state at the solution, and accept it only if the
            // enthalpy matches there.
            const Scalar rFinal = residual(static_cast<Scalar>(temperature));
            if (!(std::abs(rFinal) <= cfg.enthalpyTolerance))
                return false;
        }
        catch (const std::runtime_error&) {
            return false;
        }
        return true;
    }
};

} // namespace Opm

#endif // OPM_PH_FLASH_HPP
