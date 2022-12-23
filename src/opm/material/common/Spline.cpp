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
#include <opm/material/common/Spline.hpp>

#include <ostream>

namespace Opm
{

template<class Scalar>
void Spline<Scalar>::printCSV(Scalar xi0, Scalar xi1,
                              size_t k, std::ostream& os) const
{
    Scalar x0 = std::min(xi0, xi1);
    Scalar x1 = std::max(xi0, xi1);
    const size_t n = numSamples() - 1;
    for (size_t i = 0; i <= k; ++i) {
        double x = i*(x1 - x0)/k + x0;
        double x_p1 = x + (x1 - x0)/k;
        double y;
        double dy_dx;
        double mono = 1;
        if (!applies(x)) {
            if (x < x_(0)) {
                dy_dx = evalDerivative(x_(0));
                y = (x - x_(0))*dy_dx + y_(0);
                mono = (dy_dx>0)?1:-1;
            }
            else if (x > x_(n)) {
                dy_dx = evalDerivative(x_(n));
                y = (x - x_(n))*dy_dx + y_(n);
                mono = (dy_dx>0)?1:-1;
            }
            else {
                throw std::runtime_error("The sampling points given to a spline must be sorted by their x value!");
            }
        }
        else {
            y = eval(x);
            dy_dx = evalDerivative(x);
            mono = monotonic(x, x_p1, /*extrapolate=*/true);
        }

        os << x << " " << y << " " << dy_dx << " " << mono << "\n";
    }
}

template void Spline<double>::printCSV(double,double,size_t,std::ostream&) const;
template void Spline<float>::printCSV(float,float,size_t,std::ostream&) const;

}
