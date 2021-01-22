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

#ifndef OPM_NUMERICALAQUIFERCELL_HPP
#define OPM_NUMERICALAQUIFERCELL_HPP

#include <optional>

namespace Opm {
    class DeckRecord;
    class EclipseGrid;
    class FieldPropsManager;

    struct NumericalAquiferCell {
        NumericalAquiferCell(const DeckRecord&, const EclipseGrid&, const FieldPropsManager&);
        size_t aquifer_id; // aquifer id
        size_t I, J, K; // indices for the grid block
        double area; // cross-sectional area
        double length;
        double porosity;
        double permeability;
        double depth; // by default the grid block depth will be used
        std::optional<double> init_pressure; // by default, the grid pressure from equilibration will be used
        int pvttable; // by default, the block PVTNUM
        int sattable; // saturation table number, by default, the block value
        double transmissibility;
        size_t global_index;

        double cellVolume() const;
        double poreVolume() const;
        bool operator == (const NumericalAquiferCell& other) const;
    };
}

#endif //OPM_NUMERICALAQUIFERCELL_HPP
