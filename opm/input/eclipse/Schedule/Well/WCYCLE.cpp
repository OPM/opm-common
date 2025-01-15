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

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <algorithm>

#include <fmt/format.h>

namespace Opm {

WCYCLE WCYCLE::serializationTestObject()
{
    WCYCLE result;

    result.entries_.emplace("W1", Entry{1.0, 2.0, 3.0, 4.0, false});
    result.entries_.emplace("W2", Entry{5.0, 6.0, 7.0, 8.0, true});

    return result;
}

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
    const std::string name = record.getItem<ParserKeywords::WCYCLE::WELL>().getTrimmedString(0);
    entries_.insert_or_assign(name, Entry {
        record.getItem<ParserKeywords::WCYCLE::ON_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::OFF_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::START_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::MAX_TIMESTEP>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::CONTROLLED_TIMESTEP>().getTrimmedString(0) == "YES"
    });
}

bool WCYCLE::operator==(const WCYCLE& that) const
{
    return this->entries_ == that.entries_;
}

double WCYCLE::nextTimeStep(const double current_time,
                            const double dt,
                            const WellMatcher& wmatch,
                            const WellTimeMap& open_times,
                            const WellTimeMap& close_times,
                            std::function<bool(const std::string&)> opens_this_step) const
{
    double next_dt = dt;
    for (const auto& [name, wce]: entries_) {
        if (wce.off_time > 0 && wce.controlled_time_step) {
            for (const auto& w : wmatch.wells(name)) {
                const auto otime_it = open_times.find(w);
                if (otime_it == open_times.end()) {
                    continue;
                }
                const double target_time = otime_it->second + wce.on_time;
                if (target_time > current_time &&
                    target_time < current_time + next_dt &&
                    !opens_this_step(w))
                {
                    next_dt = std::min(next_dt, target_time - current_time);
                    OpmLog::info(fmt::format("Adjusting time step to {} days "
                                             "to match cycling period for well {}",
                                              unit::convert::to(next_dt, unit::day), w));
                }
            }
        }

        if (wce.on_time > 0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto ctime_it = close_times.find(w);
                if (ctime_it == close_times.end()) {
                    continue;
                }
                const double target_time = ctime_it->second + wce.off_time;
                if (wce.controlled_time_step &&
                    target_time > current_time &&
                    target_time < current_time + next_dt)
                {
                    next_dt = std::min(next_dt, target_time - current_time);
                    OpmLog::info(fmt::format("Adjusting time step to {} days "
                                             "to match cycling period for well {}",
                                              unit::convert::to(next_dt, unit::day), w));
                }
                if (current_time >= target_time && wce.on_time > 0) {
                    OpmLog::info(fmt::format("Cycling well {} opening, "
                                             "setting max timestep {} days",
                                              w, unit::convert::to(wce.max_time_step, unit::day)));
                    next_dt = std::min(next_dt, wce.max_time_step);
                }
            }
        }
    }

    return next_dt;
}

WCYCLE::WellIsOpenMap
WCYCLE::wellStatus(const double current_time,
                   const WellMatcher& wmatch,
                   WellTimeMap& open_times,
                   WellTimeMap& close_times) const
{
    WellIsOpenMap result;
    for (const auto& [name, wce] : entries_) {
        for (const auto& w : wmatch.wells(name)) {
            if (wce.on_time > 0) {
                const auto ctime_it = close_times.find(w);
                if (ctime_it != close_times.end()) {
                    const double target_time = ctime_it->second + wce.off_time;
                    if (current_time < target_time) {
                        result.emplace(w, false);
                    } else {
                        result.emplace(w, true);
                        if (wce.off_time > 0) {
                            close_times.erase(ctime_it);
                            open_times.insert_or_assign(w, current_time);
                        }
                        OpmLog::info(fmt::format("Cycling well {} opened", w));
                    }
                }
            }
            if (wce.off_time > 0) {
                const auto otime_it = open_times.find(w);
                if (otime_it != open_times.end()) {
                    const double target_time = otime_it->second + wce.on_time;
                    if (current_time < target_time) {
                        result.emplace(w, true);
                    } else {
                        result.emplace(w, false);
                        open_times.erase(otime_it);
                        if (wce.on_time > 0) {
                            close_times.insert_or_assign(w, current_time);
                        }
                        OpmLog::info(fmt::format("Cycling well {} shut", w));
                    }
                } else if (wce.on_time < 0) {
                    result.emplace(w, false);
                }
            }
        }
    }

    return result;
}

WCYCLE::WellEfficiencyVec
WCYCLE::efficiencyScale(const double curr_time,
                        const double dt,
                        const WellMatcher& wmatch,
                        const WellTimeMap& open_times,
                        std::function<bool(const std::string&)> schedule_open) const
{
    std::vector<std::pair<std::string, double>> result;
    for (const auto& [name, wce] : entries_) {
        if (wce.on_time > 0 && wce.startup_time > 0.0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto otime_it = open_times.find(w);
                if (otime_it == open_times.end()) {
                    continue;
                }
                const double target_time = curr_time - otime_it->second;
                if (target_time < wce.startup_time && !schedule_open(w)) {
                    const double scale = target_time + dt > wce.startup_time ?
                        1.0 : (target_time + dt) / wce.startup_time;
                    OpmLog::info(fmt::format("Scaling well {} efficiency factor by {}",
                                             w, scale));
                    result.emplace_back(w, scale);
                }
            }
        }
    }

    return result;
}

} // end of Opm namespace
