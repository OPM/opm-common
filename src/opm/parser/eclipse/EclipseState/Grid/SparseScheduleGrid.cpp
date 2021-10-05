/*
  Copyright 2021 Statoil ASA.

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

#include <opm/parser/eclipse/EclipseState/Grid/SparseScheduleGrid.hpp>

#include <stdexcept>

std::size_t Opm::SparseScheduleGrid::getActiveIndex(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("BUG: SparseScheduleGrid::activeIndex called on a sparse grid missing the cell");
}

std::size_t Opm::SparseScheduleGrid::getGlobalIndex(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("BUG: SparseScheduleGrid::getGlobalIndex called on a sparse grid missing the cell");
}

bool Opm::SparseScheduleGrid::isCellActive(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("BUG: SparseScheduleGrid::cellActive called on a sparse grid missing the cell");
}

double Opm::SparseScheduleGrid::getCellDepth(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("BUG: SparseScheduleGrid::getCellDepth called on a sparse grid missing the cell");
}

std::array<double, 3> Opm::SparseScheduleGrid::getCellDimensions(std::size_t, std::size_t, std::size_t) const {
    throw std::logic_error("BUG: SparseScheduleGrid::getCellDims called on a sparse grid missing the cell");
}
