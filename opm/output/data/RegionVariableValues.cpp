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

#include <opm/output/data/RegionVariableValues.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>
#include <opm/output/data/RegionVariableView.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

std::unique_ptr<Opm::data::RegionVariableValues>
Opm::data::RegionVariableValues::clone() const
{
    return std::make_unique<RegionVariableValues>(*this);
}

void
Opm::data::RegionVariableValues::
defineVariables(const RegionsetVariableDescriptor& descr,
                const std::vector<bool>&           is_cumulative)
{
    this->descr_.emplace(std::cref(descr));

    this->partitionVariables(is_cumulative);
    this->allocateValues();
}

void Opm::data::RegionVariableValues::prepareValueAccumulation()
{
    if (!this->descr_) {
        throw std::logic_error {
            "RegionVariableValues::prepareValueAccumulation(): "
            "call defineVariables() before prepareValueAccumulation()"
        };
    }

    std::ranges::fill(this->increment_, 0.0);
}

void Opm::data::RegionVariableValues::commitValues()
{
    if (!this->descr_) {
        throw std::logic_error {
            "RegionVariableValues::commitValues(): "
            "call defineVariables() before commitValues()"
        };
    }

    this->communicateIncrement();

    const auto end_cum = this->end_cum_ * this->descr_->get().numVariableSlots();

    // values += increment for cumulative quantities.
    std::transform(this->increment_.begin(), this->increment_.begin() + end_cum,
                   this->values_   .begin(), this->values_   .begin(), std::plus<>{});

    // values = increment for non-cumulative quantities.
    std::copy(this->increment_.begin() + end_cum, this->increment_.end(),
              this->values_   .begin() + end_cum);
}

void
Opm::data::RegionVariableValues::
addRegionValue(const std::size_t var_ix,
               const std::size_t regset_ix,
               const std::size_t region_ix,
               const double      x)
{
    if (! (var_ix < this->storage_ix_.size())) {
        return;
    }

    const auto num_slots = this->descr_->get().numVariableSlots();
    const auto view_ix   = this->storage_ix_[var_ix];

    RegionVariableView {
        std::span { this->increment_ }.subspan(view_ix * num_slots, num_slots),
        *this->descr_
    }.element(regset_ix, region_ix) += x;
}

std::optional<Opm::data::RegionVariableView<const double>>
Opm::data::RegionVariableValues::values(const std::size_t var_ix) const
{
    if (! (var_ix < this->storage_ix_.size())) {
        return {};
    }

    const auto num_slots = this->descr_->get().numVariableSlots();
    const auto view_ix   = this->storage_ix_[var_ix];

    return {
        RegionVariableView {
            std::span { this->values_ }.subspan(view_ix * num_slots, num_slots),
            *this->descr_
        }
    };
}

// ===========================================================================
// Private member functions below separator
// ===========================================================================

void
Opm::data::RegionVariableValues::
partitionVariables(const std::vector<bool>& is_cumulative)
{
    // Ranges::iota() is not available until C++23, so we use std::iota() instead.
    auto i = std::vector<std::vector<bool>::size_type>(is_cumulative.size());
    std::iota(i.begin(), i.end(), std::vector<bool>::size_type{0});

    const auto non_cumulatives = std::ranges::stable_partition
        (i, [&is_cumulative](const auto ix) { return is_cumulative[ix]; });

    this->end_cum_ = std::ranges::distance
        (std::ranges::begin(i), std::ranges::begin(non_cumulatives));

    this->storage_ix_.assign(i.size(), std::size_t{0});
    std::ranges::for_each(i, [n = std::size_t{0}, this]
                          (const auto var) mutable
                          { this->storage_ix_[var] = n++; });
}

void Opm::data::RegionVariableValues::allocateValues()
{
    const auto num_elem = this->storage_ix_.size() *
        this->descr_->get().numVariableSlots();

    this->values_.assign(num_elem, 0.0);
    this->increment_.assign(num_elem, 0.0);
}
