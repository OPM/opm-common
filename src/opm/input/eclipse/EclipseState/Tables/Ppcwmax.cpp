/*
  Copyright 2023 NORCE

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
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Ppcwmax.hpp>

#include <iostream>

namespace Opm {

Ppcwmax::Ppcwmax(const Deck& deck) {
    using PPCWMAX = ParserKeywords::PPCWMAX;
    if (!deck.hasKeyword<PPCWMAX>())
        return;
    
    const auto& keyword = deck.get<PPCWMAX>().back();
    for (const auto& record : keyword) {
        // Get first column value, which is max. allowable capillary pressure
        const double max_cap_pres = record.getItem<PPCWMAX::MAXIMUM_CAPILLARY_PRESSURE>().getSIDouble(0);

        // Store second column value as bool (yes/no = true/false)
        bool option;
        const auto& optn = record.getItem<PPCWMAX::MODIFY_CONNATE_SATURATION>().get< std::string >(0);
        if (optn == "YES")
            option = true;
        else if (optn == "NO")
            option = false;
        else
            throw std::runtime_error("Second column input in PPCWMAX must be YES or NO!");
        
        // Store input data
        this->data.emplace_back(max_cap_pres, option);
    }
}

bool Ppcwmax::empty() const {
    return this->data.empty();
}

std::size_t Ppcwmax::size() const {
    return this->data.size();
}

Ppcwmax Ppcwmax::serializationTestObject() {
    Ppcwmax ppcwmax;
    return ppcwmax;
}


const PpcwmaxRecord& Ppcwmax::operator[](const std::size_t index) const {
    return this->data.at(index);
}

}
