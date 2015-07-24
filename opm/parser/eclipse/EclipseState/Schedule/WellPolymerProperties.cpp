#include <opm/parser/eclipse/EclipseState/Schedule/WellPolymerProperties.hpp>

#include <string>
#include <vector>

namespace Opm {

    WellPolymerProperties::WellPolymerProperties() {
        m_polymerConcentration = 0.0;
        m_saltConcentration = 0.0;
    }

    bool WellPolymerProperties::operator==(const WellPolymerProperties& other) const {
        if ((m_polymerConcentration == other.m_polymerConcentration) &&
            (m_saltConcentration == other.m_saltConcentration))
            return true;
        else
            return false;

    }

    bool WellPolymerProperties::operator!=(const WellPolymerProperties& other) const {
        return !(*this == other);
    }
}
