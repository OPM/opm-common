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

#ifndef OPM_SINGLENUMERICALAQUIFER_HPP
#define OPM_SINGLENUMERICALAQUIFER_HPP

#include <vector>

#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferConnection.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>

namespace Opm {

    class SingleNumericalAquifer {
    public:
        explicit SingleNumericalAquifer(const size_t aqu_id);

        void addAquiferCell(const NumericalAquiferCell& aqu_cell);
        void addAquiferConnection(const NumericalAquiferConnection& aqu_con);

        // TODO: the following two can be made one function. Let us see
        // how we use them at the end
        size_t numCells() const;
        const NumericalAquiferCell* getCellPrt(size_t index) const;

        bool operator==(const SingleNumericalAquifer& other) const;

        /*
        void addAquiferConnection(const NumericalAquiferConnection& aqu_con);
        void updateCellProps(const EclipseGrid& grid,
                             std::vector<double>& pore_volume,
                             std::vector<int>& satnum,
                             std::vector<int>& pvtnum,
                             std::vector<double>& cell_depth) const;
        std::array<std::set<size_t>, 3> transToRemove(const EclipseGrid& grid) const;
        void appendNNC(const EclipseGrid &grid, const FieldPropsManager &fp, NNC &nnc) const;
        void appendConnectionNNC(const EclipseGrid &grid, const FieldPropsManager &fp, const std::vector<int>& actnum, NNC &nnc) const;
        size_t numCells() const;
        size_t id() const;
        double initPressure() const;
        bool hasCell(const size_t global_index) const;
        const std::vector<NumericalAquiferCell>& cells() const;
        const std::unordered_map<size_t, double> cellVolumes() const; */
        private:
            // Maybe this id_ is not necessary
            // Because if it is a map, the id will be there
            // Then adding aquifer cells will be much easier with the
            // default constructor
            size_t id_;
            std::vector<NumericalAquiferCell> cells_;
            std::vector<NumericalAquiferConnection> connections_;
        };
}


#endif //OPM_SINGLENUMERICALAQUIFER_HPP
