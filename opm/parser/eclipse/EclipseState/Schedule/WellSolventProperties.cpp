#include <opm/parser/eclipse/EclipseState/Schedule/WellSolventProperties.hpp>

#include <string>
#include <vector>

namespace Opm {

    WellSolventProperties::WellSolventProperties() {
        m_solventConcentration = 0.0;
    }

    bool WellSolventProperties::operator==(const WellSolventProperties& other) const {
        if (m_solventConcentration == other.m_solventConcentration)
            return true;
        else
            return false;

    }

    bool WellSolventProperties::operator!=(const WellSolventProperties& other) const {
        return !(*this == other);
    }
}
