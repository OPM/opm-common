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

#ifndef REGION_SET_MATCHER_HPP
#define REGION_SET_MATCHER_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// \file Facility for Identifying Region Collections Matching a UDQ Region Set.

namespace Opm {
    class FIPRegionStatistics;
} // namespace Opm

namespace Opm {

class RegionSetMatcher;

/// Result Set From RegionSetMatcher's Matching Process
class RegionSetMatchResult
{
public:
    /// Region Index Range for Single Region Set
    class RegionIndexRange
    {
    public:
        /// Simple forward iterator over a region index range.
        class Iterator
        {
        public:
            /// Iterator's category (forward iterator)
            using iterator_category = std::forward_iterator_tag;

            /// Iterator's value type
            using value_type = int;

            /// Iterator's difference type
            using difference_type = int;

            /// Iterator's pointer type (return type from operator->())
            using pointer = int*;

            /// Iterator's reference type (return type from operator*())
            using reference = int&;

            /// Pre-increment operator.
            ///
            /// \return *this.
            Iterator& operator++()
            {
                ++this->i_;

                return *this;
            }

            /// Post-increment operator.
            ///
            /// \return Iterator pointing to element prior to increment result.
            Iterator operator++(int)
            {
                auto iter = *this;

                ++(*this);

                return iter;
            }

            /// Dereference operator
            ///
            /// \return Element at current position.
            reference operator*() { return this->i_; }

            /// Indirection operator
            ///
            /// \return Pointer to element at current position.
            pointer operator->() { return &this->i_; }

            /// Equality predicate
            ///
            /// \param[in] that Object to which \c *this will be compared for equality.
            ///
            /// \return Whether or not \c *this equals \p that.
            bool operator==(Iterator that) const
            {
                return this->i_ == that.i_;
            }

            /// Inequality predicate
            ///
            /// \param[in] that Object to which \c *this will be compared for inequality.
            ///
            /// \return \code ! (*this == that)
            bool operator!=(Iterator that) const
            {
                return ! (*this == that);
            }

            friend class RegionIndexRange;

        private:
            /// Constructor
            ///
            /// Accessible to RegionIndexRange only.
            ///
            /// \param[in] index range element value.
            explicit Iterator(int i) : i_{i} {}

            /// Index range element value
            int i_;
        };

        /// Start of Range.
        Iterator begin() const { return Iterator{this->begin_}; }

        /// End of Range.
        Iterator end() const { return Iterator{this->end_}; }

        /// Predicate for empty index range.
        bool empty() const { return this->end_ <= this->begin_; }

        /// Name of region set to which this index range is attached.
        std::string_view regionSet() const { return this->region_; }

        friend class RegionSetMatchResult;

    private:
        /// Beginning of index range
        int begin_{};

        /// End of Range
        int end_{};

        /// Name of region set to which this region index range is attached
        std::string_view region_{};

        /// Default Constructor.
        ///
        /// Empty range.
        ///
        /// For use by RegionSetMatchResult only.
        RegionIndexRange() = default;

        /// Non-Empty Range
        ///
        /// For use by RegionSetMatchResult only.
        ///
        /// \param[in] beginID Minimum region index value.
        ///
        /// \param[in] endID One more than the maximum region index value.
        ///
        /// \param[in] region Name of region set to which this index range
        ///    is attached.
        RegionIndexRange(int beginID, int endID, std::string_view region)
            : begin_  { beginID }
            , end_    { endID }
            , region_ { region }
        {}
    };

    /// Predicate for whether or not result set is empty.
    ///
    /// \return Whether or not result set is empty.
    bool empty() const
    {
        return this->regionIDRange_.empty();
    }

