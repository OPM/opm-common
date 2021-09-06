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

#include <string>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionValue.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Condition.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Enums.hpp>

#include "ActionParser.hpp"


namespace Opm {
namespace Action {


namespace {

Comparator comparator(TokenType tt) {
    if (tt == TokenType::op_eq)
        return Comparator::EQUAL;

    if (tt == TokenType::op_gt)
        return Comparator::GREATER;

    if (tt == TokenType::op_lt)
        return Comparator::LESS;

    if (tt == TokenType::op_le)
        return Comparator::LESS_EQUAL;

    if (tt == TokenType::op_ge)
        return Comparator::GREATER_EQUAL;

    return Comparator::INVALID;
}


std::string cmp2string(Comparator cmp) {
    if (cmp == Comparator::EQUAL)
        return "=";

    if (cmp == Comparator::GREATER)
        return ">";

    if (cmp == Comparator::LESS)
        return "<";

    if (cmp == Comparator::LESS_EQUAL)
        return "<=";

    if (cmp == Comparator::GREATER_EQUAL)
        return ">=";

    throw std::logic_error("Bug in opm/flow - should not be here");
}

std::string strip_quotes(const std::string& s) {
    if (s[0] == '\'')
        return s.substr(1, s.size() - 2);
    else
        return s;
}

}

void Quantity::add_arg(const std::string& arg) {
    this->args.push_back(strip_quotes(arg));
}

bool Quantity::date() const {
    if (this->quantity == "DAY")
        return true;

    if (this->quantity == "MNTH")
        return true;

    if (this->quantity == "MONTH")
        return true;

    if (this->quantity == "YEAR")
        return true;

    return false;
}



Condition::Condition(const std::vector<std::string>& tokens, const KeywordLocation& location) {
    std::size_t token_index = 0;
    if (tokens[0] == "(") {
        this->left_paren = true;
        token_index += 1;
    }
    this->lhs = Quantity(tokens[token_index]);
    token_index += 1;

    while (true) {
        if (token_index >= tokens.size())
            break;

        auto comp = comparator( Parser::get_type(tokens[token_index]) );
        if (comp == Comparator::INVALID) {
            this->lhs.add_arg(tokens[token_index]);
            token_index += 1;
        } else {
            this->cmp = comp;
            this->cmp_string = cmp2string(this->cmp);
            token_index += 1;
            break;
        }
    }

    if (token_index >= tokens.size())
        throw std::invalid_argument("Could not determine right hand side / comparator for ACTIONX keyword at " + location.filename + ":" + std::to_string(location.lineno));

    this->rhs = Quantity(tokens[token_index]);
    token_index++;

    while (true) {
        if (token_index >= tokens.size())
            break;

        auto token_type = Parser::get_type(tokens[token_index]);
        if (token_type == TokenType::op_and)
            this->logic = Logical::AND;
        else if (token_type == TokenType::op_or)
            this->logic = Logical::OR;
        else if (token_type == TokenType::close_paren)
            this->right_paren = true;
        else
            this->rhs.add_arg(tokens[token_index]);

        token_index++;
    }
}


bool Condition::operator==(const Condition& data) const {
    return lhs == data.lhs &&
           left_paren == data.left_paren &&
           right_paren == data.right_paren &&
           rhs == data.rhs &&
           logic == data.logic &&
           cmp == data.cmp &&
           cmp_string == data.cmp_string;
}

bool Condition::open_paren() const {
    return this->left_paren && !this->right_paren;
}

bool Condition::close_paren() const {
    return !this->left_paren && this->right_paren;
}

Logical Condition::logic_from_int(int int_logic) {
    if (int_logic == 0)
        return Logical::END;

    if (int_logic == 1)
        return Logical::AND;

    if (int_logic == 2)
        return Logical::OR;

    throw std::logic_error("Unknown integer value");
}

int Condition::logic_as_int() const {
    switch (this->logic) {
    case Logical::END:
        return 0;
    case Logical::AND:
        return 1;
    case Logical::OR:
        return 2;
    default:
        throw std::logic_error("What the f...?");
    }
}

Comparator Condition::comparator_from_int(int cmp_int) {
    switch (cmp_int) {
    case 1:
        return Comparator::GREATER;
    case 2:
        return Comparator::LESS;
    case 3:
        return Comparator::GREATER_EQUAL;
    case 4:
        return Comparator::LESS_EQUAL;
    case 5:
        return Comparator::EQUAL;
    default:
        throw std::logic_error("What the f...?");
    }
}

int Condition::comparator_as_int() const {
    switch (this->cmp) {
    case Comparator::GREATER:
        return 1;
    case Comparator::LESS:
        return 2;
    case Comparator::GREATER_EQUAL:
        return 3;
    case Comparator::LESS_EQUAL:
        return 4;
    case Comparator::EQUAL:
        return 5;
    default:
        throw std::logic_error("What the f...?");
    }
}

}
}
