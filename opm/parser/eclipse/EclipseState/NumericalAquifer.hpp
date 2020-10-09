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

namespace Opm {
    class Deck;
    class DeckRecord;
    class NumericalAquiferConnections;
    class NumAquiferCon;
    class EclipseGrid;

    struct NumericalAquiferCell {
        explicit NumericalAquiferCell(const DeckRecord&);
        // TODO: we might not want this default constructor
        NumericalAquiferCell() = default;
        // TODO: what constructor should be?
        // TODO: we might not need it
        int aquifer_id; // aquifer id
        // TODO: what is the best way to organize them, maybe a std::map from <I,J,K>
        int I, J, K; // indices for the grid block
        double area; // cross-sectional area
        double length;
        double porosity;
        double permeability;
        // TODO: what is the better way to handle defaulted input?
        double depth = -1.e100; // by default the grid block depth will be used
        double init_pressure = -1.e100; // by default, the grid pressure from equilibration will be used
        int pvttable = -1; // by default, the block PVTNUM
        int sattable = -1; // saturation table number, by default, the block value
    };

    class SingleNumericalAquifer {
    public:
        explicit SingleNumericalAquifer(const int aqu_id);
        void addAquiferCell(const NumericalAquiferCell& aqu_cell);
        void addAquiferConnection(const NumAquiferCon& aqu_con);
    private:
        // Maybe this id_ is not necessary
        // Because if it is a map, the id will be there
        // Then adding aquifer cells will be much easier with the
        // default constructor
        int id_;
        std::vector<NumericalAquiferCell> cells_;
        std::vector<NumAquiferCon> connections_;
    };

    class NumericalAquifers {
    public:
        NumericalAquifers() = default;
        explicit NumericalAquifers(const Deck& deck, const EclipseGrid& grid);

        bool hasAquifer(const int aquifer_id) const;
    private:
        // std::un_ordered_map
        std::unordered_map<int, SingleNumericalAquifer> aquifers_;

        void addAquiferCell(const NumericalAquiferCell& aqu_cell);
        void addAquiferConnections(const Deck& deck, const EclipseGrid& grid);
    };
}
#endif // OPM_NUMERICAL_AQUIFER_HPP