    /// Predicate for whether or not result set applies to a single
    /// region in a single region set.
    ///
    /// \return Whether or not result set is a single region in a single
    ///   region set.  Useful to distinguish whether or not this result set
    ///   generates a scalar UDQ or a UDQ set in the context of a region
    ///   level UDQ.
    bool isScalar() const
    {
        return (this->regionIDRange_.size() == std::vector<int>::size_type{2})
            && (this->regionIDRange_.back() == this->regionIDRange_.front() + 1);
    }

    /// Retrieve list of (MS) well names covered by this result set.
    ///
    /// \return List MS well names covered by this result set.
    std::vector<std::string_view> regionSets() const;

    /// Retrieve number of region sets covered by this result set.
    ///
    /// \return Number of region sets covered by this result set.
    std::size_t numRegionSets() const
    {
        return this->regionSets_.size();
    }

    /// Retrieve result set's region indices for a single region set.
    ///
    /// \param[in] regSet Named region set--e.g., FIPNUM or FIPABC.  Should
    ///    usually be one of the items in the return value from \code
    ///    regionSets() \endcode.
    ///
    /// \return range of \c regSet's region indices matching the input
    ///    request.  Empty unless \p regSet is one of the return values from
    ///    \code regionSets() \endcode.
    RegionIndexRange regions(std::string_view regSet) const;

    /// Retrieve result set's region indices for a single region set.
    ///
    /// \param[in] regSet Region set number.  Should be between zero and
    ///    \code numRegionSets() - 1 \endcode inclusive.
    ///
    /// \return range of \c regSet's region indices matching the input
    ///    request.  Empty unless \p regSet is between zero and \code
    ///    numRegionSets() - 1 \endcode inclusive.
    RegionIndexRange regions(const std::size_t regSet) const;

    friend class RegionSetMatcher;

private:
    /// List of region sets covered by this result set.
    std::vector<std::string> regionSets_{};

    /// Name-to-index lookup table.
    ///
    /// User, i.e., the RegionSetMatcher, must call \code
    /// establishNameLookupIndex() \endcode to prepare the lookup table.
    std::vector<std::vector<std::string>::size_type> regionSetIndex_{};

    /// Minimum and maximum region IDs for all region sets in this result set.
    std::vector<int> regionIDRange_{};

    /// Build region set name to region set number lookup index.
    ///
    /// For use by RegionSetMatcher only.
    void establishNameLookupIndex();

    /// Add non-empty range of region indices for single region set to
    /// result set.
    ///
    /// For use by RegionSetMatcher only.
    ///
    /// \param[in] regSet Name of region set (e.g., FIPNUM or FIPABC).
    ///
    /// \param[in] beginRegID Minimum region ID in match result for \p regSet.
    ///
    /// \param[in] endRegID One more than the maximum region ID in match
    ///   result for \p regSet.  Must not be less than \p minRegID.
    void addRegionIndices(const std::string& regSet,
                          int                beginRegID,
                          int                endRegID);
};

/// Encapsulation of Matching Process for Region Level Expressions
///
/// Primary use case is determining the set of region indices used to define
/// region level UDQs, or to evaluate region level expressions which go into
/// other UDQs, e.g., at the field level.  Typical region level quantities in
/// this context are
///
///    ROPR         - Oil production rate in all regions of *all* region sets
///    ROPR_NUM     - Oil production rate in all regions of
///                   standard/predefined 'FIPNUM' region set
///    ROPR_ABC     - Oil production rate in all regions of
///                   user defined 'FIPABC' region set.
///    ROPR_ABC 42  - Oil production rate in region 42 of user defined
///                   'FIPABC' region set.
///
/// The user initiates the matching process by constructing a SetDescriptor
/// object, filling in the known pieces of information--the vector name and
/// region ID--if applicable.  A SetDescriptor region ID will match all
/// regions in the pertinent region set or region sets.
///
/// The matching process, \code RegionSetMatcher::findRegions() \endcode,
/// forms a \c RegionSetMatchResult object which holds a list of matching
/// region sets and their associate/corresponding matching region IDs.
class RegionSetMatcher
{
public:
    /// Description of Particular Region Set Collection
    ///
    /// User specified.
    class SetDescriptor
    {
    public:
        /// Assign request's region number.
        ///
        /// Non-positive matches all regions.
        ///
        /// \param[in] region Requests's region number.
        ///
        /// \return \code *this \endcode.
        SetDescriptor& regionID(const int region);

