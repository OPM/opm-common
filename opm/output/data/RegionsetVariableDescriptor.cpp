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

#include <opm/output/data/RegionsetVariableDescriptor.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>

std::unique_ptr<Opm::data::RegionsetVariableDescriptor>
Opm::data::RegionsetVariableDescriptor::clone() const
{
    return std::make_unique<RegionsetVariableDescriptor>(*this);
}

void Opm::data::RegionsetVariableDescriptor::prepareDescriptorSet()
{
    this->startPtr_.clear();
    this->regsetMaxID_.emplace();
}

void Opm::data::RegionsetVariableDescriptor::addRegionSet(const int maxRegionID)
{
    if (! this->regsetMaxID_.has_value()) {
        throw std::logic_error {
            "Cannot register a new region set before "
            "calling prepareDescriptorSet() or after "
            "calling finaliseDescriptorSet()"
        };
    }

    this->regsetMaxID_->push_back(std::max(maxRegionID, -1));
}

void Opm::data::RegionsetVariableDescriptor::finaliseDescriptorSet()
{
    if (! this->regsetMaxID_.has_value()) {
        throw std::logic_error {
            "Cannot finalise descriptor set before "
            "calling prepareDescriptorSet()"
        };
    }

    this->communicateGlobalRegsetMaxIDs();
    this->defineStartPointers();

    // Release ID storage to signal end of finalisation step.
    this->regsetMaxID_.reset();
}

void Opm::data::RegionsetVariableDescriptor::defineStartPointers()
{
    this->startPtr_.assign(this->regsetMaxID_->size() + 1, std::size_t{0});

    std::ranges::transform(*this->regsetMaxID_, this->startPtr_.begin() + 1,
                           [](const int regsetMaxID)
                           {
                               // Note: +1 for the maximum region ID itself.
                               return (regsetMaxID < 0)
                                   ? std::size_t{0}
                                   : static_cast<std::size_t>(regsetMaxID) + 1;
                           });

    std::partial_sum(this->startPtr_.begin(),
                     this->startPtr_.end(),
                     this->startPtr_.begin());
}
