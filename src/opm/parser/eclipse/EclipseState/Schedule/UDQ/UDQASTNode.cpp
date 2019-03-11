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

UDQASTNode::UDQASTNode() :
    type(UDQTokenType::error)
{}


UDQASTNode::UDQASTNode(UDQTokenType type) :
    type(type)
{
    if (type == UDQTokenType::error)
        return;

    if (type == UDQTokenType::end)
        return;

    throw std::invalid_argument("The one argument constructor is only available for error and end");
}

UDQASTNode::UDQASTNode(double scalar_value) :
    type(UDQTokenType::number),
    scalar_value(scalar_value)
{}




UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& func_name,
                       const UDQASTNode& arg) :
    type(type_arg),
    string_value(func_name)
{
    this->arglist.push_back(arg);
}


UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& func_name,
                       const UDQASTNode& left,
                       const UDQASTNode& right) :
    type(type_arg),
    string_value(func_name)
{
    this->arglist.push_back(left);
    this->arglist.push_back(right);
}





UDQASTNode::UDQASTNode(UDQTokenType type_arg,
                       const std::string& string_value,
                       const std::vector<std::string>& selector) :
    type(type_arg),
    string_value(string_value),
    selector(selector)
{
    if (type_arg == UDQTokenType::number)
        this->scalar_value = std::stod(string_value);
}



UDQWellSet UDQASTNode::eval_wells(const UDQContext& context) {
    const auto& wells = context.wells();
    if (this->type == UDQTokenType::ecl_expr) {
        auto res = UDQWellSet(this->string_value, wells);
        if (this->selector.size() > 0) {
            int fnmatch_flags = 0;
            const std::string& well_pattern = this->selector[0];
            if (well_pattern.find("*") == std::string::npos)
                throw std::invalid_argument("When evaluating a well UDQ you can not use fully qualified well variables");

            for (const auto& well : wells) {
                if (fnmatch(well_pattern.c_str(), well.c_str(), fnmatch_flags) == 0)
                    res.assign(well, context.get_well_var(well, this->string_value));
            }
        } else {
            for (const auto& well : wells)
                res.assign(well, context.get_well_var(well, this->string_value));
        }

        return res;
    }

    if (UDQ::scalarFunc(this->type))
        throw std::invalid_argument("Can not invoke scalar function for well set");


    if (UDQ::elementalUnaryFunc(this->type)) {
        auto input_arg = this->arglist[0];
        auto func_arg = input_arg.eval_wells(context);

        const auto& udqft = context.function_table();
        const UDQUnaryElementalFunction& func = dynamic_cast<const UDQUnaryElementalFunction&>(udqft.get(this->string_value));
        auto udq_set = func.eval(func_arg);

        return UDQWellSet(this->string_value, wells, udq_set);
    }

    if (UDQ::binaryFunc(this->type)) {
        auto left_arg  = this->arglist[0].eval_wells(context);
        auto right_arg = this->arglist[1].eval_wells(context);

        const auto& udqft = context.function_table();
        const UDQBinaryFunction& func = dynamic_cast<const UDQBinaryFunction&>(udqft.get(this->string_value));
        auto udq_set = func.eval(left_arg, right_arg);

        return UDQWellSet(this->string_value, wells, udq_set);
    }

    if (this->type == UDQTokenType::number)
        return UDQWellSet(this->string_value, wells, this->scalar_value);

    throw std::invalid_argument("UDQASTNode::eval_wells: not implemented function type: " + std::to_string(static_cast<int>(this->type)));
}

}
