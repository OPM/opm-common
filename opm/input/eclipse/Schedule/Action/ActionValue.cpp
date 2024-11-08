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
#include <string_view>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

namespace {

std::string tokenString(const Opm::Action::TokenType op)
{
    switch (op) {
    case Opm::Action::TokenType::op_eq:       return "=";
    case Opm::Action::TokenType::op_ge:       return ">=";
    case Opm::Action::TokenType::op_le:       return "<=";
    case Opm::Action::TokenType::op_ne:       return "!=";
    case Opm::Action::TokenType::op_gt:       return ">";
    case Opm::Action::TokenType::op_lt:       return "<";
    case Opm::Action::TokenType::op_or:       return "OR";
    case Opm::Action::TokenType::op_and:      return "AND";
    case Opm::Action::TokenType::open_paren:  return "(";
    case Opm::Action::TokenType::close_paren: return ")";

    default:
        return fmt::format("Unknown Operator '{}'",
                           static_cast<std::underlying_type_t<Opm::Action::TokenType>>(op));
    }
}

bool scalarComparisonHolds(const double                 lhs,
                           const Opm::Action::TokenType op,
                           const double                 rhs)
{
    // Alternatives listed in order of increasing enumerator values in
    // TokenType.
    switch (op) {
    case Opm::Action::TokenType::op_gt: return lhs >  rhs;
    case Opm::Action::TokenType::op_ge: return lhs >= rhs;
    case Opm::Action::TokenType::op_lt: return lhs <  rhs;
    case Opm::Action::TokenType::op_le: return lhs <= rhs;
    case Opm::Action::TokenType::op_eq: return lhs == rhs;
    case Opm::Action::TokenType::op_ne: return lhs != rhs;

    default:
        throw std::invalid_argument {
            fmt::format("Unexpected operator '{}' -- expected comparison",
                        tokenString(op))
        };
    }
}

bool isComparisonOperator(const Opm::Action::TokenType op)
{
    // Alternatives listed in order of increasing enumerator values in
    // TokenType.
    return (op == Opm::Action::TokenType::op_gt)
        || (op == Opm::Action::TokenType::op_ge)
        || (op == Opm::Action::TokenType::op_lt)
        || (op == Opm::Action::TokenType::op_le)
        || (op == Opm::Action::TokenType::op_eq)
        || (op == Opm::Action::TokenType::op_ne)
        ;
}

} // Anonymous namespace

Opm::Action::Value::Value(const double value)
    : scalar_value_ { value }
    , is_scalar_    { true }
{}

Opm::Action::Value::Value(std::string_view wname,
                          const double     value)
    : scalar_value_ { 0.0 }
{
    this->add_well(wname, value);
}

Opm::Action::Result
Opm::Action::Value::eval_cmp(const TokenType op, const Value& rhs) const
{
    if (! isComparisonOperator(op)) {
        throw std::invalid_argument {
            fmt::format("Invalid comparison operator '{}'", tokenString(op))
        };
    }

    if (! rhs.is_scalar_) {
        throw std::invalid_argument {
            fmt::format("The right hand side of {} must be a scalar value",
                        tokenString(op))
        };
    }

    if (this->is_scalar_) {
        return Result {
            scalarComparisonHolds(this->scalar(), op, rhs.scalar())
        };
    }

    return this->evalWellComparisons(op, rhs.scalar());
}

void Opm::Action::Value::add_well(std::string_view well, const double value)
{
    if (this->is_scalar_) {
        throw std::invalid_argument {
            "This value node has been created as a "
            "scalar node - cannot add well variables"
        };
    }

    this->well_values_.emplace_back(well, value);
}

double Opm::Action::Value::scalar() const
{
    if (! this->is_scalar_) {
        throw std::invalid_argument {
            "This value node represents a well list and "
            "cannot be evaluated in scalar context"
        };
    }

    return this->scalar_value_;
}

// ===========================================================================
// Private member functions
// ===========================================================================

Opm::Action::Result
Opm::Action::Value::evalWellComparisons(const TokenType op,
                                        const double    rhs) const
{
    auto matching_wells = std::vector<std::string> {};

    for (const auto& [well, value] : this->well_values_) {
        if (scalarComparisonHolds(value, op, rhs)) {
            matching_wells.push_back(well);
        }
    }

    return Result {!matching_wells.empty(), matching_wells};
}
