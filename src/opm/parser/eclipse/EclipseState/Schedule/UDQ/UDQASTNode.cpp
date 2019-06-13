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



UDQASTNode::UDQASTNode(UDQTokenType type) :
    type(type),
    var_type(UDQVarType::NONE)
{
    if (type == UDQTokenType::error)
        return;

    if (type == UDQTokenType::end)
        return;

    throw std::invalid_argument("The one argument constructor is only available for error and end");
}

UDQASTNode::UDQASTNode(double scalar_value) :
    type(UDQTokenType::number),
    var_type(UDQVarType::SCALAR),
    scalar_value(scalar_value)
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





UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& string_value,
                       const std::vector<std::string>& selector) :
    type(type_arg),
    var_type(UDQ::targetType(string_value)),
    string_value(string_value),
    selector(selector)
{
    if (type_arg == UDQTokenType::number)
        this->scalar_value = std::stod(string_value);

    if (type_arg == UDQTokenType::ecl_expr)
        this->var_type = UDQ::targetType(string_value);
}




UDQSet UDQASTNode::eval(UDQVarType target_type, const UDQContext& context) const {
    if (this->type == UDQTokenType::ecl_expr) {
        if (this->var_type == UDQVarType::WELL_VAR) {
            const auto& wells = context.wells();

            if (this->selector.size() > 0) {
                int fnmatch_flags = 0;
                const std::string& well_pattern = this->selector[0];
                if (well_pattern.find("*") == std::string::npos)
                    /*
                      The well name has been fully qualified - i.e. this
                       evaulates to a scalar, which will then subsequently be
                       scattered to all wells.
                    */
                    return UDQSet::scalar(this->string_value, context.get_well_var(well_pattern, this->string_value));
                else {
                    auto res = UDQSet::wells(this->string_value, wells);
                    for (const auto& well : wells) {
                        if (fnmatch(well_pattern.c_str(), well.c_str(), fnmatch_flags) == 0)
                            res.assign(well, context.get_well_var(well, this->string_value));
                    }
                    return res;
                }
            } else {
                auto res = UDQSet::wells(this->string_value, wells);
                for (const auto& well : wells)
                    res.assign(well, context.get_well_var(well, this->string_value));
                return res;
            }
        }

        if (this->var_type == UDQVarType::GROUP_VAR) {
            const auto& groups = context.groups();

            if (this->selector.size() > 0) {
                int fnmatch_flags = 0;
                const std::string& group_pattern = this->selector[0];
                if (group_pattern.find("*") == std::string::npos)
                    /*
                      The group name has been fully qualified - i.e. this
                       evaulates to a scalar, which will then subsequently be
                       scattered to all groups.
                    */
                    return UDQSet::scalar(this->string_value, context.get_group_var(group_pattern, this->string_value));
                else {
                    auto res = UDQSet::groups(this->string_value, groups);
                    for (const auto& group : groups) {
                        if (fnmatch(group_pattern.c_str(), group.c_str(), fnmatch_flags) == 0)
                            res.assign(group, context.get_group_var(group, this->string_value));
                    }
                    return res;
                }
            } else {
                auto res = UDQSet::groups(this->string_value, groups);
                for (const auto& group : groups)
                    res.assign(group, context.get_group_var(group, this->string_value));
                return res;
            }
        }
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


}
