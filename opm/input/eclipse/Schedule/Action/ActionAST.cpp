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

#include <opm/input/eclipse/Schedule/Action/ActionAST.hpp>

#include <opm/input/eclipse/Schedule/Action/ASTNode.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionParser.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

Opm::Action::AST::AST() = default;

Opm::Action::AST::AST(const std::vector<std::string>& tokens)
    : condition { Parser::parseCondition(tokens) }
{}

Opm::Action::AST::~AST() = default;

Opm::Action::AST::AST(const AST& rhs)
{
    if (rhs.condition != nullptr) {
        this->condition = std::make_unique<ASTNode>(*rhs.condition);
    }
}

Opm::Action::AST::AST(AST&& rhs)
    : condition { std::move(rhs.condition) }
{}

Opm::Action::AST&
Opm::Action::AST::operator=(const AST& rhs)
{
    if (this != &rhs) {
        if (rhs.condition == nullptr) {
            this->condition.reset();
        }
        else {
            this->condition = std::make_unique<ASTNode>(*rhs.condition);
        }
    }

    return *this;
}

Opm::Action::AST&
Opm::Action::AST::operator=(AST&& rhs)
{
    if (this != &rhs) {
        this->condition = std::move(rhs.condition);
    }

    return *this;
}

Opm::Action::AST Opm::Action::AST::serializationTestObject()
{
    AST result;
    result.condition = std::make_unique<ASTNode>(ASTNode::serializationTestObject());

    return result;
}

Opm::Action::Result
Opm::Action::AST::eval(const Action::Context& context) const
{
    if ((this->condition == nullptr) || this->condition->empty()) {
        return Result { false };
    }

    return this->condition->eval(context);
}

bool Opm::Action::AST::operator==(const AST& data) const
{
    const auto isNull = this->condition == nullptr;

    return (isNull == (data.condition == nullptr))
        && (isNull || (*this->condition == *data.condition));
}

void Opm::Action::AST::required_summary(std::unordered_set<std::string>& required_summary) const
{
    if ((this->condition == nullptr) || this->condition->empty()) {
        return;
    }

    this->condition->required_summary(required_summary);
}
