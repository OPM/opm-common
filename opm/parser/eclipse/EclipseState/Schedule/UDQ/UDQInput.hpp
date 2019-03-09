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


#ifndef UDQINPUT_HPP_
#define UDQINPUT_HPP_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQExpression.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>


namespace Opm {


    class UDQInput {
    public:
        void add_record(const DeckRecord& record);
        const std::vector<UDQExpression>& expressions() const noexcept;
        const std::string& unit(const std::string& key) const;
        bool has_unit(const std::string& keyword) const;
        bool has_keyword(const std::string& keyword) const;
        void assign_unit(const std::string& keyword, const std::string& unit);
        const std::vector<UDQAssign>& assignments() const;
        std::vector<UDQAssign> assignments(UDQVarType var_type) const;
    private:
        std::vector<UDQExpression> m_expressions;
        std::vector<UDQAssign> m_assignments;
        std::unordered_map<std::string, std::string> units;
        std::unordered_set<std::string> keywords;
    };
}



#endif
