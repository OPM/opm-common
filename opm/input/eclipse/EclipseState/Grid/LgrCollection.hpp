/*
 Copyright (C) 2023 Equinor
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

#ifndef OPM_PARSER_LGR_COLLECTION_HPP
#define OPM_PARSER_LGR_COLLECTION_HPP

#include <opm/input/eclipse/EclipseState/Util/OrderedMap.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <array>
#include <cstddef>
#include <string>

namespace Opm {

    class Deck;
    class DeckRecord;
    class GridDims;
    class GRIDSection;
    class KeywordLocation;

class LgrCollection {
public:
    LgrCollection();
    LgrCollection(const GRIDSection& gridSection, const EclipseGrid& grid, const Deck& deck);

   static LgrCollection serializationTestObject();

   explicit LgrCollection(const Deck& deck);

    std::size_t size() const;
    bool hasLgr(const std::string& lgrName) const;
    Carfin& getLgr(const std::string& lgrName);
    const Carfin& getLgr(const std::string& lgrName) const;
    Carfin& getLgr(std::size_t lgrIndex);
    const Carfin& getLgr(std::size_t lgrIndex) const;

    void addLgr(const EclipseGrid& grid,
                const DeckRecord& lgrRecord,
                const KeywordLocation& location);

    /// LGRPILLR 'BOX': refined pillars from each box's own first layer, as the
    /// reference, instead of the column's (stacked boxes on sheared pillars then
    /// do not conform).
    bool pillarsFromBoxLayer() const { return m_pillarsFromBoxLayer; }

    /// The level-zero (I,J,K) whose refinement holds cell ijk of LGR number
    /// lgrNumber (1-based, deck order), up through nested parents; graded boxes
    /// included.
    std::array<int,3> levelZeroIJK(std::size_t lgrNumber, std::array<int,3> ijk) const;

    bool operator==(const LgrCollection& data) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_lgrs);
        serializer(m_pillarsFromBoxLayer);
    }

private:
    OrderedMap<Carfin, 8> m_lgrs;
    bool m_pillarsFromBoxLayer{false};

};
}

#endif // OPM_PARSER_LGR_COLLECTION_HPP
