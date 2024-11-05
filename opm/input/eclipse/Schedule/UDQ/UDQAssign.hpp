/*
  Copyright 2018 Statoil ASA.

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

#ifndef UDQASSIGN_HPP_
#define UDQASSIGN_HPP_

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm::RestartIO {
    class RstUDQ;
}

namespace Opm {

/// Representation of a UDQ ASSIGN statement
class UDQAssign
{
public:
    /// Type alias for a vector of strings.  Simplifies function signatures.
    using VString = std::vector<std::string>;

    /// Type alias for a vector of enumerated items.  Simplifies function
    /// signatures.
    using VEnumItems = std::vector<UDQSet::EnumeratedItems>;

    /// Call-back function type for a well/group name matcher.  Takes a
    /// selector and returns a vector of matching well/group names.
    using WGNameMatcher = std::function<VString(const VString&)>;

    /// Call-back function type for a matcher of enumerated items.  Takes a
    /// selector and returns a vector of such items.
    using ItemMatcher = std::function<VEnumItems(const VString&)>;

    /// Default constructor.
    UDQAssign() = default;

    /// Constructor.
    ///
    /// \param[in] keyword UDQ name.
    ///
    /// \param[in] selector Collection of entity names to which this
    /// assignment applies.  Might, for instance, be a selection of well or
    /// group names for a well/group level UDQ, or an empty vector for a
    /// scalar/field level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities named in the \p
    /// selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    UDQAssign(const std::string& keyword,
              const VString&     selector,
              double             value,
              std::size_t        report_step);

    /// Constructor.
    ///
    /// \param[in] keyword UDQ name.
    ///
    /// \param[in] selector Collection of named and numbered entities to
    /// which this assignment applies.  Might, for instance, be a selection
    /// of well segments for a segment level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities identified in
    /// the \p selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    UDQAssign(const std::string& keyword,
              const VEnumItems&  selector,
              double             value,
              std::size_t        report_step);

    /// Constructor.
    ///
    /// Assumes ownership over the selector.
    ///
    /// \param[in] keyword UDQ name.
    ///
    /// \param[in] selector Collection of named and numbered entities to
    /// which this assignment applies.  Might, for instance, be a selection
    /// of well segments for a segment level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities identified in
    /// the \p selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    UDQAssign(const std::string& keyword,
              VEnumItems&&       selector,
              double             value,
              std::size_t        report_step);

    /// Constructor.
    ///
    /// Reconstitutes an assignment from restart file information
    ///
    /// \param[in] keyword UDQ name.
    ///
    /// \param[in] assignRst Aggregate UDQ assignment information restored
    /// from restart file information.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    UDQAssign(const std::string&       keyword,
              const RestartIO::RstUDQ& assignRst,
              const std::size_t        report_step);

    /// Create a serialisation test object.
    static UDQAssign serializationTestObject();

    /// Name of UDQ to which this assignment applies.
    const std::string& keyword() const;

    /// Kind of UDQ to which this assignment applies.
    UDQVarType var_type() const;

    /// Add new record to existing UDQ assignment.
    ///
    /// \param[in] selector Collection of entity names to which this
    /// assignment applies.  Might, for instance, be a selection of well or
    /// group names for a well/group level UDQ, or an empty vector for a
    /// scalar/field level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities named in the \p
    /// selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_record(const VString& selector,
                    double         value,
                    std::size_t    report_step);

    /// Add new record to existing UDQ assignment.
    ///
    /// \param[in] selector Collection of named and numbered entities to
    /// which this assignment applies.  Might, for instance, be a selection
    /// of well segments for a segment level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities identified in
    /// the \p selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_record(const VEnumItems& selector,
                    double            value,
                    std::size_t       report_step);

    /// Add new record to existing UDQ assignment.
    ///
    /// Assumes ownership over the selector.
    ///
    /// \param[in] selector Collection of named and numbered entities to
    /// which this assignment applies.  Might, for instance, be a selection
    /// of well segments for a segment level UDQ.
    ///
    /// \param[in] value Numeric UDQ value for the entities identified in
    /// the \p selector.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_record(VEnumItems&& selector,
                    double       value,
                    std::size_t  report_step);

    /// Add new record to existing UDQ assignment.
    ///
    /// Reconstitutes an assignment from restart file information.  Mostly
    /// needed for interface compatibility in generic code.
    ///
    /// \param[in] assignRst Aggregate UDQ assignment information restored
    /// from restart file information.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_record(const RestartIO::RstUDQ& assignRst,
                    const std::size_t        report_step);

    /// Apply current assignment to a selection of enumerated items.
    ///
    /// \param[in] items Collection of named and numbered entities to which
    /// apply this assignment.  Might, for instance, be a selection of well
    /// segments for a segment level UDQ.
    ///
    /// \return UDQ set of the current assignment applied to \p items.
    /// Items known at construction time, or defined in subsequent calls to
    /// member function add_record(), will have a defined value in the
    /// resulting UDQ set while unrecognised items will have an undefined
    /// value.
    UDQSet eval(const VEnumItems& items) const;

    /// Apply current assignment to a selection of named items.
    ///
    /// \param[in] wells Collection of named entities to which apply this
    /// assignment.  Might, for instance, be a selection of well names for a
    /// well level UDQ.
    ///
    /// \return UDQ set of the current assignment applied to \p wells.
    /// Named items known at construction time, or defined in subsequent
    /// calls to member function add_record(), will have a defined value in
    /// the resulting UDQ set while unrecognised items will have an
    /// undefined value.
    UDQSet eval(const VString& wells) const;

    /// Construct scalar UDQ set for a scalar UDQ assignment
    ///
    /// Throws an exception of type \code std::invalid_argument \endcode if
    /// this assignment statement does not pertain to a scalar or field
    /// level UDQ.
    ///
    /// \return UDQ set of the current assignment of a scalar UDQ.
    UDQSet eval() const;

    /// Apply current assignment to a selection of named items.
    ///
    /// \param[in] wgNames Backing sequence of wells/groups for which to
    /// create a UDQ well/group set.  Return value will be sized according
    /// to this sequence.
    ///
    /// \param[in] matcher Call-back for identifying wells/groups matching a
    /// selector.  Might for instance be a wrapper around \code
    /// WellMatcher::wells(pattern) \endcode.
    ///
    /// \return UDQ set of the current assignment applied to \p wgNames.
    /// Wells/groups identified by the \p matcher, will have a defined value
    /// in the resulting UDQ set while unrecognised wells/groups will have
    /// an undefined value.
    UDQSet eval(const VString& wgNames, WGNameMatcher matcher) const;

    /// Apply current assignment to a selection of enumerated items.
    ///
    /// \param[in] items Collection of named and numbered entities to which
    /// apply this assignment.  Might, for instance, be a selection of well
    /// segments for a segment level UDQ.  Return value will be sized
    /// according to this sequence.
    ///
    /// \param[in] matcher Call-back for identifying items matching a
    /// selector.
    ///
    /// \return UDQ set of the current assignment applied to \p items.
    /// Those items that are identified by the \p matcher, will have a
    /// defined value in the resulting UDQ set while unrecognised items will
    /// have an undefined value.
    UDQSet eval(const VEnumItems& items, ItemMatcher matcher) const;

    /// Time at which this assignment happens.
    ///
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    ///
    /// \return Constructor argument of the same name.
    std::size_t report_step() const;

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const UDQAssign& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_keyword);
        serializer(m_var_type);
        serializer(records);
    }

private:
    // If the same keyword is assigned several times the different
    // assignment records are assembled in one UDQAssign instance.  This is
    // an attempt to support restart in a situation where a full UDQ ASSIGN
    // statement can be swapped with a UDQ DEFINE statement.
    struct AssignRecord
    {
        /// Collection of entity names to which this assignment applies.
        ///
        /// Might, for instance, be a selection of well or group names for a
        /// well/group level UDQ, or an empty vector for a scalar/field
        /// level UDQ.
        ///
        /// Empty for an enumerated assignment record.
        VString input_selector{};

        /// Collection of named and numbered entities to which this
        /// assignment applies.
        ///
        /// Might, for instance, be a selection of well segments for a
        /// segment level UDQ.
        ///
        /// Empty for a named assignment record.
        VEnumItems numbered_selector{};

        /// Numeric UDQ value for the entities identified in the selector.
        double value{};

        /// Time at which this assignment happens.
        ///
        /// Assignments should be performed exactly once and the time value
        /// ensures this behaviour.
        std::size_t report_step{};

        /// Default constructor.
        AssignRecord() = default;

        /// Constructor.
        ///
        /// \param[in] selector Collection of entity names to which this
        /// assignment applies.  Might, for instance, be a selection of well
        /// or group names for a well/group level UDQ, or an empty vector
        /// for a scalar/field level UDQ.
        ///
        /// \param[in] value_arg Numeric UDQ value for the entities named in
        /// the \p selector.
        ///
        /// \param[in] report_step_arg Time at which this assignment
        /// happens.  Assignments should be performed exactly once and the
        /// time value ensures this behaviour.
        AssignRecord(const VString&    selector,
                     const double      value_arg,
                     const std::size_t report_step_arg)
            : input_selector(selector)
            , value         (value_arg)
            , report_step   (report_step_arg)
        {}

        /// Constructor.
        ///
        /// \param[in] selector Collection of named and numbered entities to
        /// which this assignment applies.  Might, for instance, be a
        /// selection of well segments for a segment level UDQ.
        ///
        /// \param[in] value_arg Numeric UDQ value for the entities
        /// identified in the \p selector.
        ///
        /// \param[in] report_step_arg Time at which this assignment
        /// happens.  Assignments should be performed exactly once and the
        /// time value ensures this behaviour.
        AssignRecord(const VEnumItems& selector,
                     const double      value_arg,
                     const std::size_t report_step_arg)
            : numbered_selector(selector)
            , value            (value_arg)
            , report_step      (report_step_arg)
        {}

        /// Constructor.
        ///
        /// Assumes ownership over the selector.
        ///
        /// \param[in] selector Collection of named and numbered entities to
        /// which this assignment applies.  Might, for instance, be a
        /// selection of well segments for a segment level UDQ.
        ///
        /// \param[in] value_arg Numeric UDQ value for the entities
        /// identified in the \p selector.
        ///
        /// \param[in] report_step_arg Time at which this assignment happens.
        /// Assignments should be performed exactly once and the time value
        /// ensures this behaviour.
        AssignRecord(VEnumItems&&      selector,
                     const double      value_arg,
                     const std::size_t report_step_arg)
            : numbered_selector(std::move(selector))
            , value            (value_arg)
            , report_step      (report_step_arg)
        {}

        /// Apply assignment record to existing UDQ set
        ///
        /// Populates members of UDQ set that are known to the current
        /// assignment record.
        ///
        /// \param[in,out] values UDQ set for which to assign elements.
        void eval(UDQSet& values) const;

        /// Apply assignment record to existing UDQ set
        ///
        /// \param[in] matcher Call-back for identifying wells/groups
        /// matching a selector.  Might for instance be a wrapper around
        /// \code WellMatcher::wells(pattern) \endcode.
        ///
        /// \param[in,out] values UDQ set for which to assign elements.
        void eval(WGNameMatcher matcher, UDQSet& values) const;

        /// Apply assignment record to existing UDQ set
        ///
        /// \param[in] matcher Call-back for identifying items matching a
        /// selector.
        ///
        /// \param[in,out] values UDQ set for which to assign elements.
        void eval(ItemMatcher matcher, UDQSet& values) const;

        /// Equality predicate
        ///
        /// \param[in] data Record against which the current object will be
        /// compared for equality.
        ///
        /// \return \code *this == data \endcode.
        bool operator==(const AssignRecord& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->input_selector);
            serializer(this->numbered_selector);
            serializer(this->value);
            serializer(this->report_step);
        }

    private:
        /// Apply assignment record to existing UDQ set
        ///
        /// \param[in] items Collection of named and numbered entities to
        /// which this assignment applies.  Typically the \c
        /// numbered_selector or a sequence defined by a matcher applied to
        /// the \c input_selector.
        ///
        /// \param[in,out] values UDQ set for which to assign elements.
        void assignEnumeration(const VEnumItems& items, UDQSet& values) const;
    };

    /// Name of UDQ to which this assignment applies.
    std::string m_keyword{};

    /// Kind of UDQ to which this assignment applies.
    UDQVarType m_var_type{UDQVarType::NONE};

    /// Assignment records for this UDQ assignment.
    std::vector<AssignRecord> records{};

    /// Reconstitute well or group level assignment from restart file
    /// information
    ///
    /// \param[in] assignRst Aggregate UDQ assignment information restored
    /// from restart file information.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_well_or_group_records(const RestartIO::RstUDQ& assignRst,
                                   const std::size_t        report_step);

    /// Reconstitute segment level assignment from restart file information
    ///
    /// \param[in] assignRst Aggregate UDQ assignment information restored
    /// from restart file information.
    ///
    /// \param[in] report_step Time at which this assignment happens.
    /// Assignments should be performed exactly once and the time value
    /// ensures this behaviour.
    void add_segment_records(const RestartIO::RstUDQ& assignRst,
                             const std::size_t        report_step);
};

} // namespace Opm

#endif // UDQASSIGN_HPP_
