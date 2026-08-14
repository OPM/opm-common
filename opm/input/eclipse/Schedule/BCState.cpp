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
#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Schedule/BCState.hpp>

#include <algorithm>
#include <string>

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

    if (s == "NONE")
        return BCType::NONE;

    throw std::invalid_argument("Not recognized boundary condition type: " + s);
}

BCMECHType bcmechtype(const std::string& s) {
    if (s == "FREE")
        return BCMECHType::FREE;

    if (s == "FIXED")
        return BCMECHType::FIXED;

    if (s == "NONE")
        return BCMECHType::NONE;

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

    if (s == "MICR")
        return BCComponent::MICR;

    if (s == "OXYG")
        return BCComponent::OXYG;

    if (s == "UREA")
        return BCComponent::UREA;

    if (s == "NONE")
        return BCComponent::NONE;

    throw std::invalid_argument("Not recognized boundary condition compononet: " + s);
}

}
}

using BCKEY = ParserKeywords::BCPROP;
BCState::BCFace BCState::BCFace::fromBCProp(const DeckRecord& record)
{
    BCFace bcpropface;
    bcpropface.index = record.getItem<BCKEY::INDEX>().get<int>(0);
    bcpropface.bctype = fromstring::bctype(record.getItem<BCKEY::TYPE>().get<std::string>(0));
    bcpropface.component = fromstring::component(record.getItem<BCKEY::COMPONENT>().get<std::string>(0));
    bcpropface.rate = record.getItem<BCKEY::RATE>().getSIDouble(0);
    if (const auto& P = record.getItem<BCKEY::PRESSURE>(); ! P.defaultApplied(0)) {
        bcpropface.pressure = P.getSIDouble(0);
    }
    if (const auto& T = record.getItem<BCKEY::TEMPERATURE>(); ! T.defaultApplied(0)) {
        bcpropface.temperature = T.getSIDouble(0);
    }
    return bcpropface;
}

using BCMECHKEY = ParserKeywords::BCMECH;
BCState::BCFace BCState::BCFace::fromBCMech(const DeckRecord& record)
{
    BCFace bcmechface;
    bcmechface.index = record.getItem<BCMECHKEY::INDEX>().get<int>(0);
    bcmechface.bcmechtype =
        fromstring::bcmechtype(record.getItem<BCMECHKEY::MECHTYPE>().get<std::string>(0));

    MechBCValue mechbcvaluetmp;
    if (const auto& P = record.getItem<BCMECHKEY::STRESSXX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[0] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BCMECHKEY::STRESSYY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[1] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BCMECHKEY::STRESSZZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[2] = P.getSIDouble(0);
    }
    mechbcvaluetmp.stress[3] = 0;
    mechbcvaluetmp.stress[4] = 0;
    mechbcvaluetmp.stress[5] = 0;
    if (const auto& P = record.getItem<BCMECHKEY::DISPX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[0] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BCMECHKEY::DISPY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[1] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BCMECHKEY::DISPZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[2] = P.getSIDouble(0);
    }
    mechbcvaluetmp.fixeddir[0] = record.getItem<BCMECHKEY::FIXEDX>().get<int>(0);
    mechbcvaluetmp.fixeddir[1] = record.getItem<BCMECHKEY::FIXEDY>().get<int>(0);
    mechbcvaluetmp.fixeddir[2] = record.getItem<BCMECHKEY::FIXEDZ>().get<int>(0);
    bcmechface.mechbcvalue = mechbcvaluetmp;

    return bcmechface;
}

BCState::BCFace BCState::BCFace::serializationTestObject()
{
    BCFace result;
    result.index = 100;
    result.bctype = BCType::RATE;
    result.bcmechtype = BCMECHType::FIXED;
    result.component = BCComponent::GAS;
    result.rate = 101.0;
    result.pressure = 102.0;
    result.temperature = 103.0;
    result.mechbcvalue = MechBCValue::serializationTestObject();
    return result;
}


bool BCState::BCFace::operator==(const BCState::BCFace& other) const {
    return this->index == other.index &&
           this->bctype == other.bctype &&
           this->bcmechtype == other.bcmechtype &&
           this->component == other.component &&
           this->rate == other.rate &&
           this->pressure == other.pressure &&
           this->temperature == other.temperature &&
           this->mechbcvalue == other.mechbcvalue;
}



void BCState::updateBCProp(const DeckRecord& record)
{
    const BCFace bcnew = BCFace::fromBCProp(record);
    auto it = std::ranges::find_if(m_faces,
                                   [&bcnew](const auto& bc)
                                   {
                                       return bc.index == bcnew.index &&
                                              bc.component == bcnew.component;
                                   });
    if (it != m_faces.end()) {
        it->bctype = bcnew.bctype;
        it->component = bcnew.component;
        it->rate = bcnew.rate;
        it->pressure = bcnew.pressure;
        it->temperature = bcnew.temperature;
    } else {
        this->m_faces.emplace_back(bcnew);
    }
}

void BCState::updateBCMech(const DeckRecord& record)
{
    const BCFace bcnew = BCFace::fromBCMech(record);
    auto it = std::ranges::find_if(m_faces,
                                   [&bcnew](const auto& bc)
                                   {
                                       return bc.index == bcnew.index;
                                   });
    if (it != m_faces.end()) {
        it->bcmechtype = bcnew.bcmechtype;
        it->mechbcvalue = bcnew.mechbcvalue;
    } else {
        this->m_faces.emplace_back(bcnew);
    }
}


BCState BCState::serializationTestObject()
{
    BCState result;
    result.m_faces = {BCFace::serializationTestObject()};

    return result;
}


std::size_t BCState::size() const {
    return this->m_faces.size();
}

std::vector<BCState::BCFace>::const_iterator BCState::begin() const {
    return this->m_faces.begin();
}

std::vector<BCState::BCFace>::const_iterator BCState::end() const {
    return this->m_faces.end();
}

const BCState::BCFace& BCState::operator[](int index) const
{
    const auto it = std::ranges::find_if(m_faces,
                                         [index](const auto& bc)
                                         { return bc.index == index; });

    if (it != m_faces.end()) {
        return *it;
    }

    // add throw
    return this->m_faces[0];
}

bool BCState::operator==(const BCState& other) const {
    return this->m_faces == other.m_faces;
}


} //namespace Opm
