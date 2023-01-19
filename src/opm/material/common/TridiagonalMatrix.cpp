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
#include <opm/material/common/TridiagonalMatrix.hpp>

#include <ostream>

namespace Opm {

template<class Scalar>
void TridiagonalMatrix<Scalar>::print(std::ostream& os) const
{
    size_t n = size();

    // row 0
    os << at(0, 0) << "\t"
       << at(0, 1) << "\t";

    if (n > 3)
        os << "\t";
    if (n > 2)
        os << at(0, n-1);
    os << "\n";

    // row 1 .. n - 2
    for (unsigned rowIdx = 1; rowIdx < n-1; ++rowIdx) {
        if (rowIdx > 1)
            os << "\t";
        if (rowIdx == n - 2)
            os << "\t";

        os << at(rowIdx, rowIdx - 1) << "\t"
           << at(rowIdx, rowIdx) << "\t"
           << at(rowIdx, rowIdx + 1) << "\n";
    }

    // row n - 1
    if (n > 2)
        os << at(n-1, 0) << "\t";
    if (n > 3)
        os << "\t";
    if (n > 4)
        os << "\t";
    os << at(n-1, n-2) << "\t"
       << at(n-1, n-1) << "\n";
}

template void TridiagonalMatrix<double>::print(std::ostream&) const;
template void TridiagonalMatrix<float>::print(std::ostream&) const;

}
