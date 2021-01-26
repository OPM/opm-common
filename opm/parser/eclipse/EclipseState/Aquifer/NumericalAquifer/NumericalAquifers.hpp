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

#ifndef OPM_NUMERICALAQUIFERS_HPP
#define OPM_NUMERICALAQUIFERS_HPP

#include <unordered_map>

#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/SingleNumericalAquifer.hpp>

namespace Opm {
    class Deck;
    class EclipseGrid;
    class FieldPropsManager;
    class SingleNumericalAquifer;

    class NumericalAquifers {
    public:
        NumericalAquifers() = default;
        NumericalAquifers(const Deck& deck, const EclipseGrid& grid, const FieldPropsManager& field_props);

        size_t numAquifer() const;
        bool hasAquifer(size_t aquifer_id) const;
        bool hasCell(size_t global_index) const;
        const SingleNumericalAquifer& getAquifer(size_t aquifer_id) const;
        const NumericalAquiferCell& getCell(size_t global_index) const;
        bool operator==(const NumericalAquifers& other) const;

        static NumericalAquifers serializeObject();
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer.map(this->aquifers_);
            // TODO: serialize pointers is problematic, for pointers should we regenerate the pointers?
            // TODO: if yes, we need to write the function to generate this->aquifer_cells_
            // serializer.map(this->aquifer_cells_);
        }

    private:
        std::unordered_map <size_t, SingleNumericalAquifer> aquifers_;
        std::unordered_map<size_t, const NumericalAquiferCell*> aquifer_cells_;

        void addAquiferCell(const NumericalAquiferCell& aqu_cell);

        void addAquiferConnections(const Deck &deck, const EclipseGrid &grid);
    };
}

#endif //OPM_NUMERICALAQUIFERS_HPP
