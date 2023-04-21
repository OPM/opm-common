/*
  Copyright 2020 Equinor ASA.
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

#include <string>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Schedule/BC.hpp>

namespace Opm {
namespace {

namespace fromstring {

BCType bctype(const std::string& s) {
    if (s == "RATE")
        return BCType::RATE;

    if (s == "FREE")
        return BCType::FREE;

    if (s == "DIRICHLET")
        return BCType::DIRICHLET;

    if (s == "THERMAL")
        return BCType::THERMAL;

    if (s == "CLOSED")
        return BCType::CLOSED;

    throw std::invalid_argument("Not recognized boundary condition type: " + s);
}


BCComponent component(const std::string& s) {
    if (s == "OIL")
        return BCComponent::OIL;

    if (s == "GAS")
        return BCComponent::GAS;

    if (s == "WATER")
        return BCComponent::WATER;

    if (s == "SOLVENT")
        return BCComponent::SOLVENT;

    if (s == "POLYMER")
        return BCComponent::POLYMER;

    if (s == "NONE")
        return BCComponent::NONE;

    throw std::invalid_argument("Not recognized boundary condition compononet: " + s);
}

}
}

using BCKEY = ParserKeywords::BC;
BCVAL::BCFace::BCFace(const DeckRecord& record) :
    index(record.getItem<BCKEY::INDEX>().get<int>(0)),
    bctype(fromstring::bctype(record.getItem<BCKEY::TYPE>().get<std::string>(0))),
    component(fromstring::component(record.getItem<BCKEY::COMPONENT>().get<std::string>(0))),
    rate(record.getItem<BCKEY::RATE>().getSIDouble(0))
{
    if (const auto& P = record.getItem<BCKEY::PRESSURE>(); ! P.defaultApplied(0)) {
        pressure = P.getSIDouble(0);
    }
    if (const auto& T = record.getItem<BCKEY::TEMPERATURE>(); ! T.defaultApplied(0)) {
        temperature = T.getSIDouble(0);
    }
}

BCVAL::BCFace BCVAL::BCFace::serializationTestObject()
{
    BCFace result;
    result.index = 100;
    result.bctype = BCType::RATE;
    result.component = BCComponent::GAS;
    result.rate = 101.0;
    result.pressure = 102.0;
    result.temperature = 103.0;

    return result;
}


bool BCVAL::BCFace::operator==(const BCVAL::BCFace& other) const {
    return this->index == other.index &&
           this->bctype == other.bctype &&
           this->component == other.component &&
           this->rate == other.rate &&
           this->pressure == other.pressure &&
           this->temperature == other.temperature;
}



BCVAL::BCVAL(const Deck& /*deck*/) {
// TODO Read the indices from BCCON and initial with NONE type???
}

void BCVAL::updateBC(const DeckRecord& record) {
    const BCVAL::BCFace bcnew( record );
    for (auto& bc : m_faces) {
        if (bc.index == bcnew.index)
            {
                bc = bcnew;
                return;
            }
    }
    this->m_faces.emplace_back( bcnew );   
}



BCVAL BCVAL::serializationTestObject()
{
    BCVAL result;
    result.m_faces = {BCFace::serializationTestObject()};

    return result;
}


std::size_t BCVAL::size() const {
    return this->m_faces.size();
}

std::vector<BCVAL::BCFace>::const_iterator BCVAL::begin() const {
    return this->m_faces.begin();
}

std::vector<BCVAL::BCFace>::const_iterator BCVAL::end() const {
    return this->m_faces.end();
}

BCVAL::BCFace BCVAL::operator[](std::size_t index) const {
    for (auto& bc : m_faces) {
        if (bc.index == index)
            {
                return bc;
            }
    }
    // add throw
    return this->m_faces[0];
}

bool BCVAL::operator==(const BCVAL& other) const {
    return this->m_faces == other.m_faces;
}


} //namespace Opm

