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
 * \copydoc Opm::C10
 */
#ifndef OPM_C10_HPP
#define OPM_C10_HPP

#include "Component.hpp"

#include <opm/material/IdealGas.hpp>
#include <opm/material/common/MathToolbox.hpp>

#include <cmath>

namespace Opm
{

/*!
 * \ingroup Components
 *
 * \brief Properties of pure molecular n-Decane \f$C_10\f$.
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class C10 : public Component<Scalar, C10<Scalar> >
{
    typedef ::Opm::IdealGas<Scalar> IdealGas;

public:
    /*!
     * \brief A human readable name for NDecane.
     */
    static const char* name()
    { return "C10"; }

    /*!
     * \brief The molar mass in \f$\mathrm{[kg/mol]}\f$ of molecular n-Decane.
     */
    static Scalar molarMass()
    { return 0.0142;}

    /*!
     * \brief Returns the critical temperature \f$\mathrm{[K]}\f$ of molecular n-Decane.
     */
    static Scalar criticalTemperature()
    { return 617.7; /* [K] */ }

    /*!
     * \brief Returns the critical pressure \f$\mathrm{[Pa]}\f$ of molecular n-Decane.
     */
    static Scalar criticalPressure()
    { return 2.10e6; /* [N/m^2] */ }

    /*!
     * \brief Critical volume of \f$C_10\f$ [m3/kmol].
     */
    static Scalar criticalVolume() {return 6.098e-1; }

    /*!
     * \brief Acentric factor of \f$C_10\f$.
     */
    static Scalar acentricFactor() { return 0.488; }


};

} // namespace Opm

#endif
