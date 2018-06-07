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

#include <opm/parser/eclipse/EclipseState/Schedule/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellTestState.hpp>

namespace Opm {

    void WellTestState::addClosedWell(const std::string& well_name, WellTestConfig::Reason reason, double sim_time) {
        if (this->has(well_name, reason))
            return;

        this->wells.push_back({well_name, reason, sim_time, 0});
    }


    void WellTestState::openWell(const std::string& well_name) {
        wells.erase(std::remove_if(wells.begin(),
                                   wells.end(),
                                   [&well_name](const ClosedWell& well) { return (well.name == well_name); }),
                    wells.end());
    }


    void WellTestState::drop(const std::string& well_name, WellTestConfig::Reason reason) {
        wells.erase(std::remove_if(wells.begin(),
                                   wells.end(),
                                   [&well_name, reason](const ClosedWell& well) { return (well.name == well_name && well.reason == reason); }),
                    wells.end());
    }


    bool WellTestState::has(const std::string well_name, WellTestConfig::Reason reason) const {
        const auto well_iter = std::find_if(wells.begin(),
                                            wells.end(),
                                            [&well_name, &reason](const ClosedWell& well)
                                             {
                                                return (reason == well.reason && well.name == well_name);
                                            });
        return (well_iter != wells.end());
    }

    size_t WellTestState::size() const {
        return this->wells.size();
    }


    std::vector<std::pair<std::string, WellTestConfig::Reason>> WellTestState::update(const WellTestConfig& config, double sim_time) {
        std::vector<std::pair<std::string, WellTestConfig::Reason>> output;
        for (auto& closed_well : this->wells) {
            if (config.has(closed_well.name, closed_well.reason)) {
                const auto& well_config = config.get(closed_well.name, closed_well.reason);
                double elapsed = sim_time - closed_well.last_test;

                if (elapsed >= well_config.test_interval)
                    if (well_config.num_test == 0 || (closed_well.num_attempt < well_config.num_test)) {
                        closed_well.last_test = sim_time;
                        closed_well.num_attempt += 1;
                        output.push_back(std::make_pair(closed_well.name, closed_well.reason));
                    }

            }
        }
        return output;
    }
}


