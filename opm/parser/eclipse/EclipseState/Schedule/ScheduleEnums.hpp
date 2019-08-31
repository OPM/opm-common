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

#ifndef SCHEDULE_ENUMS_H
#define SCHEDULE_ENUMS_H

#include <string>

namespace Opm {




    namespace GroupInjection {

          enum ControlEnum {
            NONE = 0,
            RATE = 1,
            RESV = 2,
            REIN = 4,
            VREP = 8,
            FLD  = 16
        };

        const std::string ControlEnum2String( ControlEnum enumValue );
        ControlEnum ControlEnumFromString( const std::string& stringValue );
    }


    namespace GroupProductionExceedLimit {
        enum ActionEnum {
            NONE = 0,
            CON = 1,
            CON_PLUS = 2,   // String: "+CON"
            WELL = 3,
            PLUG = 4,
            RATE = 5
        };

        const std::string ActionEnum2String( ActionEnum enumValue );
        ActionEnum ActionEnumFromString( const std::string& stringValue );
    }

    // A group can have both injection controls and production controls set at
    // the same time, i.e. this enum is used as a bitmask.
    enum class GroupType : unsigned {
        NONE = 0,
        PRODUCTION = 1,
        INJECTION = 2,
        MIXED = 3
    };

    GroupType operator |(GroupType lhs, GroupType rhs);
    GroupType operator &(GroupType lhs, GroupType rhs);


    enum class GuideRateTarget {
        OIL = 0,
        LIQ = 1,
        GAS = 2,
        RES = 3,
        COMB = 4,
        NONE = 5
    };
    GuideRateTarget GuideRateTargetFromString(const std::string& s);


    namespace GroupProduction {

        enum ControlEnum {
            NONE = 0,
            ORAT = 1,
            WRAT = 2,
            GRAT = 4,
            LRAT = 8,
            CRAT = 16,
            RESV = 32,
            PRBL = 64,
            FLD  = 128
        };

        const std::string ControlEnum2String( GroupProduction::ControlEnum enumValue );
        GroupProduction::ControlEnum ControlEnumFromString( const std::string& stringValue );

        enum class GuideRateDef  {
            OIL = 0,
            WAT = 1,
            GAS = 2,
            LIQ = 3,
            COMB = 4,
            WGA =  5,
            CVAL = 6,
            INJV = 7,
            POTN = 8,
            FORM = 9,
            NO_GUIDE_RATE = 10
        };

        GroupProduction::GuideRateDef GetGuideRateFromString( const std::string& stringValue );

    }

    namespace GuideRate {
        enum GuideRatePhaseEnum {
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
        const std::string GuideRatePhaseEnum2String( GuideRatePhaseEnum enumValue );
        GuideRatePhaseEnum GuideRatePhaseEnumFromString( const std::string& stringValue );
    }

    namespace RFTConnections {
        enum RFTEnum {
            YES = 1,
            REPT = 2,
            TIMESTEP = 3,
            FOPN = 4,
            NO = 5
        };

        const std::string RFTEnum2String(RFTEnum enumValue);

        RFTEnum RFTEnumFromString(const std::string &stringValue);
    }
    namespace PLTConnections{
        enum PLTEnum{
            YES      = 1,
            REPT     = 2,
            TIMESTEP = 3,
            NO       = 4
        };
        const std::string PLTEnum2String( PLTEnum enumValue);
        PLTEnum PLTEnumFromString( const std::string& stringValue);
    }

    enum OilVaporizationEnum{
        UNDEF = 0,
        VAPPARS = 1,
        DRDT = 2 // DRSDT or DRVDT
    };


    namespace WellSegment{

        enum LengthDepthEnum {
            INC = 0,
            ABS = 1
        };
        const std::string LengthDepthEnumToString(LengthDepthEnum enumValue);
        LengthDepthEnum LengthDepthEnumFromString(const std::string& stringValue);

        enum CompPressureDropEnum {
            HFA = 0,
            HF_ = 1,
            H__ = 2
        };
        const std::string CompPressureDropEnumToString(CompPressureDropEnum enumValue);
        CompPressureDropEnum CompPressureDropEnumFromString(const std::string& stringValue);

        enum MultiPhaseModelEnum {
            HO = 0,
            DF = 1
        };
        const std::string MultiPhaseModelEnumToString(MultiPhaseModelEnum enumValue);
        MultiPhaseModelEnum MultiPhaseModelEnumFromString(const std::string& stringValue);

    }



    enum class GroupWellQueryMode {
        Immediate,
        Recursive
    };

}

#endif
