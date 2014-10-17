#include <opm/parser/eclipse/EclipseState/Schedule/WellInjectionProperties.hpp>

#include <string>
#include <vector>

namespace Opm {
    
    WellInjectionProperties::WellInjectionProperties() {
        surfaceInjectionRate=0.0; 
        reservoirInjectionRate=0.0; 
        BHPLimit=0.0; 
        THPLimit=0.0; 
        predictionMode=true;
        injectionControls=0;
        injectorType = WellInjector::WATER;
        controlMode = WellInjector::CMODE_UNDEFINED;
    }
    
}
