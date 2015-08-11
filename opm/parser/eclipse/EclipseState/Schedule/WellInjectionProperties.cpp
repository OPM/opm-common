#include <opm/parser/eclipse/EclipseState/Schedule/WellInjectionProperties.hpp>

#include <string>
#include <vector>

namespace Opm {

    WellInjectionProperties::WellInjectionProperties() {
        surfaceInjectionRate=0.0;
        reservoirInjectionRate=0.0;
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
}
