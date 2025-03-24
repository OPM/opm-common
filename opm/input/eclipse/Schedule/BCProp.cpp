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
#include <opm/input/eclipse/Schedule/BCProp.hpp>

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
BCProp::BCFace::BCFace(const DeckRecord& record) :
    index(record.getItem<BCKEY::INDEX>().get<int>(0)),
    bctype(fromstring::bctype(record.getItem<BCKEY::TYPE>().get<std::string>(0))),
    bcmechtype(fromstring::bcmechtype(record.getItem<BCKEY::MECHTYPE>().get<std::string>(0))),
    component(fromstring::component(record.getItem<BCKEY::COMPONENT>().get<std::string>(0))),
    rate(record.getItem<BCKEY::RATE>().getSIDouble(0))
{
    if (const auto& P = record.getItem<BCKEY::PRESSURE>(); ! P.defaultApplied(0)) {
        pressure = P.getSIDouble(0);
    }
    if (const auto& T = record.getItem<BCKEY::TEMPERATURE>(); ! T.defaultApplied(0)) {
        temperature = T.getSIDouble(0);
    }

    MechBCValue mechbcvaluetmp;
    bool allDefault = true;
    if (const auto& P = record.getItem<BCKEY::STRESSXX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[0] = P.getSIDouble(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::STRESSYY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[1] = P.getSIDouble(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::STRESSZZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[2] = P.getSIDouble(0);
        allDefault = false;
    }
    mechbcvaluetmp.stress[3] = 0;
    mechbcvaluetmp.stress[4] = 0;
    mechbcvaluetmp.stress[5] = 0;
    if (const auto& P = record.getItem<BCKEY::DISPX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[0] = P.getSIDouble(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::DISPY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[1] = P.getSIDouble(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::DISPZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[2] = P.getSIDouble(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::FIXEDX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[0] = P.get<int>(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::FIXEDY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[1] = P.get<int>(0);
        allDefault = false;
    }
    if (const auto& P = record.getItem<BCKEY::FIXEDZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[2] = P.get<int>(0);
        allDefault = false;
    }
    if (!allDefault) {
        mechbcvalue = mechbcvaluetmp;
    }
}

BCProp::BCFace BCProp::BCFace::serializationTestObject()
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


bool BCProp::BCFace::operator==(const BCProp::BCFace& other) const {
    return this->index == other.index &&
           this->bctype == other.bctype &&
           this->bcmechtype == other.bcmechtype &&
           this->component == other.component &&
           this->rate == other.rate &&
           this->pressure == other.pressure &&
           this->temperature == other.temperature &&
           this->mechbcvalue == other.mechbcvalue;
}



void BCProp::updateBCProp(const DeckRecord& record)
{
    const BCProp::BCFace bcnew(record);
    auto it = std::find_if(m_faces.begin(), m_faces.end(),
                           [&bcnew](const auto& bc)
                           {
                               return bc.index == bcnew.index &&
                                      bc.component == bcnew.component;
                           });
    if (it != m_faces.end()) {
        *it = bcnew;
    } else {
        this->m_faces.emplace_back(bcnew);
    }
}



BCProp BCProp::serializationTestObject()
{
    BCProp result;
    result.m_faces = {BCFace::serializationTestObject()};

    return result;
}


std::size_t BCProp::size() const {
    return this->m_faces.size();
}

std::vector<BCProp::BCFace>::const_iterator BCProp::begin() const {
    return this->m_faces.begin();
}

std::vector<BCProp::BCFace>::const_iterator BCProp::end() const {
    return this->m_faces.end();
}

const BCProp::BCFace& BCProp::operator[](int index) const
{
    const auto it = std::find_if(m_faces.begin(), m_faces.end(),
                                [index](const auto& bc)
                                 {
                                     return bc.index == index;
                                 });

    if (it != m_faces.end()) {
        return *it;
    }

    // add throw
    return this->m_faces[0];
}

bool BCProp::operator==(const BCProp& other) const {
    return this->m_faces == other.m_faces;
}


} //namespace Opm
