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

#include <opm/input/eclipse/Schedule/ScheduleRestartInfo.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <opm/io/eclipse/rst/state.hpp>

namespace Opm {

ScheduleRestartInfo::ScheduleRestartInfo(const RestartIO::RstState * rst, const Deck& deck)
{
    if (rst) {
        const auto& [t,r] = rst->header.restart_info();
        this->time = t;
        this->report_step = r;
        this->skiprest = deck.hasKeyword<ParserKeywords::SKIPREST>();
    }
}


bool ScheduleRestartInfo::operator==(const ScheduleRestartInfo& other) const
{
    return this->time == other.time &&
        this->report_step == other.report_step &&
        this->skiprest == other.skiprest;
}


ScheduleRestartInfo ScheduleRestartInfo::serializationTestObject()
{
    ScheduleRestartInfo rst_info;
    rst_info.report_step = 12345;
    rst_info.skiprest = false;
    return rst_info;
}

}
