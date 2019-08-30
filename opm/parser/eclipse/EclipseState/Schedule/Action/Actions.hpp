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

#include <string>
#include <ctime>
#include <map>

#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>

namespace Opm {
namespace Action {

/*
  The Actions class is a container of ACTIONX keywords. The main functionality
  is to provide a list of ACTIONX keywords which are ready to be evaluated.
*/

class Actions {
public:
    Actions() = default;
    size_t size() const;
    bool empty() const;
    void add(const ActionX& action);
    bool ready(std::time_t sim_time) const;
    const ActionX& at(const std::string& name) const;
    std::vector<const ActionX *> pending(std::time_t sim_time) const;
private:
    std::map<std::string, ActionX> actions;
};
}
}
#endif
