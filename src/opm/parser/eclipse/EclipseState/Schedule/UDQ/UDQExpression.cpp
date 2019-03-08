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

#include <vector>
#include <string>
#include <iostream>

#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQExpression.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>






namespace Opm {

    /*
      The tokenizer algorithm has two problems with '*':

      1. When used to specify a wildcard set - like 'P*' for all well starting
         with 'P' the tokenizer will interpret the '*' as a multiplication sign
         and split into independent tokens 'P' and '*'.

      2. For items like '2*(1+WBHP)' the parsing code will expand the 2*
         operator to the repeated tokens : (1+WBHP), (1+WBHP)
    */
  UDQExpression::UDQExpression(UDQAction action, const std::string& keyword_in, const std::vector<std::string>& input_data) :
      m_action(action),
      m_keyword(keyword_in),
      m_var_type(UDQ::varType(keyword_in))
  {
        for (const std::string& item : input_data) {
            if (RawConsts::is_quote()(item[0])) {
                this->data.push_back(item.substr(1, item.size() - 2));
                continue;
            }

            const std::vector<std::string> splitters = {"TU*[]", "(", ")", "[", "]", ",", "+", "-", "/", "*", "==", "!=", "^", ">=", "<=", ">", "<"};
            size_t offset = 0;
            size_t pos = 0;
            while (pos < item.size()) {
                size_t splitter_index = 0;
                while (splitter_index < splitters.size()) {
                    const std::string& splitter = splitters[splitter_index];
                    size_t find_pos = item.find(splitter, pos);
                    if (find_pos == pos) {
                        if (pos > offset)
                            this->data.push_back(item.substr(offset, pos - offset));
                        this->data.push_back(splitter);
                        pos = find_pos + 1;
                        offset = pos;
                        break;
                    }
                    splitter_index++;
                }
                if (splitter_index == splitters.size())
                    pos += 1;
            }
            if (pos > offset)
                this->data.push_back(item.substr(offset, pos - offset));
        }
    }

    UDQExpression::UDQExpression(const DeckRecord& record) :
        UDQExpression(UDQ::actionType(record.getItem("ACTION").get<std::string>(0)),
                      record.getItem("QUANTITY").get<std::string>(0),
                      record.getItem("DATA").getData<std::string>())

    {
    }


    const std::vector<std::string>& UDQExpression::tokens() const {
        return this->data;
    }


    UDQAction UDQExpression::action() const {
        return this->m_action;
    }


    const std::string& UDQExpression::keyword() const {
        return this->m_keyword;
    }




}

