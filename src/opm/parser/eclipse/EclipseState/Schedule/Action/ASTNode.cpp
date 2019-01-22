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


#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>

#include "ASTNode.hpp"
#include "ActionValue.hpp"

namespace Opm {
ASTNode::ASTNode() :
    type(TokenType::error)
{}


ASTNode::ASTNode(TokenType type_arg):
    type(type_arg)
{}


ASTNode::ASTNode(double value) :
    type(TokenType::number),
    number(value)
{}


ASTNode::ASTNode(TokenType type_arg, const std::string& func_arg, const std::vector<std::string>& arg_list_arg):
    type(type_arg),
    func(func_arg),
    arg_list(arg_list_arg)
{}

size_t ASTNode::size() const {
    return this->children.size();
}


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


bool ASTNode::eval(const ActionContext& context, WellSet& matching_wells) const {
    if (this->children.size() == 0)
        throw std::invalid_argument("bool eval should not reach leafnodes");

    if (this->type == TokenType::op_or || this->type == TokenType::op_and) {
        bool value = (this->type == TokenType::op_and);
        WellSet wells;
        for (const auto& child : this->children) {
            /*
              Observe that we require that the set of matching wells is
              evaluated for all conditions, to ensure that we must protect
              against C++ short circuting of the logical operators, that is
              currently achieved by evaluating the child value first.
            */
            if (this->type == TokenType::op_or) {
                value = child.eval(context, wells) || value;
                matching_wells.add(wells);
            } else {
                value = child.eval(context, wells) && value;
                matching_wells.intersect(wells);
            }
        }
        return value;
    }

    auto v1 = this->children[0].value(context);
    auto v2 = this->children[1].value(context);
    return v1.eval_cmp(this->type, v2, matching_wells);
}

}
