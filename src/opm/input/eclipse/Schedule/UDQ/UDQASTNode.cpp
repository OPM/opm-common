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

#include <opm/input/eclipse/Schedule/UDQ/UDQASTNode.hpp>

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQFunction.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQFunctionTable.hpp>

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

namespace {

bool is_udq(const std::string& key)
{
    return (key.size() >= std::string::size_type{2})
        && (key[1] == 'U');
}

Opm::UDQVarType init_type(const Opm::UDQTokenType token_type)
{
    if (token_type == Opm::UDQTokenType::number) {
        return Opm::UDQVarType::SCALAR;
    }

    if (Opm::UDQ::scalarFunc(token_type)) {
        return Opm::UDQVarType::SCALAR;
    }

    return Opm::UDQVarType::NONE;
}

} // Anonymous namespace

namespace Opm {

UDQASTNode::UDQASTNode()
    : UDQASTNode(UDQTokenType::error)
{}

UDQASTNode::UDQASTNode(const UDQTokenType type_arg)
    : var_type(UDQVarType::NONE)
    , type    (type_arg)
{
    if ((this->type == UDQTokenType::error) ||
        (this->type == UDQTokenType::binary_op_add) ||
        (this->type == UDQTokenType::binary_op_sub))
    {
        return;
    }

    throw std::invalid_argument {
        "Single argument AST node constructor available only "
        "for error and binary addition/subtraction tokens"
    };
}

UDQASTNode::UDQASTNode(double numeric_value)
    : var_type(init_type(UDQTokenType::number))
    , type    (UDQTokenType::number)
    , value   (numeric_value)
{}

UDQASTNode::UDQASTNode(const UDQTokenType                       type_arg,
                       const std::variant<std::string, double>& value_arg)
    : var_type(init_type(type_arg))
    , type    (type_arg)
    , value   (value_arg)
{}

UDQASTNode::UDQASTNode(const UDQTokenType                       type_arg,
                       const std::variant<std::string, double>& value_arg,
                       const UDQASTNode&                        left_arg)
    : UDQASTNode(type_arg, value_arg)
{
    if (UDQ::scalarFunc(type_arg)) {
        this->var_type = UDQVarType::SCALAR;
    }
    else {
        this->var_type = left_arg.var_type;
    }

    this->left = std::make_unique<UDQASTNode>(left_arg);
}

UDQASTNode::UDQASTNode(const UDQTokenType                       type_arg,
                       const std::variant<std::string, double>& value_arg,
                       const UDQASTNode&                        left_arg,
                       const UDQASTNode&                        right_arg)
    : var_type(init_type(type_arg))
    , type    (type_arg)
    , value   (value_arg)
{
    this->set_left(left_arg);
    this->set_right(right_arg);
}

UDQASTNode::UDQASTNode(const UDQTokenType                       type_arg,
                       const std::variant<std::string, double>& value_arg,
                       const std::vector<std::string>&          selector_arg)
    : var_type(init_type(type_arg))
    , type    (type_arg)
    , value   (value_arg)
    , selector(selector_arg)
{
    if (type_arg == UDQTokenType::ecl_expr) {
        this->var_type = UDQ::targetType(std::get<std::string>(this->value), this->selector);
    }

    if ((this->var_type == UDQVarType::CONNECTION_VAR) ||
        (this->var_type == UDQVarType::REGION_VAR) ||
        (this->var_type == UDQVarType::SEGMENT_VAR) ||
        (this->var_type == UDQVarType::AQUIFER_VAR) ||
        (this->var_type == UDQVarType::BLOCK_VAR))
    {
        throw std::invalid_argument {
            "UDQ variable of type: " + UDQ::typeName(this->var_type) + " not yet supported in flow"
        };
    }
}

UDQASTNode UDQASTNode::serializationTestObject()
{
    UDQASTNode result;
    result.var_type = UDQVarType::REGION_VAR;
    result.type = UDQTokenType::error;
    result.value = "test1";
    result.selector = {"test2"};
    result.sign = -1;

    UDQASTNode left = result;
    result.left = std::make_shared<UDQASTNode>(left);

    return result;
}

UDQSet
UDQASTNode::eval(const UDQVarType  target_type,
                 const UDQContext& context) const
{
    if (this->type == UDQTokenType::ecl_expr) {
        return this->sign * this->eval_expression(context);
    }

    if (UDQ::scalarFunc(this->type)) {
        return this->sign * this->eval_scalar_function(target_type, context);
    }

    if (UDQ::elementalUnaryFunc(this->type)) {
        return this->sign * this->eval_elemental_unary_function(target_type, context);
    }

    if (UDQ::binaryFunc(this->type)) {
        return this->sign * this->eval_binary_function(target_type, context);
    }

    if (this->type == UDQTokenType::number) {
        return this->sign * this->eval_number(target_type, context);
    }

    throw std::invalid_argument {
        "Should not be here ... this->type: " + std::to_string(static_cast<int>(this->type))
    };
}

bool UDQASTNode::valid() const
{
    return this->type != UDQTokenType::error;
}

std::set<UDQTokenType> UDQASTNode::func_tokens() const
{
    auto tokens = std::set<UDQTokenType>{};
    this->func_tokens(tokens);

    return tokens;
}

void UDQASTNode::update_type(const UDQASTNode& arg)
{
    if (this->var_type == UDQVarType::NONE) {
        this->var_type = arg.var_type;
    }
    else {
        this->var_type = UDQ::coerce(this->var_type, arg.var_type);
    }
}

void UDQASTNode::set_left(const UDQASTNode& arg)
{
    this->left = std::make_unique<UDQASTNode>(arg);
    this->update_type(arg);
}

void UDQASTNode::set_right(const UDQASTNode& arg)
{
    this->right = std::make_unique<UDQASTNode>(arg);
    this->update_type(arg);
}

void UDQASTNode::scale(double sign_factor)
{
    this->sign *= sign_factor;
}

UDQASTNode* UDQASTNode::get_left() const
{
    return this->left.get();
}

UDQASTNode* UDQASTNode::get_right() const
{
    return this->right.get();
}

bool UDQASTNode::operator==(const UDQASTNode& data) const
{
    if ((this->left && !data.left) ||
        (!this->left && data.left))
    {
        return false;
    }

    if (this->left && !(*this->left == *data.left)) {
        return false;
    }

    if ((this->right && !data.right) ||
        (!this->right && data.right))
    {
        return false;
    }

    if (this->right && !(*this->right == *data.right)) {
        return false;
    }

    return (type == data.type)
        && (var_type == data.var_type)
        && (value == data.value)
        && (selector == data.selector)
        ;
}

void UDQASTNode::required_summary(std::unordered_set<std::string>& summary_keys) const
{
    if (this->type == UDQTokenType::ecl_expr) {
        if (std::holds_alternative<std::string>(this->value)) {
            const auto& keyword = std::get<std::string>(this->value);
            if (!is_udq(keyword)) {
                summary_keys.insert(keyword);
            }
        }
    }

    if (this->left) {
        this->left->required_summary(summary_keys);
    }

    if (this->right) {
        this->right->required_summary(summary_keys);
    }
}

UDQSet
UDQASTNode::eval_expression(const UDQContext& context) const
{
    const auto& string_value = std::get<std::string>(this->value);
    const auto data_type = UDQ::targetType(string_value);

    if (data_type == UDQVarType::WELL_VAR) {
        return this->eval_well_expression(string_value, context);
    }

    if (data_type == UDQVarType::GROUP_VAR) {
        return this->eval_group_expression(string_value, context);
    }

    if (data_type == UDQVarType::FIELD_VAR) {
        return UDQSet::scalar(string_value, context.get(string_value));
    }

    if (const auto scalar = context.get(string_value); scalar.has_value()) {
        return UDQSet::scalar(string_value, scalar.value());
    }

    throw std::logic_error {
        "Should not be here: var_type: '"
        + UDQ::typeName(data_type)
        + "' stringvalue: '"
        + string_value + '\''
    };
}

UDQSet
UDQASTNode::eval_well_expression(const std::string& string_value,
                                 const UDQContext&  context) const
{
    const auto& all_wells = context.wells();

    if (this->selector.empty()) {
        auto res = UDQSet::wells(string_value, all_wells);

        for (const auto& well : all_wells) {
            res.assign(well, context.get_well_var(well, string_value));
        }

        return res;
    }

    const auto& well_pattern = this->selector.front();

    if (well_pattern.find('*') == std::string::npos) {
        // The right hand side is a fully qualified well name without any
        // '*', in this case the right hand side evaluates to a *scalar* -
        // and that scalar value is distributed among all the wells in the
        // result set.
        return UDQSet::scalar(string_value, context.get_well_var(well_pattern, string_value));
    }
    else {
        // The right hand side is a set of wells.  The result set will be
        // updated for all wells in the right hand set, wells missing in the
        // right hand set will be undefined in the result set.
        auto res = UDQSet::wells(string_value, all_wells);
        for (const auto& wname : context.wells(well_pattern)) {
            res.assign(wname, context.get_well_var(wname, string_value));
        }

        return res;
    }
}

UDQSet
UDQASTNode::eval_group_expression(const std::string& string_value,
                                  const UDQContext&  context) const
{
    if (! this->selector.empty()) {
        const std::string& group_pattern = this->selector[0];
        if (group_pattern.find("*") == std::string::npos) {
            return UDQSet::scalar(string_value, context.get_group_var(group_pattern, string_value));
        }

        throw std::logic_error("Group names with wildcards is not yet supported");
    }

    const auto& groups = context.groups();

    auto res = UDQSet::groups(string_value, groups);
    for (const auto& group : groups) {
        res.assign(group, context.get_group_var(group, string_value));
    }

    return res;
}

UDQSet
UDQASTNode::eval_scalar_function(const UDQVarType  target_type,
                                 const UDQContext& context) const
{
    const auto& string_value = std::get<std::string>(this->value);
    const auto& udqft = context.function_table();

    const auto& func = dynamic_cast<const UDQScalarFunction&>(udqft.get(string_value));

    return func.eval(this->left->eval(target_type, context));
}

UDQSet
UDQASTNode::eval_elemental_unary_function(const UDQVarType  target_type,
                                          const UDQContext& context) const
{
    const auto& string_value = std::get<std::string>(this->value);
    const auto func_arg = this->left->eval(target_type, context);

    const auto& udqft = context.function_table();
    const auto& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get(string_value));

