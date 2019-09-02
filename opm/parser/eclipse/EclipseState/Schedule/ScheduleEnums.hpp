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



}

#endif
