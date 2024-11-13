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

#include <opm/input/eclipse/Schedule/Action/State.hpp>

#include <opm/io/eclipse/rst/state.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>

#include <algorithm>
#include <cstddef>
#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {
    std::pair<std::string, std::size_t>
    makeID(const Opm::Action::ActionX& action)
    {
        return { action.name(), action.id() };
    }
}

bool Opm::Action::State::MatchSet::hasWell(const std::string& well) const
{
    // Note: this->wells_ is typically unsorted, so use linear search.
    return std::find(this->wells_.begin(), this->wells_.end(), well)
        != this->wells_.end();
}

Opm::Action::State::MatchSet
Opm::Action::State::MatchSet::serializationTestObject()
{
    using namespace std::string_literals;

    auto mset = MatchSet{};

    mset.wells_.assign({ "P1"s, "P2"s, "I"s });

    return mset;
}

bool Opm::Action::State::MatchSet::operator==(const MatchSet& that) const
{
    return this->wells_ == that.wells_;
}

// ---------------------------------------------------------------------------

std::size_t Opm::Action::State::run_count(const ActionX& action) const
{
    auto count_iter = this->run_state.find(makeID(action));

    return (count_iter == this->run_state.end())
        ? std::size_t{0} : count_iter->second.run_count;
}

std::time_t Opm::Action::State::run_time(const ActionX& action) const
{
    auto statePos = this->run_state.find(makeID(action));
    if (statePos == this->run_state.end()) {
        throw std::invalid_argument {
            fmt::format("Action {} has never run", action.name())
        };
    }

    return statePos->second.last_run;
}

void Opm::Action::State::add_run(const ActionX&    action,
                                 const std::time_t run_time,
                                 const Result&     result)
{
    {
        const auto& [statePos, inserted] = this->run_state
            .emplace(makeID(action), run_time);

        if (! inserted) {
            statePos->second.add_run(run_time);
        }
    }

    if (const auto wellRange = result.matches().wells(); ! wellRange.empty()) {
        const auto resultInsert = this->last_result
            .emplace(std::piecewise_construct,
                     std::forward_as_tuple(action.name()),
                     std::forward_as_tuple());

        resultInsert.first->second.wells_
            .assign(wellRange.begin(), wellRange.end());
    }
}

void Opm::Action::State::add_run(const PyAction& action, const bool result)
{
    this->m_python_result.insert_or_assign(action.name(), result);
}

const Opm::Action::State::MatchSet*
Opm::Action::State::result(const std::string& action) const
{
    auto iter = this->last_result.find(action);

    return (iter == this->last_result.end())
        ? nullptr : &iter->second;
}

std::optional<bool>
Opm::Action::State::python_result(const std::string& action) const
{
    auto iter = this->m_python_result.find(action);
    if (iter == this->m_python_result.end()) {
        return std::nullopt;
    }

    return iter->second;
}

// When restoring from restart file we initialize the number of times it has
// run and the last run time.  From the evaluation only the 'true'
// evaluation is restored, not the well/group set.
void Opm::Action::State::load_rst(const Actions&             action_config,
                                  const RestartIO::RstState& rst_state)
{
    for (const auto& rst_action : rst_state.actions) {
        if (! (rst_action.run_count > 0)) {
            continue;
        }

        this->add_run(action_config[rst_action.name],
                      rst_action.last_run.value(),
                      Action::Result{ true });
    }
}

bool Opm::Action::State::operator==(const State& other) const
{
    return (this->m_python_result == other.m_python_result)
        && (this->run_state == other.run_state)
        && (this->last_result == other.last_result)
        ;
}

Opm::Action::State Opm::Action::State::serializationTestObject()
{
    State st;

    st.run_state.emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::string{"ACTION"}, 100),
                         std::forward_as_tuple(RunState::serializationTestObject()));

    st.last_result.emplace("ACTION", MatchSet::serializationTestObject());
    st.m_python_result.emplace("PYACTION", false);

    return st;
}
