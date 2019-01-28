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


#ifndef ActionAST_HPP
#define ActionAST_HPP

#include <string>
#include <vector>
#include <memory>

namespace Opm {

class ActionContext;
class ASTNode;



class ActionAST{
public:
    ActionAST() = default;
    explicit ActionAST(const std::vector<std::string>& tokens);
    bool eval(const ActionContext& context, std::vector<std::string>& matching_wells) const;
private:
    /*
      The use of a pointer here is to be able to create this class with only a
      forward declaration of the ASTNode class. Would have prefered to use a
      unique_ptr, but that requires writing custom destructors - the use of a
      shared_ptr does not imply any shared ownership.
    */
    std::shared_ptr<ASTNode> condition;
};
}
#endif
