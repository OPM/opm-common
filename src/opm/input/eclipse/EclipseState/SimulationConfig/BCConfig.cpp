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
#include <opm/common/utility/OpmInputError.hpp>


namespace Opm {

using BC = ParserKeywords::BCCON;
BCConfig::BCRegion::BCRegion(const DeckRecord& record, const GridDims& grid) :
    index(record.getItem<BC::INDEX>().get<int>(0)),
    i1(0),
    i2(grid.getNX() - 1),
    j1(0),
    j2(grid.getNY() - 1),
    k1(0),
    k2(grid.getNZ() - 1),
    dir(FaceDir::FromString(record.getItem<BC::DIRECTION>().get<std::string>(0)))
{
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
    //
    MechBCValue mechbcvaluetmp;
    if (const auto& P = record.getItem<BC::STRESSXX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[0] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BC::STRESSYY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[1] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BC::STRESSZZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.stress[2] = P.getSIDouble(0);
    }
    mechbcvaluetmp.stress[3] = 0;
    mechbcvaluetmp.stress[4] = 0;
    mechbcvaluetmp.stress[5] = 0;
    if (const auto& P = record.getItem<BC::DISPX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[0] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BC::DISPY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[1] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BC::DISPZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.disp[2] = P.getSIDouble(0);
    }
    if (const auto& P = record.getItem<BC::FIXEDX>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[0] = P.get<int>(0);
    }
    if (const auto& P = record.getItem<BC::FIXEDY>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[1] = P.get<int>(0);
    }
    if (const auto& P = record.getItem<BC::FIXEDZ>(); ! P.defaultApplied(0)) {
        mechbcvaluetmp.fixeddir[2] = P.get<int>(0);
    }
    mechbcvalue = mechbcvaluetmp;
}

BCConfig::BCRegion BCConfig::BCRegion::serializationTestObject()
{
    BCRegion result;
    result.index = 10;
    result.i1 = 12;
    result.i2 = 13;
    result.j1 = 13;
    result.j2 = 14;
    result.k1 = 15;
    result.k2 = 16;
    result.dir = FaceDir::XPlus;

    return result;
}


bool BCConfig::BCRegion::operator==(const BCConfig::BCRegion& other) const {
    return this->index == other.index &&
           this->i1 == other.i1 &&
           this->i2 == other.i2 &&
           this->j1 == other.j1 &&
           this->j2 == other.j2 &&
           this->k1 == other.k1 &&
           this->k2 == other.k2 &&
           this->dir == other.dir;
}


BCConfig::BCConfig(const Deck& deck) {

    if(deck.hasKeyword<ParserKeywords::BC>()){
        for (const auto* keyword : deck.getKeywordList<ParserKeywords::BC>()) {
            const std::string reason = "ERROR: The BC keyword is obsolete. \n "
                            "Instead use BCCON in the GRID section to specify the connections. \n "
                            "And BCPROP in the SCHEDULE section to specify the type and values. \n"
                            "Check the OPM manual for details.";
            throw OpmInputError { reason, keyword->location()};
        }
    }

    GridDims grid( deck );
    for (const auto& kw: deck.getKeywordList<ParserKeywords::BCCON>()) {
        for (const auto& record : *kw)
            this->m_faces.emplace_back( record, grid );
    }
}


BCConfig BCConfig::serializationTestObject()
{
    BCConfig result;
    result.m_faces = {BCRegion::serializationTestObject()};

    return result;
}


std::size_t BCConfig::size() const {
    return this->m_faces.size();
}

std::vector<BCConfig::BCRegion>::const_iterator BCConfig::begin() const {
    return this->m_faces.begin();
}

std::vector<BCConfig::BCRegion>::const_iterator BCConfig::end() const {
    return this->m_faces.end();
}

bool BCConfig::operator==(const BCConfig& other) const {
    return this->m_faces == other.m_faces;
}


} //namespace Opm