    return func.eval(func_arg);
}

UDQSet
UDQASTNode::eval_binary_function(const UDQVarType  target_type,
                                 const UDQContext& context) const
{
    const auto left_arg = this->left->eval(target_type, context);
    const auto right_arg = this->right->eval(target_type, context);
    const auto& string_value = std::get<std::string>(this->value);

    const auto& udqft = context.function_table();
    const auto& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get(string_value));

    return func.eval(left_arg, right_arg);
}

UDQSet
UDQASTNode::eval_number(const UDQVarType  target_type,
                        const UDQContext& context) const
{
    const auto dummy_name = std::string { "DUMMY" };
    const auto numeric_value = std::get<double>(this->value);

    switch (target_type) {
    case UDQVarType::WELL_VAR:
        return UDQSet::wells(dummy_name, context.wells(), numeric_value);

    case UDQVarType::GROUP_VAR:
        return UDQSet::groups(dummy_name, context.groups(), numeric_value);

    case UDQVarType::SCALAR:
        return UDQSet::scalar(dummy_name, numeric_value);

    case UDQVarType::FIELD_VAR:
        return UDQSet::field(dummy_name, numeric_value);

    default:
        throw std::invalid_argument {
            "Unsupported target_type: " + std::to_string(static_cast<int>(target_type))
        };
    }
}

void UDQASTNode::func_tokens(std::set<UDQTokenType>& tokens) const
{
    tokens.insert(this->type);

    if (this->left) {
        this->left->func_tokens(tokens);
    }

    if (this->right) {
        this->right->func_tokens(tokens);
    }
}

UDQASTNode operator*(const UDQASTNode&lhs, double sign_factor)
{
    UDQASTNode prod = lhs;

    prod.scale(sign_factor);

    return prod;
}

UDQASTNode operator*(double lhs, const UDQASTNode& rhs)
{
    return rhs * lhs;
}

} // namespace Opm
