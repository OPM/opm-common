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

#ifndef UDQASTNODE_HPP
#define UDQASTNODE_HPP

#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQWellSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>



namespace Opm {

class UDQASTNode {
public:
    UDQASTNode();

    UDQASTNode(UDQTokenType type_arg);
    UDQASTNode(UDQTokenType type_arg, const std::string& string_value, const std::vector<std::string>& selector);
    UDQASTNode(UDQTokenType type_arg, const std::string& func_name, const UDQASTNode& arg);
    UDQASTNode(UDQTokenType type_arg, const std::string& func_name, const UDQASTNode& left, const UDQASTNode& right);
    UDQWellSet eval_wells(const UDQContext& context);
    UDQTokenType type;

private:
    std::string string_value;
    double scalar_value;
    std::vector<std::string> selector;
    std::vector<UDQASTNode> arglist;
};

}

#endif
