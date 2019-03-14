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

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

namespace Opm {

UDQAssign::UDQAssign(const std::string& keyword, const std::vector<std::string>& selector, double value) :
    m_keyword(keyword),
    m_var_type(UDQ::varType(keyword)),
    m_selector(selector),
    m_value(value)
{}


const std::string& UDQAssign::keyword() const {
    return this->m_keyword;
}

const std::vector<std::string>& UDQAssign::selector() const {
    return this->m_selector;
}

double UDQAssign::value() const {
    return this->m_value;
}

UDQVarType UDQAssign::var_type() const {
    return this->m_var_type;
}

UDQWellSet UDQAssign::eval_wells(const std::vector<std::string>& wells) const {
    if (this->m_var_type != UDQVarType::WELL_VAR)
        throw std::runtime_error("This is a bug - eval_wells called for UDQ variable which is not a well quantity.");

    UDQWellSet ws(this->m_keyword, wells);

    if (this->m_selector.empty())
        ws.assign(this->m_value);
    else
        ws.assign(this->m_selector[0], this->m_value);

    return ws;
}
}
