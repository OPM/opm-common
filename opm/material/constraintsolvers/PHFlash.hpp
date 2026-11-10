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
 * The isothermal flash (PTFlash) is reused unmodified. The search solves
 * r(T) = H(T) - hSpec, with the mixture enthalpy H of MixtureEnthalpy
 * (caloric or eos_departure model) or of a replacement provider, in up to
 * three stages. Each falls back to the next stage if it does not converge or
 * if a flash at its solution misses the enthalpy tolerance:
 *
 *   1. With the built-in eos_departure model, a phase split carried by the
 *      incoming state and the temperature are solved for together by
 *      Newton's method, without an isothermal flash.
 *   2. A safeguarded Newton iteration on T from the incoming temperature.
 *   3. A bracket search, refined by false position.
 *
 * See PHFlash::solve() for the stages and their derivatives.
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
 * - Where the isothermal flash or the enthalpy cannot be evaluated (see the
 *   note on solve()), that temperature is skipped and the search goes on past
 *   it, both while it looks for a bracket and on a bracket. A root inside a
 *   temperature range that cannot be evaluated is not found, nor is one that
 *   the search does not reach within its limits on evaluations and iterations.
 */
#ifndef OPM_PH_FLASH_HPP
#define OPM_PH_FLASH_HPP

#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

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
    //! Temperature tolerance [K]: the bracket search has converged when the
    //! bracket is at most this wide, and a Newton iteration when its step in
    //! T is. Also the resolution to which the boundary of a temperature range
    //! that cannot be evaluated is located, and the distance within which a
    //! root at that boundary is accepted.
    Scalar temperatureTolerance = 1e-6;
    //! Acceptance tolerance on the enthalpy residual |H(T) - hSpec| [J/mol],
    //! checked at the converged temperature. Where the enthalpy jumps, the
    //! bracket converges onto the jump although there is no root; this check
    //! reports that case as a failure.
    Scalar enthalpyTolerance = 1.0;
    //! Iteration limit of each Newton iteration on T and of the bracket
    //! search. A Newton stage that reaches it hands over to the next stage,
    //! and the bracket search fails. The combined Newton iteration of stage 1
    //! has a limit of its own.
    int maxIterations = 100;
    EnthalpyModel model = EnthalpyModel::caloric;
    //! Run the Newton stages of PHFlash::solve() from the incoming
    //! temperature and phase split before the bracket search. Off by default:
    //! the bracket search alone reads only the pressure and the overall
    //! composition of the incoming state.
    bool newtonStart = false;
};

