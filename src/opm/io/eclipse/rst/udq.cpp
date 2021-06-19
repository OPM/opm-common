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

#include <opm/io/eclipse/rst/udq.hpp>

namespace Opm {
namespace RestartIO {

RstUDQ::RstUDQ(const std::string& name_arg, const std::string& unit_arg, const std::string& define_arg, UDQUpdate update_arg)
    : name(name_arg)
    , unit(unit_arg)
    , var_type(UDQ::varType(name_arg))
    , define(define_arg)
    , status(update_arg)
{}

RstUDQ::RstUDQ(const std::string& name_arg, const std::string& unit_arg)
    : name(name_arg)
    , unit(unit_arg)
    , var_type(UDQ::varType(name_arg))
{}


void RstUDQ::add_well_value(const std::string& wname, double value) {
    this->well_values.emplace_back( wname, value );
}

void RstUDQ::add_group_value(const std::string& wname, double value) {
    this->group_values.emplace_back( wname, value );
}

void RstUDQ::add_field_value(double value) {
    this->field_value = value;
}

}
}
