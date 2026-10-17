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
 * \copydoc Opm::RachfordRice
 */
#ifndef OPM_RACHFORD_RICE_HPP
#define OPM_RACHFORD_RICE_HPP

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/Exceptions.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Opm
{

/*!
 * \brief The Rachford-Rice equation and the solvers for it.
 *
 * The vapour fraction \f$V = 1 - L\f$ solves
 *
 * \f[ g(V) = \sum_i \hat z_i \frac{K_i - 1}{1 + V (K_i - 1)} = 0, \f]
 *
 * with \f$\hat z\f$ the overall composition normalised over the components
 * that are present. The phase split follows from the equilibrium ratios and
 * the overall composition alone, so nothing here needs a fluid system: the
 * number of components is the length of the vectors handed in.
 */
class RachfordRice
{
    // Preserve the small residual differences near unit ratios when the
    // input vectors use float, without rounding their composition again.
    template <class Vector>
    using WorkingScalar = std::common_type_t<typename Vector::field_type, double>;

public:
    /*!
     * \brief The liquid fraction the equilibrium ratios and composition imply.
     *
     * Single-phase states return an endpoint. Two-phase states use Newton,
     * with bisection fallback when Newton leaves the bracket or stalls.
     *
     * \throws std::invalid_argument when the vectors are empty or differ in length.
     * \throws NumericalProblem for a non-finite or negative value, a non-positive
     *         composition total, or all ratios one. Callers recover from these,
     *         so they are not logged.
     */
    template <class Vector>
    static typename Vector::field_type solve(const Vector& K, const Vector& z, int verbosity = 0)
    {
        using field_type = typename Vector::field_type;

        const auto compositionTotal = validateInputs(K, z);

        if (const auto endpoint = singlePhaseEndpoint(K, z, compositionTotal)) {
            return reportSolution(static_cast<field_type>(*endpoint), verbosity, " (endpoint)");
        }

        const auto [Vmin, Vmax] = vapourFractionBracket(K, z);

        if (const auto L = newton(K, z, compositionTotal, Vmin, Vmax, verbosity)) {
            return reportSolution(static_cast<field_type>(*L), verbosity);
        }

        return reportSolution(static_cast<field_type>(bisection(K, z, compositionTotal, verbosity)),
                              verbosity,
                              " (Bisection)");
    }

private:
    /*!
     * \brief Reject input that has no determinate phase split.
     *
     * \return The sum of the overall mole fractions.
     */
    template <class Vector>
    static WorkingScalar<Vector> validateInputs(const Vector& K, const Vector& z)
    {
        if (K.size() == 0) {
            OPM_THROW(std::invalid_argument, "Rachford-Rice requires at least one component");
        }

        if (K.size() != z.size()) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("Rachford-Rice received {} equilibrium ratios and {} "
                                  "overall mole fractions",
                                  K.size(),
                                  z.size()));
        }

        bool allRatiosAreOne = true;
        WorkingScalar<Vector> compositionTotal = 0;
        for (std::size_t compIdx = 0; compIdx < K.size(); ++compIdx) {
            if (!std::isfinite(z[compIdx]) || z[compIdx] < 0) {
                OPM_THROW_NOLOG(NumericalProblem,
                                fmt::format("Rachford-Rice received an invalid mole fraction at "
                                            "component {}",
                                            compIdx));
            }
            compositionTotal += z[compIdx];
            if (z[compIdx] == 0) {
                continue;
            }
            if (!std::isfinite(K[compIdx]) || K[compIdx] < 0) {
                OPM_THROW_NOLOG(
                    NumericalProblem,
                    fmt::format("Rachford-Rice received an invalid equilibrium ratio at "
                                "component {}",
                                compIdx));
            }
            allRatiosAreOne = allRatiosAreOne && (K[compIdx] == 1);
        }

        if (!std::isfinite(compositionTotal) || compositionTotal <= 0) {
            OPM_THROW_NOLOG(NumericalProblem,
                            "Rachford-Rice requires a finite, positive composition total");
        }

        // The residual is identically zero when every ratio is one.
        if (allRatiosAreOne) {
            OPM_THROW_NOLOG(NumericalProblem,
                            "Rachford-Rice cannot determine the phase split when all equilibrium "
                            "ratios are one");
        }

        return compositionTotal;
    }

    //! \brief The residual \f$g(V)\f$ and its negative slope \f$-g'(V)\f$.
    template <class Vector>
    static std::pair<WorkingScalar<Vector>, WorkingScalar<Vector>>
    residual(const Vector& K,
             WorkingScalar<Vector> V,
             const Vector& z,
             WorkingScalar<Vector> compositionTotal)
    {
        using field_type = WorkingScalar<Vector>;
        field_type value = 0;
        field_type negativeSlope = 0;
        for (std::size_t compIdx = 0; compIdx < K.size(); ++compIdx) {
            if (z[compIdx] == 0) {
                continue;
            }
            const field_type normalizedZ = z[compIdx] / compositionTotal;
            const field_type dK = static_cast<field_type>(K[compIdx]) - 1;
            // Form the ratio before squaring it, so large ratios cannot overflow.
            const field_type dKOverB = dK / (1 + V * dK);
            value += normalizedZ * dKOverB;
            negativeSlope += normalizedZ * dKOverB * dKOverB;
        }
        return {value, negativeSlope};
    }

    /*!
     * \brief The endpoint of a state that has no interior root.
     *
     * A two-phase root exists only when g(V = 0) > 0 > g(V = 1). Otherwise
     * the physical solution is a single-phase endpoint.
     *
     * \return The endpoint, or nothing when the state is two-phase.
     */
    template <class Vector>
    static std::optional<WorkingScalar<Vector>>
    singlePhaseEndpoint(const Vector& K, const Vector& z, WorkingScalar<Vector> compositionTotal)
    {
        using field_type = WorkingScalar<Vector>;
        if (residual(K, field_type {0}, z, compositionTotal).first <= 0) {
            return field_type {1};
        }
        if (residual(K, field_type {1}, z, compositionTotal).first >= 0) {
            return field_type {0};
        }
        return std::nullopt;
    }

    //! \brief The vapour fraction bracket the extreme equilibrium ratios imply.
    //!
    //! Only the components that are present take part.
    template <class Vector>
    static std::pair<WorkingScalar<Vector>, WorkingScalar<Vector>>
    vapourFractionBracket(const Vector& K, const Vector& z)
    {
        using field_type = WorkingScalar<Vector>;
        field_type Kmin = std::numeric_limits<field_type>::max();
        field_type Kmax = std::numeric_limits<field_type>::lowest();
        for (std::size_t compIdx = 0; compIdx < K.size(); ++compIdx) {
            if (z[compIdx] == 0) {
                continue;
            }
            if (K[compIdx] < Kmin) {
                Kmin = K[compIdx];
            }
            if (K[compIdx] > Kmax) {
                Kmax = K[compIdx];
            }
        }

        return {1 / (1 - Kmax), 1 / (1 - Kmin)};
    }

    /*!
     * \brief Newton on the residual, inside the bracket the extreme ratios imply.
     *
     * \return The liquid fraction, or nothing when an iterate leaves the
     *         bracket or the iteration does not converge.
     */
    template <class Vector>
    static std::optional<WorkingScalar<Vector>> newton(const Vector& K,
                                                       const Vector& z,
                                                       WorkingScalar<Vector> compositionTotal,
                                                       WorkingScalar<Vector> Vmin,
                                                       WorkingScalar<Vector> Vmax,
                                                       int verbosity)
    {
        using field_type = WorkingScalar<Vector>;
        // A converging Newton needs a few dozen steps at most. Near unit ratios
        // round-off keeps the step above tolerance, so hand over to bisection early.
        constexpr int itmax = 100;
        const field_type precision
            = 10 * std::numeric_limits<typename Vector::field_type>::epsilon();
        const field_type tol = std::max(static_cast<field_type>(1e-12), precision);

        auto V = (Vmin + Vmax) / 2;
        if (verbosity == 3 || verbosity == 4) {
            OpmLog::debug(fmt::format(
                "Initial guess {}c : V = {} and [Vmin, Vmax] = [{}, {}]", K.size(), V, Vmin, Vmax));
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "abs(step)", "V"));
        }

        for (int iteration = 1; iteration < itmax; ++iteration) {
            const auto [r, negativeSlope] = residual(K, V, z, compositionTotal);
            const auto delta = r / negativeSlope;
            V += delta;

            if (!(V >= Vmin && V <= Vmax)) {
                if (verbosity == 3 || verbosity == 4) {
                    OpmLog::debug(fmt::format("V = {} is not within the range [Vmin, Vmax], solve "
                                              "using Bisection method!",
                                              V));
                }
                return std::nullopt;
            }

            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", iteration, std::abs(delta), V));
            }

            // Check for convergence. The residual alone does not bound the
            // error in V when the ratios approach one and g flattens, so the
            // step has to be small as well.
            if (std::abs(r) < tol && std::abs(delta) <= tol * (1 + std::abs(V))) {
                const auto L = 1 - V;
                // solve() established a root in (0, 1). Recover with bisection
                // if round-off takes the converged iterate outside that interval.
                if (L < 0 || L > 1) {
                    return std::nullopt;
                }
                return L;
            }
        }

        return std::nullopt;
    }

    /*!
     * \brief Bisection in the liquid fraction on [0, 1].
     *
     * solve() calls it only once g(L = 0) < 0 < g(L = 1), and g increases with
     * L, so the root stays bracketed. Every step halves the interval exactly,
     * so the width test ends the loop.
     */
    template <class Vector>
    static WorkingScalar<Vector> bisection(const Vector& K,
                                           const Vector& z,
                                           WorkingScalar<Vector> compositionTotal,
                                           int verbosity)
    {
        using field_type = WorkingScalar<Vector>;
        const field_type precision
            = 10 * std::numeric_limits<typename Vector::field_type>::epsilon();
        const field_type intervalTolerance = std::max(static_cast<field_type>(1e-10), precision);

        if (verbosity >= 3) {
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "g(Lmid)", "L"));
        }

        field_type Lmin = 0;
        field_type Lmax = 1;
        for (int iteration = 0;; ++iteration) {
            const field_type L = (Lmin + Lmax) / 2;
            const field_type gMid = residual(K, 1 - L, z, compositionTotal).first;
            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", iteration, gMid, L));
            }

            // A small residual alone gives no accuracy guarantee near unit
            // ratios. Stop on an exact root or the phase-fraction interval.
            if (gMid == 0 || (Lmax - Lmin) / 2 <= intervalTolerance) {
                return L;
            }
            if (gMid < 0) {
                Lmin = L;
            } else {
                Lmax = L;
            }
        }
    }

    //! \brief Log the converged liquid fraction and hand it back.
    template <class FieldType>
    static FieldType reportSolution(FieldType L, int verbosity, std::string_view method = {})
    {
        if (verbosity >= 1) {
            OpmLog::debug(
                fmt::format("Rachford-Rice{} converged to final solution L = {}", method, L));
        }
        return L;
    }
};

} // namespace Opm

#endif // OPM_RACHFORD_RICE_HPP
