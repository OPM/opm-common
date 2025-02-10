/*
  Copyright (C) 2020 SINTEF Digital

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

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferHelpers.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>

#include <cstddef>
#include <optional>
#include <unordered_set>
#include <vector>

namespace {
    bool cellInsideReservoirAndActive(const Opm::EclipseGrid& grid,
                                      const int i, const int j, const int k,
                                      const std::vector<int>& actnum,
                                      const std::optional<const std::unordered_set<std::size_t>>& numerical_aquifer_cells)
    {
        if ((i < 0) || (j < 0) || (k < 0)
            || (std::size_t(i) > grid.getNX() - 1)
            || (std::size_t(j) > grid.getNY() - 1)
            || (std::size_t(k) > grid.getNZ() - 1))
        {
            return false;
        }

        const std::size_t globalIndex = grid.getGlobalIndex(i, j, k);
        if (! actnum[globalIndex]) {
            return false;
        }

        // We consider a numerical aquifer cell is outside the reservoir, so
        // we can create aquifer connection between a reservoir cell and a
        // numerical aquifer cell.
        const bool is_numerical_aquifer_cells = numerical_aquifer_cells.has_value()
            && (numerical_aquifer_cells->count(globalIndex) > 0);

        return ! is_numerical_aquifer_cells;
    }
} // Anonymous namespace

namespace Opm::AquiferHelpers {

    bool neighborCellInsideReservoirAndActive(const EclipseGrid& grid, const int i, const int j, const int k,
                                              const Opm::FaceDir::DirEnum faceDir, const std::vector<int>& actnum,
                                              const std::optional<const std::unordered_set<std::size_t>>& numerical_aquifer_cells)
    {
        switch (faceDir) {
        case FaceDir::XMinus:
            return cellInsideReservoirAndActive(grid, i - 1, j, k, actnum, numerical_aquifer_cells);

        case FaceDir::XPlus:
            return cellInsideReservoirAndActive(grid, i + 1, j, k, actnum, numerical_aquifer_cells);

        case FaceDir::YMinus:
            return cellInsideReservoirAndActive(grid, i, j - 1, k, actnum, numerical_aquifer_cells);

        case FaceDir::YPlus:
            return cellInsideReservoirAndActive(grid, i, j + 1, k, actnum, numerical_aquifer_cells);

        case FaceDir::ZMinus:
            return cellInsideReservoirAndActive(grid, i, j, k - 1, actnum, numerical_aquifer_cells);

        case FaceDir::ZPlus:
            return cellInsideReservoirAndActive(grid, i, j, k + 1, actnum, numerical_aquifer_cells);

        default:
            throw std::runtime_error("Unknown FaceDir enum " + std::to_string(faceDir));
        }
    }

} // namespace Opm::AquiferHelpers
