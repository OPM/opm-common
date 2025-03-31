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

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

bool Opm::CompletedCells::Cell::Props::operator==(const Props& other) const
{
    return (this->active_index == other.active_index)
        && (this->permx == other.permx)
        && (this->permy == other.permy)
        && (this->permz == other.permz)
        && (this->poro == other.poro)
        && (this->satnum == other.satnum)
        && (this->pvtnum == other.pvtnum)
        && (this->ntg == other.ntg)
        ;
}

Opm::CompletedCells::Cell::Props
Opm::CompletedCells::Cell::Props::serializationTestObject()
{
    Props props;

    props.permx = 10.0;
    props.permy = 78.0;
    props.permz = 45.4;
    props.poro = 0.321;
    props.satnum = 3;
    props.pvtnum = 5;
    props.ntg = 45.1;

    return props;
}

// ===========================================================================

bool Opm::CompletedCells::Cell::is_active() const
{
    return this->props.has_value();
}

std::size_t Opm::CompletedCells::Cell::active_index() const
{
    return this->props->active_index;
}

bool Opm::CompletedCells::Cell::operator==(const Cell& other) const
{
    return (this->global_index == other.global_index)
        && (this->i == other.i)
        && (this->j == other.j)
        && (this->k == other.k)
        && (this->depth == other.depth)
        && (this->dimensions == other.dimensions)
        && (this->props == other.props)
        ;
}

Opm::CompletedCells::Cell
Opm::CompletedCells::Cell::serializationTestObject()
{
    Cell cell { 0, 1, 1, 1 };

    cell.props = Props::serializationTestObject();
    cell.depth = 12345;
    cell.dimensions = {1.0,2.0,3.0};

    return cell;
}

// ===========================================================================

Opm::CompletedCells::CompletedCells(const Opm::GridDims& dims_)
    : dims { dims_ }
{}

Opm::CompletedCells::CompletedCells(const std::size_t nx,
                                    const std::size_t ny,
                                    const std::size_t nz)
    : dims { nx, ny, nz }
{}

const Opm::CompletedCells::Cell&
Opm::CompletedCells::get(const std::size_t i,
                         const std::size_t j,
                         const std::size_t k) const
{
    return this->cells.at(this->dims.getGlobalIndex(i, j, k));
}

std::pair<Opm::CompletedCells::Cell*, bool>
Opm::CompletedCells::try_get(const std::size_t i,
                             const std::size_t j,
                             const std::size_t k)
{
    const auto g = this->dims.getGlobalIndex(i, j, k);

    const auto& [pos, inserted] = this->cells.try_emplace(g, g, i, j, k);

    return { &pos->second, !inserted };
}

bool Opm::CompletedCells::operator==(const Opm::CompletedCells& other) const
{
    return (this->dims == other.dims)
        && (this->cells == other.cells)
        ;
}

Opm::CompletedCells
Opm::CompletedCells::serializationTestObject()
{
    Opm::CompletedCells cells(2,3,4);
    cells.cells.emplace(7, Opm::CompletedCells::Cell::serializationTestObject());

    return cells;
}
