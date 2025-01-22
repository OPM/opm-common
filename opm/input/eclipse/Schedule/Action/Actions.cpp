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

#include <opm/input/eclipse/Schedule/Action/Actions.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>

#include <algorithm>
#include <cstddef>
#include <ctime>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace {

    template <typename ActionCollection>
    auto findActionByName(ActionCollection&& actions,
                          const std::string& name)
    {
        return std::find_if(std::begin(actions), std::end(actions),
                            [&name](const auto& action)
                            { return action.name() == name; });
    }

} // Anonymous namespace

namespace Opm::Action {

Actions::Actions(const std::vector<ActionX>&  action,
                 const std::vector<PyAction>& pyaction)
    : actions_   { action }
    , pyactions_ { pyaction }
{}

Actions Actions::serializationTestObject()
{
    Actions result;
    result.actions_ = {ActionX::serializationTestObject()};
    result.pyactions_ = {PyAction::serializationTestObject()};

    return result;
}

void Actions::add(const ActionX& action)
{
    auto iter = findActionByName(this->actions_, action.name());
    if (iter == this->actions_.end()) {
        this->actions_.push_back(action);
    }
    else {
        const auto id = iter->id() + 1;

        *iter = action;
        iter->update_id(id);
    }
}

void Actions::add(const PyAction& pyaction)
{
    auto iter = findActionByName(this->pyactions_, pyaction.name());
    if (iter == this->pyactions_.end()) {
        this->pyactions_.push_back(pyaction);
    }
    else {
        *iter = pyaction;
    }
}

std::size_t Actions::ecl_size() const
{
    return this->actions_.size();
}

std::size_t Actions::py_size() const
{
    return this->pyactions_.size();
}

int Actions::max_input_lines() const
{
    std::size_t max_il = 0;

    for (const auto& act : this->actions_) {
        if (act.keyword_strings().size() > max_il) {
            max_il = act.keyword_strings().size();
        }
    }

    return static_cast<int>(max_il);
}

bool Actions::empty() const
{
    return this->actions_.empty() && this->pyactions_.empty();
}

bool Actions::ready(const State& state, const std::time_t sim_time) const
{
    return std::any_of(this->actions_.begin(), this->actions_.end(),
                       [&state, sim_time](const auto& action)
                       { return action.ready(state, sim_time); });
}

const ActionX& Actions::operator[](const std::string& name) const
{
    auto iter = findActionByName(this->actions_, name);
    if (iter == this->actions_.end()) {
        throw std::range_error {
            fmt::format("ACTIONX named '{}' is not "
                        "known in current run.", name)
        };
    }

    return *iter;
}

const ActionX& Actions::operator[](const std::size_t index) const
{
    return this->actions_[index];
}

std::vector<const ActionX*>
Actions::pending(const State& state, const std::time_t sim_time) const
{
    auto action_vector = std::vector<const ActionX*> {};

    for (const auto& action : this->actions_) {
        if (action.ready(state, sim_time)) {
            action_vector.push_back(&action);
        }
    }

    return action_vector;
}

std::vector<const PyAction*>
Actions::pending_python(const State& state) const
{
    auto pyaction_vector = std::vector<const PyAction*> {};

    for (const auto& pyaction : this->pyactions_) {
        if (pyaction.ready(state)) {
            pyaction_vector.push_back(&pyaction);
        }
    }

    return pyaction_vector;
}

bool Actions::has(const std::string& name) const
{
    return findActionByName(this->actions_, name)
        != this->actions_.end();
}

bool Actions::operator==(const Actions& data) const
{
    return (this->actions_ == data.actions_)
        && (this->pyactions_ == data.pyactions_);
}

} // namespace Opm::Action
