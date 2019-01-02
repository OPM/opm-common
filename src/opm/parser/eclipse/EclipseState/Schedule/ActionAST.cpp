/*
  Copyright 2018 Equinor ASA.

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
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <opm/parser/eclipse/EclipseState/Schedule/ActionAST.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ActionContext.hpp>

namespace Opm {

ActionParser::ActionParser(const std::vector<std::string>& tokens_arg) :
    tokens(tokens_arg)
{}


TokenType ActionParser::get_type(const std::string& arg) const {
    std::string lower_arg = arg;
    std::for_each(lower_arg.begin(),
                  lower_arg.end(),
                  [](char& c) {
                      c = std::tolower(static_cast<unsigned char>(c));
                  });

    if (lower_arg == "and")
        return TokenType::op_and;

    if (lower_arg == "or")
        return TokenType::op_or;

    if (lower_arg == "(")
        return TokenType::open_paren;

    if (lower_arg == ")")
        return TokenType::close_paren;

    if (lower_arg == ">" || lower_arg == ".gt.")
        return TokenType::op_gt;

    if (lower_arg == ">=" || lower_arg == ".ge.")
        return TokenType::op_ge;

    if (lower_arg == "<=" || lower_arg == ".le.")
        return TokenType::op_le;

    if (lower_arg == "<" || lower_arg == ".lt.")
        return TokenType::op_lt;

    if (lower_arg == "<=" || lower_arg == ".le.")
        return TokenType::op_le;

    if (lower_arg == "=" || lower_arg == ".eq.")
        return TokenType::op_eq;

    if (lower_arg == "!=" || lower_arg == ".ne.")
        return TokenType::op_ne;

    {
        char * end_ptr;
        std::strtod(lower_arg.c_str(), &end_ptr);
        if (std::strlen(end_ptr) == 0)
            return TokenType::number;
    }

    return TokenType::ecl_expr;
}

ParseNode ActionParser::next() {
    this->current_pos++;
    if (static_cast<size_t>(this->current_pos) == this->tokens.size())
        return TokenType::end;

    std::string arg = this->tokens[this->current_pos];
    return ParseNode(get_type(arg), arg);
}


ParseNode ActionParser::current() const {
    if (static_cast<size_t>(this->current_pos) == this->tokens.size())
        return TokenType::end;

    std::string arg = this->tokens[this->current_pos];
    return ParseNode(get_type(arg), arg);
}


size_t ActionParser::pos() const {
    return this->current_pos;
}


/*****************************************************************/
ActionValue::ActionValue(double value) :
    scalar_value(value),
    is_scalar(true)
{ }


double ActionValue::scalar() const {
    if (!this->is_scalar)
        throw std::invalid_argument("This value node represents a well list and can not be evaluated in scalar context");

    return this->scalar_value;
}


void ActionValue::add_well(const std::string& well, double value) {
    if (this->is_scalar)
        throw std::invalid_argument("This value node has been created as a scalar node - can not add well variables");

    this->well_values.emplace_back(well, value);
}


namespace {

    bool eval_cmp_scalar(double lhs, TokenType op, double rhs) {
        switch (op) {

        case TokenType::op_eq:
            return lhs == rhs;

        case TokenType::op_ge:
            return lhs >= rhs;

        case TokenType::op_le:
            return lhs <= rhs;

        case TokenType::op_ne:
            return lhs != rhs;

        case TokenType::op_gt:
            return lhs > rhs;

        case TokenType::op_lt:
            return lhs < rhs;

        default:
            throw std::invalid_argument("Incorrect operator type - expected comparison");
        }

    }
}


bool ActionValue::eval_cmp_wells(TokenType op, double rhs, std::vector<std::string>& matching_wells) const {
    bool ret_value = false;
    for (const auto& pair : this->well_values) {
        const std::string& well = pair.first;
        const double value = pair.second;
        /*
          It is less than clear how the matching_wells should be treated when
          multiple conditons are nested; in the current implementation a
          matching well is quite simply added to the list. This has two
          consequences which are not-obviously-correct:

          1. The wells might appear multiple times.

          2. If a well matches in *one* conditon, and not in another - it will
             be in the matching_wells list.
        */

        if (eval_cmp_scalar(value, op, rhs)) {
            matching_wells.push_back(well);
            ret_value = true;
        }
    }
    return ret_value;
}


bool ActionValue::eval_cmp(TokenType op, const ActionValue& rhs, std::vector<std::string>& matching_wells) const {
    if (op == TokenType::number ||
        op == TokenType::ecl_expr ||
        op == TokenType::open_paren ||
        op == TokenType::close_paren ||
        op == TokenType::op_and ||
        op == TokenType::op_or ||
        op == TokenType::end ||
        op == TokenType::error)
        throw std::invalid_argument("Invalid operator");

    if (!rhs.is_scalar)
        throw std::invalid_argument("The right hand side must be a scalar value");

    if (this->is_scalar)
        return eval_cmp_scalar(this->scalar(), op, rhs.scalar());

    return this->eval_cmp_wells(op, rhs.scalar(), matching_wells);
}

/*****************************************************************/

void ASTNode::add_child(const ASTNode& child) {
    this->children.push_back(child);
}

