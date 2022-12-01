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

#ifndef SEGMENT_MATCHER_HPP
#define SEGMENT_MATCHER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// \file Facility for Identifying Specific Well Segments Matching a UDQ Segment Set.

namespace Opm {
    class ScheduleState;
} // namespace Opm

namespace Opm {

/// Encapsulation of Matching Process for MSW Segment Sets
///
/// Primary use case is determining the set of MSW segments used to define
/// segment level UDQs.  Typical segment quantities in this context are
///
///    SOFR         - Oil flow rate in all segments in all MS wells
///    SOFR 'P'     - Oil flow rate in all segments of MS well 'P'
///    SOFR 'P'  1  - Oil flow rate in segment 1 of MS well 'P'
///    SOFR 'P*'    - Oil flow rate in all segments of all MS wells whose name
///                   begins with 'P'
///    SOFR 'P*' 27 - Oil flow rate in segment 27 of all MS wells whose name
///                   begins with 'P'
///    SOFR '*' 27  - Oil flow rate in segment 27 of all MS wells
///    SOFR '*' '*' - Oil flow rate in all segments of all MS wells
///
/// The user initiates the matching process by constructing a SetDescriptor
/// object, filling in the known pieces of information--well name patterns
/// and segment numbers--if applicable.  A SetDescriptor with no well name
/// or segment number will match all segments in all MS wells.
///
/// The matching process, \code SegmentMatcher::findSegments() \endcode,
/// forms a \c SegmentSet object which holds a list of matching MS wells and
/// their associate/corresponding matching segment numbers.
class SegmentMatcher
{
public:
    /// Result Set From Matching Process
    class SegmentSet
    {
    public:
        /// Demarcation of Start/End of Segment Range for Single MS Well
        using WellSegmentRangeIterator = std::vector<int>::const_iterator;

        /// Segment Range for Single MS Well.
        class WellSegmentRange
        {
        public:
            /// Start of Range
            WellSegmentRangeIterator begin() const { return this->begin_; }

            /// End of Range
            WellSegmentRangeIterator end() const { return this->end_; }

            /// Name of well to which this segment range is attached
            std::string_view well() const { return this->well_; }

            friend class SegmentSet;

        private:
            /// Start of Range
            WellSegmentRangeIterator begin_{};

            /// End of Range
            WellSegmentRangeIterator end_{};

            /// Name of well to which this segment range is attached
            std::string_view well_{};

            /// Default Constructor.
            ///
            /// Empty range.
            ///
            /// For use by SegmentSet only.
            WellSegmentRange() = default;

            /// Non-Empty Range
            ///
            /// For use by SegmentSet only.
            ///
            /// \param[in] begin Start of range.
            /// \param[in] end End of range.
            /// \param[in] well Name of well to which this segment range is
            ///    attached
            WellSegmentRange(WellSegmentRangeIterator begin,
                             WellSegmentRangeIterator end,
                             std::string_view         well)
                : begin_{ begin }
                , end_  { end }
                , well_ { well }
            {}
        };

        /// Default Constructor.
        SegmentSet();

        /// Predicate for whether or not segment set is empty.
        ///
        /// \return Whether or not segment set is empty.
        bool empty() const
        {
            return this->segments_.empty();
        }

        /// Predicate for whether or not segment set applies to a single
        /// segment in a single MS well.
        ///
        /// \return Whether or not segment set is a single segment in a
        ///   single MS well.  Useful to distinguish whether or not this
        ///   segment set generates a scalar UDQ or a UDQ set in the context
        ///   of a segment level UDQ.
        bool isScalar() const
        {
            return this->segments_.size() == std::vector<int>::size_type{1};
        }

        /// Retrieve list of (MS) well names covered by this result set.
        ///
        /// \return List MS well names covered by this result set.
        std::vector<std::string_view> wells() const;

        /// Retrieve number of (MS) wells covered by this result set.
        ///
        /// \return Number of MS wells covered by this result set.
        std::size_t numWells() const
        {
            return this->wells_.size();
        }

        /// Retrive result set's segments for single MS well.
        ///
        /// \param[in] well Named MS well.  Should usually be one of the
        ///    items in the return value from \code wells() \endcode.
        ///
        /// \return range of \c well's segments matching the input request.
        ///    Empty unless \p well is one of the return values from \code
        ///    wells() \endcode.
        WellSegmentRange segments(std::string_view well) const;

        /// Retrive result set's segments for single MS well.
        ///
        /// \param[in] well Named MS well.  Should usually be one of the
        ///    items in the return value from \code wells() \endcode.
        ///
        /// \return range of \c well's segments matching the input request.
        ///    Empty unless \p well is one of the return values from \code
        ///    wells() \endcode.
        WellSegmentRange segments(const std::size_t well) const;

        friend class SegmentMatcher;

    private:
        /// List of MS wells covered by this result set.
        std::vector<std::string> wells_{};

