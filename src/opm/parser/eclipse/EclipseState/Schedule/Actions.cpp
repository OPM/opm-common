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

#include <opm/parser/eclipse/EclipseState/Schedule/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ActionX.hpp>

namespace Opm {

size_t Actions::size() const {
    return this->actions.size();
}


bool Actions::empty() const {
    return this->actions.empty();
}


void Actions::add(const ActionX& action) {
    auto iter = this->actions.find(action.name());
    if (iter != this->actions.end())
        this->actions.erase(iter);

    this->actions.insert(std::pair<std::string,ActionX>(action.name(), action));
}

ActionX& Actions::at(const std::string& name) {
    return this->actions.at(name);
}


bool Actions::ready(std::time_t sim_time) const {
    for (const auto& pair : this->actions) {
        if (pair.second.ready(sim_time))
            return true;
    }
    return false;
}


std::vector<ActionX *> Actions::pending(std::time_t sim_time) {
    std::vector<ActionX *> action_vector;
    for (auto& pair : this->actions) {
        auto& action = pair.second;
        if (action.ready(sim_time))
            action_vector.push_back( &action );
    }
    return action_vector;
}


}
