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

#include <fnmatch.h>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQWellSet.hpp>

namespace Opm {

UDQWellSet::UDQWellSet(const std::string& name, const std::vector<std::string>& wells) :
    UDQSet(name, wells.size())
{
    for (std::size_t index = 0; index < wells.size(); index++)
        this->well_index.emplace( std::make_pair(wells[index], index));
}


void UDQWellSet::assign(const std::string& well, double value) {
    if (well.find('*') == std::string::npos) {
        std::size_t index = this->wellIndex(well);
        UDQSet::assign(index, value);
    } else {
        int flags = 0;
        for (const auto& pair : this->well_index) {
            if (fnmatch(well.c_str(), pair.first.c_str(), flags) == 0)
                UDQSet::assign(pair.second, value);
        }
    }
}

void UDQWellSet::assign(double value) {
    UDQSet::assign(value);
}

const UDQScalar& UDQWellSet::operator[](const std::string& well) const {
    std::size_t index = this->wellIndex(well);
    return UDQSet::operator[](index);
}


std::size_t UDQWellSet::wellIndex(const std::string& well) const {
    const auto& pair_ptr = this->well_index.find(well);
    if (pair_ptr == this->well_index.end())
        throw std::invalid_argument("No such well: " + well);

    return pair_ptr->second;
}


}
