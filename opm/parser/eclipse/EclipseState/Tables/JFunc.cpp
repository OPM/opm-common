/*
 Copyright 2016  Statoil ASA.

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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/JFunc.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/J.hpp>

namespace Opm {

    JFunc::JFunc(const Deck& deck) :
                    m_exists(deck.hasKeyword("JFUNC"))
    {
        if (!m_exists)
            return;
        const auto& kw = *deck.getKeywordList<ParserKeywords::JFUNC>()[0];
        const auto& rec = kw.getRecord(0);
        const auto& kw_flag = rec.getItem("FLAG").get<std::string>(0);
        if (kw_flag == "BOTH")
            flag = Flag::BOTH;
        else if (kw_flag == "WATER")
            flag = Flag::WATER;
        else if (kw_flag == "GAS")
            flag = Flag::GAS;
        else
            throw std::invalid_argument("Illegal JFUNC FLAG, must be BOTH, WATER, or GAS.  Was \"" + kw_flag + "\".");

        if (flag != Flag::WATER)
            goSurfaceTension = rec.getItem("GO_SURFACE_TENSION").get<double>(0);

        if (flag != Flag::GAS)
            owSurfaceTension = rec.getItem("OW_SURFACE_TENSION").get<double>(0);

        alphaFactor = rec.getItem("ALPHA_FACTOR").get<double>(0);
        betaFactor = rec.getItem("BETA_FACTOR").get<double>(0);

        const auto kw_dir = rec.getItem("DIRECTION").get<std::string>(0);
        if (kw_dir == "XY")
            direction = Direction::XY;
        else if (kw_dir == "X")
            direction = Direction::X;
        else if (kw_dir == "Y")
            direction = Direction::Y;
        else if (kw_dir == "Z")
            direction = Direction::Z;
        else
            throw std::invalid_argument("Illegal JFUNC DIRECTION, must be XY, X, Y, or Z.  Was \"" + kw_dir + "\".");
    }

    double JFunc::getAlphaFactor() const {
        return alphaFactor;
    }

    double JFunc::getBetaFactor() const {
        return betaFactor;
    }

    double JFunc::getgoSurfaceTension() const {
        if (flag == JFunc::Flag::WATER)
            throw std::invalid_argument("Cannot get gas-oil with WATER JFUNC");
        return goSurfaceTension;
    }

    double JFunc::getowSurfaceTension() const {
        if (flag == JFunc::Flag::GAS)
            throw std::invalid_argument("Cannot get oil-water with GAS JFUNC");
        return owSurfaceTension;
    }

    const JFunc::Flag& JFunc::getJFuncFlag() const {
        return flag;
    }

    const JFunc::Direction& JFunc::getDirection() const {
        return direction;
    }
} // Opm::