/*!
 * \brief The isenthalpic (P-H) flash: solves H(p, T, z) = hSpec for the
 *        temperature by a bracketed search over the isothermal flash,
 *        optionally preceded by Newton's method from the incoming state,
 *        with the mixture-enthalpy model selected in the configuration.
 *
 * EnthalpyCalc provides the enthalpy. A replacement needs a static
 * mixtureEnthalpy(fluidState, cpTable, eosType, model) returning the molar
 * enthalpy [J/mol] of a flashed state, as MixtureEnthalpy does. A replacement
 * is evaluated on the caller's state flashed at a trial temperature, and gets
 * the bracket search alone, whatever cfg.newtonStart, unless it declares
 * static constexpr bool newtonStage = true. It then also takes part in the
 * Newton stage on T, which evaluates it on a fluid-state type of its own and,
 * for the slope, on copies of such a state moved slightly along the tangent
 * of the phase split (see slopeAlongFlash()), where L can lie slightly
 * outside [0, 1] and the mole fractions of a phase need not sum to one.
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
     * The stages run in this order. A stage's solution is accepted once the
     * state flashed there meets cfg.enthalpyTolerance. Each stage falls back
     * to the next if it does not converge or if its solution is not accepted.
     *
     *   1. With cfg.newtonStart and the built-in eos_departure model, Newton's
     *      method from the incoming phase split (fromIncomingSplit()): on the
     *      split and T together for two phases, on T with the enthalpy of the
     *      phase for one, without an isothermal flash. The two-phase Jacobian
     *      is by AD, with the temperature derivative of the residual enthalpy
     *      as a central difference (MixtureEnthalpy::departurePhaseEnthalpy);
     *      the single-phase slope is taken as in stage 2. Where the flash at
     *      the solution does not confirm it, the stage runs once more from
     *      the state that flash left.
     *   2. With cfg.newtonStart and the built-in provider, or a replacement
     *      that declares newtonStage, a safeguarded Newton iteration on T
     *      from the incoming temperature (newton()), one isothermal flash per
     *      iterate (none with the built-in caloric model, see below). dH/dT
     *      is a central difference of H along the tangent of the phase split,
     *      which the flash carries by AD (slopeAlongFlash()).
     *   3. False position (illinois()) on the bracket that stage 2 found, or
     *      else on one searched for over the interval (findBracket()).
     *
     * With the caloric model of the built-in provider, H does not depend on
     * the split: stages 2 and 3 then evaluate it without a flash, and dH/dT
     * is the heat capacity.
     *
     * \param[in,out] fluidState in: the pressure, the overall mole
     *        fractions, and, with cfg.newtonStart, the temperature where
     *        stages 1 and 2 start (moved to the nearer end of the search
     *        interval if outside it, and replaced by its midpoint if not
     *        finite). For stage 1, the liquid fraction and phase compositions
     *        as PTFlash leaves them, only as a starting point. A two-phase
     *        state with a zero mole fraction in either phase skips the stage,
     *        as does a single-phase state whose two phases do not hold one
     *        composition summing to one.
     *        Out, on success: the solution temperature, and the
     *        liquid fraction and phase compositions of the state flashed
     *        there. On failure the state is left at the last trial and is not
     *        valid.
     * \param hSpec specified molar mixture enthalpy [J/mol]
     * \param cfg the enthalpy model and the temperature search
     * \param twoPhaseMethod iteration scheme of the isothermal flash
     * \param ptTolerance convergence tolerance of the isothermal flash
     * \param eosType cubic equation of state of the isothermal flash and of
     *        the eos_departure enthalpy
     * \param verbosity verbosity of the isothermal flash; at 1 or more a
     *        solution from the incoming phase split, a Newton stage that
     *        hands over to the next stage, and a bracket found next to a
     *        temperature that cannot be evaluated, are logged
     * \return true when a stage converged and the residual |H(T) - hSpec| of
     *         the state flashed at its solution is within
     *         cfg.enthalpyTolerance.
     *         false for an all-zero cp table, a search interval that is empty
     *         after limiting it to the cp fit range, a failed search, a failed
     *         flash at the solution, or a final residual above the tolerance.
     *
     * \note A std::runtime_error or a Dune::FMatrixError from the isothermal
     *       flash or the enthalpy, and a residual that is not finite, are
     *       treated as "cannot evaluate here"; std::logic_error propagates.
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
        // An all-zero table makes H(T) identically zero, which would let
        // hSpec = 0 "succeed" at an arbitrary end of the interval.
        Scalar cpMagnitude = 0.0;
        for (const auto& cp : cfg.cpTable) {
            cpMagnitude += std::abs(cp.c0) + std::abs(cp.c1)
                         + std::abs(cp.c2) + std::abs(cp.c3);
        }
        if (!(cpMagnitude > 0.0)) {
            return false;
        }

        // Search only where every cp polynomial is inside its fit range.
        Scalar tLo = cfg.tempMin;
        Scalar tHi = cfg.tempMax;
        for (const auto& cp : cfg.cpTable) {
            tLo = std::max(tLo, cp.minTemperature);
            tHi = std::min(tHi, cp.maxTemperature);
        }
        if (!(tLo < tHi)) {
            return false;
        }

        // Newton's method starts from the incoming temperature; the bracket
        // search alone does not read it.
        const Scalar incoming = cfg.newtonStart ? getValue(fluidState.temperature(0))
                                                : std::numeric_limits<Scalar>::quiet_NaN();
        const Scalar start = std::isfinite(incoming) ? std::clamp(incoming, tLo, tHi)
                                                     : Scalar(0.5) * (tLo + tHi);

        WarmStart warmStart(std::min(Scalar(0.1) * (tHi - tLo), Scalar(15.0)));

        auto residualAt = [&](const Scalar T) {
            using ValueType = typename FluidState::ValueType;
            return flashedResidual(fluidState, ValueType(T), hSpec, cfg, warmStart,
                                   twoPhaseMethod, ptTolerance, eosType, verbosity);
        };

        // With the caloric model of the built-in provider, H = sum_i z_i h_i(T)
        // does not depend on the phase split, and the search needs no flash.
        // Its slope is sum_i z_i cp_i(T).
        const bool searchWithoutFlash = builtInProvider && cfg.model == EnthalpyModel::caloric;

        auto caloricTrial = [&](const Scalar T) -> std::optional<SlopedTrial> {
            Scalar h = 0.0;
            Scalar cp = 0.0;
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                const Scalar z = getValue(fluidState.moleFraction(compIdx));
                h += z * cfg.cpTable[compIdx].enthalpyIntegral(T);
                cp += z * cfg.cpTable[compIdx].heatCapacity(T);
            }
            const Scalar r = h - hSpec;
            if (!std::isfinite(r)) {
                return std::nullopt;
            }
            return SlopedTrial{Trial{T, r}, std::isfinite(cp) ? std::optional<Scalar>(cp)
                                                              : std::nullopt};
        };

        // r = H(T) - hSpec; empty where it cannot be evaluated at T or is not
        // finite.
        auto evaluate = [&](const Scalar T) -> std::optional<Scalar> {
            if (!searchWithoutFlash) {
                return residualAt(T);
            }
            if (const auto trial = caloricTrial(T)) {
                return trial->trial.residual;
            }
            return std::nullopt;
        };

        // Flash the state at the solution, and accept it only if the
        // enthalpy matches there.
        auto withinTolerance = [&](const std::optional<Scalar>& residual) {
            return residual && std::abs(*residual) <= cfg.enthalpyTolerance;
        };
        auto acceptAt = [&](const Scalar T) { return withinTolerance(residualAt(T)); };

        // Stage 1: a phase split carried by the incoming state is solved for
        // together with the temperature, without an isothermal flash; one
        // flash at the solution verifies it. Where that flash does not
        // confirm it, the stage runs once more, from the state that flash left.
        if constexpr (builtInProvider) {
            if (cfg.newtonStart && cfg.model == EnthalpyModel::eos_departure) {
                Scalar from = start;
                for (int attempt = 0; attempt < 2; ++attempt) {
                    const auto temperature = fromIncomingSplit(fluidState, from, hSpec, tLo, tHi,
                                                               cfg, ptTolerance, eosType,
                                                               verbosity);
                    if (!temperature) {
                        break;
                    }
                    const auto residual = residualAt(*temperature);
                    if (withinTolerance(residual)) {
                        if (verbosity >= 1) {
                            OpmLog::debug(fmt::format("PHFlash: solved from the incoming phase "
                                                      "split at {} K",
                                                      *temperature));
                        }
                        return true;
                    }
                    if (verbosity >= 1) {
                        OpmLog::debug(fmt::format("PHFlash: the flash at {} K does not confirm "
                                                  "the solution from the incoming phase split",
                                                  *temperature));
                    }
                    if (!residual) {
                        break;
                    }
                    from = *temperature;
                }
            }
        }

        // Stage 2: Newton's method on T; its bracket, if any, seeds stage 3.
        std::optional<Bracket> bracket;
        if constexpr (newtonStageProvider) {
            if (cfg.newtonStart) {
                // The stage flashes a copy of the state whose temperature is
                // its one AD variable: the isothermal flash then carries
                // dL/dT, dx/dT and dy/dT, which give the slope of r without a
                // further flash.
                SearchState searchState;
                searchState.setPressure(FluidSystem::oilPhaseIdx,
                                        getValue(fluidState.pressure(FluidSystem::oilPhaseIdx)));
                searchState.setPressure(FluidSystem::gasPhaseIdx,
                                        getValue(fluidState.pressure(FluidSystem::gasPhaseIdx)));
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    searchState.setMoleFraction(compIdx,
                                                getValue(fluidState.moleFraction(compIdx)));
                }

                // r and dr/dT; empty where r cannot be evaluated at T.
                auto evaluateWithSlope = [&](const Scalar T) -> std::optional<SlopedTrial> {
                    if (searchWithoutFlash) {
                        return caloricTrial(T);
                    }
                    const auto r = flashedResidual(searchState, SearchEval::createVariable(T, 0),
                                                   hSpec, cfg, warmStart, twoPhaseMethod,
                                                   ptTolerance, eosType, verbosity);
                    if (!r) {
                        return std::nullopt;
                    }
                    return SlopedTrial{Trial{T, *r},
                                       slopeAlongFlash(searchState, tLo, tHi, cfg, eosType)};
                };

                const auto outcome = newton(evaluateWithSlope, start, tLo, tHi, cfg);
                if (outcome.temperature && acceptAt(*outcome.temperature)) {
                    return true;
                }
                bracket = outcome.bracket;
                if (verbosity >= 1) {
                    const auto stop = outcome.temperature
                                          ? "is not confirmed by the flash at its solution"
                                          : "did not converge";
                    OpmLog::debug(fmt::format("PHFlash: the Newton stage from {} K {}; the "
                                              "bracket search goes on {}",
                                              start, stop,
                                              bracket ? "from its bracket" : "over the interval"));
                }
            }
        }

        // Stage 3: false position on a bracket.
        if (!bracket) {
            bracket = findBracket(evaluate, tLo, tHi, cfg, verbosity);
            if (!bracket) {
                return false;
            }
        }
        const auto temperature = illinois(evaluate, *bracket, cfg, verbosity);
        if (!temperature) {
            return false;
        }
        return acceptAt(*temperature);
    }

private:
    //! A temperature [K] and its residual r = H(T) - hSpec [J/mol].
    struct Trial
    {
        Scalar temperature;
        Scalar residual;
    };

    //! Two evaluated temperatures lo.temperature <= hi.temperature whose
    //! residuals have opposite signs, or that hold a zero residual.
    struct Bracket
    {
        Trial lo;
        Trial hi;
    };

    //! Limit on the evaluations of one bracketToward() call.
    static constexpr int maxBoundarySteps = 64;

    //! A trial with the slope dr/dT [J/(mol K)], where that can be evaluated.
    struct SlopedTrial
    {
        Trial trial;
        std::optional<Scalar> slope;
    };

    //! Where the Newton stage stopped: the converged temperature, if it
    //! converged, and the bracket it found, if any.
    struct NewtonOutcome
    {
        std::optional<Scalar> temperature;
        std::optional<Bracket> bracket;
    };

    //! A converged split of the combined Newton stage: its temperature and
    //! liquid fraction, which lies outside (0, 1) where the mixture is a
    //! single phase at that temperature (a negative flash).
    struct SplitSolution
    {
        Scalar temperature;
        Scalar liquidFraction;
    };

    //! Limit on one Newton step [K]. A poor slope, at a kink of H(T) or far
    //! from the root, cannot throw a trial across the interval.
    static constexpr Scalar maxNewtonStep = 30.0;

    //! Distance [K] from T of the two states whose enthalpies give dr/dT:
    //! small against the curvature of H(T), large against its round-off. An
    //! error in the slope changes the Newton step, not the solution, which
    //! the residual decides.
    static constexpr Scalar slopeStep = 1e-3;

    //! The state of the Newton stage: temperature is the one AD variable.
    using SearchEval = DenseAd::Evaluation<Scalar, 1>;
    using SearchState = CompositionalFluidState<SearchEval, FluidSystem>;

    static constexpr bool builtInProvider =
        std::is_same_v<EnthalpyCalc, MixtureEnthalpy<Scalar, FluidSystem>>;

    //! Whether the Newton stage on T, which evaluates the provider on
    //! SearchState, runs with this provider. A replacement opts in, since a
    //! check of the call alone does not see into the body of a template.
    static constexpr bool newtonStageProvider =
        builtInProvider || requires { requires EnthalpyCalc::newtonStage; };

    //! The state of the combined Newton stage, with AD variables x (slots
    //! 0 to n-1), y (n to 2n-1), L (2n) and T (2n+1).
    static constexpr int numSplitUnknowns = 2 * numComponents + 2;
    using SplitEval = DenseAd::Evaluation<Scalar, numSplitUnknowns>;
    using SplitState = CompositionalFluidState<SplitEval, FluidSystem>;

    //! Limit on the iterations of the combined Newton stage. From a nearby
    //! split it converges in a few; one that needs more hands over.
    static constexpr int maxSplitIterations = 20;

    //! Below this sum_i ln(y_i/x_i)^2 the two phases count as one, as in
    //! PTFlash.
    static constexpr Scalar coincidentPhases = 1e-5;

    /*!
     * \brief Warm start between nearby trials: the equilibrium ratios of a
     *        converged two-phase trial seed the next isothermal flash better
     *        than the Wilson estimate.
     *
     * Two rules keep this sound:
     *
     *   1. Only K is reused; L is always reset to -1. PTFlash runs its
     *      stability test only when L is outside (0, 1), and the test starts
     *      from the incoming K. A reused interior L would skip the stability
     *      test, and near a phase boundary a stale split can converge to the
     *      trivial root x = y = z without throwing. Every split is therefore
     *      checked for stability at its own (T, p, z).
     *
     *   2. The seed is reused only within maxStep of the temperature that
     *      produced it. With rule 1 this is a performance choice, not a
     *      correctness guard. The two ends of the interval are evaluated back
     *      to back, too far apart to seed each other.
     *
     * Only a two-phase split with strictly positive x and y is cached
     * (single-phase ratios carry no information, and a zero mole fraction
     * would cache K = 0). The cached ratios are y/x of the converged state;
     * fluidState.K() is an input of the flash, not a result.
     */
    struct WarmStart
    {
        explicit WarmStart(const Scalar window)
            : maxStep(window)
        {}

        //! Seed a flash at T from the cached ratios; false if they do not apply.
        template <class FluidState>
        bool seed(FluidState& fluidState, const Scalar T) const
        {
            using ValueType = typename FluidState::ValueType;

            if (!(valid && std::abs(T - temperature) <= maxStep)) {
                return false;
            }
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                fluidState.setKvalue(compIdx, ValueType(K[compIdx]));
            }
            fluidState.setLvalue(ValueType(-1.0)); // rule 1
            return true;
        }

        //! Cache the converged split of a state flashed at T, if it qualifies.
        template <class FluidState>
        void store(const FluidState& fluidState, const Scalar T)
        {
            const Scalar L = getValue(fluidState.L());
            valid = L > 0.0 && L < 1.0;
            if (!valid) {
                return;
            }
            temperature = T;
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                const Scalar x = getValue(
                    fluidState.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
                const Scalar y = getValue(
                    fluidState.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
                if (!(x > 0.0) || !(y > 0.0)) {
                    valid = false;
                    return;
                }
                K[compIdx] = y / x;
            }
        }

        void drop()
        {
            valid = false;
        }

    private:
        Scalar maxStep;
        bool valid = false;
        std::array<Scalar, numComponents> K{};
        Scalar temperature = 0.0;
    };

    /*!
     * \brief Flash the state at T and return r = H(T) - hSpec of the flashed
     *        state.
     *
     * A warm-started flash that fails drops the seed and retries cold. The
     * derivatives that temperature carries are propagated by the flash.
     *
     * \return r(T), or nothing where the isothermal flash or the enthalpy
     *         throws std::runtime_error or Dune::FMatrixError, or where r is
     *         not finite
     */
    template <class FluidState>
    static std::optional<Scalar> flashedResidual(FluidState& fluidState,
                                                 const typename FluidState::ValueType& temperature,
                                                 const Scalar hSpec,
                                                 const Config& cfg,
                                                 WarmStart& warmStart,
                                                 const PTFlashMethod twoPhaseMethod,
                                                 const Scalar ptTolerance,
                                                 const EOSType& eosType,
                                                 const int verbosity)
    {
        using Flash = PTFlash<Scalar, FluidSystem, true>;
        using ValueType = typename FluidState::ValueType;

        const Scalar T = getValue(temperature);
        fluidState.setTemperature(temperature);

        bool solved = false;
        if (warmStart.seed(fluidState, T)) {
            try {
                Flash::solve(fluidState, twoPhaseMethod, ptTolerance, eosType, verbosity);
                solved = true;
            }
            catch (const std::runtime_error&) {
                warmStart.drop();
            }
            catch (const Dune::FMatrixError&) {
                warmStart.drop();
            }
        }

        try {
            if (!solved) {
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    fluidState.setKvalue(compIdx, fluidState.wilsonK_(compIdx));
                }
                fluidState.setLvalue(ValueType(-1.0));
                Flash::solve(fluidState, twoPhaseMethod, ptTolerance, eosType, verbosity);
            }
            warmStart.store(fluidState, T);

            const Scalar r = getValue(EnthalpyCalc::mixtureEnthalpy(
                fluidState, cfg.cpTable, eosType, cfg.model)) - hSpec;
            if (std::isfinite(r)) {
                return r;
            }
        }
        catch (const std::runtime_error&) {
        }
        catch (const Dune::FMatrixError&) {
        }
        return std::nullopt;
    }

    /*!
     * \brief dr/dT of a search state flashed at T.
     *
     * The isothermal flash leaves dL/dT, dx/dT and dy/dT in the derivatives of
     * the state. The enthalpy is evaluated on two copies of the state moved
     * along that tangent to slopeStep on either side of T, within [tLo, tHi],
     * and dr/dT is their difference quotient. This needs no further flash,
     * and holds for single-phase states too, whose split does not move. The
     * enthalpy provider returns values only, which rules out AD for dr/dT.
     *
     * \return dr/dT, or nothing where the enthalpy cannot be evaluated on a
     *         copy or the quotient is not finite
     */
    static std::optional<Scalar> slopeAlongFlash(const SearchState& state,
                                                 const Scalar tLo,
                                                 const Scalar tHi,
                                                 const Config& cfg,
                                                 const EOSType& eosType)
    {
        const Scalar T = state.temperature(0).value();
        const Scalar below = std::max(T - slopeStep, tLo);
        const Scalar above = std::min(T + slopeStep, tHi);
        if (!(above > below)) {
            return std::nullopt;
        }

        auto enthalpyAt = [&](const Scalar t) {
            const Scalar s = t - T;
            SearchState moved = state;
            moved.setTemperature(SearchEval(t));
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                for (const auto phaseIdx : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
                    const auto& x = state.moleFraction(phaseIdx, compIdx);
                    moved.setMoleFraction(phaseIdx, compIdx,
                                          SearchEval(x.value() + s * x.derivative(0)));
                }
            }
            moved.setLvalue(SearchEval(state.L().value() + s * state.L().derivative(0)));
            return getValue(EnthalpyCalc::mixtureEnthalpy(moved, cfg.cpTable, eosType, cfg.model));
        };

        try {
            const Scalar slope = (enthalpyAt(above) - enthalpyAt(below)) / (above - below);
            if (!std::isfinite(slope)) {
                return std::nullopt;
            }
            return slope;
        }
        catch (const std::runtime_error&) {
            return std::nullopt;
        }
        catch (const Dune::FMatrixError&) {
            return std::nullopt;
        }
    }

    /*!
     * \brief Safeguarded Newton iteration on r(T) from t in [tLo, tHi].
     *
     * H increases with T, so a trial with r > 0 bounds the root from above
     * and one with r < 0 from below. The Newton step is taken where the slope
     * is positive and the step stays inside these bounds. Otherwise the
     * iteration bisects between the bounds once both are evaluated, and before
     * that moves toward the root by maxNewtonStep. No step is longer than
     * maxNewtonStep. The iteration has converged when the Newton step is
     * within cfg.temperatureTolerance, and then returns the temperature after
     * that step, or when the bounds are.
     *
     * It stops without a temperature where a trial cannot be evaluated, where
     * it reaches an end of the interval without a sign change, or after
     * cfg.maxIterations.
     *
     * \param evaluate returns r(T) and dr/dT, or nothing where r cannot be
     *        evaluated
     * \return the converged temperature, if any, and the bracket of the
     *         evaluated bounds, if both are
     */
    template <class EvaluateWithSlope>
    static NewtonOutcome newton(const EvaluateWithSlope& evaluate,
                                Scalar t,
                                const Scalar tLo,
                                const Scalar tHi,
                                const Config& cfg)
    {
        std::optional<Trial> below; // evaluated, with r < 0
        std::optional<Trial> above; // evaluated, with r > 0
        Scalar lower = tLo;
        Scalar upper = tHi;
        auto bracket = [&]() -> std::optional<Bracket> {
            if (below && above) {
                return Bracket{*below, *above};
            }
            return std::nullopt;
        };

        for (int iteration = 0; iteration < cfg.maxIterations; ++iteration) {
            const auto sloped = evaluate(t);
            if (!sloped) {
                return {std::nullopt, bracket()};
            }
            const Trial& trial = sloped->trial;
            if (trial.residual == 0.0) {
                return {t, bracket()};
            }
            if (trial.residual > 0.0) {
                above = trial;
                upper = t;
            }
            else {
                below = trial;
                lower = t;
            }

            const bool newtonStep = sloped->slope && *sloped->slope > 0.0;
            const Scalar step = newtonStep ? -trial.residual / *sloped->slope : 0.0;
            // The step costs no evaluation and makes the residual at the
            // solution quadratically small rather than of order slope * step.
            if (newtonStep && std::abs(step) <= cfg.temperatureTolerance) {
                return {std::clamp(t + step, lower, upper), bracket()};
            }
            if (below && above && upper - lower <= cfg.temperatureTolerance) {
                return {t, bracket()};
            }

            Scalar next = t + step;
            if (!(next > lower && next < upper)) {
                if (below && above) {
                    next = Scalar(0.5) * (lower + upper);
                }
                else {
                    next = trial.residual > 0.0 ? t - maxNewtonStep : t + maxNewtonStep;
                }
            }
            next = std::clamp(next, t - maxNewtonStep, t + maxNewtonStep);
            next = std::clamp(next, lower, upper);
            if (next == t) {
                return {std::nullopt, bracket()};
            }
            t = next;
        }
        return {std::nullopt, bracket()};
    }

    /*!
     * \brief Solution temperature from the phase split of the incoming state,
     *        without an isothermal flash.
     *
     * A two-phase split (0 < L < 1, positive x and y, distinct phases) starts
     * splitNewton() from the temperature start. Where that converges to a
     * liquid fraction outside (0, 1), singlePhaseNewton() goes on from its
     * temperature. A single-phase state as PTFlash leaves it (L = 0 or 1,
     * x = y summing to one) starts singlePhaseNewton().
     *
     * \return the temperature, or nothing for another incoming state or where
     *         the iteration does not converge
     */
    template <class FluidState>
    static std::optional<Scalar> fromIncomingSplit(const FluidState& fluidState,
                                                   const Scalar start,
                                                   const Scalar hSpec,
                                                   const Scalar tLo,
                                                   const Scalar tHi,
                                                   const Config& cfg,
                                                   const Scalar ptTolerance,
                                                   const EOSType& eosType,
                                                   const int verbosity)
    {
        const Scalar L = getValue(fluidState.L());
        std::array<Scalar, numComponents> x{};
        std::array<Scalar, numComponents> y{};
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            x[compIdx] = getValue(fluidState.moleFraction(FluidSystem::oilPhaseIdx, compIdx));
            y[compIdx] = getValue(fluidState.moleFraction(FluidSystem::gasPhaseIdx, compIdx));
        }

        std::optional<Scalar> temperature;
        if (L == 0.0 || L == 1.0) {
            // PTFlash leaves both phases of a single-phase state at the
            // overall composition; a state that was not flashed is not used.
            const Scalar sumX = std::accumulate(x.begin(), x.end(), Scalar(0.0));
            if (x != y || !(std::abs(sumX - 1.0) <= 1e-6)) {
                return std::nullopt;
            }
            temperature = singlePhaseNewton(fluidState, start, hSpec, tLo, tHi, cfg, eosType);
        }
        else {
            const auto inUnitInterval = [](const Scalar w) { return w > 0.0 && w <= 1.0; };
            if (!(L > 0.0 && L < 1.0) || !std::ranges::all_of(x, inUnitInterval)
                || !std::ranges::all_of(y, inUnitInterval) || phasesCoincide(x, y))
            {
                return std::nullopt;
            }
            const auto split = splitNewton(fluidState, x, y, L, start, hSpec, tLo, tHi, cfg,
                                           ptTolerance, eosType);
            if (split && split->liquidFraction > 0.0 && split->liquidFraction < 1.0) {
                temperature = split->temperature;
            }
            else if (split) {
                temperature = singlePhaseNewton(fluidState, split->temperature, hSpec, tLo, tHi,
                                                cfg, eosType);
            }
        }
        if (!temperature && verbosity >= 1) {
            OpmLog::debug(fmt::format("PHFlash: the Newton stage on the incoming phase split "
                                      "from {} K did not converge",
                                      start));
        }
        return temperature;
    }

    /*!
     * \brief newton() on the enthalpy of the overall composition z as one
     *        phase; its slope is dH/dT at fixed composition.
     */
    template <class FluidState>
    static std::optional<Scalar> singlePhaseNewton(const FluidState& fluidState,
                                                   const Scalar start,
                                                   const Scalar hSpec,
                                                   const Scalar tLo,
                                                   const Scalar tHi,
                                                   const Config& cfg,
                                                   const EOSType& eosType)
    {
        SearchState state;
        state.setPressure(FluidSystem::oilPhaseIdx,
                          getValue(fluidState.pressure(FluidSystem::oilPhaseIdx)));
        state.setPressure(FluidSystem::gasPhaseIdx,
                          getValue(fluidState.pressure(FluidSystem::gasPhaseIdx)));
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            const Scalar z = getValue(fluidState.moleFraction(compIdx));
            state.setMoleFraction(compIdx, z);
            state.setMoleFraction(FluidSystem::oilPhaseIdx, compIdx, z);
            state.setMoleFraction(FluidSystem::gasPhaseIdx, compIdx, z);
        }
        // Either label: MixtureEnthalpy takes one phase of z on its stable root.
        state.setLvalue(1.0);

        // The composition carries no derivatives, so the slope along the
        // (empty) flash tangent is dH/dT at fixed composition.
        auto evaluate = [&](const Scalar T) -> std::optional<SlopedTrial> {
            state.setTemperature(SearchEval(T));
            try {
                const Scalar r = getValue(EnthalpyCalc::mixtureEnthalpy(
                    state, cfg.cpTable, eosType, cfg.model)) - hSpec;
                if (!std::isfinite(r)) {
                    return std::nullopt;
                }
                return SlopedTrial{Trial{T, r}, slopeAlongFlash(state, tLo, tHi, cfg, eosType)};
            }
            catch (const std::runtime_error&) {
                return std::nullopt;
            }
            catch (const Dune::FMatrixError&) {
                return std::nullopt;
            }
        };
        return newton(evaluate, start, tLo, tHi, cfg).temperature;
    }

    //! Whether the compositions x and y are one phase (see coincidentPhases).
    static bool phasesCoincide(const std::array<Scalar, numComponents>& x,
                               const std::array<Scalar, numComponents>& y)
    {
        Scalar measure = 0.0;
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            const Scalar logRatio = std::log(y[compIdx] / x[compIdx]);
            measure += logRatio * logRatio;
        }
        return measure < coincidentPhases;
    }

    /*!
     * \brief Combined Newton iteration on the phase split and the
     *        temperature, from the two-phase split (x, y, L) at T.
     *
     * The equations are those of the Newton iteration of PTFlash (component
     * balances, equal fugacities scaled by the pressure, closure) and
     * H(x, y, L, T) = hSpec under the eos_departure model, with the phase
     * enthalpies of MixtureEnthalpy::departurePhaseEnthalpy. The step is
     * damped to keep the mole fractions positive and to move T by at most
     * maxNewtonStep; L is not bounded. The split is neither checked for
     * stability nor labelled: near a critical point the iteration can end
     * with the two phases exchanged. The caller verifies the solution with a
     * flash that starts from its own initial K, not from this split.
     *
     * \return T after the last step, and L, where the flash equations hold to
     *         ptTolerance and that step is within cfg.temperatureTolerance;
     *         nothing where T leaves [tLo, tHi], the Jacobian is singular, a
     *         value is not finite, the phases coincide, or maxSplitIterations
     *         is reached
     */
    template <class FluidState>
    static std::optional<SplitSolution> splitNewton(const FluidState& fluidState,
                                                    std::array<Scalar, numComponents> x,
                                                    std::array<Scalar, numComponents> y,
                                                    Scalar L,
                                                    Scalar T,
                                                    const Scalar hSpec,
                                                    const Scalar tLo,
                                                    const Scalar tHi,
                                                    const Config& cfg,
                                                    const Scalar ptTolerance,
                                                    const EOSType& eosType)
    {
        constexpr int liquidFractionIdx = 2 * numComponents;
        constexpr int temperatureIdx = 2 * numComponents + 1;
        constexpr auto oilPhaseIdx = FluidSystem::oilPhaseIdx;
        constexpr auto gasPhaseIdx = FluidSystem::gasPhaseIdx;
        using BuiltIn = MixtureEnthalpy<Scalar, FluidSystem>;

        const Scalar pressure = getValue(fluidState.pressure(oilPhaseIdx));
        std::array<Scalar, numComponents> z{};
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            z[compIdx] = getValue(fluidState.moleFraction(compIdx));
        }

        SplitState state;
        state.setPressure(oilPhaseIdx, pressure);
        state.setPressure(gasPhaseIdx, getValue(fluidState.pressure(gasPhaseIdx)));

        Dune::FieldMatrix<Scalar, numSplitUnknowns, numSplitUnknowns> jacobian;
        Dune::FieldVector<Scalar, numSplitUnknowns> residual;
        Dune::FieldVector<Scalar, numSplitUnknowns> step;
        auto setRow = [&](const int row, const SplitEval& equation) {
            residual[row] = equation.value();
            for (int col = 0; col < numSplitUnknowns; ++col) {
                jacobian[row][col] = equation.derivative(col);
            }
        };

        for (int iteration = 0; iteration < maxSplitIterations; ++iteration) {
            if (!(T >= tLo && T <= tHi)) {
                return std::nullopt;
            }
            state.setTemperature(SplitEval::createVariable(T, temperatureIdx));
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                state.setMoleFraction(oilPhaseIdx, compIdx,
                                      SplitEval::createVariable(x[compIdx], compIdx));
                state.setMoleFraction(gasPhaseIdx, compIdx,
                                      SplitEval::createVariable(y[compIdx],
                                                                numComponents + compIdx));
            }
            const SplitEval liquidFraction = SplitEval::createVariable(L, liquidFractionIdx);
            state.setLvalue(liquidFraction);

            try {
                typename FluidSystem::template ParameterCache<SplitEval> paramCache(eosType);
                for (const auto phaseIdx : {oilPhaseIdx, gasPhaseIdx}) {
                    paramCache.updatePhase(state, phaseIdx);
                    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                        state.setFugacityCoefficient(phaseIdx, compIdx,
                            FluidSystem::fugacityCoefficient(state, paramCache, phaseIdx, compIdx));
                    }
                }

                SplitEval sumX = 0.0;
                SplitEval sumY = 0.0;
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    const SplitEval& xi = state.moleFraction(oilPhaseIdx, compIdx);
                    const SplitEval& yi = state.moleFraction(gasPhaseIdx, compIdx);
                    setRow(compIdx, liquidFraction * xi + (1.0 - liquidFraction) * yi - z[compIdx]);
                    setRow(numComponents + compIdx,
                           (state.fugacity(oilPhaseIdx, compIdx)
                            - state.fugacity(gasPhaseIdx, compIdx)) / pressure);
                    sumX += xi;
                    sumY += yi;
                }
                setRow(liquidFractionIdx, sumX - sumY);
                const SplitEval hOil = BuiltIn::departurePhaseEnthalpy(state, oilPhaseIdx,
                                                                       cfg.cpTable, eosType);
                const SplitEval hGas = BuiltIn::departurePhaseEnthalpy(state, gasPhaseIdx,
                                                                       cfg.cpTable, eosType);
                setRow(temperatureIdx,
                       liquidFraction * hOil + (1.0 - liquidFraction) * hGas - hSpec);
                jacobian.solve(step, residual);
            }
            catch (const std::runtime_error&) {
                return std::nullopt;
            }
            catch (const Dune::FMatrixError&) {
                return std::nullopt;
            }
            const auto isFinite = [](const Scalar v) { return std::isfinite(v); };
            if (!std::ranges::all_of(step, isFinite)) {
                return std::nullopt;
            }

            Scalar flashNorm = 0.0;
            for (int row = 0; row < temperatureIdx; ++row) {
                flashNorm += residual[row] * residual[row];
            }
            if (std::sqrt(flashNorm) < ptTolerance
                && std::abs(step[temperatureIdx]) <= cfg.temperatureTolerance)
            {
                if (phasesCoincide(x, y)) {
                    return std::nullopt;
                }
                // As in newton(), the last step costs no evaluation and makes
                // the residual at the solution quadratically small.
                return SplitSolution{std::clamp(T - step[temperatureIdx], tLo, tHi), L};
            }

            Scalar alpha = 1.0;
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                if (x[compIdx] - step[compIdx] <= 0.0) {
                    alpha = std::min(alpha, Scalar(0.9) * x[compIdx] / step[compIdx]);
                }
                const Scalar dy = step[numComponents + compIdx];
                if (y[compIdx] - dy <= 0.0) {
                    alpha = std::min(alpha, Scalar(0.9) * y[compIdx] / dy);
                }
            }
            alpha = std::min(alpha, maxNewtonStep / std::abs(step[temperatureIdx]));
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                x[compIdx] -= alpha * step[compIdx];
                y[compIdx] -= alpha * step[numComponents + compIdx];
            }
            L -= alpha * step[liquidFractionIdx];
            T -= alpha * step[temperatureIdx];
        }
        return std::nullopt;
    }

    /*!
     * \brief A bracket of the root of r(T) on [tLo, tHi].
     *
     * Both ends are evaluated. With both evaluated, they are the bracket if
     * their residuals differ in sign or one is zero; otherwise hSpec is not
     * attained on the interval. Where one end cannot be evaluated, the search
     * starts from the other (bracketToward). Where neither can, a few interior
     * temperatures are tried for a start, and the search goes from there
     * toward the end the residual points to (H increases with T), then toward
     * the other.
     *
     * \param evaluate returns r(T), or nothing where r cannot be evaluated
     * \return the bracket, or nothing
     */
    template <class Evaluate>
    static std::optional<Bracket> findBracket(const Evaluate& evaluate,
                                              const Scalar tLo,
                                              const Scalar tHi,
                                              const Config& cfg,
                                              const int verbosity)
    {
        const auto rLo = evaluate(tLo);
        const auto rHi = evaluate(tHi);
        if (rLo && rHi) {
            if ((*rLo < 0.0) == (*rHi < 0.0) && *rLo != 0.0 && *rHi != 0.0) {
                return std::nullopt; // hSpec is not attained on the interval
            }
            return Bracket{Trial{tLo, *rLo}, Trial{tHi, *rHi}};
        }
        if (rLo) {
            return bracketToward(evaluate, Trial{tLo, *rLo}, tHi, cfg, verbosity);
        }
        if (rHi) {
            return bracketToward(evaluate, Trial{tHi, *rHi}, tLo, cfg, verbosity);
        }

        constexpr std::array<Scalar, 7> probes{0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
        for (const Scalar frac : probes) {
            const Scalar c = tLo + frac * (tHi - tLo);
            if (const auto rc = evaluate(c)) {
                const Trial start{c, *rc};
                const Scalar first = *rc > 0.0 ? tLo : tHi;
                const Scalar second = *rc > 0.0 ? tHi : tLo;
                if (auto bracket = bracketToward(evaluate, start, first, cfg, verbosity)) {
                    return bracket;
                }
                return bracketToward(evaluate, start, second, cfg, verbosity);
            }
        }
        return std::nullopt;
    }

    /*!
     * \brief A bracket between the evaluated trial s and the temperature far,
     *        which cannot be evaluated.
     *
     * The temperatures met between s and far that cannot be evaluated are
     * kept; they split the range into gaps. The gap next to s is bisected: an
     * evaluated midpoint with a zero residual or one of the other sign closes
     * the bracket, one of the same sign becomes s, and one that cannot be
     * evaluated is kept. Once that gap is within cfg.temperatureTolerance, s
     * is accepted as a root on the boundary if the secant through the last
     * two evaluated temperatures places the root within that tolerance of it.
     * Otherwise the gaps beyond the temperatures that could not be evaluated
     * are tried at their midpoints, widest first: a temperature there can
     * still be evaluated and close a bracket. The search gives up when every
     * gap is within the tolerance, or after maxBoundarySteps evaluations. A
     * root inside a range that cannot be evaluated is not found.
     *
     * \return the bracket, or nothing
     */
    template <class Evaluate>
    static std::optional<Bracket> bracketToward(const Evaluate& evaluate,
                                                Trial s,
                                                const Scalar far,
                                                const Config& cfg,
                                                const int verbosity)
    {
        if (s.residual == 0.0) {
            return Bracket{s, s};
        }

        // Temperatures that cannot be evaluated, ordered from s toward far.
        const Scalar direction = far > s.temperature ? 1.0 : -1.0;
        auto beyond = [direction](const Scalar x, const Scalar y) {
            return direction * (y - x) > 0.0;
        };
        std::vector<Scalar> failed{far};

        Trial previous = s;
        for (int step = 0; step < maxBoundarySteps; ++step) {
            Scalar m = Scalar(0.5) * (s.temperature + failed.front());
            const bool nearGapOpen = std::abs(failed.front() - s.temperature) > cfg.temperatureTolerance
                                     && m != s.temperature && m != failed.front();
            if (!nearGapOpen) {
                if (previous.temperature != s.temperature && previous.residual != s.residual
                    && std::abs(s.residual * (s.temperature - previous.temperature)
                                / (s.residual - previous.residual)) <= cfg.temperatureTolerance)
                {
                    return Bracket{s, s};
                }
                // The widest gap beyond the first temperature that cannot be
                // evaluated.
                std::size_t widest = failed.size();
                Scalar width = cfg.temperatureTolerance;
                for (std::size_t i = 0; i + 1 < failed.size(); ++i) {
                    const Scalar w = std::abs(failed[i + 1] - failed[i]);
                    const Scalar mid = Scalar(0.5) * (failed[i] + failed[i + 1]);
                    if (w > width && mid != failed[i] && mid != failed[i + 1]) {
                        widest = i;
                        width = w;
                    }
                }
                if (widest == failed.size()) {
                    return std::nullopt;
                }
                m = Scalar(0.5) * (failed[widest] + failed[widest + 1]);
            }

            const auto rm = evaluate(m);
            if (!rm) {
                failed.insert(std::upper_bound(failed.begin(), failed.end(), m,
                                               [&beyond](const Scalar x, const Scalar y) {
                                                   return beyond(x, y);
                                               }),
                              m);
                continue;
            }
            const Trial trial{m, *rm};
            if (trial.residual == 0.0 || (trial.residual < 0.0) != (s.residual < 0.0)) {
                if (verbosity >= 1) {
                    OpmLog::debug(fmt::format("PHFlash: bracket [{}, {}] K found next to "
                                              "{} K, where the enthalpy cannot be evaluated",
                                              std::min(s.temperature, m),
                                              std::max(s.temperature, m),
                                              failed.front()));
                }
                return s.temperature < m ? Bracket{s, trial} : Bracket{trial, s};
            }
            // Same sign: the root lies beyond m. The temperatures between s
            // and m that cannot be evaluated are behind the search now.
            failed.erase(failed.begin(),
                         std::find_if(failed.begin(), failed.end(),
                                      [&](const Scalar f) { return beyond(m, f); }));
            previous = s;
            s = trial;
        }
        return std::nullopt;
    }

    /*!
     * \brief Illinois false position on a bracket, to cfg.temperatureTolerance.
     *
     * A trial that cannot be evaluated is replaced by up to three other
     * temperatures in the bracket; the bracket is only ever updated with an
     * evaluated temperature and its own residual. Where the replacements cannot
     * be evaluated either, the root can still lie next to them: bracketToward()
     * searches from the lower end toward the failed trial, then from the upper
     * end, and the iteration goes on from the bracket it finds. Without one, or
     * after cfg.maxIterations, the search fails.
     *
     * \return the temperature of the root, or nothing
     */
    template <class Evaluate>
    static std::optional<Scalar> illinois(const Evaluate& evaluate,
                                          Bracket bracket,
                                          const Config& cfg,
                                          const int verbosity)
    {
        Trial& a = bracket.lo;
        Trial& b = bracket.hi;

        // The Illinois rule halves the weight of an end kept twice in a row.
        // The residuals stay as evaluated: bracketToward() reads them.
        Scalar weightA = 1.0;
        Scalar weightB = 1.0;
        int retained = 0; // -1: a was replaced last, +1: b was replaced last
        for (int iteration = 0; iteration < cfg.maxIterations; ++iteration) {
            if (a.residual == 0.0) {
                return a.temperature;
            }
            if (b.residual == 0.0) {
                return b.temperature;
            }
            const Scalar mid = Scalar(0.5) * (a.temperature + b.temperature);
            if (b.temperature - a.temperature <= cfg.temperatureTolerance
                || mid == a.temperature || mid == b.temperature)
            {
                return mid; // or the bracket cannot shrink in Scalar
            }
            const Scalar ra = weightA * a.residual;
            const Scalar rb = weightB * b.residual;
            Scalar t = (a.temperature * rb - b.temperature * ra) / (rb - ra);
            if (!(t > a.temperature && t < b.temperature)) {
                t = mid;
            }

            auto rt = evaluate(t);
            if (!rt) {
                for (const Scalar frac : {Scalar(0.5), Scalar(0.25), Scalar(0.75)}) {
                    const Scalar alternative = a.temperature + frac * (b.temperature - a.temperature);
                    if (std::abs(alternative - t) <= cfg.temperatureTolerance) {
                        continue;
                    }
                    rt = evaluate(alternative);
                    if (rt) {
                        t = alternative;
                        break;
                    }
                }
                if (!rt) {
                    auto narrowed = bracketToward(evaluate, a, t, cfg, verbosity);
                    if (!narrowed) {
                        narrowed = bracketToward(evaluate, b, t, cfg, verbosity);
                    }
                    if (!narrowed) {
                        return std::nullopt;
                    }
                    bracket = *narrowed;
                    weightA = 1.0;
                    weightB = 1.0;
                    retained = 0;
                    continue;
                }
            }

            if (*rt == 0.0) {
                return t;
            }
            if ((*rt < 0.0) == (a.residual < 0.0)) {
                a = Trial{t, *rt};
                weightA = 1.0;
                if (retained == -1) {
                    weightB *= Scalar(0.5); // b kept twice in a row
                }
                retained = -1;
            }
            else {
                b = Trial{t, *rt};
                weightB = 1.0;
                if (retained == +1) {
                    weightA *= Scalar(0.5); // a kept twice in a row
                }
                retained = +1;
            }
        }
        return std::nullopt;
    }
};

} // namespace Opm

#endif // OPM_PH_FLASH_HPP
