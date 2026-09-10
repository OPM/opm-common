// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 NORCE.
  Copyright 2022 SINTEF Digital, Mathematics and Cybernetics.

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
 * \copydoc Opm::PTFlash
 */
#ifndef OPM_CHI_FLASH_HPP
#define OPM_CHI_FLASH_HPP

#include <opm/material/fluidmatrixinteractions/NullMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>
#include <opm/material/Constants.hpp>
#include <opm/material/eos/CubicEOS.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <dune/common/fvector.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/classname.hh>

#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace Opm {

/*!
 * \brief Determines the phase compositions, pressures and saturations
 *        given the total mass of all components for the chiwoms problem.
 *
 */
template <class Scalar, class FluidSystem, bool isThermal = false>
class PTFlash
{
    static constexpr int numPhases = FluidSystem::numPhases;
    static constexpr int numComponents = FluidSystem::numComponents;
    enum { oilPhaseIdx = FluidSystem::oilPhaseIdx};
    enum { gasPhaseIdx = FluidSystem::gasPhaseIdx};
    enum { waterPhaseIdx = FluidSystem::waterPhaseIdx};
    static constexpr int numMiscibleComponents = FluidSystem::numMiscibleComponents;
    static constexpr int numMisciblePhases = FluidSystem::numMisciblePhases; //oil, gas
    static constexpr int numEq = numMisciblePhases + numMisciblePhases * numMiscibleComponents;

    using EOSType = CompositionalConfig::EOSType;

    // Tangent-plane distance below which a trial phase does not split off.
    static constexpr Scalar stabilityTolerance = 1e-5;
    // Successive substitution has converged, or has collapsed onto the
    // trivial solution with all equilibrium ratios at one.
    static constexpr Scalar substitutionTolerance = 1e-10;
    static constexpr Scalar trivialSolutionTolerance = 1e-5;
    // Rachford-Rice bisection stops on the residual or the interval width.
    static constexpr Scalar bisectionResidualTolerance = 1e-16;
    static constexpr Scalar bisectionWidthTolerance = 1e-10;
    // Slope of the Wilson correlation for the initial equilibrium ratios.
    static constexpr Scalar wilsonSlope = 5.3727;

public:
    /*!
     * \brief Calculates the fluid state from the global mole fractions of the components and the phase pressures
     *
     */
    template <class FluidState>
    static bool solve(FluidState& fluid_state,
                      const std::string& twoPhaseMethod,
                      Scalar flash_tolerance,
                      const EOSType& eos_type,
                      int verbosity = 0)
    {
        using ScalarFluidState = CompositionalFluidState<Scalar, FluidSystem>;
        // Solve on a value-only copy; derivatives are reconstructed on the
        // caller's state after the scalar flash has converged.
        ScalarFluidState scalar_state;

        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            scalar_state.setKvalue(compIdx, Opm::getValue(fluid_state.K(compIdx)));
            scalar_state.setMoleFraction(compIdx, Opm::getValue(fluid_state.moleFraction(compIdx)));
        }

        scalar_state.setLvalue(Opm::getValue(fluid_state.L()));
        scalar_state.setPressure(FluidSystem::oilPhaseIdx,
                                 Opm::getValue(fluid_state.pressure(FluidSystem::oilPhaseIdx)));
        scalar_state.setPressure(FluidSystem::gasPhaseIdx,
                                 Opm::getValue(fluid_state.pressure(FluidSystem::gasPhaseIdx)));

        scalar_state.setTemperature(Opm::getValue(fluid_state.temperature(0)));

        const auto is_single_phase = flash_solve_scalar_(
            scalar_state, twoPhaseMethod, flash_tolerance, eos_type, verbosity);

