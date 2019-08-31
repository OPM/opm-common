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

    namespace GroupInjection {

        const std::string ControlEnum2String( ControlEnum enumValue ) {
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


        ControlEnum ControlEnumFromString( const std::string& stringValue ) {
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

    }

    /*****************************************************************/

    namespace GroupProduction {

        const std::string ControlEnum2String( ControlEnum enumValue ) {
            switch( enumValue ) {
            case NONE:
                return "NONE";
            case ORAT:
                return "ORAT";
            case WRAT:
                return "WRAT";
            case GRAT:
                return "GRAT";
            case LRAT:
                return "LRAT";
            case CRAT:
                return "CRAT";
            case RESV:
                return "RESV";
            case PRBL:
                return "PRBL";
            case FLD:
                return "FLD";
            default:
                throw std::invalid_argument("Unhandled enum value");
            }
        }


        ControlEnum ControlEnumFromString( const std::string& stringValue ) {
            if (stringValue == "NONE")
                return NONE;
            else if (stringValue == "ORAT")
                return ORAT;
            else if (stringValue == "WRAT")
                return WRAT;
            else if (stringValue == "GRAT")
                return GRAT;
            else if (stringValue == "LRAT")
                return LRAT;
            else if (stringValue == "CRAT")
                return CRAT;
            else if (stringValue == "RESV")
                return RESV;
            else if (stringValue == "PRBL")
                return PRBL;
            else if (stringValue == "FLD")
                return FLD;
            else
                throw std::invalid_argument("Unknown enum state string: " + stringValue );
        }

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

        const std::string ActionEnum2String( ActionEnum enumValue ) {
            switch(enumValue) {
            case NONE:
                return "NONE";
            case CON:
                return "CON";
            case CON_PLUS:
                return "+CON";
            case WELL:
                return "WELL";
            case PLUG:
                return "PLUG";
            case RATE:
                return "RATE";
            default:
                throw std::invalid_argument("unhandled enum value");
            }
        }


        ActionEnum ActionEnumFromString( const std::string& stringValue ) {

            if (stringValue == "NONE")
                return NONE;
            else if (stringValue == "CON")
                return CON;
            else if (stringValue == "+CON")
                return CON_PLUS;
            else if (stringValue == "WELL")
                return WELL;
            else if (stringValue == "PLUG")
                return PLUG;
            else if (stringValue == "RATE")
                return RATE;
            else
                throw std::invalid_argument("Unknown enum state string: " + stringValue );
        }

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

    namespace WellSegment{

        const std::string LengthDepthEnumToString(LengthDepthEnum enumValue) {
            switch (enumValue) {
                case INC:
                    return "INC";
                case ABS:
                    return "ABS";
                default:
                    throw std::invalid_argument("unhandled LengthDepthEnum value");
            }
        }

        LengthDepthEnum LengthDepthEnumFromString(const std::string& string ) {
            if (string == "INC") {
                return INC;
            } else if (string == "ABS") {
                return ABS;
            } else {
                throw std::invalid_argument("Unknown enum string: " + string + " for LengthDepthEnum");
            }
        }

        const std::string CompPressureDropEnumToString(CompPressureDropEnum enumValue) {
            switch (enumValue) {
                case HFA:
                    return "HFA";
                case HF_:
                    return "HF-";
                case H__:
                    return "H--";
                default:
                    throw std::invalid_argument("unhandled CompPressureDropEnum value");
            }
        }

        CompPressureDropEnum CompPressureDropEnumFromString( const std::string& string ) {

            if (string == "HFA") {
                return HFA;
            } else if (string == "HF-") {
                return HF_;
            } else if (string == "H--") {
                return H__;
            } else {
                throw std::invalid_argument("Unknown enum string: " + string + " for CompPressureDropEnum");
            }
        }

        const std::string MultiPhaseModelEnumToString(MultiPhaseModelEnum enumValue) {
            switch (enumValue) {
                case HO:
                    return "HO";
                case DF:
                    return "DF";
                default:
                    throw std::invalid_argument("unhandled MultiPhaseModelEnum value");
            }
        }

        MultiPhaseModelEnum MultiPhaseModelEnumFromString(const std::string& string ) {

            if ((string == "HO") || (string == "H0")) {
                return HO;
            } else if (string == "DF") {
                return DF;
            } else {
                throw std::invalid_argument("Unknown enum string: " + string + " for MultiPhaseModelEnum");
            }

        }

    }

    GroupType operator|(GroupType lhs, GroupType rhs) {
        return static_cast<GroupType>(static_cast<std::underlying_type<GroupType>::type>(lhs) | static_cast<std::underlying_type<GroupType>::type>(rhs));
    }

    GroupType operator&(GroupType lhs, GroupType rhs) {
        return static_cast<GroupType>(static_cast<std::underlying_type<GroupType>::type>(lhs) & static_cast<std::underlying_type<GroupType>::type>(rhs));
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
