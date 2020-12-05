/*
  Copyright (C) 2020 Equinor ASA
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

  ?You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPM_NUMERICAL_AQUIFER_HPP
#define OPM_NUMERICAL_AQUIFER_HPP

#include <vector>
#include <unordered_map>
#include <set>
#include <array>

namespace Opm {
    class Deck;
    class DeckRecord;
    class NumericalAquiferConnections;
    struct NumAquiferCon;
    class EclipseGrid;
    class FieldPropsManager;
    class NNC;
    namespace Fieldprops {
        class TranCalculator;
    }

    struct NumericalAquiferCell {
        NumericalAquiferCell(const DeckRecord&, const EclipseGrid&, const FieldPropsManager&);
        size_t aquifer_id; // aquifer id
        size_t I, J, K; // indices for the grid block
        double area; // cross-sectional area
        double length;
        double porosity;
        double permeability;
        double depth; // by default the grid block depth will be used
        double init_pressure; // by default, the grid pressure from equilibration will be used
        int pvttable; // by default, the block PVTNUM
        int sattable; // saturation table number, by default, the block value
        double pore_volume; // pore volume
        double transmissibility;
        size_t global_index;
        double cellVolume() const;
    };

    class SingleNumericalAquifer {
    public:
        explicit SingleNumericalAquifer(const size_t aqu_id);
        void addAquiferCell(const NumericalAquiferCell& aqu_cell);
        void addAquiferConnection(const NumAquiferCon& aqu_con);
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
        const std::unordered_map<size_t, double> cellVolumes() const;
    private:
        // Maybe this id_ is not necessary
        // Because if it is a map, the id will be there
        // Then adding aquifer cells will be much easier with the
        // default constructor
        size_t id_;
        std::vector<NumericalAquiferCell> cells_;
        std::vector<NumAquiferCon> connections_;
    };

    class NumericalAquifers {
    public:
        NumericalAquifers() = default;
        NumericalAquifers(const Deck& deck, const EclipseGrid& grid, const FieldPropsManager& field_props);

        bool hasAquifer(const size_t aquifer_id) const;
        bool hasCell(const size_t cell_global_index) const;
        void updateCellProps(const EclipseGrid& grid,
                             std::vector<double>& pore_volume,
                             std::vector<int>& satnum,
                             std::vector<int>& pvtnum,
                             std::vector<double>& cell_depth) const;
        std::array<std::set<size_t>, 3> transToRemove(const EclipseGrid& grid) const;
        // TODO: maybe better wrap with other more direct functions, let us see the usage first
        const std::unordered_map<size_t, const NumericalAquiferCell>& aquiferCells() const;
        void appendNNC(const EclipseGrid &grid, const FieldPropsManager &fp, NNC &nnc) const;
        void appendConnectionNNC(const EclipseGrid &grid, const FieldPropsManager &fp, const std::vector<int>& actnum, NNC &nnc) const;
        const NumericalAquiferCell& getCell(const size_t cell_global_index) const;
        const std::unordered_map<size_t, SingleNumericalAquifer>& aquifers() const;
        bool active() const;
        const std::unordered_map<size_t, double> cellVolumes() const;
        void addAquiferConnections(const Deck& deck, const EclipseGrid& grid, const std::vector<int>& actnum);

    private:
        // std::un_ordered_map
        std::unordered_map<size_t, SingleNumericalAquifer> aquifers_;
        // TODO: it is a little wasteful, just convenience for now
        std::unordered_map<size_t, const NumericalAquiferCell> aquifer_cells_;

        void addAquiferCell(const NumericalAquiferCell& aqu_cell);
    };
}
#endif // OPM_NUMERICAL_AQUIFER_HPP

