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
#include <fnmatch.h>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunction.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

#include "UDQASTNode.hpp"

namespace Opm {



UDQASTNode::UDQASTNode(UDQTokenType type_arg) :
    type(type_arg),
    var_type(UDQVarType::NONE)
{
    if (type == UDQTokenType::error)
        return;

    if (type == UDQTokenType::end)
        return;

    throw std::invalid_argument("The one argument constructor is only available for error and end");
}

UDQASTNode::UDQASTNode(double scalar_value_arg) :
    type(UDQTokenType::number),
    var_type(UDQVarType::SCALAR),
    scalar_value(scalar_value_arg)
{}




UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& func_name,
                       const UDQASTNode& arg) :
    type(type_arg),
    string_value(func_name)
{
    if (UDQ::scalarFunc(type_arg))
        this->var_type = UDQVarType::SCALAR;
    else if (UDQ::elementalUnaryFunc(type_arg))
        this->var_type = arg.var_type;

    this->arglist.push_back(arg);
}


UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& func_name,
                       const UDQASTNode& left,
                       const UDQASTNode& right) :
    type(type_arg),
    var_type((left.var_type == UDQVarType::SCALAR) ? right.var_type : left.var_type ),
    string_value(func_name)
{
    this->arglist.push_back(left);
    this->arglist.push_back(right);
}


namespace {

bool is_scalar(UDQVarType var_type, const std::vector<std::string>& selector)
{
    if (var_type == UDQVarType::SCALAR)
        return true;

    if (var_type == UDQVarType::FIELD_VAR)
        return true;

    if (var_type == UDQVarType::WELL_VAR) {
        if (selector.empty())
            return false;
        return (selector[0].find("*") == std::string::npos);
    }

    if (var_type == UDQVarType::GROUP_VAR) {
        if (selector.empty())
            return false;
        return (selector[0].find("*") == std::string::npos);
    }
}
}

UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& string_value_arg,
                       const std::vector<std::string>& selector_arg) :
    type(type_arg),
    var_type(UDQ::targetType(string_value_arg)),
    string_value(string_value_arg),
    selector(selector_arg)
{
    if (type_arg == UDQTokenType::number)
        this->scalar_value = std::stod(string_value);

    if (type_arg == UDQTokenType::ecl_expr)
        this->var_type = UDQ::targetType(string_value);

    if (this->var_type == UDQVarType::CONNECTION_VAR ||
        this->var_type == UDQVarType::REGION_VAR ||
        this->var_type == UDQVarType::SEGMENT_VAR ||
        this->var_type == UDQVarType::AQUIFER_VAR ||
        this->var_type == UDQVarType::BLOCK_VAR)
        throw std::logic_error("UDQ variable of type: " + UDQ::typeName(this->var_type) + " not yet supported in flow");
}




UDQSet UDQASTNode::eval(UDQVarType target_type, const UDQContext& context) const {
    if (this->type == UDQTokenType::ecl_expr) {
        if (this->var_type == UDQVarType::WELL_VAR) {
            const auto& wells = context.wells();

            if (this->selector.size() > 0) {
                const std::string& well_pattern = this->selector[0];
                if (well_pattern.find("*") == std::string::npos)
                    return UDQSet::scalar(this->string_value, context.get_well_var(well_pattern, this->string_value));
                else {
                    auto res = UDQSet::wells(this->string_value, wells);
                    int fnmatch_flags = 0;
                    for (const auto& well : wells) {
                        if (fnmatch(well_pattern.c_str(), well.c_str(), fnmatch_flags) == 0) {
                            if (context.has_well_var(well, this->string_value))
                                res.assign(well, context.get_well_var(well, this->string_value));
                        }
                    }
                    return res;
                }
            } else {
                auto res = UDQSet::wells(this->string_value, wells);
                for (const auto& well : wells) {
                    if (context.has_well_var(well, this->string_value))
                        res.assign(well, context.get_well_var(well, this->string_value));
                }
                return res;
            }
        }

        if (this->var_type == UDQVarType::GROUP_VAR) {
            if (this->selector.size() > 0) {
                const std::string& group_pattern = this->selector[0];
                if (group_pattern.find("*") == std::string::npos)
                    return UDQSet::scalar(this->string_value, context.get_group_var(group_pattern, this->string_value));
                else
                    throw std::logic_error("Group names with wildcards is not yet supported");
            } else {
                const auto& groups = context.groups();
                auto res = UDQSet::groups(this->string_value, groups);
                for (const auto& group : groups) {
                    if (context.has_group_var(group, this->string_value))
                        res.assign(group, context.get_group_var(group, this->string_value));
                }
                return res;
            }
        }

        if (this->var_type == UDQVarType::FIELD_VAR)
            return UDQSet::scalar(this->string_value, context.get(this->string_value));

        throw std::logic_error("Should not be here: var_type: " + UDQ::typeName(this->var_type));
    }


    if (UDQ::scalarFunc(this->type)) {
        const auto& udqft = context.function_table();
        const UDQScalarFunction& func = dynamic_cast<const UDQScalarFunction&>(udqft.get(this->string_value));
        return func.eval( this->arglist[0].eval(target_type, context) );
    }


    if (UDQ::elementalUnaryFunc(this->type)) {
        auto input_arg = this->arglist[0];
        auto func_arg = input_arg.eval(target_type, context);

        const auto& udqft = context.function_table();
        const UDQUnaryElementalFunction& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get(this->string_value));
        return func.eval(func_arg);
    }

    if (UDQ::binaryFunc(this->type)) {
        auto left_arg  = this->arglist[0].eval(target_type, context);
        auto right_arg = this->arglist[1].eval(target_type, context);

        const auto& udqft = context.function_table();
        const UDQBinaryFunction& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get(this->string_value));
        auto res = func.eval(left_arg, right_arg);
        return func.eval(left_arg, right_arg);
    }

    if (this->type == UDQTokenType::number) {
        switch(target_type) {
        case UDQVarType::WELL_VAR:
            return UDQSet::wells(this->string_value, context.wells(), this->scalar_value);
        case UDQVarType::SCALAR:
            return UDQSet::scalar(this->string_value, this->scalar_value);
        case UDQVarType::FIELD_VAR:
            return UDQSet::field(this->string_value, this->scalar_value);
        default:
            throw std::invalid_argument("Unsupported target_type: " + std::to_string(static_cast<int>(target_type)));
        }
    }

    throw std::invalid_argument("Should not be here ...");
}

void UDQASTNode::func_tokens(std::set<UDQTokenType>& tokens) const {
    tokens.insert( this->type );
    for (const auto& arg : this->arglist)
        arg.func_tokens(tokens);
}

std::set<UDQTokenType> UDQASTNode::func_tokens() const {
    std::set<UDQTokenType> tokens;
    this->func_tokens(tokens);
    return tokens;
}


}
