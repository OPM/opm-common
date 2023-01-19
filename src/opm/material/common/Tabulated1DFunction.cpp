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
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <ostream>

namespace Opm {

template<class Scalar>
void Tabulated1DFunction<Scalar>::
printCSV(Scalar xi0, Scalar xi1, unsigned k, std::ostream& os) const
{
    Scalar x0 = std::min(xi0, xi1);
    Scalar x1 = std::max(xi0, xi1);
    const int n = numSamples() - 1;
    for (unsigned i = 0; i <= k; ++i) {
        double x = i*(x1 - x0)/k + x0;
        double y;
        double dy_dx;
        if (!applies(x)) {
            if (x < xValues_[0]) {
                dy_dx = evalDerivative(xValues_[0]);
                y = (x - xValues_[0])*dy_dx + yValues_[0];
            }
            else if (x > xValues_[n]) {
                dy_dx = evalDerivative(xValues_[n]);
                y = (x - xValues_[n])*dy_dx + yValues_[n];
            }
            else {
                throw std::runtime_error("The sampling points given to a "
                                         "function must be sorted by their x value!");
            }
        }
        else {
            y = eval(x);
            dy_dx = evalDerivative(x);
        }

        os << x << " " << y << " " << dy_dx << "\n";
    }
}

template void
Tabulated1DFunction<double>::printCSV(double,double,
                                      unsigned,std::ostream&) const;
template void
Tabulated1DFunction<float>::printCSV(float,float,
                                     unsigned,std::ostream&) const;

} // namespace Opm
