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

#ifndef ACTION_PARSER_HPP
#define ACTION_PARSER_HPP

#include <vector>
#include <string>

#include "ASTNode.hpp"
#include "ActionValue.hpp"

namespace Opm {

struct ParseNode {
    ParseNode(TokenType type_arg, const std::string& value_arg) :
        type(type_arg),
        value(value_arg)
    {}

    // Implicit converting constructor.
    ParseNode(TokenType type_arg) : ParseNode(type_arg, "")
    {}


    TokenType type;
    std::string value;
};



class ActionParser {
public:
    static ASTNode parse(const std::vector<std::string>& tokens);

private:
    ActionParser(const std::vector<std::string>& tokens);

    TokenType get_type(const std::string& arg) const;
    ParseNode current() const;
    ParseNode next();
    size_t pos() const;
    ASTNode parse_cmp();
    ASTNode parse_op();
    ASTNode parse_left();
    ASTNode parse_right();
    ASTNode parse_and();
    ASTNode parse_or();

    const std::vector<std::string>& tokens;
    ssize_t current_pos = -1;
};


}

#endif
