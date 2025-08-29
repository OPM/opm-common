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

#include <config.h>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/EclipseState/Geochemistry/GenericSpeciesConfig.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <string>

namespace {
    std::string fromSpeciesType(Opm::SpeciesType s)
    {
        switch (s) {
            case Opm::SpeciesType::SPECIES: return "SPECIES";
            case Opm::SpeciesType::MINERAL: return "MINERAL";
            case Opm::SpeciesType::IONEX: return "IONEX";
            default: throw std::invalid_argument("Unknown SpeciesType");
        };
    };
}

namespace Opm {

void GenericSpeciesConfig::initializeSpeciesType(const DeckItem& item, const Deck& deck, SpeciesType s)
{
    const auto& species_type = fromSpeciesType(s);
    InfoLogger logger(species_type + " tables", 3);
    for (std::size_t i = 0; i < item.getData<std::string>().size(); ++i) {
        const auto& species_name = item.getTrimmedString(i);
        checkSpeciesName(species_name, s);

        // Naming rule: First letter in fromSpeciesType() + BLK or VDP + first four characters of species name
        std::string blk_name = species_type.substr(0, 1) + "BLK" + species_name.substr(0, 4);
        std::string vdp_name = species_type.substr(0, 1) + "VDP" + species_name.substr(0, 4);

        if (deck.hasKeyword(blk_name)) {
            const auto& blk_keyword = deck[blk_name].back();
            initFromXBLK(blk_keyword, species_name, logger);
        }
        else if (deck.hasKeyword(vdp_name)) {
            const auto& vdp_keyword = deck[vdp_name].back();
            initFromXVDP(vdp_keyword, species_name, logger);
        }
        else {
            initEmpty(species_name);
        }
    }
}

void GenericSpeciesConfig::initFromXBLK(const DeckKeyword& sblk_keyword,
                            const std::string& species_name,
                            InfoLogger& logger)
{
    double inv_volume = 1.0;
    auto sblk_conc = sblk_keyword.getRecord(0).getItem(0).getData<double>();
    logger(sblk_keyword.location().format("Loading concentration from {keyword} in {file} line {line}"));

    std::transform(sblk_conc.begin(), sblk_conc.end(), sblk_conc.begin(),
                    [inv_volume](const auto& c) { return c * inv_volume; });

    this->species.emplace_back(species_name, std::move(sblk_conc));
}

void GenericSpeciesConfig::initFromXVDP(const DeckKeyword& svdp_keyword,
                      const std::string& species_name,
                      InfoLogger& logger)
{
    double inv_volume = 1.0;
    auto svdp_table = svdp_keyword.getRecord(0).getItem(0);
    logger(svdp_keyword.location().format("Loading concentration from {keyword} in {file} line {line}"));

    this->species.emplace_back(species_name, SpeciesVdTable(svdp_table, inv_volume, species.size()));
}

void GenericSpeciesConfig::initEmpty(const std::string& species_name)
{
    this->species.emplace_back(species_name);
}

void GenericSpeciesConfig::checkSpeciesName(const std::string& species_name, SpeciesType s)
{
    std::string msg;
    const auto& species_type = fromSpeciesType(s);

    // Names greater than eight char will create problems later (Restart output, etc.)
    if (species_name.size() > 8) {
        msg = fmt::format("Item {0} in {1} is longer than the 8 character limit! "
                          "Tips: Check if nickname for {0} is shorter.",
                          species_name, species_type);
    }

    // If species name with four characters already exist there will be possible ambiguities in other keywords
    auto iter = std::find_if(this->species.begin(), this->species.end(),
                             [&species_name](const SpeciesEntry& single_species)
                             { return single_species.name.substr(0, 4) == species_name.substr(0, 4);}
                            );
    if (iter != this->species.end()) {
        if (iter->name == species_name) {
            msg = fmt::format("Duplicate items {} in {} are not allowed!", species_name, species_type);
        }
        else {
            msg = fmt::format("The first four characters of item {} in {} overlaps with item {}! "
                            "Tips: Try replacing one of the items with their nickname.",
                            species_name, species_type, iter->name);
        }
    }

    // Write errors
    if (!msg.empty()) {
        OpmLog::error(msg);
        throw std::runtime_error(msg);
    }
}

GenericSpeciesConfig GenericSpeciesConfig::serializationTestObject()
{
    GenericSpeciesConfig result;
    result.species = {{"test", {1.0}}};

    return result;
}

const GenericSpeciesConfig::SpeciesEntry& GenericSpeciesConfig::operator[](std::size_t index) const {
    return this->species.at(index);
}

const GenericSpeciesConfig::SpeciesEntry& GenericSpeciesConfig::operator[](const std::string& name) const {
    auto iter = std::find_if(this->species.begin(), this->species.end(),
                                [&name](const SpeciesEntry& single_species)
                                { return single_species.name == name;}
                            );

    if (iter == this->species.end())
        throw std::logic_error(fmt::format("No such species: {}", name));

    return *iter;
}

std::size_t GenericSpeciesConfig::size() const {
    return this->species.size();
}

bool GenericSpeciesConfig::empty() const {
    return this->species.empty();
}

const std::vector<GenericSpeciesConfig::SpeciesEntry>::const_iterator GenericSpeciesConfig::begin() const {
    return this->species.begin();
}

const std::vector<GenericSpeciesConfig::SpeciesEntry>::const_iterator GenericSpeciesConfig::end() const {
    return this->species.end();
}

bool GenericSpeciesConfig::operator==(const GenericSpeciesConfig& other) const {
    return this->species == other.species;
}

} // namespace Opm