/*
  Copyright 2019 Equinor ASA.

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
#ifndef RFT_CONFIG_HPP
#define RFT_CONFIG_HPP

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>

namespace Opm {

class TimeMap;
class RFTConfig {
public:
    explicit RFTConfig(const TimeMap& time_map);
    bool rft(const std::string& well, std::size_t report_step) const;
    bool plt(const std::string& well, std::size_t report_step) const;
    bool getWellOpenRFT(const std::string& well_name, std::size_t report_step) const;
    void setWellOpenRFT(std::size_t report_step);
    void setWellOpenRFT(const std::string& well_name);

    bool active(std::size_t report_step) const;
    std::size_t firstRFTOutput() const;
    void updateRFT(const std::string& well, std::size_t report_step, RFTConnections::RFTEnum value);
    void updatePLT(const std::string& well, std::size_t report_step, PLTConnections::PLTEnum value);
    void addWellOpen(const std::string& well, std::size_t report_step);
private:
    const TimeMap& tm;
    std::pair<bool, std::size_t> well_open_rft_time;
    std::unordered_set<std::string> well_open_rft_name;
    std::unordered_map<std::string, std::size_t> well_open;
    std::unordered_map<std::string, DynamicState<std::pair<RFTConnections::RFTEnum, std::size_t>>> rft_config;
    std::unordered_map<std::string, DynamicState<std::pair<PLTConnections::PLTEnum, std::size_t>>> plt_config;
};

}

#endif
