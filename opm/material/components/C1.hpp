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

#include <opm/material/IdealGas.hpp>
#include <opm/material/common/MathToolbox.hpp>

#include <cmath>

namespace Opm
{

/*!
 * \ingroup Components
 *
 * \brief Properties of pure molecular methane \f$C_1\f$.
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class C1 : public Component<Scalar, C1<Scalar> >
{
    typedef ::Opm::IdealGas<Scalar> IdealGas;

public:
    /*!
     * \brief A human readable name for NDecane.
     */
    static const char* name()
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




};

} // namespace Opm

#endif
