/*
  Copyright 2021 Equinor ASA.

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

#ifndef SCHEDULE_TSTEP_HPP
#define SCHEDULE_TSTEP_HPP

#include <chrono>
#include <memory>
#include <optional>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvg.hpp>

namespace Opm {

    /*
      The purpose of the ScheduleState class is to hold the entire Schedule
      information, i.e. wells and groups and so on, at exactly one point in
      time. The ScheduleState class itself has no dynamic behavior, the dynamics
      is handled by the Schedule instance owning the ScheduleState instance.
    */


    class ScheduleState {
    public:
        ScheduleState() = default;
        explicit ScheduleState(const std::chrono::system_clock::time_point& start_time);
        ScheduleState(const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time);
        ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time);
        ScheduleState(const ScheduleState& src, const std::chrono::system_clock::time_point& start_time, const std::chrono::system_clock::time_point& end_time);


        std::chrono::system_clock::time_point start_time() const;
        std::chrono::system_clock::time_point end_time() const;
        ScheduleState next(const std::chrono::system_clock::time_point& next_start);

        bool operator==(const ScheduleState& other) const;
        static ScheduleState serializeObject();

        void pavg(PAvg pavg);
        const PAvg& pavg() const;


        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(m_start_time);
            serializer(m_end_time);
            serializer(m_pavg);
        }

    private:
        std::chrono::system_clock::time_point m_start_time;
        std::optional<std::chrono::system_clock::time_point> m_end_time;

        std::shared_ptr<PAvg> m_pavg;
    };
}

#endif