        /// Assign request's region number.
        ///
        /// String version.  Supports both quoted and unquoted strings.
        /// Wildcard ('*') and string representation of a negative number
        /// (e.g., '-1'), match all regions.
        ///
        /// \param[in] region Requests's region number.  Must be a text
        ///    representation of an integer or one of the recognized options
        ///    for matching all regions.  Throws exception otherwise.
        ///
        /// \return \code *this \endcode.
        SetDescriptor& regionID(std::string_view region);

        /// Retrieve request's region number
        ///
        /// \return Region number.  Unset if request matches all regions.
        const std::optional<int>& regionID() const
        {
            return this->regionId_;
        }

        /// Assign request's vector name
        ///
        /// \param[in] vector.  Summary vector keyword, e.g., ROPR_ABC.
        ///
        /// \return \code *this \endcode.
        SetDescriptor& vectorName(std::string_view vector);

        /// Retrieve request's region set name
        ///
        /// \return Region set name.  Unset if request matches all region
        ///    sets (e.g., vector = ROPR).
        const std::optional<std::string>& regionSet() const
        {
            return this->regionSet_;
        }

    private:
        /// Request's well name or well name pattern.  Unset if request
        /// applies to all MS wells.
        std::optional<std::string> regionSet_{};

        /// Request's region index.  Unset if request applies to all
        /// regions of pertinent region set.
        std::optional<int> regionId_{};
    };

    /// Default constructor
    ///
    /// Disabled.
    RegionSetMatcher() = delete;

    /// Constructor
    ///
    /// \param[in] fipRegStats Basic statistics about model's region sets.
    explicit RegionSetMatcher(const FIPRegionStatistics& fipRegStats);

    /// Copy constructor
    ///
    /// Disabled.
    ///
    /// \param[in] rhs Source object.
    RegionSetMatcher(const RegionSetMatcher& rhs) = delete;

    /// Move constructor
    ///
    /// \param[in,out] rhs Source object.  Left in empty state upon return.
    RegionSetMatcher(RegionSetMatcher&& rhs);

    /// Assignment operator
    ///
    /// Disabled.
    ///
    /// \param[in] rhs Source object.
    ///
    /// \return \code *this \endcode.
    RegionSetMatcher& operator=(const RegionSetMatcher& rhs) = delete;

    /// Move-assignment operator
    ///
    /// \param[in,out] rhs Source object.  Left in empty state upon return.
    ///
    /// \return \code *this \endcode.
    RegionSetMatcher& operator=(RegionSetMatcher&& rhs);

    /// Destructor
    ///
    /// Needed to implement "pimpl" idiom.
    ~RegionSetMatcher();

    /// Determine collection of region sets and corresponding region indices
    /// matching an input set description.
    ///
    /// Set is typically derived from description of user defined quantities
    /// at the region level, e.g.,
    ///
    /// \code
    ///   UDQ
    ///   DEFINE RURAL ROPR_NUM + RWPR_NUM /
    ///   /
    /// \endcode
    ///
    /// which represents the surface level liquid production rate for all
    /// regions in the region set 'FIPNUM'.
    ///
    /// \param[in] regSet selection description.
    ///
    /// \return Collection of region sets and corresponding region indices
    ///    matching input set description.
    RegionSetMatchResult findRegions(const SetDescriptor& regSet) const;

private:
    /// Implementation class.
    class Impl;

    /// Pointer to implementation.
    std::unique_ptr<Impl> pImpl_{};
};

} // namespace Opm

#endif // REGION_SET_MATCHER_HPP
