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

#ifndef OPM_CO2PARAMETERS_HPP
#define OPM_CO2PARAMETERS_HPP
#include <config.h>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/components/helperStructs.hpp>
#include <vector>


namespace Opm {

class CO2Tables {
public:
	Opm::UniformTabulated2DFunction< double >   tabulatedEnthalpy;
	Opm::UniformTabulated2DFunction< double >   tabulatedDensity;
	static constexpr double brineSalinity = 1.000000000000000e-01;

	CO2Tables();
	// 	 : tabulatedEnthalpy{tabulatedEnthalpyStruct.xMin,
	// 								tabulatedEnthalpyStruct.xMax,
	// 								tabulatedEnthalpyStruct.numX,
	// 								tabulatedEnthalpyStruct.yMin,
	// 								tabulatedEnthalpyStruct.yMax,
	// 								tabulatedEnthalpyStruct.numY,
	// 								tabulatedEnthalpyStruct.vals},
	// 		tabulatedDensity{tabulatedDensityStruct.xMin,
	// 							  tabulatedDensityStruct.xMax,
	// 							  tabulatedDensityStruct.numX,
	// 							  tabulatedDensityStruct.yMin,
	// 							  tabulatedDensityStruct.yMax,
	// 							  tabulatedDensityStruct.numY,
	// 							  tabulatedDensityStruct.vals}
	// {}
};


// class CO2Parameters::CO2Tables {
// public:
//     // Define the members and methods as needed
//     CO2Tables() {
//         // Initialize the members
//         // Example:
//         // numX = static_cast<unsigned int>(tabulatedEnthalpyStruct.TabulatedEnthalpyTraits::numX);
//         // numY = static_cast<unsigned int>(tabulatedEnthalpyStruct.TabulatedEnthalpyTraits::numY);
//     }
// }

class CO2Parameters
{
public:
    CO2Parameters(){
      co2Tables_ = CO2Tables();
    }
    CO2Tables co2Tables_;
};

} // namespace Opm

#endif // OPM_CO2PARAMETERS_HPP
