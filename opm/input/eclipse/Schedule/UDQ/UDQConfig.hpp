/*
  Copyright 2019 Equinor ASA.

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

#ifndef UDQINPUT_HPP_
#define UDQINPUT_HPP_

#include <opm/input/eclipse/Schedule/UDQ/UDQAssign.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQDefine.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQInput.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQParams.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDT.hpp>

#include <opm/input/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/input/eclipse/EclipseState/Util/IOrderSet.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {

    class DeckRecord;
    class KeywordLocation;
    class RegionSetMatcher;
    class Schedule;
    class SegmentMatcher;
    class SummaryState;
    class UDQState;
    class WellMatcher;

} // namespace Opm

namespace Opm::RestartIO {
    struct RstState;
    class  RstUDQ;
} // namespace Opm::RestartIO

namespace Opm {

    /// Collection of all user-defined quantities in the current simulation run
    class UDQConfig
    {
    public:
        /// Factory function for constructing region set matchers.
        using RegionSetMatcherFactory = std::function<std::unique_ptr<RegionSetMatcher>()>;

        /// Factory function for constructing segment set matchers.
        using SegmentMatcherFactory = std::function<std::unique_ptr<SegmentMatcher>()>;

        /// Default constructor
        UDQConfig() = default;

        /// Constructor
        ///
        /// Main constructor for a base run.
        ///
        /// \param[in] params UDQ parameters from UDQPARAM keyword.
        explicit UDQConfig(const UDQParams& params);

        /// Constructor
        ///
        /// Main constructor for a restarted simulation run.
        ///
        /// \param[in] params UDQ parameters from UDQPARAM keyword.
        ///
        /// \param[in] rst_state Object state from restart file information.
        UDQConfig(const UDQParams& params, const RestartIO::RstState& rst_state);

        /// Create a serialisation test object.
        static UDQConfig serializationTestObject();

        /// Retrieve unit string for a particular UDQ
        ///
        /// \param[in] key Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return Input unit string associated to \p key.  Throws an
        /// exception of type \code std::invalid_argument \endcode if no
        /// unit string exists for \p key.
        const std::string& unit(const std::string& key) const;

        /// Query whether or not a particular UDQ has an associated unit
        /// string.
        ///
        /// \param[in] keyword Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return Whether or not there is a unit string associated to \p
        /// keyword.
        bool has_unit(const std::string& keyword) const;

        /// Query whether or not a particular UDQ exists in the collection.
        ///
        /// \param[in] keyword Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return whether or not \p keyword exists in the collection.
        bool has_keyword(const std::string& keyword) const;

        /// Incorporate a single UDQ record into the known collection
        ///
        /// \param[in] create_segment_matcher Factory function for
        /// constructing segment set matchers.
        ///
        /// \param[in] record UDQ keyword record, such as a DEFINE, ASSIGN,
        /// UPDATE, or UNIT statement.
        ///
        /// \param[in] location Input file/line information for the UDQ
        /// record.  Mostly for diagnostic purposes.
        ///
        /// \param[in] report_step Time at which this record is encountered.
        void add_record(SegmentMatcherFactory  create_segment_matcher,
                        const DeckRecord&      record,
                        const KeywordLocation& location,
                        std::size_t            report_step);

        /// Incorporate a unit string for a UDQ
        ///
        /// Implements the UNIT statement.
        ///
        /// \param[in] keyword Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] unit Unit string for UDQ \p keyword.
        void add_unit(const std::string& keyword,
                      const std::string& unit);

        /// Incorporate update status change for a UDQ
        ///
        /// Implements the UPDATE statement
        ///
        /// \param[in] keyword Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] report_step Time at which this record is encountered.
        ///
        /// \param[in] location Input file/line information for the UDQ
        /// record.  Mostly for diagnostic purposes.
        ///
        /// \param[in] data Update status.  Should be a single element
        /// vector containing one of the status strings ON, OFF, or NEXT.
        void add_update(const std::string&              keyword,
                        std::size_t                     report_step,
                        const KeywordLocation&          location,
                        const std::vector<std::string>& data);

        /// Incorporate a UDQ assignment.
        ///
        /// Implements the ASSIGN statement.
        ///
        /// \param[in] quantity Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] create_segment_matcher Factory function for
        /// constructing segment set matchers.
        ///
        /// \param[in] selector UDQ set element selection to which this
        /// assignment applies.  Might be a well name pattern if \p quantity
        /// is a well level UDQ.
        ///
        /// \param[in] report_step Time at which this assignment statement
        /// is encountered.
        void add_assign(const std::string&              quantity,
                        SegmentMatcherFactory           create_segment_matcher,
                        const std::vector<std::string>& selector,
                        double                          value,
                        std::size_t                     report_step);

        /// Incorporate a UDQ defining expressions.
        ///
        /// Implements the DEFINE statement.
        ///
        /// \param[in] quantity Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] location Input file/line information for the UDQ
        /// record.  Mostly for diagnostic purposes.
        ///
        /// \param[in] expression Defining expression for the UDQ.  Function
        /// add_define() parses this expression into an abstract syntax tree
        /// which is used in subsequent evaluation contexts.
        ///
        /// \param[in] report_step Time at which this assignment statement
        /// is encountered.
        void add_define(const std::string&              quantity,
                        const KeywordLocation&          location,
                        const std::vector<std::string>& expression,
                        std::size_t                     report_step);

        /// Incorporate a user defined table.
        ///
        /// Implements the UDT keyword.
        ///
        /// \param[in] name Name of user defined table.
        ///
        /// \param[in] udt Tabulated values.
        void add_table(const std::string& name, UDT udt);

        /// Clear all pending assignments
        ///
        /// Clears all internal data structures of any assignment records.
        /// Typically called at the end of a report step in order to signify
        /// that all assignments have been applied.
        ///
        /// \return Whether or not there were any active assignments in the
        /// internal representation.  Allows client code to take action, if
        /// needed, based on the knowledge that all assignments have been
        /// applied and to prepare for the next report step.
        bool clear_pending_assignments();

        /// Apply all pending assignments.
        ///
        /// Assigns new UDQ values to both the summary and UDQ state objects.
        ///
        /// \param[in] report_step Current report step.
        ///
        /// \param[in] sched Full dynamic input schedule.
        ///
        /// \param[in] wm Well name pattern matcher.
        ///
        /// \param[in] create_segment_matcher Factory function for
        /// constructing segment set matchers.
        ///
        /// \param[in,out] st Summary vectors.  For output and evaluating
        /// ACTION condition purposes.  Values pertaining to UDQs being
        /// assigned here will be updated.
        ///
        /// \param[in,out] udq_state Dynamic values for all known UDQs.
        /// Values pertaining to UDQs being assigned here will be updated.
        void eval_assign(std::size_t           report_step,
                         const Schedule&       sched,
                         const WellMatcher&    wm,
                         SegmentMatcherFactory create_segment_matcher,
                         SummaryState&         st,
                         UDQState&             udq_state) const;

        /// Compute new values for all UDQs
        ///
        /// Uses both assignment and defining expressions as applicable.
        /// Assigns new UDQ values to both the summary and UDQ state
        /// objects.
        ///
        /// \param[in] report_step Current report step.
        ///
        /// \param[in] sched Full dynamic input schedule.
        ///
        /// \param[in] wm Well name pattern matcher.
        ///
        /// \param[in] create_segment_matcher Factory function for
        /// constructing segment set matchers.
        ///
        /// \param[in] create_region_matcher Factory function for
        /// constructing region set matchers.
        ///
        /// \param[in,out] st Summary vectors.  For output and evaluating
        /// ACTION condition purposes.  Values pertaining to UDQs being
        /// assigned here will be updated.
        ///
        /// \param[in,out] udq_state Dynamic values for all known UDQs.
        /// Values pertaining to UDQs being assigned here will be updated.
        void eval(std::size_t             report_step,
                  const Schedule&         sched,
                  const WellMatcher&      wm,
                  SegmentMatcherFactory   create_segment_matcher,
                  RegionSetMatcherFactory create_region_matcher,
                  SummaryState&           st,
                  UDQState&               udq_state) const;

        /// Retrieve defining expression and evaluation object for a single
        /// UDQ
        ///
        /// \param[in] key Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return Defining expression and evaluation object for \p key.
        /// Throws an exception of type \code std::out_of_range \endcode if
        /// no such object exists for the UDQ \p key.
        const UDQDefine& define(const std::string& key) const;

        /// Retrieve any pending assignment object for a single UDQ
        ///
        /// \param[in] key Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return Pending assignment object for \p key.  Throws an
        /// exception of type \code std::out_of_range \endcode if no such
        /// object exists for the UDQ \p key.
        const UDQAssign& assign(const std::string& key) const;

        /// Retrieve defining expressions and evaluation objects for all
        /// known UDQs.
        std::vector<UDQDefine> definitions() const;

        /// Retrieve defining expressions and evaluation objects for all
        /// known UDQs of a particular category.
        ///
        /// \param[in] var_type UDQ category.
        ///
        /// \return Defining expressions and evaluation objects for all
        /// known UDQs of category \p var_type.
        std::vector<UDQDefine> definitions(UDQVarType var_type) const;

        /// Retrieve unprocessed input objects for all UDQs
        ///
        /// Needed for restart file output purposes.
        std::vector<UDQInput> input() const;

        /// Export count of all known UDQ categories in the current run.
        ///
        /// \param[out] count Count of all active UDQs of all categories in
        /// the current run.
        void exportTypeCount(std::array<int, static_cast<std::size_t>(UDQVarType::NumTypes)>& count) const;

        /// Total number of active DEFINE and ASSIGN statements.
        ///
        /// Corresponds to the length of the vector returned from input().
        std::size_t size() const;

        /// Unprocessed input object for named quantity.
        ///
        /// \param[in] keyword Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \return Unprocessed input object for \p keyword.  Throws an
        /// exception of type \code std::invalid_argument \endcode if no
        /// such named UDQ exists.
        UDQInput operator[](const std::string& keyword) const;

        /// Unprocessed input object for enumerated quantity.
        ///
        /// \param[in] insert_index Linear index in order of appearance for
        /// an individual UDQ.
        ///
        /// \return Unprocessed input object for \p keyword.  Throws an
        /// exception of type \code std::invalid_argument \endcode if no
        /// such numbered UDQ exists.
        UDQInput operator[](std::size_t insert_index) const;

        /// Retrieve pending assignment objects for all known UDQs.
        std::vector<UDQAssign> assignments() const;

        /// Retrieve pending assignment objects for all known UDQs of a
        /// particular category.
        ///
        /// \param[in] var_type UDQ category.
        ///
        /// \return Pending assignment objects for all known UDQs of
        /// category \p var_type.
        std::vector<UDQAssign> assignments(UDQVarType var_type) const;

        /// Retrieve run's active UDQ parameters
        const UDQParams& params() const;

        /// Retrieve run's active UDQ function table.
        const UDQFunctionTable& function_table() const;

        /// Retrieve run's active user defined tables.
        const std::unordered_map<std::string, UDT>& tables() const;

        /// Equality predicate.
        ///
        /// \param[in] config Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p config.
        bool operator==(const UDQConfig& config) const;

        /// Export all summary vectors needed to compute values for the
        /// current collection of user defined quantities.
        ///
        /// \param[in,out] summary_keys Named summary vectors.  Upon
        /// completion, any additional summary vectors needed to evaluate
        /// the current set of user defined quantities will be included in
        /// this set.
        void required_summary(std::unordered_set<std::string>& summary_keys) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(udq_params);
            serializer(m_definitions);
            serializer(m_assignments);
            serializer(m_tables);
            serializer(units);
            serializer(define_order);
            serializer(input_index);
            serializer(type_count);
            serializer(pending_assignments_);

            // The UDQFunction table is constant up to udq_params, so we can
            // just construct a new instance here.
            if (!serializer.isSerializing()) {
                udqft = UDQFunctionTable(udq_params);
            }
        }

    private:
        /// Run's active UDQ parameters.
        ///
        /// Initialised from the run's UDQPARAM keyword.
        UDQParams udq_params;

        /// Run's active function table table.
        UDQFunctionTable udqft;

        // The following data structures are constrained by compatibility
        // requirements in our simulation restart files.  In particular we
        // need to control the keyword ordering.  In this class the ordering
        // is maintained mainly by the input_index map which tracks the
        // insertion order of each keyword and whether the keyword (UDQ
        // name) is currently DEFINE'ed or ASSIGN'ed.

        /// Defining expressions and evaluation objects for all pertinent
        /// quantities.
        ///
        /// Keyed by UDQ name.
        std::unordered_map<std::string, UDQDefine> m_definitions{};

        /// Run's UDQ assignment statements.
        ///
        /// Keyed by UDQ name.
        std::unordered_map<std::string, UDQAssign> m_assignments{};

        /// Run's user defined input tables.
        ///
        /// Keyed by table name (i.e., TU* strings).
        std::unordered_map<std::string, UDT> m_tables{};

        /// Unit strings for some or all input UDQs.
        ///
        /// Defined only for those UDQs which have an associate UNIT
        /// statement.
        ///
        /// Keyed by UDQ name.
        std::unordered_map<std::string, std::string> units{};

        /// Ordered set of DEFINE statements.
        ///
        /// TODO: Mostly unused and should probably be removed.
        IOrderSet<std::string> define_order;

        /// Ordered set of UDQ inputs.
        OrderedMap<UDQIndex> input_index;

        /// Number of UDQs of each category currently active.
        std::map<UDQVarType, std::size_t> type_count{};

        /// List of pending assignment statements.
        ///
        /// Marked mutable because this will be modified in member function
        ///
        ///    UDQConfig::eval_assign(step, sched, context) const
        mutable std::vector<std::string> pending_assignments_{};

        /// Incorporate operation for new or existing UDQ
        ///
        /// Preserves order of operations in input_index.
        ///
        /// \param[in] quantity Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] action Kind of operation.
        void add_node(const std::string& quantity, UDQAction action);

        /// Reconstitute an assignment statement from restart file information.
        ///
        /// \param[in] udq Restart file representation of an assignment statement.
        ///
        /// \param[in] report_step Time at which this assignment is
        /// encountered.  Typically corresponds to the restart time.
        void add_assign(const RestartIO::RstUDQ& udq, const std::size_t report_step);

        /// Reconstitute a definition statement from restart file information.
        ///
        /// \param[in] udq Restart file representation of a UDQ definition
        /// statement.
        ///
        /// \param[in] report_step Time at which this definition is
        /// encountered.  Typically corresponds to the restart time.
        void add_define(const RestartIO::RstUDQ& udq, const std::size_t report_step);

        /// Apply all pending assignments.
        ///
        /// Assigns new UDQ values to both the summary and UDQ state objects.
        ///
        /// \param[in] report_step Current report step.
        ///
        /// \param[in] schedule Full dynamic input schedule.
        ///
        /// \param[in,out] context Pattern matchers and state objects.
        /// Values pertaining to UDQs being assigned here will be updated.
        void eval_assign(std::size_t     report_step,
                         const Schedule& sched,
                         UDQContext&     context) const;

        /// Compute new values for all UDQs
        ///
        /// Evaluates all applicable defining expressions.  Assigns new UDQ
        /// values to both the summary and UDQ state objects.
        ///
        /// \param[in] report_step Current report step.
        ///
        /// \param[in] schedule Full dynamic input schedule.
        ///
        /// \param[in,out] context Pattern matchers and state objects.
        /// Values pertaining to UDQs being evaluated here will be updated.
        void eval_define(std::size_t     report_step,
                         const UDQState& udq_state,
                         UDQContext&     context) const;

        /// Incorporate an enumerated assignment statement into known UDQ
        /// collection.
        ///
        /// Typically assigns a segment level UDQ for one or more segments
        /// in a single multi-segmented well.
        ///
        /// \param[in] quantity Unqualified UDQ name.  Typically names a
        /// segment level UDQ such as SUSHI.
        ///
        /// \param[in] create_segment_matcher Factory function for
        /// constructing segment set matchers.
        ///
        /// \param[in] selector UDQ set element selection to which this
        /// assignment applies.  Might be a well name and a segment number.
        ///
        /// \param[in] report_step Time at which this assignment statement
        /// is encountered.
        void add_enumerated_assign(const std::string&              quantity,
                                   SegmentMatcherFactory           create_segment_matcher,
                                   const std::vector<std::string>& selector,
                                   double                          value,
                                   std::size_t                     report_step);

        /// Common implementation of all add*assign() member functions.
        ///
        /// \tparam Args Parameter pack type for UDQAssign::add_record()
        ///
        /// \param[in] quantity Unqualified UDQ name such as FUNNY, GUITAR,
        /// WURST, or SUSHI.
        ///
        /// \param[in] args Constructor or add_record() arguments for
        /// UDQAssign object.
        template <typename... Args>
        void add_assign_impl(const std::string& quantity,
                             Args&&...          args)
        {
            // Note: Duplicate 'quantity' arguments is intentional here.
            // The first is the key in 'm_assignments', while the second is
            // the first argument to the UDQAssign constructor.
            const auto& [asgnPos, inserted] =
                this->m_assignments.try_emplace(quantity, quantity, args...);

            if (! inserted) {
                // We already have an assignment object for 'quantity'.  Add
                // a new assignment record to that object.
                asgnPos->second.add_record(std::forward<Args>(args)...);
            }
        }
    };

} // namespace Opm

#endif // UDQINPUT_HPP_
