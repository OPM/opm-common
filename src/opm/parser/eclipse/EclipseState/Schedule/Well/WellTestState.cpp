/*
  Copyright 2018 Statoil ASA.

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
#include <stdexcept>
#include <algorithm>

#include <cassert>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestState.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

namespace Opm {


    WellTestState::WTestWell::WTestWell(const std::string& wname, WellTestConfig::Reason reason_, double sim_time)
        : name(wname)
        , reason(reason_)
        , last_test(sim_time)
    {}


    void WellTestState::close_well(const std::string& well_name, WellTestConfig::Reason reason, double sim_time) {
        auto well_iter = this->wells.find(well_name);
        if (well_iter == this->wells.end())
            this->wells.emplace(well_name, WTestWell{well_name, reason, sim_time});
        else {
            well_iter->second.closed = true;
            well_iter->second.last_test = sim_time;
            well_iter->second.reason = reason;
        }
    }


    void WellTestState::open_well(const std::string& well_name) {
        auto& well = this->wells.at(well_name);
        well.closed = false;
    }

    void WellTestState::open_completions(const std::string& well_name) {
        this->completions.erase( well_name );
    }


    bool WellTestState::well_is_closed(const std::string& well_name) const {
        auto iter = this->wells.find(well_name);
        if (iter == this->wells.end())
            return false;

        return iter->second.closed;
    }


    bool WellTestState::well_is_open(const std::string& well_name) const {
        const auto& well = this->wells.at(well_name);
        return !well.closed;
    }


    size_t WellTestState::num_closed_wells() const {
        return std::count_if(this->wells.begin(), this->wells.end(), [](const auto& well_pair) { return well_pair.second.closed; });
    }

    std::vector<std::string>
    WellTestState::test_wells(const WellTestConfig& config,
                               double sim_time) {
        std::vector<std::string> output;

        for (auto& [wname, well] : this->wells) {
            if (!well.closed)
                continue;

            if (config.has(wname, well.reason)) {
                const auto& well_config = config.get(wname, well.reason);
                const double elapsed = sim_time - well.last_test;

                if (!well.wtest_report_step.has_value())
                    well.wtest_report_step = well_config.begin_report_step;

                if (well_config.begin_report_step > well.wtest_report_step) {
                    well.wtest_report_step = well_config.begin_report_step;
                    well.num_attempt = 0;
                }

                if (well_config.test_well(well.num_attempt, elapsed)) {
                    well.last_test = sim_time;
                    well.num_attempt ++;
                    output.push_back(well.name);
                }
            }
        }
        return output;
    }


    void WellTestState::close_completion(const std::string& well_name, int complnum, double sim_time) {
        auto well_iter = this->completions.find(well_name);
        if (well_iter == this->completions.end())
            this->completions.emplace(well_name, std::unordered_map<int, ClosedCompletion>{});

        this->completions[well_name].insert_or_assign(complnum, ClosedCompletion{well_name, complnum, sim_time, 0});
    }


    void WellTestState::open_completion(const std::string& well_name, int complnum) {
        const auto& well_iter = this->completions.find(well_name);
        if (well_iter == this->completions.end())
            return;

        auto& well_map = well_iter->second;
        well_map.erase(complnum);

        if (well_map.empty())
            this->completions.erase(well_name);
    }


    bool WellTestState::completion_is_closed(const std::string& well_name, const int complnum) const {
        const auto& well_iter = this->completions.find(well_name);
        if (well_iter == this->completions.end())
            return false;

        const auto& completion_iter = well_iter->second.find(complnum);
        if (completion_iter == well_iter->second.end())
            return false;

        return true;
    }

    bool WellTestState::completion_is_open(const std::string& well_name, const int complnum) const {
        const auto& well = this->completions.at(well_name);
        const auto& completion_iter = well.find(complnum);
        if (completion_iter == well.end())
            return true;

        return false;
    }


    size_t WellTestState::num_closed_completions() const {
        std::size_t count = 0;
        for (const auto& [_, comp_map] : this->completions) {
            (void)_;
            count += comp_map.size();
        }
        return count;
    }


    double WellTestState::lastTestTime(const std::string& well_name) const {
        const auto& well = this->wells.at(well_name);
        return well.last_test;
    }



    bool WellTestState::operator==(const WellTestState& other) const {
        return this->wells == other.wells &&
               this->completions == other.completions;
    }

    void WellTestState::clear() {
        this->wells.clear();
        this->completions.clear();
    }
}


