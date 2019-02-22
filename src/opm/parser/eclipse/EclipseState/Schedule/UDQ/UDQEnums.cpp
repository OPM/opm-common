/*
  Copyright 2019 Equinor ASA.

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

#include <string>
#include <stdexcept>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>


namespace Opm {
namespace UDQ {

UDQVarType varType(const std::string& keyword) {
    if (keyword[1] != 'U')
        throw std::invalid_argument("Keyword: " + keyword + " is not of UDQ type");

    char first_char =  keyword[0];
    switch(first_char) {
    case 'W':
        return UDQVarType::WELL_VAR;
    case 'G':
        return UDQVarType::GROUP_VAR;
    case 'C':
        return UDQVarType::CONNECTION_VAR;
    case 'R':
        return UDQVarType::REGION_VAR;
    case 'F':
        return UDQVarType::FIELD_VAR;
    case 'S':
        return UDQVarType::SEGMENT_VAR;
    case 'A':
        return UDQVarType::AQUIFER_VAR;
    case 'B':
        return UDQVarType::BLOCK_VAR;
    default:
        throw std::invalid_argument("Keyword: " + keyword + " is not of UDQ type");
    }

}

}
}
