/*
  Copyright 2016 Statoil ASA.

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
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellInjectionProperties.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace Opm {

    WellInjectionProperties::WellInjectionProperties() {
        surfaceInjectionRate=0.0;
        reservoirInjectionRate=0.0;
        temperature=
            Metric::TemperatureOffset
            + ParserKeywords::STCOND::TEMPERATURE::defaultValue;
        BHPLimit=0.0;
        THPLimit=0.0;
        VFPTableNumber=0;
        predictionMode=true;
        injectionControls=0;
        injectorType = WellInjector::WATER;
        controlMode = WellInjector::CMODE_UNDEFINED;
    }


    bool WellInjectionProperties::operator==(const WellInjectionProperties& other) const {
        if ((surfaceInjectionRate == other.surfaceInjectionRate) &&
            (reservoirInjectionRate == other.reservoirInjectionRate) &&
            (temperature == other.temperature) &&
            (BHPLimit == other.BHPLimit) &&
            (THPLimit == other.THPLimit) &&
            (VFPTableNumber == other.VFPTableNumber) &&
            (predictionMode == other.predictionMode) &&
            (injectionControls == other.injectionControls) &&
            (injectorType == other.injectorType) &&
            (controlMode == other.controlMode))
            return true;
        else
            return false;
    }

    bool WellInjectionProperties::operator!=(const WellInjectionProperties& other) const {
        return !(*this == other);
    }

    std::ostream& operator<<( std::ostream& stream,
                              const WellInjectionProperties& wp ) {
        return stream
            << "WellInjectionProperties { "
            << "surfacerate: "      << wp.surfaceInjectionRate << ", "
            << "reservoir rate "    << wp.reservoirInjectionRate << ", "
            << "temperature: "      << wp.temperature << ", "
            << "BHP limit: "        << wp.BHPLimit << ", "
            << "THP limit: "        << wp.THPLimit << ", "
            << "VFP table: "        << wp.VFPTableNumber << ", "
            << "prediction mode: "  << wp.predictionMode << ", "
            << "injection ctrl: "   << wp.injectionControls << ", "
            << "injector type: "    << wp.injectorType << ", "
            << "control mode: "     << wp.controlMode << " }";
    }

}
