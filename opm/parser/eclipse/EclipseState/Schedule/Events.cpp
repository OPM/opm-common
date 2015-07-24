/*
  Copyright 2015 Statoil ASA.

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
#include <cstddef>

#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>

namespace Opm {

    Events::Events() {
    }


    bool Events::hasEvent(ScheduleEvents::Events event, size_t reportStep) const {
        if (reportStep < m_events.size()) {
            uint64_t eventSum = m_events[reportStep];
            if (eventSum & event)
                return true;
            else
                return false;
        } else
            return false;
    }


    void Events::addEvent(ScheduleEvents::Events event, size_t reportStep) {
        if (m_events.size() <= reportStep)
            m_events.resize( 2 * reportStep + 1 );

        m_events[reportStep] |= event;
    }

}

