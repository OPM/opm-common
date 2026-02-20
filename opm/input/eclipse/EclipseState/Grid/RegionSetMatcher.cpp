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

#include <opm/input/eclipse/EclipseState/Grid/RegionSetMatcher.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <fmt/format.h>

class Opm::RegionSetMatcher::Impl
{
public:
    explicit Impl(const FIPRegionStatistics& fipRegStats)
        : fipRegStats_{ std::cref(fipRegStats) }
    {}

    template <class AddRegionSets>
    RegionSetMatchResult findRegions(const SetDescriptor& request,
                                     AddRegionSets        addRegions) const;

private:
    class RegIdxRange
    {
    public:
        RegIdxRange(const int begin_arg, const int end_arg)
            : begin_{begin_arg}, end_{end_arg}
        {}

        int begin() const { return this->begin_; }
        int end()   const { return this->end_; }

        bool empty() const { return this->end() <= this->begin(); }

    private:
        int begin_{};
        int end_{};
    };

    std::reference_wrapper<const FIPRegionStatistics> fipRegStats_;

    std::vector<std::string>
    candidateRegionSets(const std::optional<std::string>& regionSet) const;

    RegIdxRange
    matchingRegions(const std::string&        regSet,
                    const std::optional<int>& regionID) const;

    RegIdxRange
    matchingRegions(const std::string& regSet,
                    const int          regionID) const;

    RegIdxRange matchingRegions(const std::string& regSet) const;
};

template <class AddRegionSet>
Opm::RegionSetMatchResult
Opm::RegionSetMatcher::Impl::findRegions(const SetDescriptor& request,
                                         AddRegionSet         addRegions) const
{
    auto regSetMatchResult = RegionSetMatchResult{};

    for (const auto& regSet : this->candidateRegionSets(request.regionSet())) {
        if (const auto regions = this->matchingRegions(regSet, request.regionID());
            ! regions.empty())
        {
            addRegions(regSet, regions.begin(), regions.end(), regSetMatchResult);
        }
    }

    return regSetMatchResult;
}

std::vector<std::string>
Opm::RegionSetMatcher::Impl::
candidateRegionSets(const std::optional<std::string>& regionSet) const
{
    return regionSet.has_value()
        ? std::vector { *regionSet } // Specific region set
        : this->fipRegStats_.get().regionSets();
}

Opm::RegionSetMatcher::Impl::RegIdxRange
Opm::RegionSetMatcher::Impl::
matchingRegions(const std::string&        regSet,
                const std::optional<int>& regionID) const
{
    return regionID.has_value()
        ? this->matchingRegions(regSet, *regionID)
        : this->matchingRegions(regSet);
}

Opm::RegionSetMatcher::Impl::RegIdxRange
Opm::RegionSetMatcher::Impl::
matchingRegions(const std::string& regSet,
                const int          regionID) const
{
    const auto maxRegID = this->fipRegStats_.get().maximumRegionID(regSet);

    if ((maxRegID > 0) &&       // Region set exists
        ((regionID <= maxRegID) ||
         (regionID <= this->fipRegStats_.get().declaredMaximumRegionID())))
    {
        // 'RegionID' is within index range of 'regSet'.  Return range
        // matching exactly that region ID (end == begin + 1).
        return { regionID, regionID + 1 };
    }

    // If we get here, 'regSet' does not exist or does not contain
    // 'regionID'.  Return empty range (end <= begin).
    return { 0, 0 };
}

Opm::RegionSetMatcher::Impl::RegIdxRange
Opm::RegionSetMatcher::Impl::matchingRegions(const std::string& regSet) const
{
    // No specific region ID => All regions match request provided 'regSet'
    // exists.

    const auto maxRegID = this->fipRegStats_.get().maximumRegionID(regSet);

    if (maxRegID <= 0) {
        // 'regSet' does not exist.  Return empty range (end <= begin).
        return { 0, 0 };
    }

    // 'regSet' is a valid region set name.  Return range covering 1..MAX.
    const auto maxID =
        std::max(maxRegID, this->fipRegStats_.get().declaredMaximumRegionID());

    return { 1, maxID + 1 };
}

// ===========================================================================
// Public Interface Below Separator
// ===========================================================================

namespace {
    std::string_view dequote(std::string_view s)
    {
        auto b = s.find_first_of("'");
        if (b == std::string_view::npos) {
            return s;
        }

        auto x = s.substr(b + 1);
        auto e = x.find_first_of("'");
        if (e == std::string_view::npos) {
            throw std::invalid_argument {
                fmt::format("Invalid quoted string |{}|", s)
            };
        }

        return x.substr(0, e);
    }

    bool is_asterisk(std::string_view s)
    {
        const auto ast = std::regex { R"(\s*\*\s*)" };
        return std::regex_match(s.begin(), s.end(), ast);
    }
} // Anonymous namespace

Opm::RegionSetMatcher::SetDescriptor&
Opm::RegionSetMatcher::SetDescriptor::regionID(const int region)
{
    if (region <= 0) {
        // No specific region ID.
        this->regionId_.reset();
    }
    else {
        this->regionId_ = region;
    }

    return *this;
}

