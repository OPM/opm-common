/*
  Copyright 2023 Equinor.

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

#include <opm/input/eclipse/Schedule/Well/WCYCLE.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

namespace Opm {

bool WCYCLE::Entry::operator==(const Entry& that) const
{
    return this->on_time == that.on_time
        && this->off_time == that.off_time
        && this->startup_time == that.startup_time
        && this->max_time_step == that.max_time_step
        && this->controlled_time_step == that.controlled_time_step;
}

void WCYCLE::addRecord(const DeckRecord& record)
{
    Entry entry{};
    const std::string name = record.getItem<ParserKeywords::WCYCLE::WELL>().getTrimmedString(0);
    entry.on_time = record.getItem<ParserKeywords::WCYCLE::ON_TIME>().getSIDouble(0);
    entry.off_time = record.getItem<ParserKeywords::WCYCLE::OFF_TIME>().getSIDouble(0);
    entry.startup_time = record.getItem<ParserKeywords::WCYCLE::START_TIME>().getSIDouble(0);
    entry.max_time_step = record.getItem<ParserKeywords::WCYCLE::MAX_TIMESTEP>().getSIDouble(0);
    const std::string cont_string =
        record.getItem<ParserKeywords::WCYCLE::CONTROLLED_TIMESTEP>().getTrimmedString(0);
    entry.controlled_time_step = cont_string == "YES";
    entries_.emplace(name, entry);
}

bool WCYCLE::operator==(const WCYCLE& other) const
{
    return this->entries_ == other.entries_;
}

} // end of Opm namespace

