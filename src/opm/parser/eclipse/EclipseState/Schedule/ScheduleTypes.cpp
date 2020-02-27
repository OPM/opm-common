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
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdexcept>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleTypes.hpp>

namespace Opm {

const std::string InjectorType2String( InjectorType enumValue ) {
    switch( enumValue ) {
    case InjectorType::OIL:
        return "OIL";
    case InjectorType::GAS:
        return "GAS";
    case InjectorType::WATER:
        return "WATER";
    case InjectorType::MULTI:
        return "MULTI";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

InjectorType InjectorTypeFromString( const std::string& stringValue ) {
    if (stringValue == "OIL")
        return InjectorType::OIL;
    else if (stringValue == "WATER")
        return InjectorType::WATER;
    else if (stringValue == "WAT")
        return InjectorType::WATER;
    else if (stringValue == "GAS")
        return InjectorType::GAS;
    else if (stringValue == "MULTI")
        return InjectorType::MULTI;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}}