        /// Name-to-index lookup table.
        ///
        /// User, i.e., the SegmentMatcher, must call \code
        /// establishNameLookupIndex() \endcode to prepare the lookup table.
        std::vector<std::vector<std::string>::size_type> wellNameIndex_{};

        /// CSR start pointers for MS wells' segments.
        std::vector<std::vector<int>::size_type> segmentStart_{};

        /// All segments covered by this result set.  Structured by \c
        /// segmentStart_.
        std::vector<int> segments_{};

        /// Build well-name to well number lookup index.
        ///
        /// For use by SegmentMatcher only.
        void establishNameLookupIndex();

        /// Add non-empty range of segments for single MS well to result set.
        ///
        /// For use by SegmentMatcher only.
        ///
        /// \param[in] well Name of MS well.
        ///
        /// \param[in] segments List of segment numbers matching input
        ///    request for \p well.
        void addWellSegments(const std::string&      well,
                             const std::vector<int>& segments);
    };

    /// Description of Particular Segment Set
    ///
    /// User specified.
    class SetDescriptor
    {
    public:
        /// Default Constructor
        SetDescriptor() = default;

        /// Assign request's segment number.
        ///
        /// Non-positive matches all segments.
        ///
        /// \param[in] segNum Requests's segment number.
        ///
        /// \return \code *this \endcode.
        SetDescriptor& segmentNumber(const int segNum);

        /// Assign request's segment number.
        ///
        /// String version.  Supports both quoted and unquoted strings.
        /// Wildcard ('*') and string representation of a negative number
        /// (e.g., '-1'), match all segments.
        ///
        /// \param[in] segNum Requests's segment number.  Must be a text
        ///    representation of an integer or one of the recognized options
        ///    for matching all segments.  Throws exception otherwise.
        ///
        /// \return \code *this \endcode.
        SetDescriptor& segmentNumber(std::string_view segNum);

        /// Retrive request's segment number
        ///
        /// \return Segment number.  Unset if request matches all segments.
        const std::optional<int>& segmentNumber() const
        {
            return this->segmentNumber_;
        }

        /// Assign request's well set.
        ///
        /// \param[in] wellNamePattern Pattern for selecting specific MS
        ///    wells.  Used in normal matching, except well lists are not
        ///    supported.
        SetDescriptor& wellNames(std::string_view wellNamePattern);

        /// Retrive request's well name pattern.
        ///
        /// \return Well name pattern.  Unset if request matches all
        ///    multi-segmented wells.
        const std::optional<std::string>& wellNames() const
        {
            return this->wellNamePattern_;
        }

    private:
        /// Request's well name or well name pattern.  Unset if request
        /// applies to all MS wells.
        std::optional<std::string> wellNamePattern_{};

        /// Request's segment number.  Unset if request applies to all
        /// segments of pertinent well set.
        std::optional<int> segmentNumber_{};
    };

    /// Default constructor
    ///
    /// Disabled.
    SegmentMatcher() = delete;

    /// Constructor
    ///
    /// \param[in] mswInputData Input's notion of existing wells, both
    ///    regular and multi-segmented.  Expected to be the state of a
    ///    Schedule block at a particular report step.
    explicit SegmentMatcher(const ScheduleState& mswInputData);

    /// Copy constructor
    ///
    /// Disabled.
    ///
    /// \param[in] rhs Source object.
    SegmentMatcher(const SegmentMatcher& rhs) = delete;

    /// Move constructor
    ///
    /// \param[in,out] rhs Source object.  Left in empty state upon return.
    SegmentMatcher(SegmentMatcher&& rhs);

    /// Assignment operator
    ///
    /// Disabled.
    ///
    /// \param[in] rhs Source object.
    ///
    /// \return \code *this \endcode.
    SegmentMatcher& operator=(const SegmentMatcher& rhs) = delete;

    /// Move-assignment operator
    ///
    /// \param[in,out] rhs Source object.  Left in empty state upon return.
    ///
    /// \return \code *this \endcode.
    SegmentMatcher& operator=(SegmentMatcher&& rhs);

    /// Destructor
    ///
    /// Needed to implement "pimpl" idiom.
    ~SegmentMatcher();

    /// Determine set of MS wells and corresponding segments matching an
    /// input set description.
    ///
    /// Set is typically derived from description of user defined quantities
    /// at the segment level, e.g.,
    ///
    /// \code
    ///   UDQ
    ///   DEFINE SU-LPR1 SOFR OP01 + SWPR OP01 /
    ///   /
    /// \endcode
    ///
    /// which represents the surface level liquid production rate for all
    /// segments in the multi-segmented well OP01.
    ///
    /// \param[in] segments Segment set selection description.
    ///
    /// \return Set of MS wells and corresponding segments matching input
    ///    set description.
    SegmentSet findSegments(const SetDescriptor& segments) const;

private:
    /// Implementation class.
    class Impl;

    /// Pointer to implementation.
    std::unique_ptr<Impl> pImpl_{};
};

} // namespace Opm

#endif // SEGMENT_MATCHER_HPP
