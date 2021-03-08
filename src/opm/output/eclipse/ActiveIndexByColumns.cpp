/*
  Copyright (c) 2021 Equinor ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/output/eclipse/ActiveIndexByColumns.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

namespace {
    std::size_t columnarGlobalIdx(const std::array<int, 3>& dims,
                                  const std::array<int, 3>& ijk)
    {
        // Linear index assuming C-like loop order
        //
        //     for (i = 0 .. Nx - 1)
        //         for (j = 0 .. Ny - 1)
        //             for (k = 0 .. Nz - 1)
        //
        // as opposed to the usual Fortran-like loop order ("natural ordering")
        //
        //     for (k = 0 .. Nz - 1)
        //         for (j = 0 .. Ny - 1)
        //             for (i = 0 .. Nx - 1)
        //
        return ijk[2] + dims[2]*(ijk[1] + dims[1]*ijk[0]);
    }
}

bool Opm::ActiveIndexByColumns::operator==(const ActiveIndexByColumns& rhs) const
{
    return this->natural2columnar_ == rhs.natural2columnar_;
}

void
Opm::ActiveIndexByColumns::
buildMappingTables(const std::size_t                                           numActive,
                   const std::array<int, 3>&                                   cartDims,
                   const std::function<std::array<int, 3>(const std::size_t)>& getIJK)
{
    // Algorithm:
    //
    //   1. Mark active cells as such, using column-based active index in global array.
    //   2. Accumulate number of active cells in global array.
    //   3. Extract column-based active index from global array, push back
    //      into natural2columnar_ according to natural numbering of active cells.

    auto cartesianActive = std::vector<int>(cartDims[0] * cartDims[1] * cartDims[2], 0);
    auto colActIx = [&cartDims, &getIJK, &cartesianActive](const std::size_t activeCell) -> int&
    {
        return cartesianActive[columnarGlobalIdx(cartDims, getIJK(activeCell))];
    };

    for (auto activeCell = 0*numActive; activeCell < numActive; ++activeCell) {
        colActIx(activeCell) = 1;
    }

    // Counts number of active cells (by columns) up to and including current.
    std::partial_sum(cartesianActive.begin(), cartesianActive.end(), cartesianActive.begin());

    this->natural2columnar_.clear();
    this->natural2columnar_.reserve(numActive);
    for (auto activeCell = 0*numActive; activeCell < numActive; ++activeCell) {
        // Subtract 1 to discount current active cell.  We only need number
        // of active cells PRIOR to current.
        this->natural2columnar_.push_back(colActIx(activeCell) - 1);
    }
}

void Opm::buildColumnarActiveIndexMappingTables(const EclipseGrid&    grid,
                                                ActiveIndexByColumns& map)
{
    map.buildMappingTables(grid.getNumActive(), grid.getNXYZ(),
        [&grid](const std::size_t activeCell)
    {
        return grid.getIJK(grid.getGlobalIndex(activeCell));
    });
}
