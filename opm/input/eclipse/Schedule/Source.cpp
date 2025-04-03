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

namespace Opm {
namespace {

namespace fromstring {
SourceComponent component(const std::string& s) {
    if (s == "OIL")
        return SourceComponent::OIL;

    if (s == "GAS")
        return SourceComponent::GAS;

    if (s == "WATER")
        return SourceComponent::WATER;

    if (s == "SOLVENT")
        return SourceComponent::SOLVENT;

    if (s == "POLYMER")
        return SourceComponent::POLYMER;

    if (s == "MICR")
        return SourceComponent::MICR;

    if (s == "OXYG")
        return SourceComponent::OXYG;

    if (s == "UREA")
        return SourceComponent::UREA;

    if (s == "NONE")
        return SourceComponent::NONE;

    throw std::invalid_argument("Not recognized source component: " + s);
}

}
}

// SourceCell functions
// --------------------
using SOURCEKEY = ParserKeywords::SOURCE;
Source::SourceCell::SourceCell(const DeckRecord& record) :
    component(fromstring::component(record.getItem<SOURCEKEY::COMPONENT>().get<std::string>(0))),
    rate(record.getItem<SOURCEKEY::RATE>().getSIDouble(0)),
    hrate(std::nullopt),
    temperature(std::nullopt)
{

    if (record.getItem<SOURCEKEY::HRATE>().hasValue(0))
        hrate = record.getItem<SOURCEKEY::HRATE>().getSIDouble(0);

    if (record.getItem<SOURCEKEY::TEMP>().hasValue(0))
        temperature = record.getItem<SOURCEKEY::TEMP>().getSIDouble(0);

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

bool Source::SourceCell::operator==(const Source::SourceCell& other) const {
    return this->isSame(other.component) && 
           this->rate == other.rate &&
           this->hrate == other.hrate &&
           this->temperature == other.temperature;
}

bool Source::SourceCell::isSame(const SourceComponent& other) const {
    return this->component == other;
}

// Source functions
// ----------------
void Source::updateSource(const DeckRecord& record)
{
    const Source::SourceCell sourcenew(record);
    std::array<int, 3> ijk {record.getItem<SOURCEKEY::I>().get<int>(0)-1, 
                             record.getItem<SOURCEKEY::J>().get<int>(0)-1,
                             record.getItem<SOURCEKEY::K>().get<int>(0)-1};

    auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        auto it2 = std::find_if(it->second.begin(), it->second.end(),
                               [&sourcenew](const auto& source)
                               {
                                   return source.isSame(sourcenew.component);
                               });
        if (it2 != it->second.end()) {
            *it2 = sourcenew;
        }
        else {
            it->second.emplace_back(sourcenew);
        }
    } else {
        m_cells.insert({ijk, {sourcenew}});
    }
}

Source Source::serializationTestObject()
{
    Source result;
    std::array<int, 3> ijk = {1,1,1};
    result.m_cells = {{ijk, {SourceCell::serializationTestObject()}}};

    return result;
}

std::size_t Source::size() const {
    return this->m_cells.size();
}

std::map<std::array<int, 3>, std::vector<Source::SourceCell>>::const_iterator Source::begin() const {
    return this->m_cells.begin();
}

std::map<std::array<int, 3>, std::vector<Source::SourceCell>>::const_iterator Source::end() const {
    return this->m_cells.end();
}

bool Source::hasSource(const std::array<int, 3>& input) const
{
    return m_cells.find(input) != m_cells.end();
}

double Source::rate(const std::array<int, 3>& ijk, SourceComponent input) const
{
    auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = std::find_if(it->second.begin(), it->second.end(),
                                     [&input](const auto& source)
                                     {
                                         return source.isSame(input);
                                     });
                            
        return (it2 != it->second.end()) ? it2->rate : 0.0;
    }
    else {
        return 0.0;
    }
}

std::optional<double> Source::hrate(const std::array<int, 3>& ijk, SourceComponent input) const
{
    auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = std::find_if(it->second.begin(), it->second.end(),
                                     [&input](const auto& source)
                                     {
                                         return source.isSame(input);
                                     });

        return (it2 != it->second.end()) ? it2->hrate : std::nullopt;
    }
    else {
        return std::nullopt;
    }
}

std::optional<double> Source::temperature(const std::array<int, 3>& ijk, SourceComponent input) const
{
    auto it = m_cells.find(ijk);
    if (it != m_cells.end()) {
        const auto it2 = std::find_if(it->second.begin(), it->second.end(),
                                     [&input](const auto& source)
                                     {
                                         return source.isSame(input);
                                     });

        return (it2 != it->second.end()) ? it2->temperature : std::nullopt;
    }
    else {
        return std::nullopt;
    }
}

bool Source::operator==(const Source& other) const {
    return this->m_cells == other.m_cells;
}

} //namespace Opm
