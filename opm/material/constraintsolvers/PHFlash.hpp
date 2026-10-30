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
 * The isothermal flash (PTFlash) is reused unmodified. The temperature is
 * bracketed and refined by a bracketed false-position search on
 * r(T) = H(T) - hSpec, with the mixture enthalpy H of MixtureEnthalpy
 * (caloric or eos_departure model) or of a replacement provider.
 *
 * Contract notes:
 * - The specified enthalpy is molar [J/mol] and has to be computed on the
 *   datum of MixtureEnthalpy (the ideal gas at
 *   ComponentCp::referenceTemperature()) with the same model. A datum or model
 *   mismatch shows as a systematically wrong temperature, not as a failure.
 * - The temperature search stays inside the fit range of every cp polynomial
 *   in the table (ComponentCp::minTemperature, maxTemperature).
 * - The fluid state's value type has to be a DenseAd::Evaluation, as
 *   PTFlash::solve requires; plain-double states do not compile. Values only:
 *   after solve() the derivatives of the state are those of the final
 *   isothermal flash at the solution temperature, held fixed. The sensitivity
 *   of the solution temperature (dT/d{H,p,z}) is not propagated.
 * - With the caloric model the mixture enthalpy does not depend on the phase
 *   split (H = sum_i z_i h_i(T)) and increases strictly with T, so the root
 *   is unique. With the built-in provider this H is evaluated without a flash
 *   during the search, and the state is flashed once, at the solution.
 * - With the eos_departure model H(T) has slope kinks at bubble and dew
 *   crossings, which the search converges across. Taking a single-phase
 *   residual on the stable cubic root (MixtureEnthalpy) removes the jumps
 *   that switching the single-phase label alone would cause. A pure fluid at
 *   saturation keeps a real jump: an enthalpy between the liquid and vapour
 *   values there has a physical two-phase solution, whose vapour fraction the
 *   enthalpy fixes. A search over temperature alone cannot represent it, so
 *   solve() reports failure there. This is a limitation of the method, not a
 *   case without a solution.
 * - Where the isothermal flash or the enthalpy cannot be evaluated (throws
 *   std::runtime_error), that temperature is skipped, see PHFlash::solve. A
 *   root inside a temperature range that cannot be evaluated is not found.
 */
#ifndef OPM_PH_FLASH_HPP
#define OPM_PH_FLASH_HPP

#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <type_traits>

#include <fmt/format.h>

