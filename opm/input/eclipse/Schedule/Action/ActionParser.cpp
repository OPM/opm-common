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

#include "ActionParser.hpp"

#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace Opm { namespace Action {

Parser::Parser(const std::vector<std::string>& tokens_arg)
    : tokens(tokens_arg)
{}

TokenType Parser::get_type(const std::string& arg)
{
    std::string lower_arg = arg;
    std::for_each(lower_arg.begin(),
                  lower_arg.end(),
                  [](char& c) {
                      c = std::tolower(static_cast<unsigned char>(c));
                  });

    if (lower_arg == "and") {
        return TokenType::op_and;
    }

    if (lower_arg == "or") {
        return TokenType::op_or;
    }

    if (lower_arg == "(") {
        return TokenType::open_paren;
    }

    if (lower_arg == ")") {
        return TokenType::close_paren;
    }

    if ((lower_arg == ">") || (lower_arg == ".gt.")) {
        return TokenType::op_gt;
    }

    if ((lower_arg == ">=") || (lower_arg == ".ge.")) {
        return TokenType::op_ge;
    }

    if ((lower_arg == "<") || (lower_arg == ".lt.")) {
        return TokenType::op_lt;
    }

    if ((lower_arg == "<=") || (lower_arg == ".le.")) {
        return TokenType::op_le;
    }

    if ((lower_arg == "=") || (lower_arg == ".eq.")) {
        return TokenType::op_eq;
    }

    if ((lower_arg == "!=") || (lower_arg == ".ne.")) {
        return TokenType::op_ne;
    }

    {
        char* end_ptr = nullptr;
        std::strtod(lower_arg.c_str(), &end_ptr);

        if (std::strlen(end_ptr) == 0) {
            return TokenType::number;
        }
    }

    return TokenType::ecl_expr;
}

FuncType Parser::get_func(const std::string& arg)
{
    if (arg == "YEAR") return FuncType::time;
    if (arg == "MNTH") return FuncType::time_month;
    if (arg == "DAY")  return FuncType::time;

    using Cat = SummaryConfigNode::Category;
    SummaryConfigNode::Category cat = parseKeywordCategory(arg);
    switch (cat) {
        case Cat::Aquifer:    return FuncType::aquifer;
        case Cat::Well:       return FuncType::well;
        case Cat::Group:      return FuncType::group;
        case Cat::Connection: return FuncType::well_connection;
        case Cat::Region:     return FuncType::region;
        case Cat::Block:      return FuncType::block;
        case Cat::Segment:    return FuncType::well_segment;
        default:              return FuncType::none;
    }
}

ParseNode Parser::next()
{
    this->current_pos++;
    if (static_cast<size_t>(this->current_pos) == this->tokens.size()) {
        return ParseNode(TokenType::end);
    }

    std::string arg = this->tokens[this->current_pos];
    return ParseNode(get_type(arg), arg);
}

ParseNode Parser::current() const
{
    if (static_cast<size_t>(this->current_pos) == this->tokens.size()) {
        return ParseNode(TokenType::end);
    }

    std::string arg = this->tokens[this->current_pos];
    return ParseNode(get_type(arg), arg);
}

ASTNode Parser::parse_left()
{
    auto curr = this->current();
    if (curr.type != TokenType::ecl_expr) {
        throw std::invalid_argument {
            fmt::format("Expected expression as left hand side "
                        "of comparison, but got {} instead.",
                        curr.value)
        };
    }

    std::string func = curr.value;
    FuncType func_type = get_func(curr.value);
    std::vector<std::string> arg_list;
    curr = this->next();
    while ((curr.type == TokenType::ecl_expr) ||
           (curr.type == TokenType::number))
    {
        arg_list.push_back(curr.value);
        curr = this->next();
    }

    return ASTNode { TokenType::ecl_expr, func_type, func, arg_list };
}

