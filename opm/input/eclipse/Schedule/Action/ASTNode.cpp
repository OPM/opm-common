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
#include <opm/common/utility/String.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace {
    std::string strip_quotes(const std::string& s)
    {
        if (s.front() != '\'') {
            return s;
        }

        return s.substr(1, s.size() - 2);
    }

    std::vector<std::string>
    strip_quotes(const std::vector<std::string>& quoted_strings)
    {
        std::vector<std::string> strings{};
        strings.reserve(quoted_strings.size());

        std::transform(quoted_strings.begin(), quoted_strings.end(),
                       std::back_inserter(strings),
                       [](const std::string& qs)
                       {
                           return strip_quotes(qs);
                       });

        return strings;
    }

    bool isDefaultRegSet(const std::string& regSet)
    {
        const auto trimmedRegSet = Opm::trim_copy(strip_quotes(regSet));

        return trimmedRegSet.empty()
            || (trimmedRegSet == "1*");
    }

    std::string
    normaliseFunction(const Opm::Action::FuncType     funcType,
                      std::string_view                function,
                      const std::vector<std::string>& argListIn)
    {
        if ((funcType != Opm::Action::FuncType::region) ||
            (argListIn.size() < 2))
        {
            return std::string { function };
        }

        if (argListIn.size() > 2) {
            throw std::invalid_argument {
                fmt::format(R"(Selection "{}" is not supported for region vector '{}')",
                            fmt::join(argListIn, " "), function)
            };
        }

        if (isDefaultRegSet(argListIn.back())) {
            // The input specification is something along the lines of
            //
            //   RPR 42 1*  or  RPR 42 ' '
            //
            // This corresponds to the default 'FIPNUM' region set, so
            // return
            //
            //   RPR
            return std::string { function };
        }

        // The input specification is something along the lines of
        //
        //   RPR 42 RE4
        //
        // Normalise vector name to the common
        //
        //   RPR__RE4
        //
        // as that's the expected region vector name format elsewhere.
        return fmt::format("{:_<5}{}", function,
                           strip_quotes(argListIn.back())
                           .substr(0, 3));
    }

    std::vector<std::string>
    normaliseArgList(const Opm::Action::FuncType funcType,
                     std::vector<std::string>&&  argListIn)
    {
        if ((funcType != Opm::Action::FuncType::region) ||
            (argListIn.size() < 2))
        {
            return std::move(argListIn);
        }

        // The input specification is something along the lines of
        //
        //   RPR 42 RE4
        //
        // NormaliseFunction() creates the vector name
        //
        //   RPR__RE4
        //
        // so we only need to return the "42" here (front of argument list).
        return { std::move(argListIn.front()) };
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
                              std::string_view                func_arg,
                              const std::vector<std::string>& arg_list_arg)
    : type     (type_arg)
    , func_type(func_type_arg)
    , func     (normaliseFunction(func_type, func_arg, arg_list_arg))
    , arg_list (normaliseArgList(func_type, strip_quotes(arg_list_arg)))
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

void Opm::Action::ASTNode::add_child(ASTNode&& child)
{
    this->children.push_back(std::move(child));
}

Opm::Action::Result
Opm::Action::ASTNode::eval(const Context& context) const
{
    if (this->empty()) {
        throw std::invalid_argument {
            "ASTNode::eval() should not reach leaf nodes"
        };
    }

    if ((this->type == TokenType::op_or) ||
        (this->type == TokenType::op_and))
    {
        return this->evalLogicalOperation(context);
    }

    return this->evalComparison(context);
}

void Opm::Action::ASTNode::
required_summary(std::unordered_set<std::string>& required_summary) const
{
    if (this->type == TokenType::ecl_expr) {
        required_summary.insert(this->func);
    }

    for (const auto& node : this->children) {
        node.required_summary(required_summary);
    }
}

bool Opm::Action::ASTNode::operator==(const ASTNode& that) const
{
    return (this->type == that.type)
        && (this->func_type == that.func_type)
        && (this->func == that.func)
        && (this->arg_list == that.arg_list)
        && (this->number == that.number)
        && (this->children == that.children)
        ;
}

std::size_t Opm::Action::ASTNode::size() const
{
    return this->children.size();
}

bool Opm::Action::ASTNode::empty() const
{
    return this->size() == 0;
}

// ===========================================================================
// Private member functions
// ===========================================================================

