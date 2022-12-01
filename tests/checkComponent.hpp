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
 * \copydoc checkComponent
 */
#ifndef OPM_CHECK_COMPONENT_HPP
#define OPM_CHECK_COMPONENT_HPP

#include <dune/common/classname.hh>

#include <iostream>
#include <string>

/*!
 * \brief Ensures that a class which represents a chemical components adheres to the
 *        components API.
 *
 * Note that this does *not* imply that the methods are implemented or even make sense...
 */
template <class Component, class Evaluation>
void checkComponent()
{
    std::cout << "Testing component '" << Dune::className<Component>() << "'\n";

    // make sure the necessary typedefs exist
    typedef typename Component::Scalar Scalar;

    // make sure the necessary constants are exported
    [[maybe_unused]] bool isTabulated = Component::isTabulated;

    // test for the gas-phase functions
    Evaluation T=0, p=0;
    while (0) {
        { [[maybe_unused]] bool b = Component::gasIsCompressible(); }
        { [[maybe_unused]] bool b = Component::gasIsIdeal(); }
        { [[maybe_unused]] bool b = Component::liquidIsCompressible(); }
        { [[maybe_unused]] std::string s = Component::name(); }
        { [[maybe_unused]] Scalar M = Component::molarMass(); }
        { [[maybe_unused]] Scalar Tc = Component::criticalTemperature(); }
        { [[maybe_unused]] Scalar pc = Component::criticalPressure(); }
        { [[maybe_unused]] Scalar Vc = Component::criticalVolume(); }
        { [[maybe_unused]] Scalar Tt = Component::tripleTemperature(); }
        { [[maybe_unused]] Scalar pt = Component::triplePressure(); }
        { [[maybe_unused]] Evaluation omega  = Component::acentricFactor(); }
        { [[maybe_unused]] Evaluation pv = Component::vaporPressure(T); }
        { [[maybe_unused]] Evaluation rho = Component::gasDensity(T, p); }
        { [[maybe_unused]] Evaluation rho = Component::liquidDensity(T, p); }
        { [[maybe_unused]] Evaluation h = Component::gasEnthalpy(T, p); }
        { [[maybe_unused]] Evaluation h = Component::liquidEnthalpy(T, p); }
        { [[maybe_unused]] Evaluation u = Component::gasInternalEnergy(T, p); }
        { [[maybe_unused]] Evaluation u = Component::liquidInternalEnergy(T, p); }
        { [[maybe_unused]] Evaluation mu = Component::gasViscosity(T, p); }
        { [[maybe_unused]] Evaluation mu = Component::liquidViscosity(T, p); }
        { [[maybe_unused]] Evaluation lambda = Component::gasThermalConductivity(T, p); }
        { [[maybe_unused]] Evaluation lambda = Component::liquidThermalConductivity(T, p); }
        { [[maybe_unused]] Evaluation cp = Component::gasHeatCapacity(T, p); }
        { [[maybe_unused]] Evaluation cp = Component::liquidHeatCapacity(T, p); }
    }
    std::cout << "----------------------------------\n";
}

#endif
