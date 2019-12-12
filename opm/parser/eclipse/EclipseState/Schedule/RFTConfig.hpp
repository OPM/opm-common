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

#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

namespace Opm {

class RFTConfig {
public:
    enum class RFT {
        YES = 1,
        REPT = 2,
        TIMESTEP = 3,
        FOPN = 4,
        NO = 5
    };
    static std::string RFT2String(RFT enumValue);
    static RFT RFTFromString(const std::string &stringValue);

    enum class PLT {
        YES      = 1,
        REPT     = 2,
        TIMESTEP = 3,
        NO       = 4
    };
    static std::string PLT2String(PLT enumValue);
    static PLT PLTFromString( const std::string& stringValue);

    using RFTMap = std::unordered_map<std::string,
                                      DynamicState<std::pair<RFT, std::size_t>>>;

    using PLTMap = std::unordered_map<std::string,
                                      DynamicState<std::pair<PLT, std::size_t>>>;

    RFTConfig();
    RFTConfig(const TimeMap& tm,
              const std::pair<bool, std::size_t>& rftTime,
              const std::unordered_set<std::string>& rftName,
              const std::unordered_map<std::string, std::size_t>& wellOpen,
              const RFTMap& rconfig,
              const PLTMap& pconfig);


    explicit RFTConfig(const TimeMap& time_map);
    bool rft(const std::string& well, std::size_t report_step) const;
    bool plt(const std::string& well, std::size_t report_step) const;
    bool getWellOpenRFT(const std::string& well_name, std::size_t report_step) const;
    void setWellOpenRFT(std::size_t report_step);
    void setWellOpenRFT(const std::string& well_name);

    bool active(std::size_t report_step) const;
    std::size_t firstRFTOutput() const;
    void updateRFT(const std::string& well, std::size_t report_step, RFT value);
    void updatePLT(const std::string& well, std::size_t report_step, PLT value);
    void addWellOpen(const std::string& well, std::size_t report_step);

    const TimeMap& timeMap() const;
    const std::pair<bool, std::size_t>& wellOpenRftTime() const;
    const std::unordered_set<std::string>& wellOpenRftName() const;
    const std::unordered_map<std::string, std::size_t>& wellOpen() const;
    const RFTMap& rftConfig() const;
    const PLTMap& pltConfig() const;

    bool operator==(const RFTConfig& data) const;

private:
    TimeMap tm;
    std::pair<bool, std::size_t> well_open_rft_time;
    std::unordered_set<std::string> well_open_rft_name;
    std::unordered_map<std::string, std::size_t> well_open;
    RFTMap rft_config;
    PLTMap plt_config;
};

}

#endif
