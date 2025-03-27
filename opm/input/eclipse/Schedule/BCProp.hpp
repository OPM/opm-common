/*
  Copyright 2023 Equinor ASA.
  Copyright 2023 Norce.

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

#ifndef OPM_BC_PROP_HPP
#define OPM_BC_PROP_HPP

#include <vector>
#include <cstddef>
#include <optional>

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

namespace Opm {

class Deck;
class DeckRecord;

enum class BCType {
     RATE,
     FREE,
     DIRICHLET,
     THERMAL,
     CLOSED,
     NONE
};

enum class BCMECHType {
     FREE,
     FIXED,
     NONE
};

enum class BCComponent {
     OIL,
     GAS,
     WATER,
     SOLVENT,
     POLYMER,
     MICR,
     OXYG,
     UREA,
     NONE
};

struct MechBCValue {
    std::array<double,3> disp{};
    std::array<double,6> stress{};
    std::array<bool,3> fixeddir{};

    static MechBCValue serializationTestObject()
    {
        return MechBCValue{{1.0, 2.0, 3.0},
                           {3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
                           {true, false, true}};
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(disp);
        serializer(stress);
        serializer(fixeddir);
    }

    bool operator==(const MechBCValue& other) const
    {
        return disp == other.disp &&
               stress == other.stress &&
               fixeddir == other.fixeddir;
    }
};

class BCProp
{
public:
    struct BCFace
    {
        int index{};
        BCType bctype{BCType::NONE};
        BCMECHType bcmechtype{BCMECHType::NONE};
        BCComponent component{BCComponent::NONE};
        double rate{};
        std::optional<double> pressure{};
        std::optional<double> temperature{};

        std::optional<MechBCValue> mechbcvalue{};

        BCFace() = default;
        explicit BCFace(const DeckRecord& record);

        static BCFace serializationTestObject();

        bool operator==(const BCFace& other) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(index);
            serializer(bctype);
            serializer(bcmechtype);
            serializer(component);
            serializer(rate);
            serializer(pressure);
            serializer(temperature);
            serializer(mechbcvalue);
        }
    };

    BCProp() = default;

    static BCProp serializationTestObject();

    std::size_t size() const;
    std::vector<BCFace>::const_iterator begin() const;
    std::vector<BCFace>::const_iterator end() const;
    bool operator==(const BCProp& other) const;
    const BCFace& operator[](int index) const;

    void updateBCProp(const DeckRecord& record);

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_faces);
    }

private:
    std::vector<BCFace> m_faces;
};

} // namespace Opm

#endif
