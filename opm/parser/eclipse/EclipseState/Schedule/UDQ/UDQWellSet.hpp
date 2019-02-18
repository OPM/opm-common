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

#ifndef UDQWELLSET_HPP
#define UDQWELLSET_HPP

#include <vector>
#include <string>
#include <unordered_map>


#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQSet.hpp>

namespace Opm {

class UDQWellSet : public UDQSet {
public:
    explicit UDQWellSet(const std::vector<std::string>& wells);
    void assign(const std::string& well, double value);
    void assign(double value);
    const UDQScalar& operator[](const std::string& well) const;
private:
    std::size_t wellIndex(const std::string& well) const;
    std::unordered_map<std::string, std::size_t> well_index;
};


}



#endif
