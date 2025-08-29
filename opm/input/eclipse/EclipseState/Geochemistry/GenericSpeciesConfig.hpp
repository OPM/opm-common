/*
  Copyright (C) 2020 Equinor

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

#ifndef SPECIES_CONFIG_HPP
#define SPECIES_CONFIG_HPP

#include <opm/common/OpmLog/InfoLogger.hpp>

#include <opm/input/eclipse/EclipseState/Tables/SpeciesVdTable.hpp>
#include <opm/input/eclipse/Parser/ParserKeyword.hpp>

#include <optional>
#include <string>

namespace Opm {

class Deck;
class DeckItem;

enum class SpeciesType {
    SPECIES,
    MINERAL,
    IONEX
};

class GenericSpeciesConfig {
public:
    struct SpeciesEntry {
        std::string name;
        std::optional<std::vector<double>> concentration;
        std::optional<SpeciesVdTable> svdp;

        SpeciesEntry() = default;

        SpeciesEntry(const std::string& name_, std::vector<double> concentration_)
            : name(name_)
            , concentration(concentration_)
        {}

        SpeciesEntry(const std::string& name_, SpeciesVdTable svdp_)
            : name(name_)
            , svdp(svdp_)
        {}

        SpeciesEntry(const std::string& name_)
            : name(name_)
        {}

        bool operator==(const SpeciesEntry& data) const {
            return this->name == data.name &&
                   this->concentration == data.concentration &&
                   this->svdp == data.svdp;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(name);
            serializer(concentration);
            serializer(svdp);
        }
    }; // struct SpeciesEntry

    GenericSpeciesConfig() = default;
    virtual ~GenericSpeciesConfig() = default;

    static GenericSpeciesConfig serializationTestObject();

    std::size_t size() const;
    bool empty() const;
    const std::vector<SpeciesEntry>::const_iterator begin() const;
    const std::vector<SpeciesEntry>::const_iterator end() const;
    const SpeciesEntry& operator[](const std::string& name) const;
    const SpeciesEntry& operator[](std::size_t index) const;
    bool operator==(const GenericSpeciesConfig& data) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(species);
    }

protected:
    void initializeSpeciesType(const DeckItem& item, const Deck& deck, SpeciesType s);
    void initFromXBLK(const DeckKeyword& sblk_keyword,
                      const std::string& species_name,
                      InfoLogger& logger);
    void initFromXVDP(const DeckKeyword& svdp_keyword,
                      const std::string& species_name,
                      InfoLogger& logger);
    void initEmpty(const std::string& species_name);
    void checkSpeciesName(const std::string& species_name, SpeciesType s);

private:
    std::vector<SpeciesEntry> species;
}; // class GenericSpeciesConfig

} // namespec Opm
#endif