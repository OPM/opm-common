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
    UDQAssign(const std::string&              keyword,
              const std::vector<std::string>& selector,
              double                          value,
              std::size_t                     report_step);

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
    UDQAssign(const std::string&                          keyword,
              const std::vector<UDQSet::EnumeratedItems>& selector,
              double                                      value,
              std::size_t                                 report_step);

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
    UDQAssign(const std::string&                     keyword,
              std::vector<UDQSet::EnumeratedItems>&& selector,
              double                                 value,
              std::size_t                            report_step);

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
    void add_record(const std::vector<std::string>& selector,
                    double                          value,
                    std::size_t                     report_step);

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
    void add_record(const std::vector<UDQSet::EnumeratedItems>& selector,
                    double                                      value,
                    std::size_t                                 report_step);

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
    void add_record(std::vector<UDQSet::EnumeratedItems>&& selector,
                    double                                 value,
                    std::size_t                            report_step);

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
    UDQSet eval(const std::vector<UDQSet::EnumeratedItems>& items) const;

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
    UDQSet eval(const std::vector<std::string>& wells) const;

    /// Construct scalar UDQ set for a scalar UDQ assignment
    ///
    /// Throws an exception of type \code std::invalid_argument \endcode if
    /// this assignment statement does not pertain to a scalar or field
    /// level UDQ.
    ///
    /// \return UDQ set of the current assignment of a scalar UDQ.
    UDQSet eval() const;

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
        std::vector<std::string> input_selector{};

        /// Collection of named and numbered entities to which this
        /// assignment applies.
        ///
        /// Might, for instance, be a selection of well segments for a
        /// segment level UDQ.
        ///
        /// Empty for a named assignment record.
        std::vector<UDQSet::EnumeratedItems> numbered_selector{};

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
        AssignRecord(const std::vector<std::string>& selector,
                     const double                    value_arg,
                     const std::size_t               report_step_arg)
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
        AssignRecord(const std::vector<UDQSet::EnumeratedItems>& selector,
                     const double                                value_arg,
                     const std::size_t                           report_step_arg)
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
        AssignRecord(std::vector<UDQSet::EnumeratedItems>&& selector,
                     const double                           value_arg,
                     const std::size_t                      report_step_arg)
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
