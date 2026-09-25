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
 * \brief The cubic ideal-gas heat-capacity polynomial and its closed-form
 *        enthalpy integral — the caloric EQUATIONS, species-blind.
 *
 * The coefficients (a species' caloric identity) live on the component
 * classes (e.g. C1::idealGasHeatCapacityPolynomial()); the name-keyed lookup
 * and the enthalpy reference datum live in IdealGasCaloricData. This header
 * holds only the mathematics shared by all of them.
 *
 * Units are SI throughout: temperature [K], molar heat capacity [J/(mol K)],
 * molar enthalpy [J/mol].
 */
#ifndef OPM_COMPONENT_CP_HPP
#define OPM_COMPONENT_CP_HPP

#include <array>

namespace Opm {

/*!
 * \brief Cubic ideal-gas heat-capacity polynomial of one component:
 *        cp(T) = c0 + c1*T + c2*T^2 + c3*T^3   [J/(mol K)]
 */
template <class Scalar>
struct ComponentCp {
    Scalar c0, c1, c2, c3;

    //! cp(T) [J/(mol K)]. Generic in the evaluation type (double or AD).
    template <class Eval>
    Eval heatCapacity(const Eval& T) const
    {
        return c0 + c1*T + c2*T*T + c3*T*T*T;
    }

    /*!
     * \brief Ideal-gas enthalpy h(T) = int_{T0}^{T} cp dT' [J/mol],
     *        in closed form. h(T0) = 0 by construction.
     */
    template <class Eval>
    Eval enthalpyIntegral(const Eval& T, const Scalar T0) const
    {
        return c0*(T - T0)
             + c1/2*(T*T - T0*T0)
             + c2/3*(T*T*T - T0*T0*T0)
             + c3/4*(T*T*T*T - T0*T0*T0*T0);
    }
};

//! Per-component cp table for an N-component fluid system, indexed like the
//! fluid system's component indices.
template <class Scalar, int numComponents>
using CpTable = std::array<ComponentCp<Scalar>, numComponents>;

} // namespace Opm

#endif // OPM_COMPONENT_CP_HPP
