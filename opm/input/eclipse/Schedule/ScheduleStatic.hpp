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
#ifndef SCHEDULE_STATIC_HPP
#define SCHEDULE_STATIC_HPP

#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Schedule/MessageLimits.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/RSTConfig.hpp>
#include <opm/input/eclipse/Schedule/ScheduleRestartInfo.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <memory>
#include <optional>
#include <string>

namespace Opm {

class Python;

struct ScheduleStatic
{
    std::shared_ptr<const Python> m_python_handle;
    std::string m_input_path;
    ScheduleRestartInfo rst_info;
    MessageLimits m_deck_message_limits;
    UnitSystem m_unit_system;
    Runspec m_runspec;
    RSTConfig rst_config;
    std::optional<int> output_interval;
    double sumthin{-1.0};
    bool rptonly{false};
    bool gaslift_opt_active{false};
    std::optional<OilVaporizationProperties> oilVap;
    bool slave_mode{false};

    ScheduleStatic() = default;

    explicit ScheduleStatic(std::shared_ptr<const Python> python_handle) :
        m_python_handle(python_handle)
    {}

    ScheduleStatic(std::shared_ptr<const Python> python_handle,
                   const ScheduleRestartInfo& restart_info,
                   const Deck& deck,
                   const Runspec& runspec,
                   const std::optional<int>& output_interval_,
                   const ParseContext& parseContext,
                   ErrorGuard& errors,
                   const bool slave_mode);

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_deck_message_limits);
        serializer(this->rst_info);
        serializer(m_runspec);
        serializer(m_unit_system);
        serializer(this->m_input_path);
        serializer(rst_info);
        serializer(rst_config);
        serializer(this->output_interval);
        serializer(this->gaslift_opt_active);
    }

    static ScheduleStatic serializationTestObject();

    bool operator==(const ScheduleStatic& other) const;
};

} // end namespace Opm

#endif // SCHEDULE_STATIC_HPP
