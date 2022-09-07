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

#include <opm/input/eclipse/Schedule/RFTConfig.hpp>

#include <algorithm>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

template <typename Map, typename Predicate>
bool mapContains(const Map& map, Predicate&& p)
{
    return std::any_of(map.begin(), map.end(), std::forward<Predicate>(p));
}

template <typename Map, typename Predicate>
void pruneFromMapIf(Map& map, Predicate prune)
{
    for (auto begin = map.begin(); begin != map.end(); ) {
        if (prune(*begin)) {
            begin = map.erase(begin);
        }
        else {
            ++begin;
        }
    }
}

} // Anonymous namespace

namespace Opm {

std::string RFTConfig::RFT2String(const RFT enumValue)
{
    switch (enumValue) {
    case RFT::YES:      return "YES";
    case RFT::REPT:     return "REPT";
    case RFT::TIMESTEP: return "TIMESTEP";
    case RFT::FOPN:     return "FOPN";
    case RFT::NO:       return "NO";
    }

    throw std::invalid_argument {
        "unhandled enum value " + std::to_string(static_cast<int>(enumValue))
    };
}

RFTConfig::RFT RFTConfig::RFTFromString(const std::string& stringValue)
{
    if (stringValue == "YES") {
        return RFT::YES;
    }

    if (stringValue == "REPT") {
        return RFT::REPT;
    }

    if (stringValue == "TIMESTEP") {
        return RFT::TIMESTEP;
    }

    if (stringValue == "FOPN") {
        return RFT::FOPN;
    }

    if (stringValue == "NO") {
        return RFT::NO;
    }

    throw std::invalid_argument {
        "Unknown enum state string: " + stringValue
    };
}

std::string RFTConfig::PLT2String(const PLT enumValue)
{
    switch (enumValue) {
    case PLT::YES:      return "YES";
    case PLT::REPT:     return "REPT";
    case PLT::TIMESTEP: return "TIMESTEP";
    case PLT::NO:       return "NO";
    }

    throw std::invalid_argument {
        "unhandled enum value " + std::to_string(static_cast<int>(enumValue))
    };
}

RFTConfig::PLT RFTConfig::PLTFromString(const std::string& stringValue)
{
    if (stringValue == "YES") {
        return PLT::YES;
    }

    if (stringValue == "REPT") {
        return PLT::REPT;
    }

    if (stringValue == "TIMESTEP") {
        return PLT::TIMESTEP;
    }

    if (stringValue == "NO") {
        return PLT::NO;
    }

    throw std::invalid_argument {
        "Unknown enum state string: '" + stringValue + '\''
    };
}

void RFTConfig::first_open(const bool on)
{
    this->first_open_rft = on;
}

// Note: 'mode' intentionally accepted as mutable to simplify
// implementation of 'FOPN' case.
void RFTConfig::update(const std::string& wname, RFT mode)
{
    if (mode == RFT::NO) {
        auto iter = this->rft_state.find(wname);
        if (iter != this->rft_state.end()) {
            this->rft_state.erase(iter);
        }

        return;
    }

    if (mode == RFT::FOPN) {
        // Treat FOPN as YES for wells that are already open.
        auto pos = this->open_wells.find(wname);
        if (pos != this->open_wells.end()) {
            pos->second = true;
            mode = RFT::YES;
        }
    }

    this->rft_state.insert_or_assign(wname, mode);
}

void RFTConfig::update(const std::string& wname, const PLT mode)
{
    if (mode == PLT::NO) {
        auto iter = this->plt_state.find(wname);
        if (iter != this->plt_state.end()) {
            this->plt_state.erase(iter);
        }

        return;
    }

    this->plt_state[wname] = mode;
}

bool RFTConfig::active() const
{
    return this->rft() || this->plt();
}

bool RFTConfig::rft() const
{
    return std::any_of(this->rft_state.begin(), this->rft_state.end(),
                       [](const auto& rft_pair)
                       {
                           return rft_pair.second != RFT::FOPN;
                       });
}

bool RFTConfig::rft(const std::string& wname) const
{
    auto well_iter = this->rft_state.find(wname);

    return (well_iter != this->rft_state.end())
        && (well_iter->second != RFT::FOPN);
}

bool RFTConfig::plt() const
{
    return ! this->plt_state.empty();
}

bool RFTConfig::plt(const std::string& wname) const
{
    return this->plt_state.find(wname) != this->plt_state.end();
}

std::optional<RFTConfig>
RFTConfig::well_open(const std::string& wname) const
{
    auto iter = this->open_wells.find(wname);
    if (iter != this->open_wells.end()) {
        // RFT data at well open is already recorded.  Don't trigger new RFT
        // output event.
        return {};
    }

    auto new_rft = *this;

    if (this->first_open_rft) {
        // Well opens at this time and user requests RFT data on well open
        // for all new wells.  Trigger RFT output.
        new_rft.update(wname, RFT::YES);
        new_rft.open_wells.insert_or_assign(wname, true);

        return new_rft;
    }

    auto rft_fopn = new_rft.rft_state.find(wname);
    if ((rft_fopn != new_rft.rft_state.end()) &&
        (rft_fopn->second == RFT::FOPN))
    {
        // Well opens at this time and user requests RFT data on well open
        // for this particular well.  Trigger RFT output.
        rft_fopn->second = RFT::YES;
    }

    new_rft.open_wells.insert_or_assign(wname, true);

    return new_rft;
}

std::optional<RFTConfig> RFTConfig::next() const
{
    // Next block configured by removing all 'YES' nodes from *this.  We do
    // this because the 'YES' nodes have already triggered by the time the
    // next block runs.
    const auto rft_is_yes = [](const auto& rft_pair) {
        return rft_pair.second == RFT::YES;
    };

    const auto plt_is_yes = [](const auto& plt_pair) {
        return plt_pair.second == PLT::YES;
    };

    const auto rft_has_yes = mapContains(this->rft_state, rft_is_yes);
    const auto plt_has_yes = mapContains(this->plt_state, plt_is_yes);

    if (! (rft_has_yes || plt_has_yes)) {
        // No 'YES' node in either the RFT or the PLT states.  Return
        // nullopt to signify that next block is unchanged from current.
        return {};
    }

    // Prune 'YES' nodes from both RFT and PLT states to form next block.
    auto new_rft = std::optional<RFTConfig>{*this};

    if (rft_has_yes) {
        pruneFromMapIf(new_rft->rft_state, rft_is_yes);
    }

    if (plt_has_yes) {
        pruneFromMapIf(new_rft->plt_state, plt_is_yes);
    }

    return new_rft;
}

bool RFTConfig::operator==(const RFTConfig& data) const
{
    return (this->first_open_rft == data.first_open_rft)
        && (this->rft_state == data.rft_state)
        && (this->plt_state == data.plt_state)
        && (this->open_wells == data.open_wells);
}

RFTConfig RFTConfig::serializeObject()
{
    // Establish an object in a non-default state to enable testing the
    // serialization code.  These statements "simply" record a number of
    // requests to populate every internal table.  That way we get the best
    // test coverage.

    RFTConfig rft_config;
    rft_config.first_open(true);

    // Trigger RFT output for P-1 when well opens.
    rft_config.update("P-1", RFT::FOPN);

    // Trigger PLT output for P-2 at every timestep.
    rft_config.update("P-2", PLT::TIMESTEP);

    // I-1 is an open well at this time.
    rft_config.open_wells.emplace("I-1", true);

    return rft_config;
}

} // namespace Opm
