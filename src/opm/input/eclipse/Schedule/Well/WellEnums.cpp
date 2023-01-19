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
    switch (enumValue) {
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

std::string WellInjectorCMode2String(WellInjectorCMode enumValue)
{
    switch (enumValue) {
    case WellInjectorCMode::RESV:
        return "RESV";
    case WellInjectorCMode::RATE:
        return "RATE";
    case WellInjectorCMode::BHP:
        return "BHP";
    case WellInjectorCMode::THP:
        return "THP";
    case WellInjectorCMode::GRUP:
        return "GRUP";
    default:
        throw std::invalid_argument("Unhandled enum value: " +
                                    std::to_string(static_cast<int>(enumValue)) +
                                    " in WellInjectorCMode2String");
    }
}

WellInjectorCMode WellInjectorCModeFromString(const std::string &stringValue)
{
    if (stringValue == "RATE")
        return WellInjectorCMode::RATE;
    else if (stringValue == "RESV")
        return WellInjectorCMode::RESV;
    else if (stringValue == "BHP")
        return WellInjectorCMode::BHP;
    else if (stringValue == "THP")
        return WellInjectorCMode::THP;
    else if (stringValue == "GRUP")
        return WellInjectorCMode::GRUP;
    else
        throw std::invalid_argument("Unknown control mode string: " + stringValue);
}

std::ostream& operator<<(std::ostream& os, const WellInjectorCMode& cm)
{
    os << WellInjectorCMode2String(cm);
    return os;
}

std::string WellProducerCMode2String(WellProducerCMode enumValue)
{
    switch (enumValue) {
    case WellProducerCMode::ORAT:
        return "ORAT";
    case WellProducerCMode::WRAT:
        return "WRAT";
    case WellProducerCMode::GRAT:
        return "GRAT";
    case WellProducerCMode::LRAT:
        return "LRAT";
    case WellProducerCMode::CRAT:
        return "CRAT";
    case WellProducerCMode::RESV:
        return "RESV";
    case WellProducerCMode::BHP:
        return "BHP";
    case WellProducerCMode::THP:
        return "THP";
    case WellProducerCMode::GRUP:
        return "GRUP";
    default:
        throw std::invalid_argument("Unhandled enum value: " +
                                    std::to_string(static_cast<int>(enumValue)) +
                                    " in ProducerCMode2String");
    }
}

WellProducerCMode WellProducerCModeFromString(const std::string& stringValue)
{
    if (stringValue == "ORAT")
        return WellProducerCMode::ORAT;
    else if (stringValue == "WRAT")
        return WellProducerCMode::WRAT;
    else if (stringValue == "GRAT")
        return WellProducerCMode::GRAT;
    else if (stringValue == "LRAT")
        return WellProducerCMode::LRAT;
    else if (stringValue == "CRAT")
        return WellProducerCMode::CRAT;
    else if (stringValue == "RESV")
        return WellProducerCMode::RESV;
    else if (stringValue == "BHP")
        return WellProducerCMode::BHP;
    else if (stringValue == "THP")
        return WellProducerCMode::THP;
    else if (stringValue == "GRUP")
        return WellProducerCMode::GRUP;
    else if (stringValue == "NONE")
        return WellProducerCMode::NONE;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue);
}

std::ostream& operator<<(std::ostream& os, const WellProducerCMode& cm)
{
    if (cm == WellProducerCMode::CMODE_UNDEFINED)
        os << "UNDEFINED";
    else
        os << WellProducerCMode2String(cm);
    return os;
}

WellWELTARGCMode WellWELTARGCModeFromString(const std::string& string_value)
{
    if (string_value == "ORAT")
        return WellWELTARGCMode::ORAT;

    if (string_value == "WRAT")
        return WellWELTARGCMode::WRAT;

    if (string_value == "GRAT")
        return WellWELTARGCMode::GRAT;

    if (string_value == "LRAT")
        return WellWELTARGCMode::LRAT;

    if (string_value == "CRAT")
        return WellWELTARGCMode::CRAT;

    if (string_value == "RESV")
        return WellWELTARGCMode::RESV;

    if (string_value == "BHP")
        return WellWELTARGCMode::BHP;

    if (string_value == "THP")
        return WellWELTARGCMode::THP;

    if (string_value == "VFP")
        return WellWELTARGCMode::VFP;

    if (string_value == "LIFT")
        return WellWELTARGCMode::LIFT;

    if (string_value == "GUID")
        return WellWELTARGCMode::GUID;

    throw std::invalid_argument("WELTARG control mode: " + string_value + " not recognized.");
}

std::string WellGuideRateTarget2String(WellGuideRateTarget enumValue)
{
    switch (enumValue) {
    case WellGuideRateTarget::OIL:
        return "OIL";
    case WellGuideRateTarget::WAT:
        return "WAT";
    case WellGuideRateTarget::GAS:
        return "GAS";
    case WellGuideRateTarget::LIQ:
        return "LIQ";
    case WellGuideRateTarget::COMB:
        return "COMB";
    case WellGuideRateTarget::WGA:
        return "WGA";
    case WellGuideRateTarget::CVAL:
        return "CVAL";
    case WellGuideRateTarget::RAT:
        return "RAT";
    case WellGuideRateTarget::RES:
        return "RES";
    case WellGuideRateTarget::UNDEFINED:
        return "UNDEFINED";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

WellGuideRateTarget WellGuideRateTargetFromString(const std::string& stringValue)
{
    if (stringValue == "OIL")
        return WellGuideRateTarget::OIL;
    else if (stringValue == "WAT")
        return WellGuideRateTarget::WAT;
    else if (stringValue == "GAS")
        return WellGuideRateTarget::GAS;
    else if (stringValue == "LIQ")
        return WellGuideRateTarget::LIQ;
    else if (stringValue == "COMB")
        return WellGuideRateTarget::COMB;
    else if (stringValue == "WGA")
        return WellGuideRateTarget::WGA;
    else if (stringValue == "CVAL")
        return WellGuideRateTarget::CVAL;
    else if (stringValue == "RAT")
        return WellGuideRateTarget::RAT;
    else if (stringValue == "RES")
        return WellGuideRateTarget::RES;
    else if (stringValue == "UNDEFINED")
        return WellGuideRateTarget::UNDEFINED;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

std::string WellGasInflowEquation2String(WellGasInflowEquation enumValue)
{
    switch (enumValue) {
    case WellGasInflowEquation::STD:
        return "STD";
    case WellGasInflowEquation::R_G:
        return "R-G";
    case WellGasInflowEquation::P_P:
        return "P-P";
    case WellGasInflowEquation::GPP:
        return "GPP";
    default:
        throw std::invalid_argument("Unhandled enum value");
    }
}

WellGasInflowEquation WellGasInflowEquationFromString(const std::string& stringValue)
{
    if (stringValue == "STD" || stringValue == "NO")
        return WellGasInflowEquation::STD;

    if (stringValue == "R-G" || stringValue == "YES")
        return WellGasInflowEquation::R_G;

    if (stringValue == "P-P")
        return WellGasInflowEquation::P_P;

    if (stringValue == "GPP")
        return WellGasInflowEquation::GPP;

    throw std::invalid_argument("Gas inflow equation type: " + stringValue + " not recognized");
}

}
