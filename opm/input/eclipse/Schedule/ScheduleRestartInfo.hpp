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
#ifndef SCHEDULE_RESTART_INFO_HPP
#define SCHEDULE_RESTART_INFO_HPP

#include <cstddef>
#include <ctime>

namespace Opm {

class Deck;
namespace RestartIO { struct RstState; }

struct ScheduleRestartInfo
{
    std::time_t time{0};
    std::size_t report_step{0};
    bool skiprest{false};

    ScheduleRestartInfo() = default;

    ScheduleRestartInfo(const RestartIO::RstState * rst, const Deck& deck);
    bool operator==(const ScheduleRestartInfo& other) const;
    static ScheduleRestartInfo serializationTestObject();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->time);
        serializer(this->report_step);
        serializer(this->skiprest);
    }
};

}

#endif
