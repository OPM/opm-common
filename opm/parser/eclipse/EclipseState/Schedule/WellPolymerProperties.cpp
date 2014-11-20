#include <opm/parser/eclipse/EclipseState/Schedule/WellPolymerProperties.hpp>

#include <string>
#include <vector>

namespace Opm {
    
    WellPolymerProperties::WellPolymerProperties() {
        m_polymerConcentration = 0.0;
        m_saltConcentration = 0.0;
    }
}