Opm::Action::Result
Opm::Action::ASTNode::evalLogicalOperation(const Context& context) const
{
    auto result = Result { this->type == TokenType::op_and };

    auto setOp = (this->type == TokenType::op_or)
        ? &Result::makeSetUnion          // or  => union
        : &Result::makeSetIntersection;  // and => intersection

    // Recursive evaluation down tree.
    for (const auto& child : this->children) {
        (result.*setOp)(child.eval(context));
    }

    return result;
}

Opm::Action::Result
Opm::Action::ASTNode::evalComparison(const Context& context) const
{
    auto v2 = Value {};

    // Special casing of MONTH comparisons where in addition to symbolic
    // month names we can compare with numeric month indices.  When
    // conducting such comparisons, the numeric value month value should be
    // compared to the *nearest integer* value of the right-hand side--in
    // other words the numeric month value should be rounded to the nearest
    // integer before comparison.  This means that for example
    //
    //   MNTH = 4.3
    //
    // should evaluate to true for the month of April (month index = 4) and
    // that
    //
    //   MNTH = 10.8
    //
    // should evaluate to true for the month of November (month index = 11).
    if (this->children.front().func_type == FuncType::time_month) {
        const auto& rhs = this->children[1];

        v2 = (rhs.type == TokenType::number)
            ? Value { std::round(rhs.number) }
            : rhs.nodeValue(context);
    }
    else {
        v2 = this->children[1].nodeValue(context);
    }

    return this->children.front().nodeValue(context).eval_cmp(this->type, v2);
}

Opm::Action::Value
Opm::Action::ASTNode::nodeValue(const Context& context) const
{
    if (! this->empty()) {
        throw std::invalid_argument {
            "nodeValue() method should only reach leaf nodes"
        };
    }

    if (this->type == TokenType::number) {
        return Value { this->number };
    }

    if (this->arg_list.empty()) {
        return Value { context.get(this->func) };
    }

    if (this->argListIsPattern()) {
        return this->evalListExpression(context);
    }
    else {
        return this->evalScalarExpression(context);
    }
}

Opm::Action::Value
Opm::Action::ASTNode::evalListExpression(const Context& context) const
{
    if (this->func_type != FuncType::well) {
        throw std::logic_error {
            ": attempted to action-evaluate list not of type well."
        };
    }

    return this->evalWellExpression(context);
}

Opm::Action::Value
Opm::Action::ASTNode::evalScalarExpression(const Context& context) const
{
    const auto arg_key = fmt::format("{}", fmt::join(this->arg_list, ":"));
    const auto scalar_value = context.get(this->func, arg_key);

    if (this->func_type != FuncType::well) {
        return Value { scalar_value };
    }

    return Value { this->arg_list.front(), scalar_value };
}

Opm::Action::Value
Opm::Action::ASTNode::evalWellExpression(const Context& context) const
{
    auto well_values = Value{};

    for (const auto& wname : this->getWellList(context)) {
        well_values.add_well(wname, context.get(this->func, wname));
    }

    return well_values;
}

namespace {
    std::string normalisePattern(const std::string& patt)
    {
        if (patt.front() == '\\') {
            // Trim leading '\' character since the 'patt' might be
            // something like
            //
            //    '\*P*'
            //
            // which denotes all wells (typically) whose names contain at
            // least one 'P' anywhere in the name.  Without the leading
            // backslash, the pattern would match all well lists whose names
            // begin with 'P'.
            return patt.substr(1);
        }

        return patt;
    }
} // Anonymous namespace

std::vector<std::string>
Opm::Action::ASTNode::getWellList(const Context& context) const
{
    if (this->argListIsWellList()) {
        return context.wlist_manager().wells(this->arg_list.front());
    }

    const auto& wells = context.wells(this->func);

    auto wnames = std::vector<std::string>{};
    wnames.reserve(wells.size());

    std::copy_if(wells.begin(), wells.end(), std::back_inserter(wnames),
                 [wpatt = normalisePattern(this->arg_list.front())]
                 (const auto& well) { return shmatch(wpatt, well); });

    return wnames;
}

bool Opm::Action::ASTNode::argListIsPattern() const
{
    return (this->arg_list.size() == 1)
        && (this->arg_list.front().find("*") != std::string::npos);
}

bool Opm::Action::ASTNode::argListIsWellList() const
{
    const auto& well_arg = this->arg_list.front();

    return (well_arg.size() > 1) && (well_arg.front() == '*');
}
