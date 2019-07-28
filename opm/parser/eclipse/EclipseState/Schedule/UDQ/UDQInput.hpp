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


#ifndef UDQINPUT__HPP_
#define UDQINPUT__HPP_

#include <memory>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

namespace Opm {

class UDQAssign;
class UDQDefine;

class UDQInput{
public:
    UDQInput(const UDQDefine& udq_define);
    UDQInput(const UDQAssign& udq_assign);

    template<typename T>
    const T& get() const;

    template<typename T>
    bool is() const;

    const std::string& keyword() const;
    UDQVarType var_type() const;
private:
    const UDQDefine * define;
    const UDQAssign * assign;
    const std::string m_keyword;
    UDQVarType m_var_type;
};
}



#endif
