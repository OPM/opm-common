/*
  Copyright 2018 Equinor ASA.

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

#ifndef ActionCOnfig_HPP
#define ActionCOnfig_HPP

#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>

#include <cstddef>
#include <ctime>
#include <string>
#include <vector>

namespace Opm::Action {

class State;

} // namespace Opm::Action

namespace Opm::Action {

/// Container of action keywords.
///
/// Mainly provides a list of action keywords, i.e., ACTIONX and/or
/// PYACTION, whose conditions are ready for evaluation.
class Actions
{
public:
    /// Default constructor.
    ///
    /// Resulting object is primarily useful as a target of a
    /// deserialisation operation, although may be subsequently populated
    /// through the add() member functions.
    Actions() = default;

    /// Constructor.
    ///
    /// Forms collection from sequences of individual action objects.
    ///
    /// \param[in] action Sequence of action objects formed from ACTIONX
    /// keywords.
    ///
    /// \param[in] pyactions Sequence of action objects formed from PYACTION
    /// keywords.
    Actions(const std::vector<ActionX>&  action,
            const std::vector<PyAction>& pyactions);

    /// Create a serialisation test object.
    static Actions serializationTestObject();

    /// Include ActionX object in current collection
    ///
    /// Any existing ActionX object with the same name will be replaced.
    ///
    /// \param[in] action Action object to include in the current
    /// collection.
    void add(const ActionX& action);

    /// Include PyAction object in current collection
    ///
    /// Any existing PyAction object with the same name will be replaced.
    ///
    /// \param[in] action Action object to include in the current
    /// collection.
    void add(const PyAction& pyaction);

    /// Number of ActionX objects in this collection.
    std::size_t ecl_size() const;

    /// Number of PyAction objects in this collection.
    std::size_t py_size() const;

    /// Maximum number of records in any one ACTIONX block.
    ///
    /// Needed for restart file output purposes.
    int max_input_lines() const;

    /// Whether or not this collection is empty.
    ///
    /// True if this collection has neither ActionX nor PyAction objects.
    bool empty() const;

    /// Runnability predicate.
    ///
    /// \param[in] state Current run's action state, especially run counts
    /// and trigger times.
    ///
    /// \param[in] sim_time Current simulation time.
    ///
    /// \return Whether or not any ActionX objects in the current collection
    /// are ready to run at time \p sim_time.
    bool ready(const State& state, std::time_t sim_time) const;

    /// Look up ActionX object by name.
    ///
    /// Throws an exception of type \code std::range_error \endcode if no
    /// ActionX object with the particular name exists in the current
    /// collection.
    ///
    /// \param[in] name ActionX object name.
    ///
    /// \return ActionX object whose
    const ActionX& operator[](const std::string& name) const;

    /// Look up ActionX object by linear index
    ///
    /// \param[in] index Object index.  Must be in the range
    /// 0..ecl_size()-1.
    ///
    /// \return Associated ActionX object.
    const ActionX& operator[](std::size_t index) const;

    /// Retrieve ActionX objects that are ready to run.
    ///
    /// List comprised of those ActionX objects from the internal collection
    /// whose run counts have not exceeded their associate maximum limit,
    /// and for which the minimum wait time since last execution has passed.
    ///
    /// \param[in] state Current run's action state, especially run counts
    /// and trigger times.
    ///
    /// \param[in] sim_time Current simulation time.
    ///
    /// \return Those ActionX objects that are ready to run at this time.
    std::vector<const ActionX*>
    pending(const State& state, std::time_t sim_time) const;

    /// Retrieve PyAction objects that are ready to run.
    ///
    /// \param[in] state Current run's action state, especially run counts.
    ///
    /// \return Those PyAction objects that are ready to run.
    std::vector<const PyAction*> pending_python(const State& state) const;

    /// ActionX object existence predicate
    ///
    /// \param[in] name Action name.
    ///
    /// \return Whether or not \p name is among this collection's ActionX
    /// objects.
    bool has(const std::string& name) const;

    /// Beginning of this collection's ActionX objects.
    auto begin() const { return this->actions_.begin(); }

    /// End of this collection's ActionX objects.
    auto end() const { return this->actions_.end(); }

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const Actions& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->actions_);
        serializer(this->pyactions_);
    }

private:
    /// Collection's ActionX objects.
    std::vector<ActionX> actions_{};

    /// Collection's PyAction objects.
    std::vector<PyAction> pyactions_{};
};

} // namespace Opm::Action

#endif // ActionCOnfig_HPP
