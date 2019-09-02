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

    /*****************************************************************/


    /*****************************************************************/

    namespace GroupProduction {


        GuideRateDef GetGuideRateFromString( const std::string& stringValue ) {

            if (stringValue == "OIL")
                return GuideRateDef::OIL;
            else if (stringValue == "WAT")
                return GuideRateDef::WAT;
            else if (stringValue == "GAS")
                return GuideRateDef::GAS;
            else if (stringValue == "LIQ")
                return GuideRateDef::LIQ;
            else if (stringValue == "COMB")
                return GuideRateDef::COMB;
            else if (stringValue == "WGA")
                return GuideRateDef::WGA;
            else if (stringValue == "CVAL")
                return GuideRateDef::CVAL;
            else if (stringValue == "INJV")
                return GuideRateDef::INJV;
            else if (stringValue == "POTN")
                return GuideRateDef::POTN;
            else if (stringValue == "FORM")
                return GuideRateDef::FORM;
            else
                return GuideRateDef::NO_GUIDE_RATE;

        }

    }

    /*****************************************************************/
    namespace GroupProductionExceedLimit {

    }

    /*****************************************************************/

    namespace WellProducer {
    }


    namespace WellInjector {
        /*****************************************************************/

        
    }



    namespace GuideRate {

        const std::string GuideRatePhaseEnum2String( GuideRatePhaseEnum enumValue ) {
            switch( enumValue ) {
            case OIL:
                return "OIL";
            case WAT:
                return "WAT";
            case GAS:
                return "GAS";
            case LIQ:
                return "LIQ";
            case COMB:
                return "COMB";
            case WGA:
                return "WGA";
            case CVAL:
                return "CVAL";
            case RAT:
                return "RAT";
            case RES:
                return "RES";
            case UNDEFINED:
                return "UNDEFINED";
            default:
                throw std::invalid_argument("unhandled enum value");
            }
        }

        GuideRatePhaseEnum GuideRatePhaseEnumFromString( const std::string& stringValue ) {

            if (stringValue == "OIL")
                return OIL;
            else if (stringValue == "WAT")
                return WAT;
            else if (stringValue == "GAS")
                return GAS;
            else if (stringValue == "LIQ")
                return LIQ;
            else if (stringValue == "COMB")
                return COMB;
            else if (stringValue == "WGA")
                return WGA;
            else if (stringValue == "CVAL")
                return CVAL;
            else if (stringValue == "RAT")
                return RAT;
            else if (stringValue == "RES")
                return RES;
            else if (stringValue == "UNDEFINED")
                return UNDEFINED;
            else
                throw std::invalid_argument("Unknown enum state string: " + stringValue );
        }
    }



    GuideRateTarget GuideRateTargetFromString(const std::string& s) {
        if (s == "OIL")
            return GuideRateTarget::OIL;

        if (s == "LIQ")
            return GuideRateTarget::LIQ;

        if (s == "GAS")
            return GuideRateTarget::GAS;

        if (s == "RES")
            return GuideRateTarget::RES;

        if (s == "COMB")
            return GuideRateTarget::COMB;

        if (s == "NONE")
            return GuideRateTarget::NONE;

        throw std::invalid_argument("Could not convert: " + s + " to a valid GuideRateTarget enum value");
    }
}