ActionValue ASTNode::value(const ActionContext& context) const {
    if (this->children.size() != 0)
        throw std::invalid_argument("value() method should only reach leafnodes");

    if (this->type == TokenType::number)
        return ActionValue(this->number);

    if (this->arg_list.size() == 0)
        return ActionValue(context.get(this->func));
    else {
        if (this->arg_list[0] == "*") {
            ActionValue well_values;
            for (const auto& well : context.wells(this->func))
                well_values.add_well(well, context.get(this->func, well));
            return well_values;
        } else {
            std::string arg_key = this->arg_list[0];
            for (size_t index = 1; index < this->arg_list.size(); index++)
                arg_key += ":" + this->arg_list[index];
            return ActionValue(context.get(this->func, arg_key));
        }
    }
}


bool ASTNode::eval(const ActionContext& context, std::vector<std::string>& matching_wells) const {
    if (this->children.size() == 0)
        throw std::invalid_argument("bool eval should not reach leafnodes");

    if (this->type == TokenType::op_or || this->type == TokenType::op_and) {
        bool value = (this->type == TokenType::op_and);
        for (const auto& child : this->children) {
            if (this->type == TokenType::op_or)
                value = value || child.eval(context, matching_wells);
            else
                value = value && child.eval(context, matching_wells);
        }
        return value;
    }

    auto v1 = this->children[0].value(context);
    auto v2 = this->children[1].value(context);
    return v1.eval_cmp(this->type, v2, matching_wells);
}


size_t ASTNode::size() const {
    return this->children.size();
}


/*****************************************************************/

ASTNode ActionAST::parse_left(ActionParser& parser) {
    auto current = parser.current();
    if (current.type != TokenType::ecl_expr)
        return TokenType::error;

    std::string func = current.value;
    std::vector<std::string> arg_list;
    current = parser.next();
    while (current.type == TokenType::ecl_expr || current.type == TokenType::number) {
        arg_list.push_back(current.value);
        current = parser.next();
    }

    return ASTNode(TokenType::ecl_expr, func, arg_list);
}

ASTNode ActionAST::parse_op(ActionParser& parser) {
    auto current = parser.current();
    if (current.type == TokenType::op_gt ||
        current.type == TokenType::op_ge ||
        current.type == TokenType::op_lt ||
        current.type == TokenType::op_le ||
        current.type == TokenType::op_eq ||
        current.type == TokenType::op_ne) {
        parser.next();
        return current.type;
    }
    return TokenType::error;
}


ASTNode ActionAST::parse_right(ActionParser& parser) {
    auto current = parser.current();
    if (current.type == TokenType::number) {
        parser.next();
        return ASTNode( strtod(current.value.c_str(), nullptr) );
    }

    current = parser.current();
    if (current.type != TokenType::ecl_expr)
        return TokenType::error;

    std::string func = current.value;
    std::vector<std::string> arg_list;
    current = parser.next();
    while (current.type == TokenType::ecl_expr || current.type == TokenType::number) {
        arg_list.push_back(current.value);
        current = parser.next();
    }
    return ASTNode(TokenType::ecl_expr, func, arg_list);
}



ASTNode ActionAST::parse_cmp(ActionParser& parser) {
    auto current = parser.current();

    if (current.type == TokenType::open_paren) {
        parser.next();
        auto inner_expr = this->parse_or(parser);

        current = parser.current();
        if (current.type != TokenType::close_paren)
            return TokenType::error;

        parser.next();
        return inner_expr;
    } else {
        auto left_node = parse_left(parser);
        if (left_node.type == TokenType::error)
            return TokenType::error;

        auto op_node = parse_op(parser);
        if (op_node.type == TokenType::error)
            return TokenType::error;

        auto right_node = parse_right(parser);
        if (right_node.type == TokenType::error)
            return TokenType::error;

        op_node.add_child(left_node);
        op_node.add_child(right_node);
        return op_node;
    }
}

ASTNode ActionAST::parse_and(ActionParser& parser) {
    auto left = this->parse_cmp(parser);
    if (left.type == TokenType::error)
        return TokenType::error;

    auto current = parser.current();
    if (current.type == TokenType::op_and) {
        ASTNode and_node(TokenType::op_and);
        and_node.add_child(left);

        while (parser.current().type == TokenType::op_and) {
            parser.next();
            auto next_cmp = this->parse_cmp(parser);
            if (next_cmp.type == TokenType::error)
                return TokenType::error;

            and_node.add_child(next_cmp);
        }
        return and_node;
    }

    return left;
}


ASTNode ActionAST::parse_or(ActionParser& parser) {
    auto left = this->parse_and(parser);
    if (left.type == TokenType::error)
        return TokenType::error;

    auto current = parser.current();
    if (current.type == TokenType::op_or) {
        ASTNode or_node(TokenType::op_or);
        or_node.add_child(left);

        while (parser.current().type == TokenType::op_or) {
            parser.next();
            auto next_cmp = this->parse_or(parser);
            if (next_cmp.type == TokenType::error)
                return TokenType::error;

            or_node.add_child(next_cmp);
        }
        return or_node;
    }

    return left;
}

ActionAST::ActionAST(const std::vector<std::string>& tokens) {
    ActionParser parser(tokens);
    parser.next();
    this->tree = this->parse_or(parser);
    auto current = parser.current();
    if (current.type != TokenType::end) {
        size_t index = parser.pos();
        throw std::invalid_argument("Extra unhandled data starting with token[" + std::to_string(index) + "] = " + current.value);
    }

    if (this->tree.type == TokenType::error)
        throw std::invalid_argument("Failed to parse");
}

bool ActionAST::eval(const ActionContext& context, std::vector<std::string>& matching_wells) const {
    return this->tree.eval(context, matching_wells);
}

}
