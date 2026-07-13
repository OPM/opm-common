/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#include <config.h>

#include <opm/output/data/RegionVariableMapping.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <numeric>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

void Opm::data::RegionVariableMapping::prepareRegistration()
{
    this->regsets_.clear();
    this->vars_.clear();
    this->is_cumulative_.clear();

    this->is_final_ = false;
}

void Opm::data::RegionVariableMapping::commitStructure()
{
    this->regsets_.commit();

    this->makeUniqueCumulative(this->vars_.commit());

    this->is_final_ = true;
}

void Opm::data::RegionVariableMapping::add(const RegionSet& rset)
{
    this->ensureRegistrationPossible("region set", rset.name);

    this->regsets_.add(rset.name);
}

void
Opm::data::RegionVariableMapping::add(const Variable& var, const bool is_cumulative)
{
    this->ensureRegistrationPossible("variable", var.name);

    this->vars_.add(var.name);

    this->is_cumulative_.push_back(is_cumulative);
}

// ===========================================================================
// Private member functions below separator
// ===========================================================================

std::vector<std::vector<std::string>::size_type>
Opm::data::RegionVariableMapping::NameLookup::commit()
{
    auto i = std::vector<std::vector<std::string>::size_type>(this->names_.size());
    std::iota(i.begin(), i.end(), std::vector<std::string>::size_type{});

    std::ranges::sort(i, [this](const auto i1, const auto i2)
    {
        return std::tie(this->names_[i1], i1)
            <  std::tie(this->names_[i2], i2);
    });

    {
        auto [first, last] = std::ranges::unique(i, [this](const auto i1, const auto i2)
        { return this->names_[i1] == this->names_[i2]; });

        i.erase(first, last);
    }

    auto unames = std::vector<std::string>(i.size());
    std::ranges::transform(i, unames.begin(),
                           [this](const auto ix)
                           { return this->names_[ix]; });

    this->names_.swap(unames);

    return i;
}

std::optional<std::vector<std::string>::size_type>
Opm::data::RegionVariableMapping::NameLookup::index(const std::string& name) const
{
    const auto namePos = std::ranges::lower_bound(this->names_, name);

    if ((namePos == this->names_.end()) || (*namePos != name)) {
        // Name does not exist in this->names_;
        return {};
    }

    return {
        std::ranges::distance(this->names_.begin(), namePos)
    };
}

// ---------------------------------------------------------------------------

void
Opm::data::RegionVariableMapping::
ensureRegistrationPossible(std::string_view type,
                           std::string_view name) const
{
    if (! this->is_final_) {
        // Structure is not yet committed.  This is fine.
        return;
    }

    throw std::logic_error {
        fmt::format("Cannot register a {} named '{}' after the "
                    "mapping's structure is finalised", type, name)
    };
}

void
Opm::data::RegionVariableMapping::ensureFinalStructure(std::string_view type,
                                                       std::string_view name) const
{
    if (this->is_final_) {
        // Structure is finalised.  This is fine.
        return;
    }

    throw std::logic_error {
        fmt::format("Cannot request properties of {} named '{}' "
                    "before the mapping's structure is finalised", type, name)
    };
}

void
Opm::data::RegionVariableMapping::
makeUniqueCumulative(const std::vector<std::vector<std::string>::size_type>& i)
{
    auto unique_is_cumulative = std::vector<bool>(i.size(), false);

    std::ranges::for_each(i, [this,
                              new_idx = 0 * unique_is_cumulative.size(),
                              &unique_is_cumulative]
        (const auto orig_idx) mutable
    {
        unique_is_cumulative[new_idx++] = this->is_cumulative_[orig_idx];
    });

    this->is_cumulative_.swap(unique_is_cumulative);
}
