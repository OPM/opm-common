/*
  Copyright 2020 Equinor ASA.

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
#include <opm/input/eclipse/EclipseState/SimulationConfig/BCConfig.hpp>

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

using BC = ParserKeywords::BC;
BCConfig::BCFace::BCFace(const DeckRecord& record, const GridDims& grid) :
    i1(0),
    i2(grid.getNX() - 1),
    j1(0),
    j2(grid.getNY() - 1),
    k1(0),
    k2(grid.getNZ() - 1),
    bctype(fromstring::bctype(record.getItem<BC::TYPE>().get<std::string>(0))),
    dir(FaceDir::FromString(record.getItem<BC::DIRECTION>().get<std::string>(0))),
    component(fromstring::component(record.getItem<BC::COMPONENT>().get<std::string>(0))),
    rate(record.getItem<BC::RATE>().getSIDouble(0))
{
    if (const auto& P = record.getItem<BC::PRESSURE>(); ! P.defaultApplied(0)) {
        pressure = P.getSIDouble(0);
    }
    if (const auto& T = record.getItem<BC::TEMPERATURE>(); ! T.defaultApplied(0)) {
        temperature = T.getSIDouble(0);
    }
    if (const auto& I1 = record.getItem<BC::I1>(); ! I1.defaultApplied(0)) {
        this->i1 = I1.get<int>(0) - 1;
    }
    if (const auto& I2 = record.getItem<BC::I2>(); ! I2.defaultApplied(0)) {
        this->i2 = I2.get<int>(0) - 1;
    }
    if (const auto& J1 = record.getItem<BC::J1>(); ! J1.defaultApplied(0)) {
        this->j1 = J1.get<int>(0) - 1;
    }
    if (const auto& J2 = record.getItem<BC::J2>(); ! J2.defaultApplied(0)) {
        this->j2 = J2.get<int>(0) - 1;
    }
    if (const auto& K1 = record.getItem<BC::K1>(); ! K1.defaultApplied(0)) {
        this->k1 = K1.get<int>(0) - 1;
    }
    if (const auto& K2 = record.getItem<BC::K2>(); ! K2.defaultApplied(0)) {
        this->k2 = K2.get<int>(0) - 1;
    }
}

BCConfig::BCFace BCConfig::BCFace::serializationTestObject()
{
    BCFace result;
    result.i1 = 10;
    result.i2 = 11;
    result.j1 = 12;
    result.j2 = 13;
    result.k1 = 14;
    result.k2 = 15;
    result.bctype = BCType::RATE;
    result.dir = FaceDir::XPlus;
    result.component = BCComponent::GAS;
    result.rate = 100.0;
    result.pressure = 101.0;
    result.temperature = 102.0;

    return result;
}


bool BCConfig::BCFace::operator==(const BCConfig::BCFace& other) const {
    return this->i1 == other.i1 &&
           this->i2 == other.i2 &&
           this->j1 == other.j1 &&
           this->j2 == other.j2 &&
           this->k1 == other.k1 &&
           this->k2 == other.k2 &&
           this->bctype == other.bctype &&
           this->dir == other.dir &&
           this->component == other.component &&
           this->rate == other.rate &&
           this->pressure == other.pressure &&
           this->temperature == other.temperature;
}



BCConfig::BCConfig(const Deck& deck) {
    GridDims grid( deck );
    for (const auto& kw: deck.getKeywordList<ParserKeywords::BC>()) {
        for (const auto& record : *kw)
            this->m_faces.emplace_back( record, grid );
    }
}


BCConfig BCConfig::serializationTestObject()
{
    BCConfig result;
    result.m_faces = {BCFace::serializationTestObject()};

    return result;
}


std::size_t BCConfig::size() const {
    return this->m_faces.size();
}

std::vector<BCConfig::BCFace>::const_iterator BCConfig::begin() const {
    return this->m_faces.begin();
}

std::vector<BCConfig::BCFace>::const_iterator BCConfig::end() const {
    return this->m_faces.end();
}

bool BCConfig::operator==(const BCConfig& other) const {
    return this->m_faces == other.m_faces;
}


} //namespace Opm

