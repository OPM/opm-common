/*
  Copyright 2024 Equinor ASA.

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

#include <config.h>

#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

    std::string normalisedRegsetName(const std::string& regSet)
    {
        const auto maxChar = std::string::size_type{3};
        const auto prefix = std::string_view { "FIP" };
        const auto start = prefix.size() * (regSet.find(prefix) == 0);

        return regSet.substr(start, maxChar);
    }

    std::vector<std::string> normalisedRegsetNames(std::vector<std::string> regSets)
    {
        if (std::find(regSets.begin(), regSets.end(), "FIPNUM") == regSets.end()) {
            // Standard region set FIPNUM is always present, even if not
            // explicitly mentioned in the input.
            regSets.push_back("FIPNUM");
        }

        std::ranges::transform(regSets, regSets.begin(), &normalisedRegsetName);

        return regSets;
    }

    std::vector<std::string> sorted(std::vector<std::string> strings)
    {
        std::ranges::sort(strings);
        return strings;
    }

    int localMaxRegionID(const std::string& regSet, const Opm::FieldPropsManager& fldPropsMgr)
    {
        const auto& regID = fldPropsMgr.get_int(regSet);

        return regID.empty()
            ? -1
            : *std::max_element(regID.begin(), regID.end());
    }

    std::vector<int> localMaxRegionID(const std::vector<std::string>& regSets,
                                      const Opm::FieldPropsManager&   fldPropsMgr)
    {
        auto maxRegionID = std::vector<int>(regSets.size());

        std::ranges::transform(regSets, maxRegionID.begin(),
                               [&fldPropsMgr](const std::string& regSet)
                               { return localMaxRegionID("FIP" + regSet, fldPropsMgr); });

        return maxRegionID;
    }

} // Anonymous namespace

Opm::FIPRegionStatistics::FIPRegionStatistics(const std::size_t                      declaredMaxRegID,
                                              const FieldPropsManager&               fldPropsMgr,
                                              std::function<void(std::vector<int>&)> computeGlobalMax)
    : minimumMaximumRegionID_(static_cast<int>(declaredMaxRegID))
    , regionSets_  { sorted(normalisedRegsetNames(fldPropsMgr.fip_regions())) }
    , maxRegionID_ { localMaxRegionID(regionSets_, fldPropsMgr) }
{
    computeGlobalMax(this->maxRegionID_);
}

bool Opm::FIPRegionStatistics::operator==(const FIPRegionStatistics& that) const
{
    return (this->minimumMaximumRegionID_ == that.minimumMaximumRegionID_)
        && (this->regionSets_ == that.regionSets_)
        && (this->maxRegionID_ == that.maxRegionID_)
        ;
}

Opm::FIPRegionStatistics
Opm::FIPRegionStatistics::serializationTestObject()
{
    using namespace std::string_literals;

    auto stats = FIPRegionStatistics{};

    stats.minimumMaximumRegionID_ = 42;

    stats.regionSets_  = std::vector { "ABC"s, "NUM"s, "XYZ"s, };
    stats.maxRegionID_ = std::vector { 11,     22,     33,     };

    return stats;
}

int Opm::FIPRegionStatistics::maximumRegionID(std::string_view regionSet) const
{
    const auto rset = normalisedRegsetName(std::string { regionSet });
    auto regSetPos = std::lower_bound(this->regionSets_.begin(),
                                      this->regionSets_.end(),
                                      rset);

    if ((regSetPos == this->regionSets_.end()) || (*regSetPos != rset)) {
        return -1;
    }

    return this->maxRegionID_[ std::distance(this->regionSets_.begin(), regSetPos) ];
}
