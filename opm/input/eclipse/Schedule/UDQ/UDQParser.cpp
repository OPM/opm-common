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

#include "UDQParser.hpp"

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/input/eclipse/Schedule/UDQ/UDQASTNode.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQParams.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQToken.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace {

void dump_tokens(const std::string&                target_var,
                 const std::vector<Opm::UDQToken>& tokens)
{
    std::cout << target_var << " = ";

    for (const auto& token : tokens) {
        std::cout << token.str();
    }

    std::cout << std::endl;
}

// This function is extremely weak - hopefully it can be improved in the
// future.  See the comment in UDQEnums.hpp about 'UDQ type system'.
bool static_type_check(const Opm::UDQVarType lhs,
                       const Opm::UDQVarType rhs)
{
    if (lhs == rhs) {
        return true;
    }

    if (rhs == Opm::UDQVarType::SCALAR) {
        return true;
    }

    // This does not check if the rhs evaluates to a scalar.
    if (rhs == Opm::UDQVarType::WELL_VAR) {
        return lhs == Opm::UDQVarType::WELL_VAR;
    }

    if (rhs == Opm::UDQVarType::TABLE_LOOKUP) {
        return (lhs == Opm::UDQVarType::WELL_VAR)
            || (lhs == Opm::UDQVarType::FIELD_VAR)
            || (lhs == Opm::UDQVarType::SEGMENT_VAR)
            || (lhs == Opm::UDQVarType::GROUP_VAR);
    }

    return false;
}

struct UDQParseNode
{
    UDQParseNode(const Opm::UDQTokenType                  type_arg,
                 const std::variant<std::string, double>& value_arg,
                 const std::vector<std::string>&          selector_arg)
        : type    (type_arg)
        , value   (value_arg)
        , selector(selector_arg)
    {
        if (type_arg == Opm::UDQTokenType::ecl_expr) {
            this->var_type = Opm::UDQ::
                targetType(std::get<std::string>(value_arg), selector_arg);
        }
    }

    UDQParseNode(const Opm::UDQTokenType                  type_arg,
                 const std::variant<std::string, double>& value_arg)
        : UDQParseNode(type_arg, value_arg, {})
    {}

    // Implicit converting constructor.
    explicit UDQParseNode(const Opm::UDQTokenType type_arg)
        : UDQParseNode(type_arg, "")
    {}

    std::string string() const
    {
        if (std::holds_alternative<std::string>(this->value)) {
            return std::get<std::string>(this->value);
        }
        else {
            return std::to_string(std::get<double>(this->value));
        }
    }

    Opm::UDQTokenType type;
    std::variant<std::string, double> value;
    std::vector<std::string> selector;
    Opm::UDQVarType var_type { Opm::UDQVarType::NONE };
};

class UDQParser
{
public:
    UDQParser(const Opm::UDQParams&             udq_params1,
              const std::vector<Opm::UDQToken>& tokens_)
        : udq_params(udq_params1)
        , udqft     (udq_params)
        , tokens    (tokens_)
    {}

    Opm::UDQASTNode parse_set();

    bool empty() const;
    UDQParseNode current() const;

private:
    Opm::UDQASTNode parse_cmp();
    Opm::UDQASTNode parse_add();
    Opm::UDQASTNode parse_factor();
    Opm::UDQASTNode parse_mul();
    Opm::UDQASTNode parse_pow();

    UDQParseNode next();

    Opm::UDQTokenType get_type(const std::string& arg) const;
    std::size_t current_size() const;

    const Opm::UDQParams& udq_params;
    Opm::UDQFunctionTable udqft;
    std::vector<Opm::UDQToken> tokens;

    std::vector<Opm::UDQToken>::size_type current_pos {};
};

Opm::UDQTokenType UDQParser::get_type(const std::string& arg) const
{
    auto func_type = Opm::UDQ::funcType(arg);

    if (func_type != Opm::UDQTokenType::error) {
        return func_type;
    }

    if (arg == "(") {
        return Opm::UDQTokenType::open_paren;
    }

    if (arg == ")") {
        return Opm::UDQTokenType::close_paren;
    }

    if (arg == "[") {
        return Opm::UDQTokenType::table_lookup_start;
    }

    if (arg == "]") {
        return Opm::UDQTokenType::table_lookup_end;
    }

    {
        char* end_ptr;
        std::strtod(arg.c_str(), &end_ptr);

        if (std::strlen(end_ptr) == 0) {
            return Opm::UDQTokenType::number;
        }
    }

    return Opm::UDQTokenType::ecl_expr;
}

bool UDQParser::empty() const
{
    return static_cast<std::size_t>(this->current_pos) == this->tokens.size();
}

UDQParseNode UDQParser::next()
{
    this->current_pos += 1;

    return this->current();
}

UDQParseNode UDQParser::current() const
{
    if (this->empty()) {
        return UDQParseNode{ Opm::UDQTokenType::end };
    }

    const auto& token = this->tokens[current_pos];
    if (token.type() == Opm::UDQTokenType::number) {
        return { Opm::UDQTokenType::number, token.value() };
    }

    if (token.type() == Opm::UDQTokenType::ecl_expr) {
        return { Opm::UDQTokenType::ecl_expr, token.value(), token.selector() };
    }

    return {
        this->get_type(std::get<std::string>(token.value())),
        token.value()
    };
}

Opm::UDQASTNode UDQParser::parse_factor()
{
    double sign = 1.0;

    auto curr = this->current();
    if ((curr.type == Opm::UDQTokenType::binary_op_add) ||
        (curr.type == Opm::UDQTokenType::binary_op_sub))
    {
        if (curr.type == Opm::UDQTokenType::binary_op_sub) {
            sign = -1.0;
        }

        this->next();
        curr = this->current();
    }


    if (curr.type == Opm::UDQTokenType::open_paren) {
        this->next();
        auto inner_expr = this->parse_set();

        curr = this->current();
        if (curr.type != Opm::UDQTokenType::close_paren) {
            return Opm::UDQASTNode { Opm::UDQTokenType::error };
        }

        this->next();
        return sign * inner_expr;
    }

    if (Opm::UDQ::scalarFunc(curr.type) ||
        Opm::UDQ::elementalUnaryFunc(curr.type))
    {
        auto func_node = curr;
        auto Next = this->next();

        if (Next.type == Opm::UDQTokenType::open_paren) {
            this->next();

            auto arg_expr = this->parse_set();

            curr = this->current();
            if (curr.type != Opm::UDQTokenType::close_paren) {
                return Opm::UDQASTNode { Opm::UDQTokenType::error };
            }

            this->next();

            return sign * Opm::UDQASTNode { func_node.type, func_node.value, arg_expr };
        }
        else {
            return Opm::UDQASTNode { Opm::UDQTokenType::error };
        }
    }

    auto node = Opm::UDQASTNode {
        curr.type, curr.value, curr.selector
    };

    this->next();

    return sign * node;
}

Opm::UDQASTNode UDQParser::parse_pow()
{
    auto left = this->parse_factor();
    if (this->empty()) {
        return left;
    }

    auto curr = this->current();
    if (curr.type == Opm::UDQTokenType::binary_op_pow) {
        this->next();

        if (this->empty()) {
            return Opm::UDQASTNode { Opm::UDQTokenType::error };
        }

        auto right = this->parse_mul();

        return { curr.type, curr.value, left, right };
    }

    return left;
}

Opm::UDQASTNode UDQParser::parse_mul()
{
    auto nodes = std::vector<Opm::UDQASTNode>{};
    {
        auto current_node = std::unique_ptr<Opm::UDQASTNode>{};

        while (true) {
            auto node = this->parse_pow();

            if (current_node) {
                current_node->set_right(node);
                nodes.push_back(*current_node);
            }
            else {
                nodes.push_back(node);
            }

            if (this->empty()) {
                break;
            }

            auto current_token = this->current();
            if ((current_token.type == Opm::UDQTokenType::binary_op_mul) ||
                (current_token.type == Opm::UDQTokenType::binary_op_div))
            {
                current_node = std::make_unique<Opm::UDQASTNode>
                    (current_token.type, current_token.value);

                this->next();
                if (this->empty()) {
                    return Opm::UDQASTNode { Opm::UDQTokenType::error };
                }
            }
            else {
                break;
            }
        }
    }

    auto top_node = nodes.back();

    if (nodes.size() > 1) {
        auto* curr = &top_node;

        for (std::size_t index = nodes.size() - 1; index > 0; --index) {
            curr->set_left(nodes[index - 1]);
            curr = curr->get_left();
        }
    }

    return top_node;
}

Opm::UDQASTNode UDQParser::parse_add()
{
    auto nodes = std::vector<Opm::UDQASTNode>{};

    {
        auto current_node = std::unique_ptr<Opm::UDQASTNode>{};
        while (true) {
            auto node = this->parse_mul();
            if (current_node) {
                current_node->set_right(node);
                nodes.push_back(*current_node);
            }
            else {
                nodes.push_back(node);
            }

            if (this->empty()) {
                break;
            }

            auto current_token = this->current();
            if ((current_token.type == Opm::UDQTokenType::binary_op_add) ||
                (current_token.type == Opm::UDQTokenType::binary_op_sub))
            {
                current_node = std::make_unique<Opm::UDQASTNode>
                    (current_token.type, current_token.value);

                this->next();
                if (this->empty()) {
                    return Opm::UDQASTNode { Opm::UDQTokenType::error };
                }
            }
            else if ((current_token.type == Opm::UDQTokenType::close_paren) ||
                     Opm::UDQ::cmpFunc(current_token.type) ||
                     Opm::UDQ::setFunc(current_token.type))
            {
                break;
            }
            else {
                return Opm::UDQASTNode { Opm::UDQTokenType::error };
            }
        }
    }

    auto top_node = nodes.back();

    if (nodes.size() > 1) {
        auto* curr = &top_node;

        for (std::size_t index = nodes.size() - 1; index > 0; --index) {
            curr->set_left(nodes[index - 1]);
            curr = curr->get_left();
        }
    }

    return top_node;
}

// A bit uncertain on the presedence of the comparison operators.  In normal
// C the comparion operators bind weaker than addition, i.e. for the
// assignment:
//
//    auto cmp = a + b < c;
//
// The sum (a+b) is evaluated and then compared with c, that is the order of
// presedence implemented here.  But reading the eclipse UDQ manual one can
// get the imporession that the relation operators should bind "very
// strong", i.e., that (b < c) should be evaluated first, and then the
// result of the comparison added to "a".

Opm::UDQASTNode UDQParser::parse_cmp()
{
    auto left = this->parse_add();
    if (this->empty()) {
        return left;
    }

    auto curr = this->current();

    if (Opm::UDQ::cmpFunc(curr.type)) {
        this->next();
        if (this->empty()) {
            return Opm::UDQASTNode { Opm::UDQTokenType::error };
        }

        auto right = this->parse_cmp();

        return { curr.type, curr.value, left, right };
    }

    return left;
}

Opm::UDQASTNode UDQParser::parse_set()
{
    auto left = this->parse_cmp();
    if (this->empty()) {
        return left;
    }

    auto curr = this->current();

    if (Opm::UDQ::setFunc(curr.type)) {
        this->next();
        if (this->empty()) {
            return Opm::UDQASTNode { Opm::UDQTokenType::error };
        }

        auto right = this->parse_set();

        return { curr.type, curr.value, left, right };
    }

    return left;
}

} // Anonymous namespace

