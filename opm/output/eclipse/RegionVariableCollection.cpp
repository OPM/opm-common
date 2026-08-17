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

#include <opm/output/eclipse/RegionVariableCollection.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>
#include <opm/output/data/RegionVariableValues.hpp>
#include <opm/output/data/RegionVariableView.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

Opm::RegionVariableCollection::
RegionVariableCollection(std::unique_ptr<data::RegionsetVariableDescriptor> descr,
                         std::unique_ptr<data::RegionVariableValues>        vals)
    : descr_ { std::move(descr) }
    , vals_  { std::move(vals)  }
{}

Opm::RegionVariableCollection::
RegionVariableCollection(const RegionVariableCollection& rhs)
    : descr_  { rhs.descr_->clone() }
    , vals_   { rhs.vals_->clone()  }
    , regSet_ { rhs.regSet_         }
{}

Opm::RegionVariableCollection::
RegionVariableCollection(RegionVariableCollection&& rhs) = default;

Opm::RegionVariableCollection&
Opm::RegionVariableCollection::operator=(const RegionVariableCollection& rhs)
{
    this->descr_  = rhs.descr_->clone();
    this->vals_   = rhs.vals_->clone();
    this->regSet_ = rhs.regSet_;

    return *this;
}

Opm::RegionVariableCollection&
Opm::RegionVariableCollection::operator=(RegionVariableCollection&& rhs) = default;

Opm::RegionVariableCollection::~RegionVariableCollection() = default;

void
Opm::RegionVariableCollection::
initialise(const int                          declaredMaxRegID,
           const FieldPropsManager&           fpMgr,
           const data::RegionVariableMapping& rvarMap)
{
    this->initialiseRegionDescriptors(declaredMaxRegID, fpMgr, rvarMap);

    this->initialiseRegionValues(rvarMap);
}

void
Opm::RegionVariableCollection::
addCellValue(const std::size_t var_ix,
             const std::size_t cell_ix,
             const double      x)
{
    auto regset_ix = std::size_t{0};

    // FIELD => region_ix == 0.
    this->vals_->addRegionValue(var_ix, regset_ix, /* region_ix = */ 0, x);

    for (; regset_ix < this->regSet_.size(); ++regset_ix) {
        this->vals_->addRegionValue(var_ix, regset_ix + 1, // +1 for FIELD
                                    this->regSet_[regset_ix][cell_ix],
                                    x);
    }
}

void
Opm::RegionVariableCollection::prepareValueAccumulation()
{
    this->vals_->prepareValueAccumulation();
}

void
Opm::RegionVariableCollection::commitValues()
{
    this->vals_->commitValues();
}

std::optional<std::size_t>
Opm::RegionVariableCollection::
regionSetIndex(const data::RegionVariableMapping& varMap,
               const std::string&                 regionSet) const
{
    if (regionSet == "FIELD") {
        return { std::size_t{0} };
    }

    auto i = varMap.index(data::RegionVariableMapping::RegionSet { regionSet });
    if (! i.has_value()) {
        return {};
    }

    // Note: 1+ to account for internal representation of "FIELD".
    return { 1 + *i };
}

std::optional<std::size_t>
Opm::RegionVariableCollection::
variableIndex(const data::RegionVariableMapping& varMap,
              const std::string&                 variable) const
{
    return varMap.index(data::RegionVariableMapping::Variable { variable });
}

const Opm::data::RegionVariableValues&
Opm::RegionVariableCollection::regionVariableValues() const
{
    return *this->vals_;
}

// ===========================================================================
// Private member functions below separator
// ===========================================================================

void
Opm::RegionVariableCollection::
initialiseRegionDescriptors(const int                          declaredMaxRegID,
                            const FieldPropsManager&           fpMgr,
                            const data::RegionVariableMapping& rvarMap)
{
    this->descr_->prepareDescriptorSet();

    this->regSet_.clear();

    if (rvarMap.numVariables() > 0) {
        // FIELD
        this->descr_->addRegionSet(0);

        for (const auto& regset : rvarMap.regionSets()) {
            const auto& regIDs = fpMgr.get_int(regset);

            this->regSet_.emplace_back(regIDs);

            this->descr_->addRegionSet(declaredMaxRegID, regIDs);
        }
    }

    this->descr_->finaliseDescriptorSet();
}

void Opm::RegionVariableCollection::
initialiseRegionValues(const data::RegionVariableMapping& rvarMap)
{
    auto is_cumulative = std::vector<bool>(rvarMap.numVariables(), false);

    for (std::size_t varIx = 0; varIx < is_cumulative.size(); ++varIx) {
        is_cumulative[varIx] = rvarMap.isCumulative
            (data::RegionVariableMapping::VariableIdx { varIx });
    }

    this->vals_->defineVariables(*this->descr_, is_cumulative);
}
