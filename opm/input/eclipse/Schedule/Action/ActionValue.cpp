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

#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

#if 0
inline std::string tokenString(TokenType op) {
    switch (op) {

    case TokenType::op_eq:
        return "=";

    case TokenType::op_ge:
        return ">=";

    case TokenType::op_le:
        return "<=";

    case TokenType::op_ne:
        return "!=";

    case TokenType::op_gt:
        return ">";

    case TokenType::op_lt:
        return "<";

    case TokenType::op_or:
        return "OR";

    case TokenType::op_and:
        return "AND";

    case TokenType::open_paren:
        return "(";

    case TokenType::close_paren:
        return ")";

    default:
        return "????";
    }
}
#endif

bool eval_cmp_scalar(const double lhs, const Opm::Action::TokenType op, const double rhs)
{
    switch (op) {
    case Opm::Action::TokenType::op_eq:
        return lhs == rhs;

    case Opm::Action::TokenType::op_ge:
        return lhs >= rhs;

    case Opm::Action::TokenType::op_le:
        return lhs <= rhs;

    case Opm::Action::TokenType::op_ne:
        return lhs != rhs;

    case Opm::Action::TokenType::op_gt:
        return lhs > rhs;

    case Opm::Action::TokenType::op_lt:
        return lhs < rhs;

    default:
        throw std::invalid_argument {
            "Incorrect operator type - expected comparison"
        };
    }
}

} // Anonymous namespace

Opm::Action::Value::Value(double value)
    : scalar_value(value)
    , is_scalar(true)
{}

Opm::Action::Value::Value(const std::string& wname, double value)
    : scalar_value(0.0)
{
    this->add_well(wname, value);
}

double Opm::Action::Value::scalar() const
{
    if (!this->is_scalar) {
        throw std::invalid_argument {
            "This value node represents a well list and "
            "cannot be evaluated in scalar context"
        };
    }

    return this->scalar_value;
}

void Opm::Action::Value::add_well(const std::string& well, const double value)
{
    if (this->is_scalar) {
        throw std::invalid_argument {
            "This value node has been created as a "
            "scalar node - cannot add well variables"
        };
    }

    this->well_values.emplace_back(well, value);
}

Opm::Action::Result
Opm::Action::Value::eval_cmp_wells(const TokenType op, const double rhs) const
{
    std::vector<std::string> wells;
    bool result = false;

    for (const auto& [well, value] : this->well_values) {
        if (eval_cmp_scalar(value, op, rhs)) {
            wells.push_back(well);
            result = true;
        }
    }

    return Result(result, wells);
}

Opm::Action::Result
Opm::Action::Value::eval_cmp(const TokenType op, const Value& rhs) const
{
    if ((op == TokenType::number) ||
        (op == TokenType::ecl_expr) ||
        (op == TokenType::open_paren) ||
        (op == TokenType::close_paren) ||
        (op == TokenType::op_and) ||
        (op == TokenType::op_or) ||
        (op == TokenType::end) ||
        (op == TokenType::error))
    {
        throw std::invalid_argument("Invalid operator");
    }

    if (!rhs.is_scalar) {
        throw std::invalid_argument("The right hand side must be a scalar value");
    }

    if (this->is_scalar) {
        return Result(eval_cmp_scalar(this->scalar(), op, rhs.scalar()));
    }

    return this->eval_cmp_wells(op, rhs.scalar());
}
