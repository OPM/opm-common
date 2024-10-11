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
#include <config.h>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/components/CO2Tables.hpp>
#include "co2tables.inc"
#include <vector>

namespace Opm
{

CO2Tables::CO2Tables()
{
    // Opm::TabulatedEnthalpyTraits tabulatedEnthalpyStruct;
    tabulatedEnthalpy = Opm::UniformTabulated2DFunction<double> {co2TabulatedEnthalpyTraits::xMin,
                                                                 co2TabulatedEnthalpyTraits::xMax,
                                                                 (unsigned int)co2TabulatedEnthalpyTraits::numX,
                                                                 co2TabulatedEnthalpyTraits::yMin,
                                                                 co2TabulatedEnthalpyTraits::yMax,
                                                                 (unsigned int)co2TabulatedEnthalpyTraits::numY,
                                                                 co2TabulatedEnthalpyTraits::vals};
    // TabulatedDensityTraits tabulatedDensityStruct;
    tabulatedDensity = UniformTabulated2DFunction<double> {co2TabulatedDensityTraits::xMin,
                                                                co2TabulatedDensityTraits::xMax,
                                                                (unsigned int)co2TabulatedDensityTraits::numX,
                                                                co2TabulatedDensityTraits::yMin,
                                                                co2TabulatedDensityTraits::yMax,
                                                                (unsigned int)co2TabulatedDensityTraits::numY,
                                                                co2TabulatedDensityTraits::vals};
}

} // namespace Opm
