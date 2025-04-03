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

#ifndef OPM_SOURCE_PROP_HPP
#define OPM_SOURCE_PROP_HPP

#include <array>
#include <vector>
#include <cstddef>
#include <optional>
#include <map>

namespace Opm {

class Deck;
class DeckRecord;

enum class SourceComponent {
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

class Source
{
public:
    struct SourceCell
    {
        SourceComponent component{SourceComponent::NONE};
        double rate{};
        std::optional<double> hrate{};
        std::optional<double> temperature{};

        SourceCell() = default;
        explicit SourceCell(const DeckRecord& record);

        static SourceCell serializationTestObject();

        bool operator==(const SourceCell& other) const;
        bool isSame(const SourceComponent& other) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(component);
            serializer(rate);
            serializer(hrate);
            serializer(temperature);
        }
    };

    Source() = default;

    static Source serializationTestObject();

    std::size_t size() const;
    std::map<std::array<int, 3>, std::vector<SourceCell>>::const_iterator begin() const;
    std::map<std::array<int, 3>, std::vector<SourceCell>>::const_iterator end() const;
    bool operator==(const Source& other) const;

    double rate(const std::array<int, 3>& ijk, SourceComponent input ) const;
    std::optional<double> hrate(const std::array<int, 3>& ijk, SourceComponent input ) const;
    std::optional<double> temperature(const std::array<int, 3>& ijk, SourceComponent input) const;
    bool hasSource(const std::array<int, 3>& input) const;

    void updateSource(const DeckRecord& record);

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_cells);
    }

private:
    std::map<std::array<int, 3>, std::vector<SourceCell>> m_cells;

    std::vector<SourceCell>::iterator findSourceCell(std::vector<SourceCell>& source_vec, SourceComponent input);
    std::vector<SourceCell>::const_iterator findSourceCell(const std::vector<SourceCell>& source_vec, SourceComponent input) const;
};

} // namespace Opm

#endif
