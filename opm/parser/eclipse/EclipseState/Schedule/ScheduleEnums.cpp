/*
  Copyright 2013 Statoil ASA.

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

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm {

    const std::string CompletionStateEnum2String( CompletionStateEnum enumValue ) {
        switch( enumValue ) {
        case OPEN:
            return "OPEN";
        case AUTO:
            return "AUTO";
        case SHUT:
            return "SHUT";
        default:
          throw std::invalid_argument("Unhandled enum value");
        }
    }
    
    
    CompletionStateEnum CompletionStateEnumFromString( const std::string& stringValue ) {
        if (stringValue == "OPEN")
            return OPEN;
        else if (stringValue == "SHUT")
            return SHUT;
        else if (stringValue == "AUTO")
            return AUTO;
        else
            throw std::invalid_argument("Unknown enum state string: " + stringValue );
    }

    /*****************************************************************/
    
    const std::string GroupInjectionControlEnum2String( GroupInjectionControlEnum enumValue ) {
        switch( enumValue ) {
        case NONE:
            return "NONE";
        case RATE:
            return "RATE";
        case RESV:
            return "RESV";
        case REIN:
            return "REIN";
        case VREP:
            return "VREP";
        case FLD:
            return "FLD";
        default:
          throw std::invalid_argument("Unhandled enum value");
        }
    }
    
    
    GroupInjectionControlEnum GroupInjectionControlEnumFromString( const std::string& stringValue ) {
        if (stringValue == "NONE")
            return NONE;
        else if (stringValue == "RATE")
            return RATE;
        else if (stringValue == "RESV")
            return RESV;
        else if (stringValue == "REIN")
            return REIN;
        else if (stringValue == "VREP")
            return VREP;
        else if (stringValue == "FLD")
            return FLD;
        else
            throw std::invalid_argument("Unknown enum state string: " + stringValue );
    }

    /*****************************************************************/

    const std::string PhaseEnum2String( PhaseEnum enumValue ) {
        switch( enumValue ) {
        case OIL:
            return "OIL";
        case GAS:
            return "GAS";
        case WATER:
            return "WATER";
        default:
          throw std::invalid_argument("unhandled enum value");
        }
    }
    
    PhaseEnum PhaseEnumFromString( const std::string& stringValue ) {
        if (stringValue == "OIL")
            return OIL;
        else if (stringValue == "WATER")
            return WATER;
        else if (stringValue == "GAS")
            return GAS;
        else
            throw std::invalid_argument("Unknown enum state string: " + stringValue );
    }


}

