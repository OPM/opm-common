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

#include <opm/input/eclipse/Schedule/Action/ASTNode.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>
#include <opm/input/eclipse/Schedule/Well/WList.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>

#include <opm/common/utility/shmatch.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace {
    std::string strip_quotes(const std::string& s)
    {
        if (s.front() == '\'') {
            return s.substr(1, s.size() - 2);
        }
        else {
            return s;
        }
    }

    std::vector<std::string>
    strip_quotes(const std::vector<std::string>& quoted_strings)
    {
        std::vector<std::string> strings;

        std::transform(quoted_strings.begin(), quoted_strings.end(),
                       std::back_inserter(strings),
                       [](const std::string& qs)
                       {
                           return strip_quotes(qs);
                       });

        return strings;
    }
} // Anonymous namespace

Opm::Action::ASTNode::ASTNode()
    : ASTNode { TokenType::error }
{}

Opm::Action::ASTNode::ASTNode(const TokenType type_arg)
    : ASTNode { type_arg, FuncType::none, "", {} }
{}

Opm::Action::ASTNode::ASTNode(const double value)
    : ASTNode { TokenType::number }
{
    this->number = value;
}

Opm::Action::ASTNode::ASTNode(const TokenType                 type_arg,
                              const FuncType                  func_type_arg,
                              const std::string&              func_arg,
                              const std::vector<std::string>& arg_list_arg)
    : type     (type_arg)
    , func_type(func_type_arg)
    , func     (func_arg)
    , arg_list (strip_quotes(arg_list_arg))
{}

Opm::Action::ASTNode
Opm::Action::ASTNode::serializationTestObject()
{
    ASTNode result;

    result.type = TokenType::number;
    result.func_type = FuncType::field;
    result.func = "test1";
    result.arg_list = {"test2"};
    result.number = 1.0;
    ASTNode child = result;
    result.children = {child};

    return result;
}

std::size_t Opm::Action::ASTNode::size() const
{
    return this->children.size();
}

bool Opm::Action::ASTNode::empty() const
{
    return this->size() == 0;
}

void Opm::Action::ASTNode::add_child(const ASTNode& child)
{
    this->children.push_back(child);
}

Opm::Action::Value
Opm::Action::ASTNode::value(const Action::Context& context) const
{
    if (! this->children.empty()) {
        throw std::invalid_argument {
            "value() method should only reach leafnodes"
        };
    }

    if (this->type == TokenType::number) {
        return Value { this->number };
    }

    if (this->arg_list.empty()) {
        return Value { context.get(this->func) };
    }

    // The matching code is special case to handle one-argument cases with
    // well patterns like 'P*'.
    if ((this->arg_list.size() == 1) &&
        (this->arg_list.front().find("*") != std::string::npos))
    {
        if (this->func_type != FuncType::well) {
            throw std::logic_error {
                ": attempted to action-evaluate list not of type well."
            };
        }

        const auto& well_arg = this->arg_list[0];

        auto well_values = Value{};
        auto wnames = std::vector<std::string>{};

        if ((well_arg.front() == '*') && (well_arg.size() > 1)) {
            const auto& wlm = context.wlist_manager();
            wnames = wlm.wells(well_arg);
        }
        else {
            const auto& wells = context.wells(this->func);
            std::copy_if(wells.begin(), wells.end(), std::back_inserter(wnames),
                        [&well_arg](const auto& well)
                        {
                            return shmatch(well_arg, well);
                        });
        }

        for (const auto& wname : wnames) {
            well_values.add_well(wname, context.get(this->func, wname));
        }

        return well_values;
    }
    else {
        const auto arg_key = fmt::format("{}", fmt::join(this->arg_list, ":"));
        const auto scalar_value = context.get(this->func, arg_key);

        if (this->func_type == FuncType::well) {
            return Value { this->arg_list.front(), scalar_value };
        }

        return Value { scalar_value };
    }
}

Opm::Action::Result
Opm::Action::ASTNode::eval(const Action::Context& context) const
{
    if (this->empty()) {
        throw std::invalid_argument {
            "ASTNode::eval() should not reach leafnodes"
        };
    }

    if ((this->type == TokenType::op_or) ||
        (this->type == TokenType::op_and))
    {
        auto result = Result { this->type == TokenType::op_and };

        for (const auto& child : this->children) {
            if (this->type == TokenType::op_or) {
                result |= child.eval(context);
            }
            else {
                result &= child.eval(context);
            }
        }

        return result;
    }

    auto v2 = Value {};

    // Special casing of MONTH comparisons where in addition symbolic month
    // names we can compare with numeric months, in the case of numeric
    // months the numerical value should be rounded before comparison - i.e.
    //
    //   MNTH = 4.3
    //
    // should evaluate to true for the month of April (4).
    if (this->children.front().func_type == FuncType::time_month) {
        const auto& rhs = this->children[1];

        v2 = (rhs.type == TokenType::number)
            ? Value { std::round(rhs.number) }
            : rhs.value(context);
    }
    else {
        v2 = this->children[1].value(context);
    }

    return this->children.front().value(context).eval_cmp(this->type, v2);
}

bool Opm::Action::ASTNode::operator==(const ASTNode& data) const
{
    return (type == data.type)
        && (func_type == data.func_type)
        && (func == data.func)
        && (arg_list == data.arg_list)
        && (number == data.number)
        && (children == data.children)
        ;
}

void Opm::Action::ASTNode::required_summary(std::unordered_set<std::string>& required_summary) const
{
    if (this->type == TokenType::ecl_expr) {
        required_summary.insert(this->func);
    }

    for (const auto& node : this->children) {
        node.required_summary(required_summary);
    }
}
