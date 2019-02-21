/*
  Copyright 2018 Statoil ASA.

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


#ifndef UDQASSIGN_HPP_
#define UDQASSIGN_HPP_

#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQWellSet.hpp>

namespace Opm {

class UDQAssign{
public:
    UDQAssign(const std::string& keyword, const std::vector<std::string>& selector, double value);
    const std::string& keyword() const;
    double value() const;
    const std::vector<std::string>& selector() const;
    UDQWellSet eval_wells(const std::vector<std::string>& wells) const;
private:
    std::string m_keyword;
    std::vector<std::string> m_selector;
    double m_value;
};
}



#endif
