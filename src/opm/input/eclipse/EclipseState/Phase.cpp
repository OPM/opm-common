/*
  Copyright 2016  Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <opm/input/eclipse/EclipseState/Phase.hpp>

#include <ostream>
#include <stdexcept>

namespace Opm {

Phase get_phase( const std::string& str ) {
    if( str == "OIL" ) return Phase::OIL;
    if( str == "GAS" ) return Phase::GAS;
    if( str == "WAT" ) return Phase::WATER;
    if( str == "WATER" )   return Phase::WATER;
    if( str == "SOLVENT" ) return Phase::SOLVENT;
    if( str == "POLYMER" ) return Phase::POLYMER;
    if( str == "ENERGY" ) return Phase::ENERGY;
    if( str == "POLYMW" ) return Phase::POLYMW;
    if( str == "FOAM" ) return Phase::FOAM;
    if( str == "BRINE" ) return Phase::BRINE;
    if( str == "ZFRACTION" ) return Phase::ZFRACTION;

    throw std::invalid_argument( "Unknown phase '" + str + "'" );
}

std::ostream& operator<<( std::ostream& stream, const Phase& p ) {
    switch( p ) {
        case Phase::OIL:     return stream << "OIL";
        case Phase::GAS:     return stream << "GAS";
        case Phase::WATER:   return stream << "WATER";
        case Phase::SOLVENT: return stream << "SOLVENT";
        case Phase::POLYMER: return stream << "POLYMER";
        case Phase::ENERGY:  return stream << "ENERGY";
        case Phase::POLYMW:  return stream << "POLYMW";
        case Phase::FOAM:    return stream << "FOAM";
        case Phase::BRINE:   return stream << "BRINE";
        case Phase::ZFRACTION:    return stream << "ZFRACTION";

    }

    return stream;
}

}