Opm::RegionSetMatcher::SetDescriptor&
Opm::RegionSetMatcher::SetDescriptor::regionID(std::string_view region0)
{
    auto region = dequote(region0);

    if (region.empty()) {
        // Not specified
        return this->regionID(0);
    }

    auto result = 0;
    auto [ptr, ec] { std::from_chars(region.data(), region.data() + region.size(), result) };

    if ((ec == std::errc{}) && (ptr == region.data() + region.size())) {
        // Region ID is "7", or "'-1'", or something similar.
        return this->regionID(result);
    }
    else if ((ec == std::errc::invalid_argument) && is_asterisk(region)) {
        // Region ID is '*'.  Treat as all regions.
        return this->regionID(0);
    }
    else {
        // Region ID is some unrecognized number string other than '*'.
        throw std::invalid_argument {
            fmt::format("Invalid region ID number string |{}|", region0)
        };
    }
}

Opm::RegionSetMatcher::SetDescriptor&
Opm::RegionSetMatcher::SetDescriptor::vectorName(std::string_view vector)
{
    const auto padLimit = std::string_view::size_type{5};

    if (vector.size() < padLimit) {
        // Canonical vector name like "RPR", "ROIP", or "RODEN".  Matches
        // all region sets.
        this->regionSet_.reset();
    }
    else {
        // Specific vector name like "RPR__ABC", "ROIP_NUM", or "RODENTS1".
        // Matches specific region set.
        this->regionSet_ = vector.substr(padLimit);
    }

    return *this;
}

// ---------------------------------------------------------------------------

std::vector<std::string_view>
Opm::RegionSetMatchResult::regionSets() const
{
    auto regSetColl = std::vector<std::string_view>{};
    regSetColl.reserve(this->numRegionSets());

    auto ix = std::vector<std::vector<int>::size_type>::size_type{0};
    for (const auto& regSet : this->regionSets_) {
        const auto min = this->regionIDRange_[2*ix + 0];
        const auto max = this->regionIDRange_[2*ix + 1];
        if (max >= min) {
            regSetColl.emplace_back(regSet);
        }

        ix += 1;
    }

    return regSetColl;
}

Opm::RegionSetMatchResult::RegionIndexRange
Opm::RegionSetMatchResult::regions(std::string_view regSet) const
{
    using Ix = std::vector<std::string>::size_type;

    // Get 'regSet's insertion index from list sorted alphabetically on
    // region set names.  Client must call establishNameLookupIndex() prior
    // to calling regions(string_view).

    auto ixPos =
        std::lower_bound(this->regionSetIndex_.begin(), this->regionSetIndex_.end(),
                         regSet, [this](const Ix i, std::string_view rsetName)
                         {
                             return this->regionSets_[i] < rsetName;
                         });

    if ((ixPos == this->regionSetIndex_.end()) || (this->regionSets_[*ixPos] != regSet)) {
        return this->regions(this->numRegionSets());
    }
    else {
        return this->regions(*ixPos);
    }
}

Opm::RegionSetMatchResult::RegionIndexRange
Opm::RegionSetMatchResult::regions(const std::size_t regSet) const
{
    if (regSet >= this->numRegionSets()) {
        // Non-existent region set.  Return empty range (last <= first)
        return {};
    }

    const auto beginRegID = this->regionIDRange_[2*regSet + 0];
    const auto endRegID   = this->regionIDRange_[2*regSet + 1];

    return { beginRegID, endRegID, this->regionSets_[regSet] };
}

void Opm::RegionSetMatchResult::establishNameLookupIndex()
{
    using Ix = std::vector<std::string>::size_type;

    // Sort well insertion/order indices alphabetically on region set names.
    // Enables using std::lower_bound() in regions(string_view regSet).

    this->regionSetIndex_.resize(this->regionSets_.size());
    std::iota(this->regionSetIndex_.begin(), this->regionSetIndex_.end(), Ix{0});
    std::ranges::sort(this->regionSetIndex_,
                      [this](const Ix i1, const Ix i2)
                      { return this->regionSets_[i1] < this->regionSets_[i2]; });
}

void Opm::RegionSetMatchResult::addRegionIndices(const std::string& regSet,
                                                 const int          beginRegID,
                                                 const int          endRegID)
{
    assert (endRegID > beginRegID);

    this->regionSets_.push_back(regSet);

    // Invariant: Push 'begin' before 'end'.
    this->regionIDRange_.push_back(beginRegID);
    this->regionIDRange_.push_back(endRegID);
}

// ---------------------------------------------------------------------------

Opm::RegionSetMatcher::RegionSetMatcher(const FIPRegionStatistics& fipRegStats)
    : pImpl_{ std::make_unique<Impl>(fipRegStats) }
{}

Opm::RegionSetMatcher::RegionSetMatcher(RegionSetMatcher&& rhs)
    : pImpl_{ std::move(rhs.pImpl_) }
{}

Opm::RegionSetMatcher&
Opm::RegionSetMatcher::operator=(RegionSetMatcher&& rhs)
{
    this->pImpl_ = std::move(rhs.pImpl_);
    return *this;
}

Opm::RegionSetMatcher::~RegionSetMatcher() = default;

Opm::RegionSetMatchResult
Opm::RegionSetMatcher::findRegions(const SetDescriptor& selection) const
{
    auto regSetMatchResult = this->pImpl_->
        findRegions(selection,
                    [](const std::string&    regSet,
                       const int             minRegionID,
                       const int             maxRegionID,
                       RegionSetMatchResult& matchResult)
                    {
                        matchResult.addRegionIndices(regSet, minRegionID, maxRegionID);
                    });

    regSetMatchResult.establishNameLookupIndex();

    return regSetMatchResult;
}
