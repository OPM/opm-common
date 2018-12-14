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
        if (this->hasWell(well_name, reason))
            return;

        this->wells.push_back({well_name, reason, sim_time, 0});
    }


    void WellTestState::openWell(const std::string& well_name) {
        wells.erase(std::remove_if(wells.begin(),
                                   wells.end(),
                                   [&well_name](const ClosedWell& well) { return (well.name == well_name); }),
                    wells.end());
    }


    void WellTestState::dropWell(const std::string& well_name, WellTestConfig::Reason reason) {
        wells.erase(std::remove_if(wells.begin(),
                                   wells.end(),
                                   [&well_name, reason](const ClosedWell& well) { return (well.name == well_name && well.reason == reason); }),
                    wells.end());
    }


    bool WellTestState::hasWell(const std::string& well_name, WellTestConfig::Reason reason) const {
        const auto well_iter = std::find_if(wells.begin(),
                                            wells.end(),
                                            [&well_name, &reason](const ClosedWell& well)
                                             {
                                                return (reason == well.reason && well.name == well_name);
                                            });
        return (well_iter != wells.end());
    }

    size_t WellTestState::sizeWells() const {
        return this->wells.size();
    }

    std::vector<std::pair<std::string, WellTestConfig::Reason>> WellTestState::updateWell(const WellTestConfig& config, double sim_time) {
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





    void WellTestState::addClosedCompletion(const std::string& well_name, int complnum, double sim_time) {
        if (this->hasCompletion(well_name, complnum))
            return;

        this->completions.push_back( {well_name, complnum, sim_time, 0} );
    }


    void WellTestState::dropCompletion(const std::string& well_name, int complnum) {
        completions.erase(std::remove_if(completions.begin(),
                                         completions.end(),
                                         [&well_name, complnum](const ClosedCompletion& completion) { return (completion.wellName == well_name && completion.complnum == complnum); }),
                          completions.end());
    }


    bool WellTestState::hasCompletion(const std::string& well_name, const int complnum) const {
        const auto completion_iter = std::find_if(completions.begin(),
                                                  completions.end(),
                                                  [&well_name, &complnum](const ClosedCompletion& completion)
                                                  {
                                                    return (complnum == completion.complnum && completion.wellName == well_name);
                                                  });
        return (completion_iter != completions.end());
    }

    size_t WellTestState::sizeCompletions() const {
        return this->completions.size();
    }

    std::vector<std::pair<std::string, int>> WellTestState::updateCompletion(const WellTestConfig& config, double sim_time) {
        std::vector<std::pair<std::string, int>> output;
        for (auto& closed_completion : this->completions) {
            if (config.has(closed_completion.wellName, WellTestConfig::Reason::COMPLETION)) {
                const auto& well_config = config.get(closed_completion.wellName, WellTestConfig::Reason::COMPLETION);
                double elapsed = sim_time - closed_completion.last_test;

                if (elapsed >= well_config.test_interval)
                    if (well_config.num_test == 0 || (closed_completion.num_attempt < well_config.num_test)) {
                        closed_completion.last_test = sim_time;
                        closed_completion.num_attempt += 1;
                        output.push_back(std::make_pair(closed_completion.wellName, closed_completion.complnum));
                    }
            }
        }
        return output;
    }

    double WellTestState::lastTestTime(const std::string& well_name) const {
        const auto well_iter = std::find_if(wells.begin(),
                                            wells.end(),
                                            [&well_name](const ClosedWell& well)
                                            {
                                                return (well.name == well_name);
                                            });
        if (well_iter == wells.end()) {
            throw std::runtime_error("No well named " + well_name + " found in WellTestState.");
        }
        return well_iter->last_test;
    }

}


