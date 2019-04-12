/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,

  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RFTConfig.hpp>

namespace Opm {

RFTConfig::RFTConfig(const TimeMap& time_map) :
    tm(time_map)
{
}

bool RFTConfig::rft(const std::string& well_name, std::size_t report_step) const {
    if (report_step >= this->tm.size())
        throw std::invalid_argument("Invalid ");

    const auto well_iter = this->well_open.find(well_name);
    if (well_iter != this->well_open.end()) {

        // A general "Output RFT when the well is opened" has been configured with WRFT
        if (this->well_open_rft_time.first && this->well_open_rft_time.second <= report_step) {
            if (well_iter->second == report_step)
                return true;
        }


        // A FOPN setting has been configured with the WRFTPLT keyword
        if (this->well_open_rft_name.count(well_name) > 0) {
            if (well_iter->second == report_step)
                return true;
        }
    }

    if (this->rft_config.count(well_name) == 0)
        return false;

    auto rft_pair = this->rft_config.at(well_name)[report_step];
    if (rft_pair.first == RFTConnections::YES)
        return (rft_pair.second == report_step);

    if (rft_pair.first == RFTConnections::NO)
        return false;

    if (rft_pair.first == RFTConnections::REPT)
        return true;

    if (rft_pair.first == RFTConnections::TIMESTEP)
        return true;

    return false;
}

bool RFTConfig::plt(const std::string& well_name, std::size_t report_step) const {
    if (report_step >= this->tm.size())
        throw std::invalid_argument("Invalid ");

    if (this->plt_config.count(well_name) == 0)
        return false;

    auto plt_pair = this->plt_config.at(well_name)[report_step];
    if (plt_pair.first == PLTConnections::YES)
        return (plt_pair.second == report_step);

    if (plt_pair.first == PLTConnections::NO)
        return false;

    if (plt_pair.first == PLTConnections::REPT)
        return true;

    if (plt_pair.first == PLTConnections::TIMESTEP)
        return true;

    return false;
}

void RFTConfig::updateRFT(const std::string& well_name, std::size_t report_step, RFTConnections::RFTEnum value) {
    if (value == RFTConnections::FOPN)
        this->setWellOpenRFT(well_name);
    else {
        if (this->rft_config.count(well_name) == 0) {
            auto state = DynamicState<std::pair<RFTConnections::RFTEnum, std::size_t>>(this->tm, std::make_pair(RFTConnections::NO, 0));
            this->rft_config.emplace( well_name, state );
        }
        this->rft_config.at(well_name).update(report_step, std::make_pair(value, report_step));
    }
}

void RFTConfig::updatePLT(const std::string& well_name, std::size_t report_step, PLTConnections::PLTEnum value) {
    if (this->plt_config.count(well_name) == 0) {
        auto state = DynamicState<std::pair<PLTConnections::PLTEnum, std::size_t>>(this->tm, std::make_pair(PLTConnections::NO, 0));
        this->plt_config.emplace( well_name, state );
    }
    this->plt_config.at(well_name).update(report_step, std::make_pair(value, report_step));
}


bool RFTConfig::getWellOpenRFT(const std::string& well_name, std::size_t report_step) const {
    if (this->well_open_rft_name.count(well_name) > 0)
        return true;

    return (this->well_open_rft_time.first && this->well_open_rft_time.second <= report_step);
}


void RFTConfig::setWellOpenRFT(std::size_t report_step) {
    this->well_open_rft_time = std::make_pair(true, report_step);
}

void RFTConfig::setWellOpenRFT(const std::string& well_name) {
    this->well_open_rft_name.insert( well_name );
}


void RFTConfig::addWellOpen(const std::string& well_name, std::size_t report_step) {
    if (this->well_open.count(well_name) == 0)
        this->well_open[well_name] = report_step;
}

std::size_t RFTConfig::firstRFTOutput() const {
    std::size_t first_rft = this->tm.size();
    if (this->well_open_rft_time.first) {
        // The WRFT keyword has been used to request RFT output at well open for all wells.
        std::size_t rft_time = this->well_open_rft_time.second;
        for (const auto& rft_pair : this->well_open) {
            if (rft_pair.second >= rft_time)
                first_rft = std::min(first_rft, rft_pair.second);
        }
    } else {
        // Go through the individual wells and look for first open settings
        for (const auto& rft_pair : this->well_open)
            first_rft = std::min(first_rft, rft_pair.second);
    }

    for (const auto& rft_pair : this->plt_config) {
        const auto& dynamic_state = rft_pair.second;
        auto pred = [] (const std::pair<PLTConnections::PLTEnum, std::size_t>& ) { return false; };
        int this_first_rft = dynamic_state.find_if(pred);
        if (this_first_rft >= 0)
            first_rft = std::min(first_rft, static_cast<std::size_t>(this_first_rft));
    }

    return first_rft;
}

bool RFTConfig::active(std::size_t report_step) const {
    for (const auto& rft_pair : this->rft_config) {
        if (this->rft(rft_pair.first, report_step))
            return true;
    }

    for (const auto& plt_pair : this->plt_config) {
        if (this->rft(plt_pair.first, report_step))
            return true;
    }

    return false;
}

}
