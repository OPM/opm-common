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
#include <iostream>
#include <cstring>



#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

#include "UDQParser.hpp"
#include "UDQASTNode.hpp"

namespace Opm {



namespace {

std::vector<std::string> quote_split(const std::string& item) {
    char quote_char = '\'';
    std::vector<std::string> items;
    std::size_t offset = 0;
    while (true) {
        auto quote_pos1 = item.find(quote_char, offset);
        if (quote_pos1 == std::string::npos) {
            items.push_back(item.substr(offset));
            break;
        }

        auto quote_pos2 = item.find(quote_char, quote_pos1 + 1);
        if (quote_pos2 == std::string::npos)
            throw std::invalid_argument("Unbalanced quotes in: " + item);

        if (quote_pos1 > offset)
            items.push_back(item.substr(offset, quote_pos1 - offset));
        items.push_back(item.substr(quote_pos1, 1 + quote_pos2 - quote_pos1));
        offset = quote_pos2 + 1;
    }
    return items;
}

}

template <typename T>
UDQDefine::UDQDefine(const UDQFunctionTable& udqft,
                     const std::string& keyword,
                     const std::vector<std::string>& deck_data,
                     const ParseContext& parseContext,
                     T&& errors) :
    UDQDefine(udqft, keyword, deck_data, parseContext, errors)
{}


UDQDefine::UDQDefine(const UDQFunctionTable& udqft,
                     const std::string& keyword,
                     const std::vector<std::string>& deck_data) :
    UDQDefine(udqft, keyword, deck_data, ParseContext(), ErrorGuard())
{}


UDQDefine::UDQDefine(const UDQFunctionTable& udqft,
                     const std::string& keyword,
                     const std::vector<std::string>& deck_data,
                     const ParseContext& parseContext,
                     ErrorGuard& errors) :
    m_keyword(keyword),
    m_var_type(UDQ::varType(keyword)),
    udqft(udqft)
{
    std::vector<std::string> tokens;
    for (const std::string& deck_item : deck_data) {
        for (const std::string& item : quote_split(deck_item)) {
            if (RawConsts::is_quote()(item[0])) {
                tokens.push_back(item.substr(1, item.size() - 2));
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
                            tokens.push_back(item.substr(offset, pos - offset));
                        tokens.push_back(splitter);
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
                tokens.push_back(item.substr(offset, pos - offset));

        }
    }
    this->ast = std::make_shared<UDQASTNode>( UDQParser::parse(udqft, tokens) );
    this->tokens = tokens;
}


UDQWellSet UDQDefine::eval_wells(const UDQContext& context) const {
    return this->ast->eval_wells(context);
}

UDQSet UDQDefine::eval(const UDQContext& context) const {
    switch (this->m_var_type) {
    case UDQVarType::WELL_VAR:
        return this->eval_wells(context);
    default:
        throw std::invalid_argument("UDQ subtype: not supported");
    }
}

UDQVarType UDQDefine::var_type() const {
    return this->m_var_type;
}


const std::string& UDQDefine::keyword() const {
    return this->m_keyword;
}

}
