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

    if (s == "NONE")
        return SourceComponent::NONE;

    throw std::invalid_argument("Not recognized source component: " + s);
}

}
}

using SOURCEKEY = ParserKeywords::SOURCE;
Source::SourceCell::SourceCell(const DeckRecord& record) :
    ijk({record.getItem<SOURCEKEY::I>().get<int>(0)-1,
        record.getItem<SOURCEKEY::J>().get<int>(0)-1,
        record.getItem<SOURCEKEY::K>().get<int>(0)-1}),
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
    result.ijk = {1,1,1};
    result.component = SourceComponent::GAS;
    result.rate = 101.0;
    result.hrate = 201.0;
    result.temperature = 202.0;
    return result;
}


bool Source::SourceCell::operator==(const Source::SourceCell& other) const {
    return this->isSame(other) && 
           this->rate == other.rate &&
           this->hrate == other.hrate &&
           this->temperature == other.temperature;
}

bool Source::SourceCell::isSame(const Source::SourceCell& other) const {
    return this->ijk == other.ijk &&
           this->component == other.component;
}

bool Source::SourceCell::isSame(const std::pair<std::array<int, 3>, SourceComponent>& other) const {
    return this->ijk == other.first &&
           this->component == other.second;
}


void Source::updateSource(const DeckRecord& record) {
    const Source::SourceCell sourcenew( record );
    for (auto& source : m_cells) {
        if (source.isSame(sourcenew))
            {
                source = sourcenew;
                return;
            }
    }
    this->m_cells.emplace_back( sourcenew );
}



Source Source::serializationTestObject()
{
    Source result;
    result.m_cells = {SourceCell::serializationTestObject()};

    return result;
}


std::size_t Source::size() const {
    return this->m_cells.size();
}

std::vector<Source::SourceCell>::const_iterator Source::begin() const {
    return this->m_cells.begin();
}

std::vector<Source::SourceCell>::const_iterator Source::end() const {
    return this->m_cells.end();
}

bool Source::hasSource(const std::array<int, 3>& input) const
{
    return std::any_of(m_cells.begin(), m_cells.end(),
                       [&input](const auto& source)
                       {
                           return source.ijk == input;
                       });
}

double Source::rate(const std::pair<std::array<int, 3>, SourceComponent>& input) const {
    for (const auto& source : m_cells) {
        if (source.isSame(input))
            {
                return source.rate;
            }
    }
    return 0.0;
}

double Source::hrate(const std::pair<std::array<int, 3>, SourceComponent>& input) const {
    for (auto& source : m_cells) {
        if (source.isSame(input))
            {
                return source.hrate.value();
            }
    }
    return 0.0;
}

bool Source::hasHrate(const std::pair<std::array<int, 3>, SourceComponent>& input) const {
    for (auto& source : m_cells) {
        if (source.isSame(input))
            {
                return source.hrate.has_value();
            }
    }
    return false;
}

bool Source::hasTemperature(const std::pair<std::array<int, 3>, SourceComponent>& input) const {
    for (auto& source : m_cells) {
        if (source.isSame(input))
            {
                return source.temperature.has_value();
            }
    }
    return false;
}
double Source::temperature(const std::pair<std::array<int, 3>, SourceComponent>& input) const {
    for (auto& source : m_cells) {
        if (source.isSame(input))
            {
                return source.temperature.value();
            }
    }
    return 0.0;
}

bool Source::operator==(const Source& other) const {
    return this->m_cells == other.m_cells;
}

} //namespace Opm
