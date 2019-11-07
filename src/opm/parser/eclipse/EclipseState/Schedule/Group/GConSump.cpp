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

#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>

namespace Opm {

bool GConSump::has(const std::string& name) const {
    return (groups.find(name) != groups.end());
}


const GConSump::GCONSUMPGroup& GConSump::get(const std::string& name) const {

    auto it = groups.find(name);
    if (it == groups.end())
        throw std::invalid_argument("Current GConSump obj. does not contain '" + name + "'.");
    else
        return it->second;
}


void GConSump::add(const std::string& name, const UDAValue& consumption_rate, const UDAValue& import_rate, const std::string network_node) {

    GConSump::GCONSUMPGroup& group = groups[name];

    group.consumption_rate = consumption_rate;
    group.import_rate = import_rate;
    group.network_node = network_node;
}


size_t GConSump::size() const {
    return groups.size();
}

}
