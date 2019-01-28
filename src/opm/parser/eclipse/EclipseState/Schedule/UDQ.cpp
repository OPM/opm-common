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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ.hpp>

namespace Opm {

    namespace {
        std::string strip_quotes(const std::string& s) {
            if (s[0] == '\'')
                return s.substr(1, s.size() - 2);
            else
                return s;
        }

    }


    void UDQ::add_record(const DeckRecord& record) {
        const auto& action = record.getItem("ACTION").get<std::string>(0);
        const auto& quantity = record.getItem("QUANTITY").get<std::string>(0);
        const auto& data = record.getItem("DATA").getData<std::string>();

        if (UDQExpression::actionString2Enum(action) == UDQAction::UNITS)
            this->assign_unit( quantity, data[0] );
        else
            this->m_expressions.emplace_back(action, quantity, data);
    }


    const std::vector<UDQExpression>& UDQ::expressions() const noexcept {
        return this->m_expressions;
    }

    const std::string& UDQ::unit(const std::string& key) const {
        const auto pair_ptr = this->units.find(key);
        if (pair_ptr == this->units.end())
            throw std::invalid_argument("No such UDQ quantity: " + key);

        return pair_ptr->second;
    }


    void UDQ::assign_unit(const std::string& keyword, const std::string& quoted_unit) {
        const std::string unit = strip_quotes(quoted_unit);
        const auto pair_ptr = this->units.find(keyword);
        if (pair_ptr != this->units.end()) {
            if (pair_ptr->second != unit)
                throw std::invalid_argument("Illegal to change unit of UDQ keyword runtime");

            return;
        }
        this->units[keyword] = unit;
    }
}
