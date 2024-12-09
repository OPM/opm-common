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

#include <iostream>

#include <fmt/format.h>

namespace Opm {

std::map<std::string, double> WCYCLE::close_times_;
std::map<std::string, double> WCYCLE::open_times_;

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
    entries_.emplace(name, Entry {
        record.getItem<ParserKeywords::WCYCLE::ON_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::OFF_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::START_TIME>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::MAX_TIMESTEP>().getSIDouble(0),
        record.getItem<ParserKeywords::WCYCLE::CONTROLLED_TIMESTEP>().getTrimmedString(0) == "YES"
    });
}

bool WCYCLE::operator==(const WCYCLE& other) const
{
    return this->entries_ == other.entries_;
}

double WCYCLE::nextTimeStep(const double current_time,
                            const double dt,
                            const WellMatcher& wmatch) const
{
    double next_dt = dt;
    for (const auto& [name, wce]: entries_) {
        if (wce.on_time > 0 && wce.controlled_time_step) {
            for (const auto& w : wmatch.wells(name)) {
                const auto otime_it = open_times_.find(w);
                if (otime_it == open_times_.end()) {
                    continue;
                }
                const double otime = otime_it->second;
                const double target_time = otime + wce.on_time;
                if (target_time > current_time && target_time < current_time + next_dt) {
                    next_dt = std::min(next_dt, target_time - current_time);
                    OpmLog::info(fmt::format("Adjusting time step to {} to match cycling period for well {}",
                                             unit::convert::to(next_dt, unit::day), w));
                }
            }
        }

        if (wce.off_time > 0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto ctime_it = close_times_.find(w);
                if (ctime_it == close_times_.end()) {
                    continue;
                }
                const double ctime = ctime_it->second;
                const double target_time = ctime + wce.off_time;
                bool adjusted = false;
                if (wce.controlled_time_step &&
                    target_time > current_time &&
                    target_time < current_time + next_dt)
                {
                    next_dt = std::min(next_dt, target_time - current_time);
                    OpmLog::info(fmt::format("Adjusting time step to {} to match cycling period for well {}",
                                             unit::convert::to(next_dt, unit::day), w));
                    adjusted = true;
                }
                if (!adjusted && current_time >= target_time) {
                    OpmLog::info(fmt::format("Cycling well {} will be opened, setting max timestep {}",
                                             w, wce.max_time_step));
                    next_dt = std::min(next_dt, wce.max_time_step);
                }
            }
        }
    }

    return next_dt;
}

std::vector<std::string>
WCYCLE::closeWells(const double current_time,
                   const WellMatcher& wmatch) const
{
    std::vector<std::string> result;
    for (const auto& [name, wce] : entries_) {
        if (wce.on_time > 0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto otime_it = open_times_.find(w);
                if (otime_it == open_times_.end()) {
                    continue;
                }
                const double otime = otime_it->second;
                const double target_time = otime + wce.on_time;
                if (current_time >= target_time) {
                    OpmLog::info(fmt::format("Cycling well {} shut at {}",
                                             w, unit::convert::to(current_time, unit::day)));

                    open_times_.erase(otime_it);
                    close_times_[w] = current_time;
                    result.push_back(w);
                }
            }
        }
    }

    return result;
}

std::vector<std::string>
WCYCLE::openWells(const double current_time,
                  const WellMatcher& wmatch) const
{
    std::vector<std::string> result;
    for (const auto& [name, wce] : entries_) {
        if (wce.off_time > 0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto ctime_it = close_times_.find(w);
                if (ctime_it == close_times_.end()) {
                    continue;
                }
                const double ctime = ctime_it->second;
                const double target_time = ctime + wce.off_time;
                if (current_time >= target_time) {
                    OpmLog::info(fmt::format("Cycling well {} opened at {}",
                                             w, unit::convert::to(current_time, unit::day)));
                    result.push_back(w);
                    close_times_.erase(ctime_it);
                    open_times_[w] = current_time;
                }
            }
        }
    }

    return result;
}

std::vector<std::pair<std::string, double>>
WCYCLE::efficiencyScale(const double end_time,
                         const WellMatcher& wmatch) const
{
    std::vector<std::pair<std::string, double>> result;
    for (const auto& [name, wce] : entries_) {
        if (wce.on_time > 0 && wce.startup_time > 0.0) {
            for (const auto& w : wmatch.wells(name)) {
                const auto otime_it = open_times_.find(w);
                if (otime_it == open_times_.end()) {
                    continue;
                }
                const double otime = otime_it->second;
                const double target_time = end_time - otime;
                if (target_time < wce.startup_time) {
                    OpmLog::info(fmt::format("Scaling well {} efficiency factor by {}",
                                             w, target_time / wce.startup_time));
                    result.emplace_back(w, target_time / wce.startup_time);
                }
            }
        }
    }

    return result;
}

} // end of Opm namespace
