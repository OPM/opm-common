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

#include <opm/input/eclipse/Schedule/Events.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

namespace Opm {

    Events Events::serializationTestObject()
    {
        Events result;
        result.m_events = 12345;
        return result;
    }

    void Events::addEvent(const ScheduleEvents::Events event)
    {
        this->m_events |= event;
    }

    void Events::clearEvent(std::uint64_t eventMask)
    {
        const auto diff = this->m_events & eventMask;

        this->m_events -= diff;
    }

    void Events::reset()
    {
        this->m_events = 0;
    }

    bool Events::hasEvent(const std::uint64_t eventMask) const
    {
        return (this->m_events & eventMask) != 0;
    }

    bool Events::operator==(const Events& data) const
    {
        return this->m_events == data.m_events;
    }

    // ------------------------------------------------------------------------

    WellGroupEvents WellGroupEvents::serializationTestObject()
    {
        WellGroupEvents wg;
        wg.addWell("WG1");
        wg.addGroup("GG1");
        return wg;
    }

    void WellGroupEvents::addWell(const std::string& wname)
    {
        auto events = Events{};
        events.addEvent(ScheduleEvents::NEW_WELL);

        this->m_wellgroup_events.emplace(wname, events);
    }

    void WellGroupEvents::addGroup(const std::string& gname)
    {
        auto events = Events{};
        events.addEvent(ScheduleEvents::NEW_GROUP);

        this->m_wellgroup_events.emplace(gname, events);
    }

    void WellGroupEvents::addEvent(const std::string& wgname,
                                   const ScheduleEvents::Events event)
    {
        auto events_iter = this->m_wellgroup_events.find(wgname);
        if (events_iter == this->m_wellgroup_events.end()) {
            throw std::logic_error {
                fmt::format("Adding event for unknown well/group {}", wgname)
            };
        }

        events_iter->second.addEvent(event);
    }

    void WellGroupEvents::clearEvent(const std::string& wgname,
                                     const std::uint64_t eventMask)
    {
        auto events_iter = this->m_wellgroup_events.find(wgname);
        if (events_iter != this->m_wellgroup_events.end()) {
            events_iter->second.clearEvent(eventMask);
        }
    }

    void WellGroupEvents::reset()
    {
        // Note: We can't use "m_wellgroup_events.clear()" here, because
        // that would break the precondition that addEvent() should only be
        // called for known wells/groups.

        for (auto& eventPair : this->m_wellgroup_events) {
            eventPair.second.reset();
        }
    }

    bool WellGroupEvents::has(const std::string& wgname) const
    {
        return this->m_wellgroup_events.find(wgname)
            != this->m_wellgroup_events.end();
    }

    bool WellGroupEvents::hasEvent(const std::string& wgname,
                                   const std::uint64_t eventMask) const
    {
        auto events_iter = this->m_wellgroup_events.find(wgname);
        if (events_iter == this->m_wellgroup_events.end()) {
            return false;
        }

        return events_iter->second.hasEvent(eventMask);
    }

    const Events& WellGroupEvents::at(const std::string& wgname) const
    {
        auto pos = this->m_wellgroup_events.find(wgname);
        if (pos == this->m_wellgroup_events.end()) {
            throw std::invalid_argument {
                fmt::format("Well/group {} is unknown to the events system", wgname)
            };
        }

        return pos->second;
    }

    bool WellGroupEvents::operator==(const WellGroupEvents& data) const
    {
        return this->m_wellgroup_events == data.m_wellgroup_events;
    }

} // namespace Opm