        // Transfer the converged values before reconstructing their derivatives.
        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            const auto liquid_mole_fraction = scalar_state.moleFraction(oilPhaseIdx, compIdx);
            fluid_state.setMoleFraction(oilPhaseIdx, compIdx, liquid_mole_fraction);
            const auto vapour_mole_fraction = scalar_state.moleFraction(gasPhaseIdx, compIdx);
            fluid_state.setMoleFraction(gasPhaseIdx, compIdx, vapour_mole_fraction);
        }

        updateDerivatives_(scalar_state, fluid_state, eos_type, is_single_phase);

        return is_single_phase;
    } //end solve

    /*!
     * \brief Calculates the chemical equilibrium from the component
     *        fugacities in a phase.
     *
     * This is a convenience method which assumes that the capillary pressure is
     * zero...
     */
    template <class FluidState, class ComponentVector>
    static void solve(FluidState& fluid_state,
                      const ComponentVector& globalMolarities,
                      Scalar tolerance = 0.0)
    {
        using MaterialTraits = NullMaterialTraits<Scalar, numPhases>;
        using MaterialLaw = NullMaterial<MaterialTraits>;
        using MaterialLawParams = typename MaterialLaw::Params;

        MaterialLawParams matParams;
        solve<MaterialLaw>(fluid_state, matParams, globalMolarities, tolerance);
    }

    /*!
     * \brief The liquid fraction from the Rachford-Rice equation.
     *
     * Newton's method on the vapour fraction \f$V\f$ solves
     *
     * \f[ g(V) = \sum_i \frac{z_i (K_i - 1)}{1 + V (K_i - 1)} = 0, \f]
     *
     * started at the middle of the bracket
     * \f$(1/(1 - K_{\max}),\, 1/(1 - K_{\min}))\f$ that the poles of \f$g\f$
     * define. \f$g\f$ is monotone inside the bracket, but a Newton update is
     * not constrained to remain there, so an iterate that leaves it is
     * abandoned for a bisection in \f$L\f$ over \f$[0, 1]\f$.
     *
     * \return The liquid fraction \f$L = 1 - V\f$.
     */
    template <class Vector>
    static typename Vector::field_type solveRachfordRice_g_(const Vector& K, const Vector& z, int verbosity)
    {
        // Find min and max K. Have to do a laborious for loop to avoid water component (where K=0)
        // TODO: Replace loop with Dune::min_value() and Dune::max_value() when water component is properly handled
        using field_type = typename Vector::field_type;
        constexpr field_type residual_tolerance = 1e-12;
        constexpr int max_iterations = 10000;
        field_type min_k = K[0];
        field_type max_k = K[0];
        for (int compIdx = 1; compIdx < numComponents; ++compIdx){
            if (K[compIdx] < min_k)
                min_k = K[compIdx];
            else if (K[compIdx] >= max_k)
                max_k = K[compIdx];
        }
        // Lower and upper bound for solution
        const auto min_vapour_fraction = 1 / (1 - max_k);
        const auto max_vapour_fraction = 1 / (1 - min_k);
        // Initial guess
        auto V = (min_vapour_fraction + max_vapour_fraction) / 2;
        // Print initial guess and header
        if (verbosity == 3 || verbosity == 4) {
            OpmLog::debug(fmt::format("Initial guess {}c : V = {} and [Vmin, Vmax] = [{}, {}]",
                                      numComponents,
                                      V,
                                      min_vapour_fraction,
                                      max_vapour_fraction));
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "abs(step)", "V"));
        }
        // Newton-Raphson loop
        for (int iteration = 1; iteration < max_iterations; ++iteration) {
            // Calculate function and derivative values
            field_type negative_derivative = 0.0;
            field_type residual = 0.0;
            for (int compIdx = 0; compIdx < numComponents; ++compIdx){
                const auto k_minus_one = K[compIdx] - 1.0;
                const auto denominator = 1 + V * k_minus_one;
                residual += z[compIdx] * k_minus_one / denominator;
                negative_derivative
                    += z[compIdx] * (k_minus_one * k_minus_one) / (denominator * denominator);
            }
            const auto newton_step = residual / negative_derivative;
            V += newton_step;

            // Check if V is within the bounds, and if not, we apply bisection method
            if (V < min_vapour_fraction || V > max_vapour_fraction) {
                // Print info
                if (verbosity == 3 || verbosity == 4) {
                    OpmLog::debug(fmt::format("V = {} is not within the range [Vmin, Vmax], solve "
                                              "using Bisection method!",
                                              V));
                }

                // Run bisection
                // g is monotone inside the bracket, but a Newton update is
                // not constrained to remain there. Bisection preserves the
                // physical interval.
                const field_type liquid_endpoint = 1.0;
                const field_type vapour_endpoint = 0.0;
                auto L = bisection_g_(K, liquid_endpoint, vapour_endpoint, z, verbosity);

                // Print final result
                if (verbosity >= 1) {
                    OpmLog::debug(fmt::format(
                        "Rachford-Rice (Bisection) converged to final solution L = {}", L));
                }
                return L;
            }

            // Print iteration info
            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(
                    fmt::format("{:>10}{:>16}{:>16}", iteration, Opm::abs(newton_step), V));
            }
            // Check for convergence
            if (Opm::abs(residual) < residual_tolerance) {
                auto L = 1 - V;
                // Should we make sure the range of L is within (0, 1)?

                // Print final result
                if (verbosity >= 1) {
                    OpmLog::debug(fmt::format("Rachford-Rice converged to final solution L = {}", L));
                }
                return L;
            }
        }

        // Throw error if Rachford-Rice fails
        OPM_THROW(std::runtime_error, " Rachford-Rice did not converge within maximum number of iterations");
    }

    /*!
     * \brief The isothermal flash on a scalar fluid state, without derivatives.
     *
     * The stages are:
     * -# a stability test when the state does not already carry a two-phase
     *    split (\f$L \le 0\f$ or \f$L = 1\f$), which either declares the
     *    mixture single-phase or returns starting equilibrium ratios;
     * -# for a two-phase mixture, the Rachford-Rice equation for the
     *    starting liquid fraction, then the phase compositions from
     *    equality of fugacities by successive substitution and/or Newton;
     * -# for a single-phase mixture, the phase label.
     *
     * \return Whether the mixture is single-phase.
     */
    template <typename FluidState>
    static bool flash_solve_scalar_(FluidState& fluid_state,
                                    const std::string& twoPhaseMethod,
                                    const Scalar flash_tolerance,
                                    const EOSType& eos_type,
                                    const int verbosity = 0)
    {
        // A previous two-phase result remains two-phase; otherwise reassess stability.
        bool is_single_phase = false;
        auto L_scalar = fluid_state.L();
        using ScalarVector = Dune::FieldVector<Scalar, numComponents>;
        ScalarVector K_scalar, z_scalar;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            K_scalar[compIdx] = fluid_state.K(compIdx);
            z_scalar[compIdx] = fluid_state.moleFraction(compIdx);
        }

        if ( L_scalar <= 0 || L_scalar == 1 ) {
            if (verbosity >= 1) {
                OpmLog::debug("Perform stability test (L <= 0 or L == 1)!");
            }
            phaseStabilityTest_(
                is_single_phase, K_scalar, fluid_state, z_scalar, eos_type, verbosity);
        }
        if (verbosity >= 1) {
            OpmLog::debug(fmt::format("Inputs after stability test are K = [{}], L = [{}], z = [{}], P = {}, and T = {}",
                                     fmt::join(K_scalar, " "), L_scalar, fmt::join(z_scalar, " "),
                                     fluid_state.pressure(0), fluid_state.temperature(0)));
        }
        // Update the composition if cell is two-phase
        if (!is_single_phase) {
            // Rachford Rice equation to get initial L for composition solver
            L_scalar = solveRachfordRice_g_(K_scalar, z_scalar, verbosity);
            flash_2ph(z_scalar, twoPhaseMethod, K_scalar, L_scalar, fluid_state, flash_tolerance, eos_type, verbosity);
        } else {
            // Cell is one-phase. Use Li's phase labeling method to see if it's liquid or vapor
            L_scalar = li_single_phase_label_(fluid_state, z_scalar, verbosity);
        }
        fluid_state.setLvalue(L_scalar);
        return is_single_phase;
    }

    /*!
     * \brief Bisection for the root of the Rachford-Rice equation in \f$L\f$.
     *
     * Halves the interval between its liquid and vapour endpoints on the sign
     * of \f$g(L)\f$ until either the residual or the interval width is below
     * tolerance.
     */
    template <class Vector>
    static typename Vector::field_type bisection_g_(const Vector& K,
                                                    typename Vector::field_type liquid_endpoint,
                                                    typename Vector::field_type vapour_endpoint,
                                                    const Vector& z,
                                                    int verbosity)
    {
        auto residual_at_liquid_endpoint = rachfordRice_g_(K, liquid_endpoint, z);

        // Print new header
        if (verbosity >= 3) {
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "g(Lmid)", "L"));
        }

        constexpr int max_iterations = 10000;

        auto interval_is_small = [](double first_endpoint, double second_endpoint) {
            return Opm::abs(first_endpoint - second_endpoint) / 2. < bisectionWidthTolerance;
        };

        // Bisection loop
        if (interval_is_small(liquid_endpoint, vapour_endpoint)) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Strange bisection with liquid endpoint {} "
                                  "and vapour endpoint {}",
                                  liquid_endpoint,
                                  vapour_endpoint));
        }
        for (int iteration = 0; iteration < max_iterations; ++iteration) {
            // New midpoint
            const auto L = (liquid_endpoint + vapour_endpoint) / 2;
            const auto midpoint_residual = rachfordRice_g_(K, L, z);
            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", iteration, midpoint_residual, L));
            }

            // Stop when either the residual or the bracket is sufficiently small.
            if (Opm::abs(midpoint_residual) < bisectionResidualTolerance
                || interval_is_small(liquid_endpoint, vapour_endpoint)) {
                return L;
            }
            // Preserve the half whose endpoints have opposite residual signs.
            else if (Dune::sign(midpoint_residual) != Dune::sign(residual_at_liquid_endpoint)) {
                vapour_endpoint = L;
            } else {
                liquid_endpoint = L;
                residual_at_liquid_endpoint = midpoint_residual;
            }
        }
        OPM_THROW(
            std::runtime_error,
            fmt::format(" Rachford-Rice bisection failed with {} iterations!", max_iterations));
    }

    template <class Vector, class FlashFluidState>
    static typename Vector::field_type li_single_phase_label_(const FlashFluidState& fluid_state, const Vector& z, int verbosity)
    {
        // Calculate intermediate sum
        typename Vector::field_type sumVz = 0.0;
        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            // Get component information
            const auto& V_crit = FluidSystem::criticalVolume(compIdx);

            // Sum calculation
            sumVz += (V_crit * z[compIdx]);
        }

        // Calculate approximate (pseudo) critical temperature using Li's method
        typename Vector::field_type Tc_est = 0.0;
        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            // Get component information
            const auto& V_crit = FluidSystem::criticalVolume(compIdx);
            const auto& T_crit = FluidSystem::criticalTemperature(compIdx);

            // Sum calculation
            Tc_est += (V_crit * T_crit * z[compIdx] / sumVz);
        }

        // Get temperature
        const auto& T = fluid_state.temperature(0);

        // If temperature is below estimated critical temperature --> phase = liquid; else vapor
        typename Vector::field_type L;
        if (T < Tc_est) {
            // Liquid
            L = 1.0;

            // Print
            if (verbosity >= 1) {
                OpmLog::debug(fmt::format("Cell is single-phase, liquid (L = 1.0) due to Li's phase labeling method giving T < Tc_est ({} < {})!",
                                         T, Tc_est));
            }
        }
        else {
            // Vapor
            L = 0.0;

            // Print
            if (verbosity >= 1) {
                OpmLog::debug(fmt::format("Cell is single-phase, vapor (L = 0.0) due to Li's phase labeling method giving T >= Tc_est ({} >= {})!",
                                         T, Tc_est));
            }
        }

        return L;
    }

    /*!
     * \brief Michelsen's stability analysis of the mixture.
     *
     * A vapour-like and a liquid-like trial phase are each grown from the
     * mixture with checkStability_(). The mixture is stable, and so single
     * phase, when neither trial ends with a mole number sum above one. When
     * it is not, the trial compositions give the starting equilibrium ratios
     * \f$K_i = y_i / x_i\f$ for the two-phase flash.
     *
     * \param[out] isStable Whether the mixture stays in one phase.
     * \param[in,out] K Starting ratios in, refined ratios out for a two-phase mixture.
     * \param[in,out] fluid_state The mixture; a stable one gets \f$z\f$ as both phase compositions.
     * \param z The mixture composition.
     * \param eos_type The equation of state.
     * \param verbosity Level of debug logging.
     */
    template <class FlashFluidState, class ComponentVector>
    static void phaseStabilityTest_(bool& isStable, ComponentVector& K, FlashFluidState& fluid_state, const ComponentVector& z, const EOSType& eos_type, int verbosity)
    {
        bool liquid_trial_is_trivial, vapour_trial_is_trivial;
        ComponentVector liquid_trial_composition, vapour_trial_composition;
        typename FlashFluidState::ValueType liquid_trial_sum, vapour_trial_sum;
        ComponentVector vapour_trial_k = K;
        ComponentVector liquid_trial_k = K;

        // Grow a vapour-like trial from a liquid reference state.
        if (verbosity == 3 || verbosity == 4) {
            OpmLog::debug("Stability test for vapor phase:");
        }
        checkStability_(fluid_state,
                        vapour_trial_is_trivial,
                        vapour_trial_k,
                        vapour_trial_composition,
                        vapour_trial_sum,
                        z,
                        /*is_vapour_trial=*/true,
                        eos_type,
                        verbosity);
        const bool vapour_trial_finds_no_instability
            = (vapour_trial_sum < (1.0 + stabilityTolerance)) || vapour_trial_is_trivial;

        // Grow a liquid-like trial from a vapour reference state.
        if (verbosity == 3 || verbosity == 4) {
            OpmLog::debug("Stability test for liquid phase:");
        }
        checkStability_(fluid_state,
                        liquid_trial_is_trivial,
                        liquid_trial_k,
                        liquid_trial_composition,
                        liquid_trial_sum,
                        z,
                        /*is_vapour_trial=*/false,
                        eos_type,
                        verbosity);
        const bool liquid_trial_finds_no_instability
            = (liquid_trial_sum < (1.0 + stabilityTolerance)) || liquid_trial_is_trivial;

        // Neither trial phase found a negative tangent-plane distance.
        isStable = liquid_trial_finds_no_instability && vapour_trial_finds_no_instability;
        if (isStable) {
            // A single phase has the feed composition under either phase label.
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                fluid_state.setMoleFraction(gasPhaseIdx, compIdx, z[compIdx]);
                fluid_state.setMoleFraction(oilPhaseIdx, compIdx, z[compIdx]);
            }
        }
        // The two stationary trial compositions seed the equilibrium ratios.
        else {
            for (int compIdx = 0; compIdx<numComponents; ++compIdx) {
                K[compIdx] = vapour_trial_composition[compIdx] / liquid_trial_composition[compIdx];
            }
        }
    }

