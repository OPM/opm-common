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

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <cstddef>
#include <string>

#include <fmt/format.h>
#include <iostream>

Opm::ScheduleGrid::ScheduleGrid(const Opm::EclipseGrid& ecl_grid,
                                const Opm::FieldPropsManager& fpm,
                                Opm::CompletedCells& completed_cells)
    : grid  { &ecl_grid }
    , fp    { &fpm }
    , cells { completed_cells }
{}

Opm::ScheduleGrid::ScheduleGrid(Opm::CompletedCells& completed_cells)
    : cells(completed_cells)
{}

namespace {
    double try_get_value(const Opm::FieldPropsManager& fp,
                         const std::string& kw,
                         const std::size_t active_index)
    {
        if (! fp.has_double(kw)) {
            throw std::logic_error(fmt::format("FieldPropsManager is missing keyword '{}'", kw));
        }

        return fp.try_get<double>(kw)->at(active_index);
    }

    double try_get_ntg_value(const Opm::FieldPropsManager& fp,
                             const std::string& kw,
                             const std::size_t active_index)
    {
        return fp.has_double(kw)
            ? fp.try_get<double>(kw)->at(active_index)
            : 1.0;
    }
}

const Opm::CompletedCells::Cell&
Opm::ScheduleGrid::get_cell(std::size_t i, std::size_t j, std::size_t k) const
{
    std::cout << "In ScheduleGrid::get_cell" << std::endl;

    if (this->grid == nullptr) {
        std::cout << " no EclipseGrid set!" << std::endl;
        return this->cells.get(i, j, k);
    }

    auto [valid, cellRef] = this->cells.try_get(i, j, k);

    if (!valid) {
        cellRef.depth = this->grid->getCellDepth(i, j, k);
        cellRef.dimensions = this->grid->getCellDimensions(i, j, k);

        if (this->grid->cellActive(i, j, k)) {
            auto& props = cellRef.props.emplace(CompletedCells::Cell::Props{});

            props.active_index = this->grid->getActiveIndex(i, j, k);
            props.permx = try_get_value(*this->fp, "PERMX", props.active_index);
            props.permy = try_get_value(*this->fp, "PERMY", props.active_index);
            props.permz = try_get_value(*this->fp, "PERMZ", props.active_index);
            props.poro = try_get_value(*this->fp, "PORO", props.active_index);
            props.satnum = this->fp->get_int("SATNUM").at(props.active_index);
            props.pvtnum = this->fp->get_int("PVTNUM").at(props.active_index);
            props.ntg = try_get_ntg_value(*this->fp, "NTG", props.active_index);
        }
    }

    return cellRef;
}

const Opm::EclipseGrid* Opm::ScheduleGrid::get_grid() const
{
    return this->grid;
}
