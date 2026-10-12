// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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
 * \copydoc Opm::C1
 */
#ifndef OPM_C1_HPP
#define OPM_C1_HPP

#include "Component.hpp"
#include "ComponentCp.hpp"

#include <opm/material/IdealGas.hpp>
#include <opm/material/common/MathToolbox.hpp>

#include <cmath>
#include <string_view>


namespace Opm
{

/*!
 * \ingroup Components
 *
 * \brief Properties of pure molecular methane \f$C_1\f$
 *        (CAS 74-82-8, formula CH\f$_4\f$).
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class C1 : public Component<Scalar, C1<Scalar> >
{
    typedef ::Opm::IdealGas<Scalar> IdealGas;

public:
    /*!
     * \brief A human readable name for methane.
     */
    static std::string_view name()
    { return "C1"; }

    /*!
     * \brief The molar mass in \f$\mathrm{[kg/mol]}\f$ of molecular methane.
     */
    static Scalar molarMass()
    { return 0.0160;}

    /*!
     * \brief Returns the critical temperature \f$\mathrm{[K]}\f$ of molecular methane
     */
    static Scalar criticalTemperature()
    { return 190.6; /* [K] */ }

    /*!
     * \brief Returns the critical pressure \f$\mathrm{[Pa]}\f$ of molecular methane.
     */
    static Scalar criticalPressure()
    { return 4.60e6; /* [N/m^2] */ }

    /*!
     * \brief Critical volume of \f$C_1\f$ [m3/kmol].
     */
    static Scalar criticalVolume() {return 9.863e-2; }

    /*!
     * \brief Acentric factor of \f$C_1\f$.
     */
    static Scalar acentricFactor() { return 0.011; }

    /*!
     * \brief Cubic ideal-gas heat-capacity polynomial of methane \f$\mathrm{[J/(mol\ K)]}\f$.
     *
     * Least-squares fit to the ideal-gas part of the equation of state of
     * Setzmann & Wagner, J. Phys. Chem. Ref. Data 20 (1991) 1061, over
     * 250-600 K (RMS 0.024, max 0.063 J/(mol K)), sampled with CoolProp
     * 8.0.0. Not valid outside that range.
     *
     * This is the ideal-gas heat capacity used by the compositional mixture
     * enthalpy (MixtureEnthalpy). It does not implement the Component<>
     * gasEnthalpy/gasHeatCapacity functions, which are real-fluid
     * correlations where they exist.
     */
    static constexpr ComponentCp<Scalar> idealGasHeatCapacityPolynomial()
    { return {40.1503, -8.47372e-2, 2.93012e-4, -1.96125e-7, 250.0, 600.0}; }
};

} // namespace Opm

#endif
