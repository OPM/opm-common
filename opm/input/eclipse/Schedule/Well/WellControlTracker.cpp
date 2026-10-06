/*
  Copyright 2026 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/WellControlTracker.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

void Opm::WellControlTracker::defined(const std::string&     well,
                                      const KeywordLocation& location)
{
    const auto status = this->defined_
        .try_emplace(well, this->locations_.size());

    if (status.second) {
        // New well defined, add its location and initialize its status.
        this->locations_.push_back(location);
        this->connected_.push_back(false);
        this->controlled_.push_back(false);
    }
}

void Opm::WellControlTracker::connected(const std::string& well)
{
    if (const auto index = this->wellIndex(well)) {
        this->connected_[*index] = true;
    }
}

void Opm::WellControlTracker::controlled(const std::string& well)
{
    if (const auto index = this->wellIndex(well)) {
        this->controlled_[*index] = true;
    }
}

std::vector<std::pair<std::string, Opm::KeywordLocation>>
Opm::WellControlTracker::uncontrolled() const
{
    auto result = std::vector<std::pair<std::string, KeywordLocation>>{};

    // Note: This is potentially wasteful if there are many wells, and few
    // that are actually connected and uncontrolled.  It's nevertheless
    // still simple and clear to implement it this way.
    //
    // The cost is somewhat mitigated by the fact that this function will
    // be called at most once in each simulation run.
    auto wells = std::vector<std::string>(this->defined_.size());
    for (const auto& [well, index] : this->defined_) {
        wells[index] = well;
    }

    for (auto i = 0*wells.size(); i < wells.size(); ++i) {
        if (this->connected_[i] && !this->controlled_[i]) {
            result.emplace_back(wells[i], this->locations_[i]);
        }
    }

    return result;
}

std::optional<std::vector<Opm::KeywordLocation>::size_type>
Opm::WellControlTracker::wellIndex(const std::string& well) const
{
    const auto it = this->defined_.find(well);
    if (it == this->defined_.end()) {
        return std::nullopt;
    }

    return it->second;
}
