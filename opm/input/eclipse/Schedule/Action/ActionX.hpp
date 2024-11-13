/*
  Copyright 2013 Statoil ASA.

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

#ifndef ActionX_HPP_
#define ActionX_HPP_

#include <opm/input/eclipse/Schedule/Action/ActionAST.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>
#include <opm/input/eclipse/Schedule/Action/Condition.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <cstddef>
#include <ctime>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {
class WellMatcher;
class Actdims;
} // namespace Opm

namespace Opm::RestartIO {
struct RstAction;
} // namespace Opm::RestartIO

namespace Opm::Action {
class State;
} // namespace Opm::Action

namespace Opm::Action {

// The ActionX class internalizes the ACTIONX keyword. This keyword represents a
// small in-deck programming language for the SCHEDULE section. In the deck the
// ACTIONX keyword comes together with a 'ENDACTIO' kewyord and then a list of
// regular keywords in-between. The principle is then that ACTIONX represents
// a condition, and when that condition is satisfied the keywords are applied. In
// the example below the ACTIONX keyword defines a condition on well OPX having
// watercut above 0.75. When the condition is met the WELOPEN keyword is applied,
// shutting the well.
//
//   ACTIONX
//      'NAME'  /
//      WWCT OPX > 0.50 /
//   /
//
//   WELOPEN
//      'OPX'  OPEN /
//   /
//
//   ENDACTIO

class ActionX
{
public:
    /// Keyword validity predicate.
    ///
    /// \param[in] keyword Name of SCHEDULE section keyword.
    ///
    /// \return Whether or not the current implementation supports using \p
    /// keyword in an ACTIONX block.
    static bool valid_keyword(const std::string& keyword);

    /// Default constructor.
    ///
    /// Creates an invalid object that is only usable as a target for a
    /// deserialisation process.
    ActionX();

    /// Constructor.
    ///
    /// Creates an invalid object without any triggering conditions.  Mostly
    /// usable as an error state.
    ///
    /// \param[in] name Name of action object.  Typically from first record
    /// of ACTIONX keyword.
    ///
    /// \param[in] max_run Maximum number of times this action can
    /// run/trigger.  A value of zero is treated as an unlimited number of
    /// runs.
    ///
    /// \param[in] min_wait Minimum wait time, in seconds, between two
    /// successful triggerings of this action object.
    ///
    /// \param[in] start_time Point in time at which this action object is
    /// created.  Typically the simulated time at the start of the report
    /// step at which an ACTIONX keyword is encountered.  The first wait
    /// time is relative to this start time.
    ActionX(const std::string& name,
            std::size_t max_run,
            double min_wait,
            std::time_t start_time);

    /// Constructor.
    ///
    /// Creates an invalid object that is mostly usable as an error state.
    /// Delegates to four argument constructor.
    ///
    /// \param[in] record First record of an ACTIONX keyword.
    ///
    /// \param[in] start_time Point in time at which this action object is
    /// created.  Typically the simulated time at the start of the report
    /// step at which an ACTIONX keyword is encountered.  The first wait
    /// time is relative to this start time.
    ActionX(const DeckRecord& record, std::time_t start_time);

    /// Constructor.
    ///
    /// Forms the ActionX object based on restart file information.
    ///
    /// \param[in] rst_action Restart file representation of an ACTIONX
    /// block.
    explicit ActionX(const RestartIO::RstAction& rst_action);

    /// Constructor.
    ///
    /// Internalises the triggering condition into an expression tree and
    /// records minimum wait time and maximum run counts.  Associate
    /// SCHEDULE keywords to run when the action triggers must be included
    /// through addKeyword() calls.
    ///
    /// \param[in] name Name of action object.  Typically from first record
    /// of ACTIONX keyword.
    ///
    /// \param[in] max_run Maximum number of times this action can
    /// run/trigger.  A value of zero is treated as an unlimited number of
    /// runs.
    ///
    /// \param[in] min_wait Minimum wait time, in seconds, between two
    /// successful triggerings of this action object.
    ///
    /// \param[in] start_time Point in time at which this action object is
    /// created.  Typically the simulated time at the start of the report
    /// step at which an ACTIONX keyword is encountered.  The first wait
    /// time is relative to this start time.
    ///
    /// \param[in] conditions Partially formed individual statements of the
    /// triggering conditions for this action.  For restart file output
    /// purposes.
    ///
    /// \param[in] tokens Textual representation of condition block.  Will be
    /// used to form the internal expression tree representation of the
    /// triggering condition.
    ActionX(const std::string& name,
            std::size_t max_run,
            double min_wait,
            std::time_t start_time,
            std::vector<Condition>&& conditions,
            const std::vector<std::string>& tokens);

    /// Create a serialisation test object.
    static ActionX serializationTestObject();

    /// Include SCHEDULE section keyword in block executed when action triggers.
    ///
    /// \param[in] kw SCHEDULE section keyword.
    void addKeyword(const DeckKeyword& kw);

    /// Query whether or not the current ActionX object is ready to run.
    ///
    /// Will typically consider at least the object's current run count and
    /// wait times.
    ///
    /// \param[in] state Dynamic run count and run time.
    ///
    /// \param[in] sim_time Simulated time since simulation start.
    ///
    /// \return Whether or not this ActionX object is ready to run.
    bool ready(const State& state, std::time_t sim_time) const;

    /// Evaluate the action's conditions at current dynamic state
    ///
    /// \param[in] context Current summary vectors and wells
    ///
    /// \return Condition value.  A 'true' result means the condition is
    /// satisfied while a 'false' result means the condition is not
    /// satisfied.  Any wells for which the expression is true will be
    /// included in the result set.  A 'false' result has no matching
    /// wells.
    Result eval(const Context& context) const;

    /// Retrive list of well names used in action block WELPI keywords
    ///
    /// \param[in] well_matcher Final arbiter for wells currently known to
    /// simulator.
    ///
    /// \param[in] matches List of entities for which the current action
    /// triggered.  Function wellpi_wells() will only inspect the set of
    /// matching/triggering wells, and only if a WELPI keyword in the
    /// current action's keyword block uses the well name pattern "?".
    ///
    /// \return List of well names used in this action's WELPI keywords, if
    /// any.  List returned in sorted order defined by \p well_matcher.
    std::vector<std::string>
    wellpi_wells(const WellMatcher&              well_matcher,
                 const Result::MatchingEntities& matches) const;

    /// Export all summary vectors needed to evaluate the conditions of the
    /// current ActionX object.
    ///
    /// \param[in,out] required_summary Named summary vectors.  Upon
    /// completion, any additional summary vectors needed to evaluate full
    /// condition block of the current ActionX object will be included in
    /// this set.
    void required_summary(std::unordered_set<std::string>& required_summary) const;

    /// Retrieve name of action object.
    std::string name() const { return this->m_name; }

    /// Retrieve maximum number of times this action can run/trigger.
    std::size_t max_run() const { return this->m_max_run; }

    /// Retrieve minimum wait time, in seconds of simulated time, between
    /// action triggers.
    double min_wait() const { return this->m_min_wait; }

    /// Retrieve distinguishing numeric ID of this action object.
    std::size_t id() const { return this->m_id; }

    /// Assign distinguishing numeric ID of this action object.
    ///
    /// Typically invoked only if encountering an ACTIONX keyword naming the
    /// same action object as an earlier ACTIONX keyword.
    void update_id(std::size_t id);

    /// Retrieve point in time at which this action object is created.
    ///
    /// Typically the simulated time at the start of the report step at
    /// which an ACTIONX keyword is encountered.  The first wait time is
    /// relative to this start time.
    std::time_t start_time() const { return this->m_start_time; }

    /// Start of action block SCHEDULE keyword sequence.
    auto begin() const { return this->keywords.begin(); }

    /// End of action block SCHEDULE keyword sequence.
    auto end() const { return this->keywords.end(); }

    /// Intermediate representation of triggering conditions for restart
    /// file output purposes.
    const std::vector<Condition>& conditions() const
    {
        return this->m_conditions;
    }

    /// Textual representation of action block SCHEDULE keywords.
    ///
    /// For restart file output purposes.
    std::vector<std::string> keyword_strings() const;

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const ActionX& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_max_run);
        serializer(m_min_wait);
        serializer(m_start_time);
        serializer(m_id);
        serializer(keywords);
        serializer(condition);
        serializer(m_conditions);
    }

private:
    /// Action name.
    std::string m_name{};

    /// Maximum number of times this action can run/trigger.
    std::size_t m_max_run = 0;

    /// Minimum wait time, in seconds of simulated time, between action
    /// triggers.
    double m_min_wait = 0.0;

    /// Point in time at which this action object is created.  Typically the
    /// simulated time at the start of the report step at which an ACTIONX
    /// keyword is encountered.  The first wait time is relative to this
    /// start time.
    std::time_t m_start_time;

    /// Triggering condition for this action object.
    AST condition{};

    /// Distinguishing numeric ID of this action object.
    ///
    /// Incremented each time the action object is redefined, typically
    /// because a new ACTIONX keyword with the same name is encountered.
    ///
    /// Note: Typically a small number in any real simulation run, so we
    /// might be able to use a (nominally) smaller data type here--e.g.,
    /// unsigned int or maybe even unsigned char.
    std::size_t m_id = 0;

    /// Sequence of keywords to execute when the action condition triggers.
    std::vector<DeckKeyword> keywords{};

    /// List of triggering conditions for this action object.
    ///
    /// For restart file output purposes only.
    std::vector<Condition> m_conditions{};
};

/// \brief Parse condition block of ACTIONX keyword.
///
/// \param[in] kw Keyword representation of ActionX.
///
/// \param[in] actdims Dimensions for ActionX as specified in the deck.
///
/// \param[in] start_time Point in time at which this action object is
/// created.  Typically the simulated time at the start of the report step
/// at which an ACTIONX keyword is encountered.
///
/// \return A partially formed ActionX object containing its fully
/// internalised condition block, and a list of any error conditions--pairs
/// of error categories and descriptive messages--encountered while parsing
/// the ACTIONX condition block.  If there are no parse errors--i.e., when
/// get<1>(t) is empty, then the ActionX object can be completed by calling
/// its addKeyword() member function to form the actual action block.
std::pair<ActionX, std::vector<std::pair<std::string, std::string>>>
parseActionX(const DeckKeyword& kw, const Actdims& actimds, std::time_t start_time);

} // namespace Opm::Action

#endif // ActionX_HPP_
