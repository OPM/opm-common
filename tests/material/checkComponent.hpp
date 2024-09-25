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

#include <opm/common/utility/DemangledType.hpp>

#include <iostream>
#include <tuple>

/*!
 * \brief Ensures that a class which represents a chemical components adheres to the
 *        components API.
 *
 * Note that this does *not* imply that the methods are implemented or even make sense...
 */
template <class Component, class Evaluation>
void checkComponent()
{
    std::cout << "Testing component '"
              << Opm::getDemangledType<Component>() << "', Evaluation='"
              << Opm::getDemangledType<Evaluation>() << "'\n";

    // make sure the necessary typedefs exist
    using Scalar = typename Component::Scalar;
    [[maybe_unused]] Scalar a;

    // make sure the necessary constants are exported
    std::ignore = Component::isTabulated;

    // test for the gas-phase functions
    const Evaluation T = 0, p = 0;
    while (0) {
        { std::ignore = Component::gasIsCompressible(); }
        { std::ignore = Component::gasIsIdeal(); }
        { std::ignore = Component::liquidIsCompressible(); }
        { std::ignore = Component::name(); }
        { std::ignore = Component::molarMass(); }
        { std::ignore = Component::criticalTemperature(); }
        { std::ignore = Component::criticalPressure(); }
        { std::ignore = Component::criticalVolume(); }
        { std::ignore = Component::tripleTemperature(); }
        { std::ignore = Component::triplePressure(); }
        { std::ignore = Component::acentricFactor(); }
        { std::ignore = Component::vaporPressure(T); }
        { std::ignore = Component::gasDensity(T, p); }
        { std::ignore = Component::liquidDensity(T, p); }
        { std::ignore = Component::gasEnthalpy(T, p); }
        { std::ignore = Component::liquidEnthalpy(T, p); }
        { std::ignore = Component::gasInternalEnergy(T, p); }
        { std::ignore = Component::liquidInternalEnergy(T, p); }
        { std::ignore = Component::gasViscosity(T, p); }
        { std::ignore = Component::liquidViscosity(T, p); }
        { std::ignore = Component::gasThermalConductivity(T, p); }
        { std::ignore = Component::liquidThermalConductivity(T, p); }
        { std::ignore = Component::gasHeatCapacity(T, p); }
        { std::ignore = Component::liquidHeatCapacity(T, p); }
    }
    std::cout << "----------------------------------\n";
}

#endif