namespace Opm {

/*!
 * \brief Run configuration of the isenthalpic flash: the enthalpy model and
 *        the temperature search.
 *
 * The search interval is [tempMin, tempMax] intersected with the fit range of
 * every cp polynomial in cpTable. A specified enthalpy outside the range that
 * the evaluable part of the interval attains makes solve() return false.
 *
 * Defaults: the caloric model is the one whose inversion is well posed for
 * every feed (H(T) is strictly increasing and needs no phase split). It
 * omits the latent heat and the pressure dependence of the enthalpy (no
 * Joule-Thomson effect); use eos_departure where those matter. 270-460 K
 * (about -3 to 187 C) covers reservoir and near-wellbore conditions and lies
 * inside the fit range of the cp polynomials of the components shipped with
 * OPM. Both are starting values; set them for the problem at hand.
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
    //! Temperature tolerance [K]: the search has converged when the bracket
    //! is at most this wide. Also the resolution to which the boundary of a
    //! temperature range that cannot be evaluated is located.
    Scalar temperatureTolerance = 1e-6;
    //! Acceptance tolerance on the enthalpy residual |H(T) - hSpec| [J/mol],
    //! checked at the converged temperature. Where the enthalpy jumps, the
    //! bracket converges onto the jump although there is no root; this check
    //! reports that case as a failure.
    Scalar enthalpyTolerance = 1.0;
    //! Limit on the root-search iterations; reaching it makes solve() return
    //! false.
    int maxIterations = 100;
    EnthalpyModel model = EnthalpyModel::caloric;
};

/*!
 * \brief The isenthalpic (P-H) flash: solves H(p, T, z) = hSpec for the
 *        temperature by a bracketed search over the isothermal flash, with
 *        the mixture-enthalpy model selected in the configuration.
 *
 * EnthalpyCalc provides the enthalpy. A replacement needs a static
 * mixtureEnthalpy(fluidState, cpTable, eosType, model) returning the molar
 * enthalpy [J/mol] of a flashed state, as MixtureEnthalpy does. A replacement
 * is always evaluated on a state flashed at the trial temperature.
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
     * The search evaluates both ends of the interval. Where an end cannot be
     * evaluated, it is kept, and the boundary between the temperatures that
     * can and cannot be evaluated is bisected from an evaluated one until a
     * sign change of r(T) = H(T) - hSpec closes a bracket, or the boundary is
     * located to cfg.temperatureTolerance. When neither end can be evaluated,
     * a few interior temperatures are tried for a starting point. Inside the
     * bracket, a trial that cannot be evaluated is replaced by up to three
     * other temperatures in the bracket; the bracket is only ever updated
     * with an evaluated temperature and its own residual.
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
     * \param verbosity verbosity of the isothermal flash; at 1 or more a
     *        bracket found next to a temperature that cannot be evaluated is
     *        logged
     * \return true when the bracket converged to cfg.temperatureTolerance
     *         within cfg.maxIterations and the final residual |H(T) - hSpec|
     *         is within cfg.enthalpyTolerance. false otherwise: an all-zero
     *         cp table, a search interval that is empty after limiting it to
     *         the cp fit range, no evaluable temperature found, hSpec outside
     *         the enthalpy range of the evaluable interval, a bracket in which
     *         no replacement trial can be evaluated, the iteration limit, a
     *         failed flash at the solution, or a final residual above the
     *         tolerance. On false, the state is left at the last trial and is
     *         not valid.
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

        // Flash the state at T; throws where the isothermal flash fails.
        auto flashAt = [&](const Scalar T) {
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
        };

        // The residual of the flashed state at its current temperature.
        auto flashedResidual = [&]() -> Scalar {
            return getValue(EnthalpyCalc::mixtureEnthalpy(
                fluidState, cfg.cpTable, eosType, cfg.model)) - hSpec;
        };

        // With the caloric model of the built-in provider, H = sum_i z_i h_i(T)
        // does not depend on the phase split, and the search needs no flash.
        constexpr bool builtInProvider =
            std::is_same_v<EnthalpyCalc, MixtureEnthalpy<Scalar, FluidSystem>>;
        const bool searchWithoutFlash = builtInProvider && cfg.model == EnthalpyModel::caloric;

        // r = H(T) - hSpec; false where it cannot be evaluated at T.
        auto evaluate = [&](const Scalar T, Scalar& r) -> bool {
            if (searchWithoutFlash) {
                Scalar h = 0.0;
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    h += getValue(fluidState.moleFraction(compIdx))
                         * cfg.cpTable[compIdx].enthalpyIntegral(T);
                }
                r = h - hSpec;
                return true;
            }
            try {
                flashAt(T);
                r = flashedResidual();
                return true;
            }
            catch (const std::runtime_error&) {
                return false;
            }
        };

        // From the evaluated temperature s toward f, which cannot be
        // evaluated: bisect the gap. An evaluated midpoint with a residual of
        // the other sign closes the bracket [lo, hi]; one with the same sign
        // replaces s; one that cannot be evaluated replaces f. Ends without a
        // bracket once the gap is within the temperature tolerance.
        constexpr int maxBoundarySteps = 64;
        auto bracketToward = [&](Scalar s, Scalar rs, Scalar f,
                                 Scalar& lo, Scalar& rLo, Scalar& hi, Scalar& rHi) -> bool {
            if (rs == 0.0) {
                lo = hi = s;
                rLo = rHi = rs;
                return true;
            }
            for (int step = 0; step < maxBoundarySteps
                                   && std::abs(f - s) > cfg.temperatureTolerance; ++step) {
                const Scalar m = Scalar(0.5) * (s + f);
                if (m == s || m == f)
                    break;
                Scalar rm = 0.0;
                if (!evaluate(m, rm)) {
                    f = m;
                    continue;
                }
                if ((rm < 0.0) != (rs < 0.0)) {
                    lo = std::min(s, m);
                    hi = std::max(s, m);
                    rLo = (lo == s) ? rs : rm;
                    rHi = (lo == s) ? rm : rs;
                    if (verbosity >= 1) {
                        OpmLog::debug(fmt::format("PHFlash: bracket [{}, {}] K found next to "
                                                  "{} K, where the enthalpy cannot be evaluated",
                                                  lo, hi, f));
                    }
                    return true;
                }
                s = m;
                rs = rm;
            }
            return false;
        };

        // Bracket [a, b] with residuals ra, rb of opposite signs (or a zero).
        Scalar a = tLo;
        Scalar b = tHi;
        Scalar ra = 0.0;
        Scalar rb = 0.0;
        const bool loEvaluated = evaluate(tLo, ra);
        const bool hiEvaluated = evaluate(tHi, rb);
        if (loEvaluated && hiEvaluated) {
            if ((ra < 0.0) == (rb < 0.0) && ra != 0.0 && rb != 0.0)
                return false; // hSpec is not attained on the interval
        }
        else if (loEvaluated) {
            if (!bracketToward(tLo, ra, tHi, a, ra, b, rb))
                return false;
        }
        else if (hiEvaluated) {
            if (!bracketToward(tHi, rb, tLo, a, ra, b, rb))
                return false;
        }
        else {
            // Neither end: look for an evaluated interior temperature, then
            // search toward both ends from it.
            constexpr std::array<Scalar, 7> probes{0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
            bool found = false;
            Scalar c = 0.0;
            Scalar rc = 0.0;
            for (const Scalar frac : probes) {
                c = tLo + frac * (tHi - tLo);
                if (evaluate(c, rc)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
            // H increases with T: the side the residual points to first.
            const Scalar first = rc > 0.0 ? tLo : tHi;
            const Scalar second = rc > 0.0 ? tHi : tLo;
            if (!bracketToward(c, rc, first, a, ra, b, rb)
                && !bracketToward(c, rc, second, a, ra, b, rb))
                return false;
        }

        // Illinois false position on [a, b]. A trial that cannot be evaluated
        // is replaced by other temperatures in the bracket; the bracket is
        // updated only with an evaluated temperature and its own residual.
        Scalar temperature = 0.0;
        bool converged = false;
        if (ra == 0.0) {
            temperature = a;
            converged = true;
        }
        else if (rb == 0.0) {
            temperature = b;
            converged = true;
        }
        int retained = 0; // -1: a was replaced last, +1: b was replaced last
        for (int iteration = 0; !converged && iteration < cfg.maxIterations; ++iteration) {
            const Scalar mid = Scalar(0.5) * (a + b);
            if (b - a <= cfg.temperatureTolerance || mid == a || mid == b) {
                temperature = mid; // or the bracket cannot shrink in Scalar
                converged = true;
                break;
            }
            Scalar t = (a * rb - b * ra) / (rb - ra);
            if (!(t > a && t < b))
                t = mid;

            Scalar rt = 0.0;
            if (!evaluate(t, rt)) {
                bool replaced = false;
                for (const Scalar frac : {Scalar(0.5), Scalar(0.25), Scalar(0.75)}) {
                    const Scalar alternative = a + frac * (b - a);
                    if (std::abs(alternative - t) <= cfg.temperatureTolerance)
                        continue;
                    if (evaluate(alternative, rt)) {
                        t = alternative;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                    return false;
            }

            if (rt == 0.0) {
                temperature = t;
                converged = true;
            }
            else if ((rt < 0.0) == (ra < 0.0)) {
                a = t;
                ra = rt;
                if (retained == -1)
                    rb *= Scalar(0.5); // b kept twice in a row
                retained = -1;
            }
            else {
                b = t;
                rb = rt;
                if (retained == +1)
                    ra *= Scalar(0.5); // a kept twice in a row
                retained = +1;
            }
        }
        if (!converged)
            return false;

        // Flash the state at the solution, and accept it only if the
        // enthalpy matches there.
        try {
            flashAt(temperature);
            if (!(std::abs(flashedResidual()) <= cfg.enthalpyTolerance))
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
