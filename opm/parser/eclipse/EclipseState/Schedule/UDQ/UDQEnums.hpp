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


#ifndef UDQ_ENUMS_HPP
#define UDQ_ENUMS_HPP

namespace Opm {

enum class UDQVarType {
    WELL_VAR = 1,
    CONNECTION_VAR= 2,
    FIELD_VAR = 3,
    GROUP_VAR = 4,
    REGION_VAR = 5,
    SEGMENT_VAR = 6,
    AQUIFER_VAR = 7,
    BLOCK_VAR = 8
};


namespace UDQ {

    UDQVarType varType(const std::string& keyword);

}
}

#endif
