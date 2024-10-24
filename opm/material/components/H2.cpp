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
#include <opm/material/components/H2.hpp>
#if HAVE_QUAD
#include <opm/material/common/quad.hpp>
#endif
#include "h2tables.inc"

namespace Opm {
typedef Opm::UniformTabulated2DFunction< double > TabulatedFunction;

inline const TabulatedFunction H2Tables::tabulatedEnthalpy
    {H2TabulatedEnthalpyTraits::xMin,
     H2TabulatedEnthalpyTraits::xMax,
     H2TabulatedEnthalpyTraits::numX,
     H2TabulatedEnthalpyTraits::yMin,
     H2TabulatedEnthalpyTraits::yMax,
     H2TabulatedEnthalpyTraits::numY,
     H2TabulatedEnthalpyTraits::vals};

inline const TabulatedFunction H2Tables::tabulatedDensity
    {H2TabulatedDensityTraits::xMin,
     H2TabulatedDensityTraits::xMax,
     H2TabulatedDensityTraits::numX,
     H2TabulatedDensityTraits::yMin,
     H2TabulatedDensityTraits::yMax,
     H2TabulatedDensityTraits::numY,
     H2TabulatedDensityTraits::vals};

template<>
const UniformTabulated2DFunction<double>&
H2<double>::tabulatedEnthalpy = H2Tables::tabulatedEnthalpy;
template<>
const UniformTabulated2DFunction<double>&
H2<double>::tabulatedDensity = H2Tables::tabulatedDensity;
template<>
const double H2<double>::brineSalinity = H2Tables::brineSalinity;

template<>
const UniformTabulated2DFunction<double>&
H2<float>::tabulatedEnthalpy = H2Tables::tabulatedEnthalpy;
template<>
const UniformTabulated2DFunction<double>&
H2<float>::tabulatedDensity = H2Tables::tabulatedDensity;
template<>
const float H2<float>::brineSalinity = H2Tables::brineSalinity;

#if HAVE_QUAD
template<>
const UniformTabulated2DFunction<double>&
H2<quad>::tabulatedEnthalpy = H2Tables::tabulatedEnthalpy;
template<>
const UniformTabulated2DFunction<double>&
H2<quad>::tabulatedDensity = H2Tables::tabulatedDensity;
template<>
const quad H2<quad>::brineSalinity = H2Tables::brineSalinity;
#endif

} // namespace Opm
