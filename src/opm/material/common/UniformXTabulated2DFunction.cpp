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
#include <opm/material/common/UniformXTabulated2DFunction.hpp>

#include <ostream>

namespace Opm {

template<class Scalar>
void UniformXTabulated2DFunction<Scalar>::print(std::ostream& os) const
{
    Scalar x0 = xMin();
    Scalar x1 = xMax();
    int m = numX();

    Scalar y0 = 1e30;
    Scalar y1 = -1e30;
    size_t n = 0;
    for (int i = 0; i < m; ++ i) {
        y0 = std::min(y0, yMin(i));
        y1 = std::max(y1, yMax(i));
        n = std::max(n, numY(i));
    }

    m *= 3;
    n *= 3;
    for (int i = 0; i <= m; ++i) {
        Scalar x = x0 + (x1 - x0)*i/m;
        for (size_t j = 0; j <= n; ++j) {
            Scalar y = y0 + (y1 - y0)*j/n;
            os << x << " " << y << " " << eval(x, y) << "\n";
        }
        os << "\n";
    }
}

template void UniformXTabulated2DFunction<double>::print(std::ostream&) const;
template void UniformXTabulated2DFunction<float>::print(std::ostream&) const;

} // namespace Opm
