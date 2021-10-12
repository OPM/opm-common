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


#ifndef OPM_PARSER_SPARSE_SCHEDULE_GRID_HPP
#define OPM_PARSER_SPARSE_SCHEDULE_GRID_HPP

#include <cstddef>

#include <array>
#include <map>
#include <optional>
#include <set>

#include <opm/parser/eclipse/EclipseState/Grid/ScheduleGrid.hpp>

namespace Opm {

    class SparseScheduleGrid: public ScheduleGrid {
    private:
        struct Cell {
            std::size_t i, j, k;
            std::size_t globalIndex;
            std::optional<std::size_t> activeIndex;

            double depth;
            std::array<double, 3> dimensions;
        };

        const Cell& getCell(std::size_t i, std::size_t j, std::size_t k) const;

    public:
        std::size_t getActiveIndex(std::size_t i, std::size_t j, std::size_t k) const override;
        std::size_t getGlobalIndex(std::size_t i, std::size_t j, std::size_t k) const override;

        bool isCellActive(std::size_t i, std::size_t j, std::size_t k) const override;

        double getCellDepth(std::size_t i, std::size_t j, std::size_t k) const override;
        std::array<double, 3> getCellDimensions(std::size_t i, std::size_t j, std::size_t k) const override;

        SparseScheduleGrid(const ScheduleGrid& source, const std::set<ScheduleGrid::CellKey>& loadKeys);

    private:
        typedef std::map<ScheduleGrid::CellKey, Cell> CellMap;

        CellMap loadedCells;

        static CellMap loadCells(const ScheduleGrid& source, const std::set<ScheduleGrid::CellKey>& loadKeys);
        static Cell loadCell(const ScheduleGrid& source, const ScheduleGrid::CellKey& loadKey);
};

}

#endif // OPM_PARSER_SPARSE_SCHEDULE_GRID_HPP
