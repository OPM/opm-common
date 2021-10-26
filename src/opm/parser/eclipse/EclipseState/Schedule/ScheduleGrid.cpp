/*
  Copyright 2021 Equinor ASA.

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

#include <opm/parser/eclipse/EclipseState/Schedule/CompletedCells.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

Opm::ScheduleGrid::ScheduleGrid(const Opm::EclipseGrid& ecl_grid, Opm::CompletedCells& completed_cells)
    : grid(&ecl_grid)
    , cells(completed_cells)
{}

Opm::ScheduleGrid::ScheduleGrid(Opm::CompletedCells& completed_cells)
    : cells(completed_cells)
{}

const Opm::CompletedCells::Cell& Opm::ScheduleGrid::get_cell(std::size_t i, std::size_t j, std::size_t k) const {
    if (this->grid) {
        auto [valid, cell] = this->cells.try_get(i,j,k);
        if (!valid) {
            cell.depth = this->grid->getCellDepth(i,j,k);
            cell.dimensions = this->grid->getCellDimensions(i,j,k);
            if (this->grid->cellActive(i,j,k))
                cell.active_index = this->grid->getActiveIndex(i,j,k);
        }
        return cell;
    } else
        return this->cells.get(i,j,k);
}

