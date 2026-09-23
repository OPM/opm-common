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
#include <opm/common/OpmLog/OpmLog.hpp>

#include <dune/common/math.hh>

#include <fmt/format.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace Opm {

/*!
 * \brief The Rachford-Rice equation and the solvers for it.
 *
 * The phase split follows from the equilibrium ratios and the overall
 * composition alone, so nothing here needs a fluid system: the number of
 * components is the length of the vectors handed in.
 */
class RachfordRice
{
public:
    /*!
     * \brief The liquid fraction the equilibrium ratios and composition imply.
     *
     * Newton-Raphson, falling back to bisection when a step leaves the bracket
     * the extreme equilibrium ratios define.
     */
    template <class Vector>
    static typename Vector::field_type solve(const Vector& K, const Vector& z, int verbosity)
    {
        using field_type = typename Vector::field_type;
        const auto [Vmin, Vmax] = vapourFractionBracket(K);

        if (const auto L = newton(K, z, Vmin, Vmax, verbosity)) {
            return reportSolution(*L, verbosity);
        }

        return reportSolution(bisection(K, field_type {1}, field_type {0}, z, verbosity),
                              verbosity,
                              " (Bisection)");
    }

private:
    //! \brief The vapour fraction bracket defined by the extreme ratios.
    template <class Vector>
    static std::pair<typename Vector::field_type, typename Vector::field_type>
    vapourFractionBracket(const Vector& K)
    {
        // Find min and max K. Have to do a laborious for loop to avoid water component (where K=0)
        // TODO: Replace loop with Dune::min_value() and Dune::max_value() when water component is properly handled
        using field_type = typename Vector::field_type;
        field_type Kmin = K[0];
        field_type Kmax = K[0];
        for (std::size_t compIdx = 1; compIdx < K.size(); ++compIdx) {
            if (K[compIdx] < Kmin) {
                Kmin = K[compIdx];
            } else if (K[compIdx] >= Kmax) {
                Kmax = K[compIdx];
            }
        }
        return {1 / (1 - Kmax), 1 / (1 - Kmin)};
    }

    /*!
     * \brief Newton on the residual, inside the bracket the extreme ratios imply.
     *
     * Returns nothing when an iterate leaves the bracket and bisection must take over.
     */
    template <class Vector>
    static std::optional<typename Vector::field_type> newton(
        const Vector& K,
        const Vector& z,
        typename Vector::field_type Vmin,
        typename Vector::field_type Vmax,
        int verbosity)
    {
        using field_type = typename Vector::field_type;
        constexpr field_type tol = 1e-12;
        constexpr int itmax = 10000;

        auto V = (Vmin + Vmax) / 2;
        if (verbosity == 3 || verbosity == 4) {
            OpmLog::debug(fmt::format("Initial guess {}c : V = {} and [Vmin, Vmax] = [{}, {}]",
                                      K.size(), V, Vmin, Vmax));
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "abs(step)", "V"));
        }

        for (int iteration = 1; iteration < itmax; ++iteration) {
            field_type denum = 0.0;
            field_type r = 0.0;
            for (std::size_t compIdx = 0; compIdx < K.size(); ++compIdx) {
                auto dK = K[compIdx] - 1.0;
                auto a = z[compIdx] * dK;
                auto b = (1 + V * dK);
                r += a / b;
                denum += z[compIdx] * (dK * dK) / (b * b);
            }
            auto delta = r / denum;
            V += delta;

            if (V < Vmin || V > Vmax) {
                if (verbosity == 3 || verbosity == 4) {
                    OpmLog::debug(fmt::format("V = {} is not within the range [Vmin, Vmax], "
                                              "solve using Bisection method!", V));
                }
                return std::nullopt;
            }

            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", iteration, std::abs(delta), V));
            }

            if (std::abs(r) < tol) {
                return 1 - V;
            }
        }

        OPM_THROW(std::runtime_error,
                  " Rachford-Rice did not converge within maximum number of iterations");
    }

    //! \brief The Rachford-Rice residual at a given liquid fraction.
    template <class Vector>
    static typename Vector::field_type g(const Vector& K,
                                         typename Vector::field_type L,
                                         const Vector& z)
    {
        typename Vector::field_type value = 0;
        for (std::size_t compIdx = 0; compIdx < K.size(); ++compIdx) {
            value += (z[compIdx] * (K[compIdx] - 1))
                   / (K[compIdx] - L * (K[compIdx] - 1));
        }
        return value;
    }

    //! \brief Bisection on the residual, for when Newton leaves the bracket.
    template <class Vector>
    static typename Vector::field_type bisection(const Vector& K,
                                                 typename Vector::field_type Lmin,
                                                 typename Vector::field_type Lmax,
                                                 const Vector& z,
                                                 int verbosity)
    {
        typename Vector::field_type gLmin = g(K, Lmin, z);

        if (verbosity >= 3) {
            OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", "Iteration", "g(Lmid)", "L"));
        }

        constexpr int max_it = 10000;
        auto closeLmaxLmin = [](double max_v, double min_v) {
            return std::abs(max_v - min_v) / 2. < 1e-10;
        };

        if (closeLmaxLmin(Lmax, Lmin)) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Strange bisection with Lmax {} and Lmin {}?", Lmax, Lmin));
        }
        for (int iteration = 0; iteration < max_it; ++iteration) {
            auto L = (Lmin + Lmax) / 2;
            auto gMid = g(K, L, z);
            if (verbosity == 3 || verbosity == 4) {
                OpmLog::debug(fmt::format("{:>10}{:>16}{:>16}", iteration, gMid, L));
            }

            if (std::abs(gMid) < 1e-16 || closeLmaxLmin(Lmax, Lmin)) {
                return L;
            } else if (Dune::sign(gMid) != Dune::sign(gLmin)) {
                Lmax = L;
            } else {
                Lmin = L;
                gLmin = gMid;
            }
        }
        OPM_THROW(std::runtime_error,
                  fmt::format(" Rachford-Rice bisection failed with {} iterations!", max_it));
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
