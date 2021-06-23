/*
  Copyright 2021 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RST_UDQ
#define RST_UDQ

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

namespace Opm {

namespace RestartIO {

struct RstHeader;

struct RstUDQ {
    RstUDQ(const std::string& name_arg,
           const std::string& unit_arg,
           const std::string& define_arg,
           UDQUpdate status_arg);

    RstUDQ(const std::string& name_arg,
           const std::string& unit_arg);

    void add_well_value(const std::string& wname, double value);
    void add_group_value(const std::string& wname, double value);
    void add_field_value(double value);

    std::string name;
    std::string unit;
    UDQVarType var_type;

    std::optional<std::string> define;
    UDQUpdate status;
    std::vector<std::pair<std::string, double>> well_values;
    std::vector<std::pair<std::string, double>> group_values;
    double field_value;
};


}
}




#endif
