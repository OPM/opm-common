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

#ifndef OPM_CO2TABLES_HPP
#define OPM_CO2TABLES_HPP

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>

namespace Opm {

struct co2TabulatedDensityTraits
{
    using Scalar = double;
    static const char  *name;
    static const int    numX = 200;
    static const Scalar xMin;
    static const Scalar xMax;
    static const int    numY = 500;
    static const Scalar yMin;
    static const Scalar yMax;
    static const Scalar vals[200][500];
};

struct co2TabulatedEnthalpyTraits
{
    using Scalar = double;
    static const char  *name;
    static const int    numX = 200;
    static const Scalar xMin;
    static const Scalar xMax;
    static const int    numY = 500;
    static const Scalar yMin;
    static const Scalar yMax;
    static const Scalar vals[200][500];
};

template<class Scalar = double, class ContainerT = std::vector<double>>
class CO2Tables
{
public:
    UniformTabulated2DFunction<Scalar, ContainerT> tabulatedDensity;
    UniformTabulated2DFunction<Scalar, ContainerT> tabulatedEnthalpy;
    static constexpr double brineSalinity = 1.000000000000000e-01;

    CO2Tables();

    CO2Tables(const Opm::UniformTabulated2DFunction<Scalar, ContainerT>& enthalpy,
              const Opm::UniformTabulated2DFunction<Scalar, ContainerT>& density)
        : tabulatedEnthalpy(enthalpy), tabulatedDensity(density)
    {
    }

    const Opm::UniformTabulated2DFunction<Scalar, ContainerT>& getTabulatedEnthalpy() const {
        return tabulatedEnthalpy;
    }

    const Opm::UniformTabulated2DFunction<Scalar, ContainerT>& getTabulatedDensity() const {
        return tabulatedDensity;
    }
};

} // namespace Opm

namespace Opm::gpuistl {
    template <class ViewType, class Scalar, class ContainerType>
    CO2Tables<Scalar, ViewType>
    make_view(CO2Tables<Scalar, ContainerType>& oldCO2Tables) {
        Opm::UniformTabulated2DFunction<double, ViewType> newEnthalpy = make_view<ViewType>(oldCO2Tables.tabulatedEnthalpy);
        Opm::UniformTabulated2DFunction<double, ViewType> newDensity = make_view<ViewType>(oldCO2Tables.tabulatedDensity);

        return CO2Tables<Scalar, ViewType>(newEnthalpy, newDensity);
    }

    template <class NewContainerType, class Scalar, class OldContainerType>
    CO2Tables<Scalar, NewContainerType>
    copy_to_gpu(const CO2Tables<Scalar, OldContainerType>& oldCO2Tables) {
        return CO2Tables<Scalar, NewContainerType>(
            copy_to_gpu<NewContainerType>(oldCO2Tables.tabulatedEnthalpy),
            copy_to_gpu<NewContainerType>(oldCO2Tables.tabulatedDensity)
        );
    }
}

#endif // OPM_CO2TABLES_HPP
