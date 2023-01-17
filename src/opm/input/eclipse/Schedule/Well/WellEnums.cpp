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

#include <config.h>
#include <opm/input/eclipse/Schedule/Well/WellEnums.hpp>

#include <ostream>
#include <stdexcept>

namespace Opm {

std::string WellStatus2String(WellStatus enumValue)
{
    switch(enumValue) {
    case WellStatus::OPEN:
        return "OPEN";
    case WellStatus::SHUT:
        return "SHUT";
    case WellStatus::AUTO:
        return "AUTO";
    case WellStatus::STOP:
        return "STOP";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

WellStatus WellStatusFromString(const std::string& stringValue)
{
    if (stringValue == "OPEN")
        return WellStatus::OPEN;
    else if (stringValue == "SHUT")
        return WellStatus::SHUT;
    else if (stringValue == "STOP")
        return WellStatus::STOP;
    else if (stringValue == "AUTO")
        return WellStatus::AUTO;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

std::ostream& operator<<(std::ostream& os, const WellStatus& st)
{
    os << WellStatus2String(st);
    return os;
}

}
