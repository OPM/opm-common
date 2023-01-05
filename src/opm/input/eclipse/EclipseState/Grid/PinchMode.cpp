/*
  Copyright 2015 Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/Grid/PinchMode.hpp>
#include <opm/common/utility/String.hpp>

#include <ostream>
#include <stdexcept>

namespace Opm {

std::string PinchMode2String(const PinchMode enumValue)
{
    std::string stringValue;
    switch (enumValue) {
    case PinchMode::ALL:
        stringValue = "ALL";
        break;

    case PinchMode::TOPBOT:
        stringValue = "TOPBOT";
        break;

    case PinchMode::TOP:
        stringValue = "TOP";
        break;

    case PinchMode::GAP:
        stringValue = "GAP";
        break;
    case PinchMode::NOGAP:
        stringValue = "NOGAP";
        break;
    }

    return stringValue;
}

PinchMode PinchModeFromString(const std::string& stringValue)
{
    std::string s = trim_copy(stringValue);

    PinchMode mode;
    if      (s == "ALL")    { mode = PinchMode::ALL;    }
    else if (s == "TOPBOT") { mode = PinchMode::TOPBOT; }
    else if (s == "TOP")    { mode = PinchMode::TOP;    }
    else if (s == "GAP")    { mode = PinchMode::GAP;    }
    else if (s == "NOGAP")  { mode = PinchMode::NOGAP;    }
    else {
        std::string msg = "Unsupported pinchout mode " + s;
        throw std::invalid_argument(msg);
    }

    return mode;
}

std::ostream& operator<<(std::ostream& os, const PinchMode pm)
{
    return (os << PinchMode2String(pm));
}

}