// ===========================================================================

std::unique_ptr<Opm::UDQASTNode>
Opm::parseUDQExpression(const UDQParams&             udq_params,
                        const UDQVarType             target_type,
                        const std::string&           target_var,
                        const KeywordLocation&       location,
                        const std::vector<UDQToken>& tokens,
                        const ParseContext&          parseContext,
                        ErrorGuard&                  errors)
{
    auto parser = UDQParser { udq_params, tokens };

    auto tree = parser.parse_set();

    if (! parser.empty()) {
        const auto current = parser.current();

        const auto msg_fmt =
            fmt::format("Problem parsing UDQ {}\n"
                        "In {{file}} line {{line}}.\n"
                        "Extra unhandled data starting with item {}.",
                        target_var, current.string());

        parseContext.handleError(ParseContext::UDQ_PARSE_ERROR, msg_fmt, location, errors);

        return std::make_unique<UDQASTNode>(udq_params.undefinedValue());
    }

    if (! tree.valid()) {
        std::string token_string;
        for (const auto& token : tokens) {
            token_string += token.str() + " ";
        }

        const auto msg_fmt =
            fmt::format("Failed to parse UDQ {}\n"
                        "In {{file}} line {{line}}.\n"
                        "This can be a bug in flow or a "
                        "bug in the UDQ input string.\n"
                        "UDQ input: '{}'",
                        target_var, token_string);

        parseContext.handleError(ParseContext::UDQ_PARSE_ERROR, msg_fmt, location, errors);

        return std::make_unique<UDQASTNode>(udq_params.undefinedValue());
    }

    if (! static_type_check(target_type, tree.var_type)) {
        const auto msg_fmt =
            fmt::format("Failed to parse UDQ {}\n"
                        "In {{file}} line {{line}}.\n"
                        "Invalid type conversion detected "
                        "in UDQ expression expected: {}, got: {}",
                        target_var,
                        UDQ::typeName(target_type),
                        UDQ::typeName(tree.var_type));

        parseContext.handleError(ParseContext::UDQ_TYPE_ERROR, msg_fmt, location, errors);

        if (parseContext.get(ParseContext::UDQ_TYPE_ERROR) != InputErrorAction::IGNORE) {
            dump_tokens(target_var, tokens);
        }

        return std::make_unique<UDQASTNode>(udq_params.undefinedValue());
    }

    if (tree.var_type == UDQVarType::NONE) {
        const auto msg_fmt =
            fmt::format("Failed to parse UDQ {}\n"
                        "In {{file}} line {{line}}.\n"
                        "Could not determine "
                        "expression type.", target_var);

        parseContext.handleError(ParseContext::UDQ_TYPE_ERROR, msg_fmt, location, errors);

        if (parseContext.get(ParseContext::UDQ_TYPE_ERROR) != InputErrorAction::IGNORE) {
            dump_tokens(target_var, tokens);
        }

        return std::make_unique<UDQASTNode>(udq_params.undefinedValue());
    }

    return std::make_unique<UDQASTNode>(std::move(tree));
}
