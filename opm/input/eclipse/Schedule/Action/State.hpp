/*
  Copyright 2020 Equinor ASA.

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

#ifndef ACTION_STATE_HPP
#define ACTION_STATE_HPP

#include <cstddef>
#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Opm::RestartIO {
    struct RstState;
} // namespace Opm::RestartIO

namespace Opm::Action {

    class ActionX;
    class Actions;
    class PyAction;
    class Result;

} // namespace Opm::Action

namespace Opm::Action {

/// Management information about the current run's ACTION system, especially
/// concerning the number of times each action has triggered/run and the
/// last time it was run.
class State
{
public:
    /// Matching entities from a successfully triggered ActionX object
    class MatchSet
    {
    public:
        /// Whether or not a particular well exists in the set of matching
        /// entities.
        ///
        /// \param[in] well Well name.
        ///
        /// \return Wheter or not \p well exists in the set of matching
        /// entities.
        bool hasWell(const std::string& well) const;

        /// Create a serialisation test object.
        static MatchSet serializationTestObject();

        /// Equality predicate.
        ///
        /// \param[in] that Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p that.
        bool operator==(const MatchSet& that) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->wells_);
        }

        friend class State;

    private:
        /// Set of triggering wells.
        std::vector<std::string> wells_{};
    };

    /// Record ActionX Run
    ///
    /// \param[in] action Action object.
    ///
    /// \param[in] sim_time Time at which action object ran.
    ///
    /// \param[in] result Result of evaluating the action triggers,
    /// including any matching entities such as wells.
    void add_run(const ActionX& action, std::time_t sim_time, const Result& result);

    /// Record PyAction Run
    ///
    /// \param[in] action PyAction object.
    ///
    /// \param[in] result Result of evaluating the PyAction.
    void add_run(const PyAction& action, bool result);

    /// Retrieve number of times an action has run
    ///
    /// \param[in] action Action object.
    ///
    /// \return Number of times \p action has run.
    std::size_t run_count(const ActionX& action) const;

    /// Retrieve timestamp of the last time an action ran.
    ///
    /// Will throw an exception of type std::invalid_argument if the action
    /// has never run, i.e., if \code run_count(action) == 0 \endcode.
    ///
    /// \param[in] action Action object.
    ///
    /// \return Time point of \p action's last execution.
    std::time_t run_time(const ActionX& action) const;

    /// Retrieve set of matching entities from the last time an action ran.
    ///
    /// \param[in] action Action name.
    ///
    /// \return Set of matching entities.  Nullptr if no such set
    /// exists--e.g., if the action did not yet run or if there were no
    /// matching entities the last time the action ran.
    const MatchSet* result(const std::string& action) const;

    /// Query for the result of running a PyAction.
    ///
    /// \param[in] action Action name.
    ///
    /// \return PyAction result.  Nullopt if the action has not yet run.
    std::optional<bool> python_result(const std::string& action) const;

    /// Load action state from restart file
    ///
    /// \param[in] action_config Run's ActionX and PyAction objects.
    ///
    /// \param[in] rst_state Run state (time and count) for the run's
    /// ActionX objects.
    void load_rst(const Actions& action_config,
                  const RestartIO::RstState& rst_state);

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->run_state);
        serializer(this->last_result);
        serializer(this->m_python_result);
    }

    /// Create a serialisation test object.
    static State serializationTestObject();

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const State& other) const;

private:
    /// Run counts and timestamps for individual ActionX objects.
    struct RunState
    {
        /// Default constructor.
        RunState() = default;

        /// Constructor
        ///
        /// \param[in] sim_time Time at which action object ran.
        explicit RunState(const std::time_t sim_time)
            : run_count(1)
            , last_run(sim_time)
        {}

        /// Record ActionX Run
        ///
        /// \param[in] sim_time Time at which action object ran.
        void add_run(const std::time_t sim_time)
        {
            this->last_run = sim_time;
            this->run_count += 1;
        }

        /// Create a serialisation test object.
        static RunState serializationTestObject()
        {
            RunState rs;

            rs.run_count = 100;
            rs.last_run = 123456;

            return rs;
        }

        /// Equality predicate.
        ///
        /// \param[in] other Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p other.
        bool operator==(const RunState& other) const
        {
            return (this->run_count == other.run_count)
                && (this->last_run == other.last_run);
        }

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->run_count);
            serializer(this->last_run);
        }

        /// Action run count.
        std::size_t run_count{};

        /// Timestamp of action's last run.
        std::time_t last_run{};
    };

    /// Action ID: Pair of action name and a numeric ID.
    ///
    /// Numeric ID needed to distinguish multiple definitions of the same
    /// ACTIONX name.
    using ActionID = std::pair<std::string, std::size_t>;

    /// Run count and timestamps for all ActionX objects that have run at
    /// least once.
    std::map<ActionID, RunState> run_state{};

    /// Matching entities--typically wells--from successfully triggered
    /// ActionX objects.
    std::map<std::string, MatchSet> last_result{};

    /// PyAction results.
    std::map<std::string, bool> m_python_result{};
};

} // namespace Opm::Action

#endif // ACTION_STATE_HPP
