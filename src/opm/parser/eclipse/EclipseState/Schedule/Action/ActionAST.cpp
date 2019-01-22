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

#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionAST.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>

#include "ASTNode.hpp"
#include "ActionParser.hpp"

namespace Opm {

ActionAST::ActionAST(const std::vector<std::string>& tokens) {
    auto tree = ActionParser::parse(tokens);
    this->tree.reset( new ASTNode(tree) );
}


bool ActionAST::eval(const ActionContext& context, std::vector<std::string>& matching_wells) const {
    WellSet wells;
    bool eval_result = this->tree->eval(context, wells);
    matching_wells = wells.wells();
    return eval_result;
}

}
