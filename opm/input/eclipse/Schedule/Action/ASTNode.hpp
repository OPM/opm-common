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

#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>

#include "ActionValue.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace Opm { namespace Action {

class ActionContext;
class WellSet;

class ASTNode
{
public:
    ASTNode();
    explicit ASTNode(TokenType type_arg);
    explicit ASTNode(double value);
    explicit ASTNode(TokenType type_arg,
                     FuncType func_type_arg,
                     const std::string& func_arg,
                     const std::vector<std::string>& arg_list_arg);

    static ASTNode serializationTestObject();

    Action::Result eval(const Action::Context& context) const;
    Action::Value value(const Action::Context& context) const;

    void required_summary(std::unordered_set<std::string>& required_summary) const;

    bool operator==(const ASTNode& data) const;

    TokenType type;
    FuncType func_type;
    std::string func;

    void add_child(const ASTNode& child);
    std::size_t size() const;
    bool empty() const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(type);
        serializer(func_type);
        serializer(func);
        serializer(arg_list);
        serializer(number);
        serializer(children);
    }

private:
    std::vector<std::string> arg_list{};
    double number {0.0};

    // To have a member std::vector<ASTNode> inside the ASTNode class is
    // supposedly borderline undefined behaviour; it compiles without
    // warnings and works.  Good for enough for me.
    std::vector<ASTNode> children{};
};

}} // namespace Opm::Action

#endif // ASTNODE_HPP
