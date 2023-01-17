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

#ifndef WELL_ENUMS_HPP
#define WELL_ENUMS_HPP

#include <iosfwd>
#include <string>

namespace Opm {

enum class WellStatus {
    OPEN = 1,
    STOP = 2,
    SHUT = 3,
    AUTO = 4
};

/*
  The elements in this enum are used as bitmasks to keep track
  of which controls are present, i.e. the 2^n structure must
  be intact.
*/
enum class WellInjectorCMode : int{
    RATE =  1 ,
    RESV =  2 ,
    BHP  =  4 ,
    THP  =  8 ,
    GRUP = 16 ,
    CMODE_UNDEFINED = 512
};

/*
  The items BHP, THP and GRUP only apply in prediction mode:
  WCONPROD. The elements in this enum are used as bitmasks to
  keep track of which controls are present, i.e. the 2^n
  structure must be intact. The NONE item is only used in
  WHISTCTL to cancel its effect.
*/

enum class WellProducerCMode : int {
    NONE =     0,
    ORAT =     1,
    WRAT =     2,
    GRAT =     4,
    LRAT =     8,
    CRAT =    16,
    RESV =    32,
    BHP  =    64,
    THP  =   128,
    GRUP =   256,
    CMODE_UNDEFINED = 1024
};

enum class WellWELTARGCMode {
    ORAT =  1,
    WRAT =  2,
    GRAT =  3,
    LRAT =  4,
    CRAT =  5,   // Not supported
    RESV =  6,
    BHP  =  7,
    THP  =  8,
    VFP  =  9,
    LIFT = 10,   // Not supported
    GUID = 11
};

enum class WellGuideRateTarget {
    OIL = 0,
    WAT = 1,
    GAS = 2,
    LIQ = 3,
    COMB = 4,
    WGA = 5,
    CVAL = 6,
    RAT = 7,
    RES = 8,
    UNDEFINED = 9
};

enum class WellGasInflowEquation {
    STD = 0,
    R_G = 1,
    P_P = 2,
    GPP = 3
};

std::string WellStatus2String(WellStatus enumValue);
WellStatus WellStatusFromString(const std::string& stringValue);
std::ostream& operator<<(std::ostream& os, const WellStatus& st);

std::string WellInjectorCMode2String(WellInjectorCMode enumValue);
WellInjectorCMode WellInjectorCModeFromString(const std::string& stringValue);
std::ostream& operator<<(std::ostream& os, const WellInjectorCMode& cm);

std::string WellProducerCMode2String(WellProducerCMode enumValue);
WellProducerCMode WellProducerCModeFromString(const std::string& stringValue);
std::ostream& operator<<(std::ostream& os, const WellProducerCMode& cm);

WellWELTARGCMode WellWELTARGCModeFromString(const std::string& stringValue);

std::string WellGuideRateTarget2String(WellGuideRateTarget enumValue);
WellGuideRateTarget WellGuideRateTargetFromString(const std::string& stringValue);

std::string WellGasInflowEquation2String(WellGasInflowEquation enumValue);
WellGasInflowEquation WellGasInflowEquationFromString(const std::string& stringValue);

}

#endif
