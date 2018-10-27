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


#ifndef ActionContext_HPP
#define ActionContext_HPP

#include <string>
#include <map>

namespace Opm {

/*
  This class is extremely work in progress, the api is hopefully sane; the
  implementation complete garbage.
*/

class ActionContext {
public:
    double get(const std::string& func, const std::string& arg) const;
    void   add(const std::string& func, const std::string& arg, double value);

private:
    std::map<std::string, double> values;
};
}
#endif