protected:

    /*!
     * \brief Wilson's correlation for a starting equilibrium ratio,
     *
     * \f[ K_i = \frac{p_{c,i}}{p}
     *   \exp\left[ 5.3727 (1 + \omega_i) \left( 1 - \frac{T_{c,i}}{T} \right) \right]. \f]
     */
    template <class FlashFluidState>
    static typename FlashFluidState::ValueType wilsonK_(const FlashFluidState& fluid_state, int compIdx)
    {
        const auto& acf = FluidSystem::acentricFactor(compIdx);
        const auto& T_crit = FluidSystem::criticalTemperature(compIdx);
        const auto& T = fluid_state.temperature(0);
        const auto& p_crit = FluidSystem::criticalPressure(compIdx);
        const auto& p = fluid_state.pressure(0); //for now assume no capillary pressure

        const auto equilibrium_ratio
            = Opm::exp(wilsonSlope * (1 + acf) * (1 - T_crit / T)) * (p_crit / p);
        return equilibrium_ratio;
    }

    /*!
     * \brief The Rachford-Rice function in the liquid fraction,
     *
     * \f[ g(L) = \sum_i \frac{z_i (K_i - 1)}{K_i - L (K_i - 1)}. \f]
     */
    template <class Vector>
    static typename Vector::field_type rachfordRice_g_(const Vector& K, typename Vector::field_type L, const Vector& z)
    {
        typename Vector::field_type g=0;
        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            g += (z[compIdx]*(K[compIdx]-1))/(K[compIdx]-L*(K[compIdx]-1));
        }
        return g;
    }


    /*!
     * \brief One trial phase of Michelsen's stability analysis.
     *
     * A trial phase with mole numbers \f$W_i = K_i z_i\f$ (vapour-like) or
     * \f$W_i = z_i / K_i\f$ (liquid-like) is driven to a stationary point of
     * the tangent-plane-distance function by successive substitution,
     *
     * \f[ K_i \leftarrow K_i R_i, \qquad
     *     R_i = \frac{f_i(\mathbf z)}{S f_i(\mathbf W / S)} \text{ (vapour)}, \quad
     *     R_i = \frac{S f_i(\mathbf W / S)}{f_i(\mathbf z)} \text{ (liquid)}, \f]
     *
     * with \f$S = \sum_i W_i\f$. The iteration has converged when
     * \f$\sum_i (R_i - 1)^2\f$ is small. It has collapsed onto the mixture
     * itself, the trivial solution, when \f$\sum_i \ln^2 K_i\f$ is small. At a
     * stationary point the tangent-plane distance of the normalised trial
     * composition is \f$-\ln S\f$, so a trial that ends with \f$S > 1\f$
     * establishes that the mixture is unstable.
     *
     * \param fluid_state The mixture at the pressure and temperature of the flash.
     * \param[out] isTrivial Whether the trial collapsed onto the mixture.
     * \param[in,out] K Starting ratios in, the trial's converged ratios out.
     * \param[out] trial_composition The normalised trial composition.
     * \param[out] trial_sum The mole number sum \f$S\f$ of the trial phase.
     * \param z The mixture composition.
     * \param is_vapour_trial Whether to grow a vapour-like or liquid-like trial.
     * \param eos_type The equation of state.
     * \param verbosity Level of debug logging.
     */
    template <class FlashFluidState, class ComponentVector>
    static void checkStability_(const FlashFluidState& fluid_state,
                                bool& isTrivial,
                                ComponentVector& K,
                                ComponentVector& trial_composition,
                                typename FlashFluidState::ValueType& trial_sum,
                                const ComponentVector& z,
                                bool is_vapour_trial,
                                const EOSType& eos_type,
                                int verbosity)
    {
        using FlashEval = typename FlashFluidState::ValueType;
        using CubicEOS = typename Opm::CubicEOS<Scalar, FluidSystem>;
        using ParamCache = typename FluidSystem::template ParameterCache<FlashEval>;

        // The trial composition is evaluated on its candidate phase root. The
        // feed composition supplies the reference fugacities on the opposite
        // root: liquid for a vapour-like trial, and vapour for a liquid-like one.
        // This opposite-root choice is an implementation convention, not a
        // consequence of the tangent-plane equations.
        FlashFluidState trial_state = fluid_state;
        FlashFluidState reference_state = fluid_state;
        const int trial_phase_idx
            = is_vapour_trial ? static_cast<int>(gasPhaseIdx) : static_cast<int>(oilPhaseIdx);
        const int reference_phase_idx
            = is_vapour_trial ? static_cast<int>(oilPhaseIdx) : static_cast<int>(gasPhaseIdx);

        // Setup output
        if (verbosity >= 3) {
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "K-Norm", "R-Norm"));
        }

        // Grow the trial phase out of the mixture and find a stationary point
        // of the tangent-plane-distance function.
        constexpr int max_iterations = 20000;
        for (int iteration = 0; iteration < max_iterations; ++iteration) {
            // Mole numbers of the trial phase, then its normalised composition.
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                trial_composition[compIdx]
                    = is_vapour_trial ? K[compIdx] * z[compIdx] : z[compIdx] / K[compIdx];
            }
            trial_sum = std::accumulate(
                trial_composition.begin(), trial_composition.end(), FlashEval(0.0));
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                trial_composition[compIdx] /= trial_sum;
                trial_state.setMoleFraction(trial_phase_idx, compIdx, trial_composition[compIdx]);
            }

            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                reference_state.setMoleFraction(reference_phase_idx, compIdx, z[compIdx]);
            }

            ParamCache trial_param_cache(eos_type);
            trial_param_cache.updatePhase(trial_state, trial_phase_idx);

            ParamCache reference_param_cache(eos_type);
            reference_param_cache.updatePhase(reference_state, reference_phase_idx);

            // Fugacity coefficients of the trial phase and reference phase.
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                const auto trial_fugacity_coefficient = CubicEOS::computeFugacityCoefficient(
                    trial_state, trial_param_cache, trial_phase_idx, compIdx);
                const auto reference_fugacity_coefficient = CubicEOS::computeFugacityCoefficient(
                    reference_state, reference_param_cache, reference_phase_idx, compIdx);

                trial_state.setFugacityCoefficient(
                    trial_phase_idx, compIdx, trial_fugacity_coefficient);
                reference_state.setFugacityCoefficient(
                    reference_phase_idx, compIdx, reference_fugacity_coefficient);
            }

            // The substitution factors R_i of the brief.
            ComponentVector substitution_factor;
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                const auto trial_fugacity = trial_state.fugacity(trial_phase_idx, compIdx);
                const auto reference_fugacity
                    = reference_state.fugacity(reference_phase_idx, compIdx);
                substitution_factor[compIdx] = is_vapour_trial
                    ? reference_fugacity / trial_fugacity / trial_sum
                    : trial_fugacity / reference_fugacity * trial_sum;
            }

            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                K[compIdx] *= substitution_factor[compIdx];
            }
            Scalar substitution_residual = 0.0;
            Scalar trivial_residual = 0.0;
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                const auto substitution_error = Opm::getValue(substitution_factor[compIdx]) - 1.0;
                const auto log_k = Opm::log(Opm::getValue(K[compIdx]));
                substitution_residual += substitution_error * substitution_error;
                trivial_residual += log_k * log_k;
            }

            // Print iteration info
            if (verbosity >= 3) {
                OpmLog::debug(fmt::format(
                    "{:>10}{:>16}{:>16}", iteration, trivial_residual, substitution_residual));
            }

            // Check convergence
            isTrivial = (trivial_residual < trivialSolutionTolerance);
            if (isTrivial || substitution_residual < substitutionTolerance)
                return;
            //todo: make sure that no mole fraction is smaller than 1e-8 ?
            //todo: take care of water!
        }
        OPM_THROW(std::runtime_error, " Stability test did not converge");
    }//end checkStability

    /*!
     * \brief The phase compositions from the equilibrium ratios and the
     *        liquid fraction,
     *
     * \f[ x_i = \frac{z_i}{L + (1 - L) K_i}, \qquad y_i = K_i x_i, \f]
     *
     * each renormalised to sum to one.
     */
    template <class FlashFluidState, class ComponentVector>
    static void computeLiquidVapor_(FlashFluidState& fluid_state, typename FlashFluidState::ValueType& L, ComponentVector& K, const ComponentVector& z)
    {
        // Calculate x and y, and normalize
        ComponentVector x;
        ComponentVector y;
        typename FlashFluidState::ValueType sumx=0;
        typename FlashFluidState::ValueType sumy=0;
        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            x[compIdx] = z[compIdx]/(L + (1-L)*K[compIdx]);
            sumx += x[compIdx];
            y[compIdx] = (K[compIdx]*z[compIdx])/(L + (1-L)*K[compIdx]);
            sumy += y[compIdx];
        }
        x /= sumx;
        y /= sumy;

        for (int compIdx=0; compIdx<numComponents; ++compIdx){
            fluid_state.setMoleFraction(oilPhaseIdx, compIdx, x[compIdx]);
            fluid_state.setMoleFraction(gasPhaseIdx, compIdx, y[compIdx]);
        }
    }

    /*!
     * \brief The two-phase compositions by the requested method.
     *
     * "ssi" is successive substitution, "newton" is Newton's method, and
     * "ssi+newton" uses a few substitution steps to condition the Newton
     * start and falls back to substitution should Newton fail.
     */
    template <class FluidState, class ComponentVector>
    static void flash_2ph(const ComponentVector& z_scalar,
                          const std::string& flash_2p_method,
                          ComponentVector& K_scalar,
                          typename FluidState::ValueType& L_scalar,
                          FluidState& fluid_state_scalar,
                          const Scalar flash_tolerance,
                          const EOSType& eos_type,
                          int verbosity = 0) {
        if (verbosity >= 1) {
            OpmLog::debug(fmt::format("Cell is two-phase! Solve Rachford-Rice with initial K = [{}]",
                                     fmt::join(K_scalar, " ")));
        }

        // Calculate composition using nonlinear solver
        // Newton
        bool converged = false;
        if (flash_2p_method == "newton") {
            if (verbosity >= 1) {
                OpmLog::debug("Calculate composition using Newton.");
            }
            converged = newtonComposition_(K_scalar, L_scalar, fluid_state_scalar, z_scalar, flash_tolerance, eos_type, verbosity);
        } else if (flash_2p_method == "ssi") {
            // Successive substitution
            if (verbosity >= 1) {
                OpmLog::debug("Calculate composition using Successive Substitution.");
            }
            converged = successiveSubstitutionComposition_(K_scalar, L_scalar, fluid_state_scalar, z_scalar, false, flash_tolerance, eos_type, verbosity);
        } else if (flash_2p_method == "ssi+newton") {
            converged = successiveSubstitutionComposition_(K_scalar, L_scalar, fluid_state_scalar, z_scalar, true, flash_tolerance, eos_type, verbosity);
            if (!converged) {
                // In this method Newton is an accelerator, not the
                // guarantee: when its composition update fails to converge —
                // or a diverged iterate makes the EoS report non-finite
                // mixing parameters — switch back to plain successive
                // substitution and finish the solve with the method that is
                // robust on these states, instead of failing the whole
                // solve. Newton throws rather than returning a bad result;
                // it may already have mutated some fluid state (e.g. via
                // computeLiquidVapor_()) before throwing, but the successive-
                // substitution fallback re-derives the composition from K and
                // L and overwrites that state, so restarting it here is safe.
                try {
                    converged = newtonComposition_(K_scalar, L_scalar, fluid_state_scalar, z_scalar, flash_tolerance, eos_type, verbosity);
                }
                catch (const std::runtime_error& e) {
                    if (verbosity >= 1) {
                        OpmLog::debug(fmt::format("Newton did not finish the composition update ({}); "
                                                  "switching back to successive substitution.", e.what()));
                    }
                    converged = successiveSubstitutionComposition_(K_scalar, L_scalar, fluid_state_scalar, z_scalar, false, flash_tolerance, eos_type, verbosity);
                }
            }
        } else {
            OPM_THROW(std::logic_error,
                      "unknown two phase flash method " + flash_2p_method + " is specified");
        }

        if (!converged) {
            OPM_THROW(std::runtime_error,
                      "flash calculation did not get converged with " + flash_2p_method);
        }
    }

    /*!
     * \brief Newton's method for the two-phase compositions.
     *
     * The unknowns are \f$(\mathbf x, \mathbf y, L)\f$ and the residuals,
     * assembled by assembleNewton_(), are the component balances, equal
     * fugacities and the closure
     *
     * \f[ z_i - L x_i - (1 - L) y_i = 0, \qquad
     *     f_i^{L} - f_i^{V} = 0, \qquad
     *     \sum_i x_i - \sum_i y_i = 0. \f]
     *
     * The Jacobian comes from automatic differentiation of the residuals in
     * the unknowns.
     */
    template <class FlashFluidState, class ComponentVector>
    static bool newtonComposition_(ComponentVector& K,
                                   typename FlashFluidState::ValueType& L,
                                   FlashFluidState& fluid_state,
                                   const ComponentVector& z,
                                   const Scalar tolerance,
                                   const EOSType& eos_type,
                                   int verbosity)
    {
        constexpr std::size_t num_equations = numMisciblePhases * numMiscibleComponents + 1;
        constexpr std::size_t num_unknowns = numMisciblePhases * numMiscibleComponents + 1;
        using NewtonVector = Dune::FieldVector<Scalar, num_equations>;
        using NewtonMatrix = Dune::FieldMatrix<Scalar, num_equations, num_unknowns>;

        NewtonVector newton_step(0.);
        NewtonVector residual(0.);
        NewtonMatrix jacobian(0.);

        // Compute x and y from K, L and Z
        computeLiquidVapor_(fluid_state, L, K, z);

        // Print initial condition
        if (verbosity >= 1) {
            OpmLog::debug(fmt::format(" the current L is {}", Opm::getValue(L)));
            std::vector<Scalar> liquid_composition_values(numComponents);
            std::vector<Scalar> vapour_composition_values(numComponents);
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                liquid_composition_values[compIdx]
                    = Opm::getValue(fluid_state.moleFraction(oilPhaseIdx, compIdx));
                vapour_composition_values[compIdx]
                    = Opm::getValue(fluid_state.moleFraction(gasPhaseIdx, compIdx));
            }
            OpmLog::debug(fmt::format("Initial guess: x = [{}], y = [{}], and L = {}",
                                      fmt::join(liquid_composition_values, " "),
                                      fmt::join(vapour_composition_values, " "),
                                      Opm::getValue(L)));
        }

        // Print header
        if (verbosity == 2 || verbosity == 4) {
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "Norm2(step)", "Norm2(Residual)"));
        }

        // Each flash unknown is an independent automatic-differentiation variable.
        using NewtonEval = DenseAd::Evaluation<Scalar, num_unknowns>;
        std::vector<NewtonEval> liquid_composition(numComponents);
        std::vector<NewtonEval> vapour_composition(numComponents);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            liquid_composition[compIdx]
                = NewtonEval(fluid_state.moleFraction(oilPhaseIdx, compIdx), compIdx);
            const unsigned idx = compIdx + numComponents;
            vapour_composition[compIdx]
                = NewtonEval(fluid_state.moleFraction(gasPhaseIdx, compIdx), idx);
        }
        auto liquid_fraction = NewtonEval(L, num_unknowns - 1);

        // This state carries derivatives with respect to the Newton unknowns.
        CompositionalFluidState<NewtonEval, FluidSystem> newton_state;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            newton_state.setMoleFraction(
                FluidSystem::oilPhaseIdx, compIdx, liquid_composition[compIdx]);
            newton_state.setMoleFraction(
                FluidSystem::gasPhaseIdx, compIdx, vapour_composition[compIdx]);
            newton_state.setKvalue(compIdx,
                                   vapour_composition[compIdx] / liquid_composition[compIdx]);
        }
        newton_state.setLvalue(liquid_fraction);
        newton_state.setPressure(FluidSystem::oilPhaseIdx,
                                 fluid_state.pressure(FluidSystem::oilPhaseIdx));
        newton_state.setPressure(FluidSystem::gasPhaseIdx,
                                 fluid_state.pressure(FluidSystem::gasPhaseIdx));

        // Preserve the caller's saturations, although they do not enter the
        // current flash residual.
        newton_state.setSaturation(FluidSystem::gasPhaseIdx,
                                   fluid_state.saturation(FluidSystem::gasPhaseIdx));
        newton_state.setSaturation(FluidSystem::oilPhaseIdx,
                                   fluid_state.saturation(FluidSystem::oilPhaseIdx));
        newton_state.setTemperature(fluid_state.temperature(0));

        using ParamCache = typename FluidSystem::template ParameterCache<NewtonEval>;
        ParamCache paramCache(eos_type);

        for (unsigned phaseIdx = 0; phaseIdx < numMisciblePhases; ++phaseIdx) {
            paramCache.updatePhase(newton_state, phaseIdx);
            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                NewtonEval fugacity_coefficient
                    = FluidSystem::fugacityCoefficient(newton_state, paramCache, phaseIdx, compIdx);
                newton_state.setFugacityCoefficient(phaseIdx, compIdx, fugacity_coefficient);
            }
        }
        bool converged = false;
        unsigned iteration = 0;
        constexpr unsigned max_iterations = 1000;
        while (iteration < max_iterations) {
            assembleNewton_<CompositionalFluidState<NewtonEval, FluidSystem>,
                            ComponentVector,
                            num_unknowns,
                            num_equations>(newton_state, z, jacobian, residual);
            if (verbosity >= 1) {
                OpmLog::debug(fmt::format(" newton residual is {}", residual.two_norm()));
            }
            converged = residual.two_norm() < tolerance;
            if (converged) {
                break;
            }

            jacobian.solve(newton_step, residual);
            constexpr Scalar damping_factor = 1.0;
            // updating x and y
            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                liquid_composition[compIdx] -= newton_step[compIdx] * damping_factor;
                vapour_composition[compIdx]
                    -= newton_step[compIdx + numComponents] * damping_factor;
            }
            liquid_fraction -= newton_step[num_equations - 1] * damping_factor;

            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                newton_state.setMoleFraction(
                    FluidSystem::oilPhaseIdx, compIdx, liquid_composition[compIdx]);
                newton_state.setMoleFraction(
                    FluidSystem::gasPhaseIdx, compIdx, vapour_composition[compIdx]);
                newton_state.setKvalue(compIdx,
                                       vapour_composition[compIdx] / liquid_composition[compIdx]);
            }
            newton_state.setLvalue(liquid_fraction);

            for (unsigned phaseIdx = 0; phaseIdx < numMisciblePhases; ++phaseIdx) {
                paramCache.updatePhase(newton_state, phaseIdx);
                for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                    NewtonEval fugacity_coefficient = FluidSystem::fugacityCoefficient(
                        newton_state, paramCache, phaseIdx, compIdx);
                    newton_state.setFugacityCoefficient(phaseIdx, compIdx, fugacity_coefficient);
                }
            }
            ++iteration;
        }
        if (verbosity >= 1) {
            fmt::memory_buffer buf;
            for (unsigned equation_idx = 0; equation_idx < num_equations; ++equation_idx) {
                for (unsigned unknown_idx = 0; unknown_idx < num_unknowns; ++unknown_idx) {
                    fmt::format_to(
                        std::back_inserter(buf), " {}", jacobian[equation_idx][unknown_idx]);
                }
                fmt::format_to(std::back_inserter(buf), "\n");
            }
            OpmLog::debug(fmt::to_string(buf));
        }
        if (!converged) {
            OPM_THROW(std::runtime_error,
                      fmt::format(" Newton composition update did not converge "
                                  "within {} iterations",
                                  max_iterations));
        }

        // Copy the converged unknowns to the caller and the explicit K/L outputs.
        for (unsigned idx = 0; idx < numComponents; ++idx) {
            const auto liquid_mole_fraction
                = Opm::getValue(newton_state.moleFraction(oilPhaseIdx, idx));
            fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, idx, liquid_mole_fraction);
            const auto vapour_mole_fraction
                = Opm::getValue(newton_state.moleFraction(gasPhaseIdx, idx));
            fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, idx, vapour_mole_fraction);
            const auto equilibrium_ratio = Opm::getValue(newton_state.K(idx));
            fluid_state.setKvalue(idx, equilibrium_ratio);
            K[idx] = equilibrium_ratio;
        }
        L = Opm::getValue(liquid_fraction);
        fluid_state.setLvalue(L);
        return converged;
    }

    /*!
     * \brief The residual and Jacobian of newtonComposition_() at the current
     *        state, whose values carry derivatives in the unknowns.
     */
    template <typename FlashFluidState,
              typename ComponentVector,
              std::size_t num_unknowns,
              std::size_t num_equations>
    static void assembleNewton_(const FlashFluidState& fluid_state,
                                const ComponentVector& overall_composition,
                                Dune::FieldMatrix<double, num_equations, num_unknowns>& jacobian,
                                Dune::FieldVector<double, num_equations>& residual)
    {
        using Eval = DenseAd::Evaluation<double, num_unknowns>;
        std::vector<Eval> liquid_composition(numComponents);
        std::vector<Eval> vapour_composition(numComponents);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            liquid_composition[compIdx] = fluid_state.moleFraction(oilPhaseIdx, compIdx);
            vapour_composition[compIdx] = fluid_state.moleFraction(gasPhaseIdx, compIdx);
        }
        const Eval& liquid_fraction = fluid_state.L();
        jacobian = 0.;
        residual = 0.;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            {
                // z - L*x - (1-L) * y
                const auto component_balance = -overall_composition[compIdx]
                    + liquid_fraction * liquid_composition[compIdx]
                    + (1 - liquid_fraction) * vapour_composition[compIdx];
                residual[compIdx] = Opm::getValue(component_balance);
                for (unsigned unknown_idx = 0; unknown_idx < num_unknowns; ++unknown_idx) {
                    jacobian[compIdx][unknown_idx] = component_balance.derivative(unknown_idx);
                }
            }

            {
                // f_liquid - f_vapor = 0
                const auto fugacity_difference = fluid_state.fugacity(oilPhaseIdx, compIdx)
                    - fluid_state.fugacity(gasPhaseIdx, compIdx);
                residual[compIdx + numComponents] = Opm::getValue(fugacity_difference);
                for (unsigned unknown_idx = 0; unknown_idx < num_unknowns; ++unknown_idx) {
                    jacobian[compIdx + numComponents][unknown_idx]
                        = fugacity_difference.derivative(unknown_idx);
                }
            }
        }
        // sum(x) - sum(y) = 0
        Eval liquid_composition_sum = 0.;
        Eval vapour_composition_sum = 0.;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            liquid_composition_sum += liquid_composition[compIdx];
            vapour_composition_sum += vapour_composition[compIdx];
        }
        const auto composition_closure = liquid_composition_sum - vapour_composition_sum;
        residual[num_equations - 1] = Opm::getValue(composition_closure);
        for (unsigned unknown_idx = 0; unknown_idx < num_unknowns; ++unknown_idx) {
            jacobian[num_equations - 1][unknown_idx] = composition_closure.derivative(unknown_idx);
        }
    }

    /*!
     * \brief Gives the converged flash result the derivatives of the caller.
     *
     * The flash is solved on scalars; the compositions it returns are then
     * functions of the caller's variables through the equations they
     * satisfy. Single-phase results need no work, as \f$x = y = z\f$.
     */
    template <typename FlashFluidStateScalar, typename FluidState>
    static void updateDerivatives_(const FlashFluidStateScalar& fluid_state_scalar,
                                   FluidState& fluid_state,
                                   const EOSType& eos_type,
                                   bool is_single_phase)
    {
        if (!is_single_phase)
            updateDerivativesTwoPhase_(fluid_state_scalar, fluid_state, eos_type);
        else
            updateDerivativesSinglePhase_(fluid_state_scalar, fluid_state);
    }

    /*!
     * \brief Derivatives of a two-phase result by the implicit function theorem.
     *
     * With the flash residuals \f$F(\mathbf u, \mathbf s) = 0\f$ in the
     * unknowns \f$\mathbf u = (\mathbf x, \mathbf y, L)\f$ and the state
     * variables \f$\mathbf s = (p, [T,] \mathbf z)\f$,
     *
     * \f[ \frac{\partial \mathbf u}{\partial \mathbf s}
     *   = - \left( \frac{\partial F}{\partial \mathbf u} \right)^{-1}
     *       \frac{\partial F}{\partial \mathbf s}, \f]
     *
     * and the chain rule through the caller's derivatives of \f$\mathbf s\f$
     * gives those of \f$\mathbf u\f$.
     */
    template <typename FlashFluidStateScalar, typename FluidState>
    static void updateDerivativesTwoPhase_(const FlashFluidStateScalar& fluid_state_scalar,
                                           FluidState& fluid_state,
                                           const EOSType& eos_type)
    {
        using InputEval = typename FluidState::ValueType;
        using ComponentVector = Dune::FieldVector<InputEval, numComponents>;
        ComponentVector z;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            z[compIdx] = fluid_state.moleFraction(compIdx);
        }

        constexpr std::size_t num_equations = numMisciblePhases * numMiscibleComponents + 1;
        constexpr std::size_t num_state_variables
            = isThermal ? numComponents + 2 : numComponents + 1;
        using StateEval = Opm::DenseAd::Evaluation<double, num_state_variables>;
        using StateComponentVector = Dune::FieldVector<StateEval, numComponents>;
        using StateFluidState = Opm::CompositionalFluidState<StateEval, FluidSystem>;

        // Evaluate F with s = (p, [T], z) as independent AD variables while
        // holding the converged flash unknowns u = (x, y, L) fixed.
        StateFluidState state_derivative_state;
        StateComponentVector state_composition;
        const StateEval state_pressure
            = StateEval(fluid_state_scalar.pressure(FluidSystem::oilPhaseIdx), 0);
        state_derivative_state.setPressure(FluidSystem::oilPhaseIdx, state_pressure);
        state_derivative_state.setPressure(FluidSystem::gasPhaseIdx, state_pressure);

        if constexpr (isThermal) {
            // set the temperature with derivatives
            const StateEval state_temperature = StateEval(fluid_state_scalar.temperature(0), 1);
            state_derivative_state.setTemperature(state_temperature);
        } else {
            // set the temperature as a scalar
            state_derivative_state.setTemperature(Opm::getValue(fluid_state_scalar.temperature(0)));
        }

        // composition variable offset: 2 for thermal (pressure=0, temperature=1, z starts at 2)
        //                               1 for non-thermal (pressure=0, z starts at 1)
        constexpr unsigned composition_offset = isThermal ? 2 : 1;
        for (unsigned idx = 0; idx < numComponents; ++idx) {
            state_composition[idx] = StateEval(Opm::getValue(z[idx]), idx + composition_offset);
        }
        // The converged flash unknowns are constants in this derivative pass.
        for (unsigned idx = 0; idx < numComponents; ++idx) {
            const auto liquid_mole_fraction = fluid_state_scalar.moleFraction(oilPhaseIdx, idx);
            state_derivative_state.setMoleFraction(
                FluidSystem::oilPhaseIdx, idx, liquid_mole_fraction);
            const auto vapour_mole_fraction = fluid_state_scalar.moleFraction(gasPhaseIdx, idx);
            state_derivative_state.setMoleFraction(
                FluidSystem::gasPhaseIdx, idx, vapour_mole_fraction);
            const auto equilibrium_ratio = fluid_state_scalar.K(idx);
            state_derivative_state.setKvalue(idx, equilibrium_ratio);
        }
        const auto scalar_liquid_fraction = fluid_state_scalar.L();
        state_derivative_state.setLvalue(scalar_liquid_fraction);
        // Saturations do not enter the flash residual and remain unset here.
        using StateParamCache = typename FluidSystem::template ParameterCache<StateEval>;
        StateParamCache state_param_cache(eos_type);
        for (unsigned phase_idx = 0; phase_idx < numMisciblePhases; ++phase_idx) {
            state_param_cache.updatePhase(state_derivative_state, phase_idx);
            for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                StateEval fugacity_coefficient = FluidSystem::fugacityCoefficient(
                    state_derivative_state, state_param_cache, phase_idx, comp_idx);
                state_derivative_state.setFugacityCoefficient(
                    phase_idx, comp_idx, fugacity_coefficient);
            }
        }

        using StateResidual = Dune::FieldVector<Scalar, num_equations>;
        using StateJacobian = Dune::FieldMatrix<Scalar, num_equations, num_state_variables>;
        StateJacobian state_jacobian;
        StateResidual state_residual;

        // This gives the state-variable Jacobian dF/ds.
        assembleNewton_<StateFluidState, StateComponentVector, num_state_variables, num_equations>(
            state_derivative_state, state_composition, state_jacobian, state_residual);

        // Evaluate the same residual with u = (x, y, L) as independent AD
        // variables while holding s fixed. This gives the unknown Jacobian dF/du.
        constexpr std::size_t num_flash_unknowns = numMisciblePhases * numMiscibleComponents + 1;
        using UnknownEval = Opm::DenseAd::Evaluation<double, num_flash_unknowns>;
        using ScalarComponentVector = Dune::FieldVector<double, numComponents>;
        using UnknownFluidState = Opm::CompositionalFluidState<UnknownEval, FluidSystem>;

        UnknownFluidState unknown_derivative_state;
        ScalarComponentVector overall_composition;
        for (unsigned  comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            overall_composition[comp_idx] = Opm::getValue(z[comp_idx]);
        }
        for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            const auto liquid_mole_fraction
                = UnknownEval(fluid_state_scalar.moleFraction(oilPhaseIdx, comp_idx), comp_idx);
            unknown_derivative_state.setMoleFraction(oilPhaseIdx, comp_idx, liquid_mole_fraction);
            const unsigned idx = comp_idx + numComponents;
            const auto vapour_mole_fraction
                = UnknownEval(fluid_state_scalar.moleFraction(gasPhaseIdx, comp_idx), idx);
            unknown_derivative_state.setMoleFraction(gasPhaseIdx, comp_idx, vapour_mole_fraction);
            unknown_derivative_state.setKvalue(comp_idx,
                                               vapour_mole_fraction / liquid_mole_fraction);
        }
        const auto liquid_fraction = UnknownEval(fluid_state_scalar.L(), num_flash_unknowns - 1);
        unknown_derivative_state.setLvalue(liquid_fraction);
        unknown_derivative_state.setPressure(oilPhaseIdx, fluid_state_scalar.pressure(oilPhaseIdx));
        unknown_derivative_state.setPressure(gasPhaseIdx, fluid_state_scalar.pressure(gasPhaseIdx));
        unknown_derivative_state.setTemperature(fluid_state_scalar.temperature(0));

        using UnknownParamCache = typename FluidSystem::template ParameterCache<UnknownEval>;
        UnknownParamCache unknown_param_cache(eos_type);
        for (unsigned phase_idx = 0; phase_idx < numMisciblePhases; ++phase_idx) {
            unknown_param_cache.updatePhase(unknown_derivative_state, phase_idx);
            for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                UnknownEval fugacity_coefficient = FluidSystem::fugacityCoefficient(
                    unknown_derivative_state, unknown_param_cache, phase_idx, comp_idx);
                unknown_derivative_state.setFugacityCoefficient(
                    phase_idx, comp_idx, fugacity_coefficient);
            }
        }

        using UnknownResidual = Dune::FieldVector<Scalar, num_equations>;
        using UnknownJacobian = Dune::FieldMatrix<Scalar, num_equations, num_flash_unknowns>;
        UnknownResidual unknown_residual;
        UnknownJacobian unknown_jacobian;

        assembleNewton_<UnknownFluidState,
                        ScalarComponentVector,
                        num_flash_unknowns,
                        num_equations>(
            unknown_derivative_state, overall_composition, unknown_jacobian, unknown_residual);

        // DUNE 2.6 cannot solve directly with a matrix right-hand side, so form
        // the inverse explicitly before multiplying the state Jacobian.
        // StateJacobian xx;
        // unknown_jacobian.solve(xx, state_jacobian);
        unknown_jacobian.invert();
        state_jacobian.template leftmultiply<UnknownJacobian>(unknown_jacobian);
        // state_jacobian now stores (dF/du)^-1 dF/ds; the minus sign is
        // applied while transferring each sensitivity below.

        ComponentVector liquid_composition(numComponents);
        ComponentVector vapour_composition(numComponents);
        InputEval liquid_fraction_with_derivatives = scalar_liquid_fraction;

        // Apply du/ds = -(dF/du)^-1 dF/ds, then compose it with the
        // caller's derivatives of s.

        const auto liquid_pressure = fluid_state.pressure(FluidSystem::oilPhaseIdx);
        const auto vapour_pressure = fluid_state.pressure(FluidSystem::gasPhaseIdx);
        // at the moment, the temperature is the same for both phases
        // input_temperature is not used for non-thermal case
        [[maybe_unused]] const auto& input_temperature = fluid_state.temperature(0);

        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            liquid_composition[compIdx]
                = fluid_state_scalar.moleFraction(FluidSystem::oilPhaseIdx, compIdx);
            vapour_composition[compIdx]
                = fluid_state_scalar.moleFraction(FluidSystem::gasPhaseIdx, compIdx);
        }

        // The two pressures are equal today. Keep both inputs explicit so this
        // chain rule can accommodate capillary pressure later.

        constexpr std::size_t num_derivatives = InputEval::numVars;
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            std::vector<double> derivatives(num_derivatives, 0.);
            // derivatives from P
            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                derivatives[idx] = -state_jacobian[compIdx][0] * liquid_pressure.derivative(idx);
            }
            // derivatives from T (only for thermal case)
            if constexpr (isThermal) {
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    derivatives[idx]
                        += -state_jacobian[compIdx][1] * input_temperature.derivative(idx);
                }
            }

            for (unsigned cIdx = 0; cIdx < numComponents; ++cIdx) {
                const double composition_sensitivity
                    = -state_jacobian[compIdx][cIdx + composition_offset];
                const auto& overall_mole_fraction = z[cIdx];
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    derivatives[idx]
                        += composition_sensitivity * overall_mole_fraction.derivative(idx);
                }
            }
            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                liquid_composition[compIdx].setDerivative(idx, derivatives[idx]);
            }
            // handling y
            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                derivatives[idx]
                    = -state_jacobian[compIdx + numComponents][0] * vapour_pressure.derivative(idx);
            }
            // derivatives from T (only for thermal case)
            if constexpr (isThermal) {
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    derivatives[idx] += -state_jacobian[compIdx + numComponents][1]
                        * input_temperature.derivative(idx);
                }
            }
            for (unsigned cIdx = 0; cIdx < numComponents; ++cIdx) {
                const double composition_sensitivity
                    = -state_jacobian[compIdx + numComponents][cIdx + composition_offset];
                const auto& overall_mole_fraction = z[cIdx];
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    derivatives[idx]
                        += composition_sensitivity * overall_mole_fraction.derivative(idx);
                }
            }
            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                vapour_composition[compIdx].setDerivative(idx, derivatives[idx]);
            }

            // handling derivatives of L
            std::vector<double> liquid_fraction_derivatives(num_derivatives, 0.);
            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                liquid_fraction_derivatives[idx]
                    = -state_jacobian[2 * numComponents][0] * vapour_pressure.derivative(idx);
            }
            // derivatives from T (only for thermal case)
            if constexpr (isThermal) {
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    liquid_fraction_derivatives[idx] += -state_jacobian[2 * numComponents][1]
                        * input_temperature.derivative(idx);
                }
            }
            for (unsigned cIdx = 0; cIdx < numComponents; ++cIdx) {
                const double composition_sensitivity
                    = -state_jacobian[2 * numComponents][cIdx + composition_offset];
                const auto& overall_mole_fraction = z[cIdx];
                for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                    liquid_fraction_derivatives[idx]
                        += composition_sensitivity * overall_mole_fraction.derivative(idx);
                }
            }

            for (unsigned idx = 0; idx < num_derivatives; ++idx) {
                liquid_fraction_with_derivatives.setDerivative(idx,
                                                               liquid_fraction_derivatives[idx]);
            }
        }

        // set up the mole fractions
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            fluid_state.setMoleFraction(
                FluidSystem::oilPhaseIdx, compIdx, liquid_composition[compIdx]);
            fluid_state.setMoleFraction(
                FluidSystem::gasPhaseIdx, compIdx, vapour_composition[compIdx]);
        }
        fluid_state.setLvalue(liquid_fraction_with_derivatives);
    } //end updateDerivativesTwoPhase

    /*!
     * \brief A single-phase result: both phases take the mixture composition,
     *        and the liquid fraction carries no derivatives.
     */
    template <typename FlashFluidStateScalar, typename FluidState>
    static void updateDerivativesSinglePhase_(const FlashFluidStateScalar& fluid_state_scalar,
                                              FluidState& fluid_state)
    {
        using InputEval = typename FluidState::ValueType;
        // Conversion from the scalar result gives the phase label zero derivatives.
        InputEval liquid_fraction = fluid_state_scalar.L();

        // A single phase uses the overall composition under both phase labels.
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, compIdx, fluid_state.moleFraction(compIdx) );
            fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, compIdx, fluid_state.moleFraction(compIdx) );
        }
        fluid_state.setLvalue(liquid_fraction);
    } //end updateDerivativesSinglePhase

    /*!
     * \brief Successive substitution for the two-phase compositions.
     *
     * Each step recomputes the compositions from the current ratios, then
     * updates the ratios by the fugacity ratio,
     *
     * \f[ K_i \leftarrow K_i \frac{f_i^{L}}{f_i^{V}}, \f]
     *
     * until \f$\| f^{L} / f^{V} - 1 \|\f$ is below tolerance. As a
     * conditioner for Newton it takes a few steps; on its own it may take
     * many.
     */
    template <class FlashFluidState, class ComponentVector>
    static bool successiveSubstitutionComposition_(ComponentVector& K,
                                                   typename ComponentVector::field_type& L,
                                                   FlashFluidState& fluid_state,
                                                   const ComponentVector& z,
                                                   const bool run_newton_afterwards,
                                                   const Scalar flash_tolerance,
                                                   const EOSType& eos_type,
                                                   const int verbosity)
    {
        // Limit conditioning passes before Newton; standalone substitution runs longer.
        const int max_iterations = run_newton_afterwards ? 5 : 100;

        // Print initial guess
        if (verbosity >= 1) {
            OpmLog::debug(fmt::format("Initial guess: K = [{}] and L = {}", fmt::join(K, " "), L));
        }

        if (verbosity == 2 || verbosity == 4) {
            // Print header
            const int fugacity_width = (numComponents * 12) / 2;
            const int convergence_width = fugacity_width + 7;
            OpmLog::debug(fmt::format("{:>10}{:>{}}{:>{}}",
                                      "Iteration",
                                      "fL/fV",
                                      fugacity_width,
                                      "norm2(fL/fv-1)",
                                      convergence_width));
        }
        //
        // Successive substitution loop
        //
        for (int iteration = 0; iteration < max_iterations; ++iteration) {
            // Compute (normalized) liquid and vapor mole fractions
            computeLiquidVapor_(fluid_state, L, K, z);

            // Calculate fugacity coefficient
            using ParamCache = typename FluidSystem::template ParameterCache<typename FlashFluidState::ValueType>;
            ParamCache paramCache(eos_type);
            for (int phaseIdx=0; phaseIdx<numMisciblePhases; ++phaseIdx){
                paramCache.updatePhase(fluid_state, phaseIdx);
                for (int compIdx=0; compIdx<numComponents; ++compIdx){
                    const auto fugacity_coefficient = FluidSystem::fugacityCoefficient(
                        fluid_state, paramCache, phaseIdx, compIdx);
                    fluid_state.setFugacityCoefficient(phaseIdx, compIdx, fugacity_coefficient);
                }
            }

            // Calculate fugacity ratio
            ComponentVector fugacity_ratio;
            ComponentVector fugacity_residual;
            for (int compIdx=0; compIdx<numComponents; ++compIdx){
                fugacity_ratio[compIdx] = fluid_state.fugacity(oilPhaseIdx, compIdx)
                    / fluid_state.fugacity(gasPhaseIdx, compIdx);
                fugacity_residual[compIdx] = fugacity_ratio[compIdx] - 1.0;
            }

            // Print iteration info
            if (verbosity >= 2) {
                constexpr int precision = 5;
                constexpr int fugacity_width = precision + 3;
                constexpr int convergence_width = precision + 9;
                OpmLog::debug(fmt::format("{:>5}{:>{}.{}f}{:>{}.{}e}",
                                          iteration,
                                          fmt::join(fugacity_ratio, " "),
                                          fugacity_width,
                                          precision,
                                          fugacity_residual.two_norm(),
                                          convergence_width,
                                          precision));
            }

            // Check convergence
            if (fugacity_residual.two_norm() < flash_tolerance) {
                // Print info
                if (verbosity >= 1) {
                    std::vector<typename ComponentVector::field_type> liquid_composition(
                        numComponents),
                        vapour_composition(numComponents);
                    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                        liquid_composition[compIdx]
                            = fluid_state.moleFraction(oilPhaseIdx, compIdx);
                        vapour_composition[compIdx]
                            = fluid_state.moleFraction(gasPhaseIdx, compIdx);
                    }
                    OpmLog::debug(fmt::format("Solution converged to the following result :\n"
                                              "x = [{}]\n"
                                              "y = [{}]\n"
                                              "K = [{}]\n"
                                              "L = {}",
                                              fmt::join(liquid_composition, " "),
                                              fmt::join(vapour_composition, " "),
                                              fmt::join(K, " "),
                                              L));
                }
                return true;
            }
            //  If convergence is not met, K is updated in a successive substitution manner
            else {
                // Update K
                for (int compIdx=0; compIdx<numComponents; ++compIdx){
                    K[compIdx] *= fugacity_ratio[compIdx];
                }

                // Solve Rachford-Rice to get L from updated K
                L = solveRachfordRice_g_(K, z, 0);
            }
        }
        // did not get converged. check whether we will do more newton later afterward
        {
            const std::string message
                = fmt::format("Successive substitution composition update did not "
                              "converge within {} iterations.",
                              max_iterations);
            if (!run_newton_afterwards) {
                OPM_THROW(std::runtime_error, message);
            } else if (verbosity > 0) {
                OpmLog::debug(message);
            }
        }

        return false;
    }

};//end PTFlash

} // namespace Opm

#endif
