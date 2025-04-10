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
  along with OPM.
*/

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/input/eclipse/Schedule/Source.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>

template <typename SourceCellSequence>
auto findInSequence(SourceCellSequence&& sequence, const Opm::SourceComponent comp)
{
    return std::find_if(std::begin(sequence), std::end(sequence),
                        [comp](const auto& source)
                        { return source.component == comp; });
}

namespace Opm {
namespace {

    std::optional<double> optionalItemValue(const DeckItem& i)
    {
        if (! i.hasValue(0)) {
            return {};
        }

        return i.getSIDouble(0);
    }

namespace fromstring {

SourceComponent component(const std::string& s)
{
    static const auto compMap = std::unordered_map<std::string, SourceComponent>{
        {"GAS",     SourceComponent::GAS},
        {"MICR",    SourceComponent::MICR},
        {"NONE",    SourceComponent::NONE},
        {"OIL",     SourceComponent::OIL},
        {"OXYG",    SourceComponent::OXYG},
        {"POLYMER", SourceComponent::POLYMER},
        {"SOLVENT", SourceComponent::SOLVENT},
        {"UREA",    SourceComponent::UREA},
        {"WATER",   SourceComponent::WATER},
    };

    const auto it = compMap.find(s);
    if (it != compMap.end()) {
        return it->second;
    }

    throw std::invalid_argument("Unrecognized source component: " + s);
}

}
}

// SourceCell functions
// --------------------
using SOURCEKEY = ParserKeywords::SOURCE;
Source::SourceCell::SourceCell(const DeckRecord& record)
    : component(fromstring::component(record.getItem<SOURCEKEY::COMPONENT>().get<std::string>(0)))
    , rate(record.getItem<SOURCEKEY::RATE>().getSIDouble(0))
    , hrate(optionalItemValue(record.getItem<SOURCEKEY::HRATE>()))
    , temperature(optionalItemValue(record.getItem<SOURCEKEY::TEMP>()))
{
}

Source::SourceCell Source::SourceCell::serializationTestObject()
{
    SourceCell result;
    result.component = SourceComponent::GAS;
    result.rate = 101.0;
    result.hrate = 201.0;
    result.temperature = 202.0;
    return result;
}

bool Source::SourceCell::operator==(const Source::SourceCell& other) const
{
    return this->component == other.component
        && this->rate == other.rate
        && this->hrate == other.hrate
        && this->temperature == other.temperature
        ;
}

// Source functions
// ----------------
void Source::updateSource(const DeckRecord& record)
{
    const Source::SourceCell sourcenew(record);
    std::array<int, 3> ijk {record.getItem<SOURCEKEY::I>().get<int>(0)-1,
                             record.getItem<SOURCEKEY::J>().get<int>(0)-1,
                             record.getItem<SOURCEKEY::K>().get<int>(0)-1};

    addSourceCell(ijk, sourcenew);
}

void Source::addSourceCell(const std::array<int,3>& ijk, const SourceCell& cell)
{
    auto [cellPos, inserted] = this->m_cells.try_emplace(ijk, std::vector { cell });
    if (! inserted) {
        auto sourcePos = findInSequence(cellPos->second, cell.component);

        if (sourcePos != cellPos->second.end()) {
            *sourcePos = cell;
        }
        else {
            cellPos->second.push_back(cell);
        }
    }
}

Source Source::serializationTestObject()
{
    Source result;
    const std::array<int, 3> ijk = {1,1,1};
    result.m_cells = {{ijk, {SourceCell::serializationTestObject()}}};

    return result;
}

bool Source::hasSource(const std::array<int, 3>& input) const
{
    return m_cells.find(input) != m_cells.end();
}

double Source::rate(const std::array<int, 3>& ijk, SourceComponent input) const
{
    const auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = findInSequence(it->second, input);

        return (it2 != it->second.end()) ? it2->rate : 0.0;
    }
    else {
        return 0.0;
    }
}

std::optional<double> Source::hrate(const std::array<int, 3>& ijk, SourceComponent input) const
{
    const auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = findInSequence(it->second, input);

        return (it2 != it->second.end()) ? it2->hrate : std::nullopt;
    }
    else {
        return std::nullopt;
    }
}

std::optional<double> Source::temperature(const std::array<int, 3>& ijk, SourceComponent input) const
{
    const auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = findInSequence(it->second, input);

        return (it2 != it->second.end()) ? it2->temperature : std::nullopt;
    }
    else {
        return std::nullopt;
    }
}

bool Source::operator==(const Source& other) const
{
    return this->m_cells == other.m_cells;
}

} //namespace Opm
