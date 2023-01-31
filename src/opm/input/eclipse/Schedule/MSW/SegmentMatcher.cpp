/*
  Copyright 2022 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <functional>
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

class Opm::SegmentMatcher::Impl
{
public:
    explicit Impl(const ScheduleState& mswInputData)
        : mswInputData_{ std::cref(mswInputData) }
    {}

    template <class AddWellSegments>
    SegmentSet findSegments(const SetDescriptor& request,
                            AddWellSegments      addSegments) const;

private:
    std::reference_wrapper<const ScheduleState> mswInputData_;

    std::vector<std::string>
    candidateWells(const std::optional<std::string>& wellNamePattern) const;

    std::vector<std::string>
    candidateWells(const std::string& wellNamePattern) const;

    std::vector<std::string> candidateWells() const;

    std::vector<std::string>
    candidateWells(const std::vector<std::string>& allWells) const;

    std::vector<int>
    matchingSegments(const std::string&        well,
                     const std::optional<int>& segmentNumber) const;

    std::vector<int>
    matchingSegments(const std::string& well,
                     const int          segmentNumber) const;

    std::vector<int> matchingSegments(const std::string& well) const;
};

template <class AddWellSegments>
Opm::SegmentSet
Opm::SegmentMatcher::Impl::findSegments(const SetDescriptor& request,
                                        AddWellSegments      addSegments) const
{
    auto segSet = SegmentSet{};

    for (const auto& well : this->candidateWells(request.wellNames())) {
        if (const auto& segments = this->matchingSegments(well, request.segmentNumber());
            ! segments.empty())
        {
            addSegments(well, segments, segSet);
        }
    }

    return segSet;
}

std::vector<std::string>
Opm::SegmentMatcher::Impl::
candidateWells(const std::optional<std::string>& wellNamePattern) const
{
    return wellNamePattern.has_value()
        ? this->candidateWells(*wellNamePattern)
        : this->candidateWells();
}

std::vector<std::string>
Opm::SegmentMatcher::Impl::
candidateWells(const std::string& wellNamePattern) const
{
    // Consider all MS wells matching 'wellNamePattern'.
    return this->candidateWells(WellMatcher {
            this->mswInputData_.get().well_order()
        }.wells(wellNamePattern));
}

std::vector<std::string> Opm::SegmentMatcher::Impl::candidateWells() const
{
    // No specific wellname pattern => all MS wells match.
    return this->candidateWells(this->mswInputData_.get().well_order().names());
}

std::vector<std::string>
Opm::SegmentMatcher::Impl::
candidateWells(const std::vector<std::string>& allWells) const
{
    auto candidates = std::vector<std::string>{};
    candidates.reserve(allWells.size());

    std::copy_if(allWells.begin(), allWells.end(), std::back_inserter(candidates),
                 [this](const std::string& well)
                 {
                     return this->mswInputData_.get().wells(well).isMultiSegment();
                 });

    return candidates;
}

std::vector<int>
Opm::SegmentMatcher::Impl::
matchingSegments(const std::string&        well,
                 const std::optional<int>& segmentNumber) const
{
    return segmentNumber.has_value()
        ? this->matchingSegments(well, *segmentNumber)
        : this->matchingSegments(well);
}

std::vector<int>
Opm::SegmentMatcher::Impl::
matchingSegments(const std::string& wellname,
                 const int          segmentNumber) const
{
    const auto& well = this->mswInputData_.get().wells(wellname);
    assert (well.isMultiSegment());

    if (const auto ix = well.getSegments().segmentNumberToIndex(segmentNumber);
        ix < 0)
    {
        // SegmentNumber not among well's segments.  Empty result set.
        return {};
    }

    // SegmentNumber is among well's segments.  Result set is exactly that segment.
    return { segmentNumber };
}

std::vector<int>
Opm::SegmentMatcher::Impl::matchingSegments(const std::string& wellname) const
{
    // No specific segment number => All segments match.
    auto segments = std::vector<int>{};

    const auto& well = this->mswInputData_.get().wells(wellname);
    assert (well.isMultiSegment());

    const auto& segSet = well.getSegments();

    segments.reserve(segSet.size());
    std::transform(segSet.begin(), segSet.end(), std::back_inserter(segments),
                   [](const Segment& segment) { return segment.segmentNumber(); });

    return segments;
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

Opm::SegmentMatcher::SetDescriptor&
Opm::SegmentMatcher::SetDescriptor::segmentNumber(const int segNum)
{
    if (segNum <= 0) {
        // No specific segment number
        this->segmentNumber_.reset();
    }
    else {
        this->segmentNumber_ = segNum;
    }

    return *this;
}

Opm::SegmentMatcher::SetDescriptor&
Opm::SegmentMatcher::SetDescriptor::segmentNumber(std::string_view segNum0)
{
    auto segNum = dequote(segNum0);

    if (segNum.empty()) {
        // Not specified
        return this->segmentNumber(0);
    }

    auto result = 0;
    auto [ptr, ec] { std::from_chars(segNum.data(), segNum.data() + segNum.size(), result) };

    if ((ec == std::errc{}) && (ptr == segNum.data() + segNum.size())) {
        // Segment number is "123", or "'-1'", or something similar.
        return this->segmentNumber(result);
    }
    else if ((ec == std::errc::invalid_argument) && is_asterisk(segNum)) {
        // Segment number is '*'.  Treat as all segments.
        return this->segmentNumber(0);
    }
    else {
        // Segment number is some unrecognized number string other than '*'.
        throw std::invalid_argument {
            fmt::format("Invalid segment number string |{}|", segNum0)
        };
    }
}

Opm::SegmentMatcher::SetDescriptor&
Opm::SegmentMatcher::SetDescriptor::wellNames(std::string_view wellNamePattern)
{
    if (wellNamePattern.empty()) {
        // Match all MS wells.
        this->wellNamePattern_.reset();
    }
    else {
        // Match only those MS wells whose names match 'wellNamePattern'.
        this->wellNamePattern_ = wellNamePattern;
    }

    return *this;
}

// ---------------------------------------------------------------------------

Opm::SegmentSet::SegmentSet()
{
    this->segmentStart_.push_back(this->segments_.size());
}

std::vector<std::string_view>
Opm::SegmentSet::wells() const
{
    auto wellset = std::vector<std::string_view>{};
    wellset.reserve(this->wells_.size());

    auto ix = std::vector<std::vector<int>::size_type>::size_type{0};
    for (const auto& well : this->wells_) {
        if (this->segmentStart_[ix] != this->segmentStart_[ix + 1]) {
            wellset.emplace_back(well);
        }

        ix += 1;
    }

    return wellset;
}

Opm::SegmentSet::WellSegmentRange
Opm::SegmentSet::segments(std::string_view well) const
{
    using Ix = std::vector<std::string>::size_type;

    // Get 'well's insertion index from list sorted alphabetically on well
    // names.  Client must call establishNameLookupIndex() prior to calling
    // segments(string_view).

    auto ixPos =
        std::lower_bound(this->wellNameIndex_.begin(), this->wellNameIndex_.end(),
                         well, [this](const Ix i, std::string_view wellname)
                         {
                             return this->wells_[i] < wellname;
                         });

    if ((ixPos == this->wellNameIndex_.end()) || (this->wells_[*ixPos] != well)) {
        return this->segments(this->numWells());
    }
    else {
        return this->segments(*ixPos);
    }
}

Opm::SegmentSet::WellSegmentRange
Opm::SegmentSet::segments(const std::size_t well) const
{
    if (well >= this->numWells()) {
        return {};
    }

    const auto first = this->segmentStart_[well + 0];
    const auto last  = this->segmentStart_[well + 1];

    return {
        this->segments_.begin() + first,
        this->segments_.begin() + last,
        this->wells_[well]
    };
}

void Opm::SegmentSet::establishNameLookupIndex()
{
    using Ix = std::vector<std::string>::size_type;

    // Sort well insertion/order indices alphabetically on well names.
    // Enables using std::lobwer_bound() in segments(well).

    this->wellNameIndex_.resize(this->wells_.size());
    std::iota(this->wellNameIndex_.begin(), this->wellNameIndex_.end(), Ix{0});
    std::sort(this->wellNameIndex_.begin(), this->wellNameIndex_.end(),
              [this](const Ix i1, const Ix i2)
              {
                  return this->wells_[i1] < this->wells_[i2];
              });
}

void Opm::SegmentSet::addWellSegments(const std::string&      well,
                                      const std::vector<int>& segments)
{
    assert (! segments.empty());

    this->wells_.push_back(well);

    // Note that segmentStart_.push_back() must be after segments_.insert()
    // in order to maintain CSR invariant.
    this->segments_.insert(this->segments_.end(), segments.begin(), segments.end());
    this->segmentStart_.push_back(this->segments_.size());
}

// ---------------------------------------------------------------------------

Opm::SegmentMatcher::SegmentMatcher(const ScheduleState& mswInputData)
    : pImpl_{ std::make_unique<Impl>(mswInputData) }
{}

Opm::SegmentMatcher::SegmentMatcher(SegmentMatcher&& rhs)
    : pImpl_{ std::move(rhs.pImpl_) }
{}

Opm::SegmentMatcher&
Opm::SegmentMatcher::operator=(SegmentMatcher&& rhs)
{
    this->pImpl_ = std::move(rhs.pImpl_);
    return *this;
}

Opm::SegmentMatcher::~SegmentMatcher() = default;

Opm::SegmentSet
Opm::SegmentMatcher::findSegments(const SetDescriptor& segments) const
{
    auto segSet = this->pImpl_->
        findSegments(segments,
                     [](const std::string&      well,
                        const std::vector<int>& well_segments,
                        SegmentSet&             seg_set)
                     {
                         seg_set.addWellSegments(well, well_segments);
                     });

    segSet.establishNameLookupIndex();

    return segSet;
}