ASTNode Parser::parse_op()
{
    const auto curr = this->current();

    if ((curr.type == TokenType::op_gt) ||
        (curr.type == TokenType::op_ge) ||
        (curr.type == TokenType::op_lt) ||
        (curr.type == TokenType::op_le) ||
        (curr.type == TokenType::op_eq) ||
        (curr.type == TokenType::op_ne))
    {
        this->next();

        return ASTNode { curr.type };
    }

    return ASTNode { TokenType::error };
}

ASTNode Parser::parse_right()
{
    auto curr = this->current();
    if (curr.type == TokenType::number) {
        this->next();
        return ASTNode { strtod(curr.value.c_str(), nullptr) };
    }

    curr = this->current();
    if (curr.type != TokenType::ecl_expr) {
        return ASTNode { TokenType::error };
    }

    std::string func = curr.value;
    FuncType func_type = FuncType::none;
    std::vector<std::string> arg_list;
    curr = this->next();
    while ((curr.type == TokenType::ecl_expr) ||
           (curr.type == TokenType::number))
    {
        arg_list.push_back(curr.value);
        curr = this->next();
    }

    return ASTNode { TokenType::ecl_expr, func_type, func, arg_list };
}

ASTNode Parser::parse_cmp()
{
    auto curr = this->current();

    if (curr.type == TokenType::open_paren) {
        this->next();
        auto inner_expr = this->parse_or();

        curr = this->current();
        if (curr.type != TokenType::close_paren) {
            return ASTNode { TokenType::error };
        }

        this->next();

        return ASTNode { inner_expr };
    }
    else {
        auto left_node = this->parse_left();
        if (left_node.type == TokenType::error) {
            return ASTNode { TokenType::error };
        }

        auto op_node = this->parse_op();
        if (op_node.type == TokenType::error) {
            return ASTNode { TokenType::error };
        }

        auto right_node = this->parse_right();
        if (right_node.type == TokenType::error) {
            return ASTNode { TokenType::error };
        }

        op_node.add_child(left_node);
        op_node.add_child(right_node);

        return op_node;
    }
}

ASTNode Parser::parse_and()
{
    auto left = this->parse_cmp();
    if (left.type == TokenType::error) {
        return ASTNode { TokenType::error };
    }

    auto curr = this->current();
    if (curr.type == TokenType::op_and) {
        ASTNode and_node(TokenType::op_and);
        and_node.add_child(left);

        while (this->current().type == TokenType::op_and) {
            this->next();
            auto next_cmp = this->parse_cmp();
            if (next_cmp.type == TokenType::error) {
                return ASTNode { TokenType::error };
            }

            and_node.add_child(next_cmp);
        }

        return and_node;
    }

    return left;
}

ASTNode Parser::parse_or()
{
    auto left = this->parse_and();
    if (left.type == TokenType::error) {
        return ASTNode { TokenType::error };
    }

    auto curr = this->current();
    if (curr.type == TokenType::op_or) {
        ASTNode or_node(TokenType::op_or);
        or_node.add_child(left);

        while (this->current().type == TokenType::op_or) {
            this->next();
            auto next_cmp = this->parse_or();
            if (next_cmp.type == TokenType::error) {
                return ASTNode { TokenType::error };
            }

            or_node.add_child(next_cmp);
        }

        return or_node;
    }

    return left;
}

ASTNode Parser::parse(const std::vector<std::string>& tokens)
{
    Parser parser(tokens);

    auto start_node = parser.next();
    if (start_node.type == TokenType::end) {
        return ASTNode { start_node.type };
    }

    auto tree = parser.parse_or();

    if (tree.type == TokenType::error) {
        throw std::invalid_argument {
            "Failed to parse ACTIONX condition."
        };
    }

    auto curr = parser.current();
    if (curr.type != TokenType::end) {
        const auto index = parser.current_pos;

        throw std::invalid_argument {
            fmt::format("Extra unhandled data starting with "
                        "token[{}] = {} in ACTIONX condition",
                        index, curr.value)
        };
    }

    return tree;
}

}} // namespace Opm::Action
