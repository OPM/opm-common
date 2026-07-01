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

#include <opm/input/eclipse/Schedule/Action/Condition.hpp>

#include <opm/common/utility/String.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/io/eclipse/rst/action.hpp>

#include <opm/output/eclipse/VectorItems/action.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionParser.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>
#include <opm/input/eclipse/Schedule/Action/Enums.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

namespace {

    Opm::Action::Comparator comparator(Opm::Action::TokenType tt)
    {
        switch (tt) {
        case Opm::Action::TokenType::op_eq:
            return Opm::Action::Comparator::EQUAL;

        case Opm::Action::TokenType::op_ne:
            return Opm::Action::Comparator::NOT_EQUAL;

        case Opm::Action::TokenType::op_gt:
            return Opm::Action::Comparator::GREATER;

        case Opm::Action::TokenType::op_lt:
            return Opm::Action::Comparator::LESS;

        case Opm::Action::TokenType::op_le:
            return Opm::Action::Comparator::LESS_EQUAL;

        case Opm::Action::TokenType::op_ge:
            return Opm::Action::Comparator::GREATER_EQUAL;

        default:
            return Opm::Action::Comparator::INVALID;
        }
    }

    std::string strip_quotes(const std::string& s)
    {
        if (s.front() == '\'') {
            return s.substr(1, s.size() - 2);
        }

        return s;
    }

    std::pair<std::string, std::string>
    decomposeRegionVectorName(const std::string& regQuant)
    {
        auto components = std::pair {
            regQuant.substr(0, 5),
            std::string { "NUM" }
        };

        if (regQuant.size() <= std::string::size_type{5}) {
            return components;
        }

        components.first.erase(components.first.find_last_not_of('_') + 1);
        components.second = regQuant.substr(5);

        return components;
    }

} // Anonymous namespace

namespace Opm::Action {

// ===========================================================================
// Class Action::Quantity
// ===========================================================================

Quantity::Quantity(const std::string& arg)
    : quantity_ { trim_copy(strip_quotes(trim_copy(arg))) }
{}

Quantity::Quantity(const RestartIO::RstAction::Quantity& rst_quantity)
    : quantity_ { rst_quantity.quantity() }
    , args_     { rst_quantity.args() }
{}

Quantity Quantity::serializationTestObject()
{
    auto q = Quantity { "ROPR" };

    q.add_arg("42");
    q.add_arg("ABC");
    q.finalise();

    return q;
}

int Quantity::int_type() const
{
    if (this->type_.has_value()) {
        return *this->type_;
    }

    return this->type();
}

// ---------------------------------------------------------------------------
// Private member functions for Action::Quantity below
// ---------------------------------------------------------------------------

int Quantity::type() const
{
    using QType = Opm::RestartIO::Helpers::
        VectorItems::IACN::Value::QuantityType;

    // Note: We check for (numeric) constants and dates first lest "FEB" be
    // erroneously classified as a 'F'ield level summary vector or "SEP" be
    // similarly erroneously classified as a 'S'egment level summary vector.
    if (this->isNumeric() || this->isMonthName()) {
        return QType::Const;
    }

    if (this->isDay())   { return QType::Day;   }
    if (this->isMonth()) { return QType::Month; }
    if (this->isYear())  { return QType::Year;  }

    switch (this->quantity_.front()) {
    case 'F': return QType::Field;
    case 'W': return QType::Well;
    case 'G': return QType::Group;
    case 'R': return QType::Region;
    case 'S': return QType::Segment;
    case 'C': return QType::Connection;
    case 'B': return QType::Block;
    default:  return QType::Const;
    }
}

bool Quantity::isNumeric() const
{
    if (this->quantity_.empty()) {
        return false;
    }

    auto qstr = std::string_view { this->quantity_ };
    if (qstr.front() == '+') {
        // From_chars() does not recognise leading positive signs so trim
        // that.
        qstr.remove_prefix(1);

        if (qstr.empty()) {
            // Quantity_ == '+'.  Not a number.
            return false;
        }
    }

    // Quantity_ is a number if floating-point (i.e., double) overload of
    // from_chars() can parse the full remaining quantity_ string.
    auto x = 0.0;
    auto [ptr, ec] = std::from_chars(qstr.data(), qstr.data() + qstr.size(), x);

    return (ec == std::errc{}) && (ptr == qstr.data() + qstr.size());
}

bool Quantity::isDay() const
{
    return this->quantity_ == "DAY";
}

bool Quantity::isMonth() const
{
    return (this->quantity_ == "MNTH")
        || (this->quantity_ == "MONTH");
}

bool Quantity::isMonthName() const
{
    return TimeService::valid_month(this->quantity_);
}

bool Quantity::isYear() const
{
    return this->quantity_ == "YEAR";
}

void Quantity::add_arg(const std::string& arg)
{
    this->args_.push_back(trim_copy(strip_quotes(trim_copy(arg))));
    this->type_.reset();
}

void Quantity::finalise()
{
    using QType = Opm::RestartIO::Helpers::
        VectorItems::IACN::Value::QuantityType;

    this->type_.emplace(this->type());

    switch (*this->type_) {
    case QType::Region:
        this->finaliseRegion();
        break;

    case QType::Field:
    case QType::Well:
    case QType::Group:
    case QType::Segment:
    case QType::Connection:
    case QType::Const:
    case QType::Day:
    case QType::Month:
    case QType::Year:
    case QType::Block:
        // Nothing to do.
        break;
    }
}

void Quantity::finaliseRegion()
{
    // Request of the form
    //
    //   ROPR         -- All regions of built-in FIPNUM region set
    //   ROPR 42      -- Region 42 of built-in FIPNUM region set
    //   ROPR 42 ABC  -- Region 42 of user-defined FIPABC region set
    //   ROPR_ABC     -- All regions of user-defined FIPABC region set
    //   ROPR_ABC 42  -- Region 42 of user-defined FIPABC region set
    //
    // Create canonicalised lookup key (quantity_)
    //
    //    ROPR_set
    //
    // and ensure that args_ contains only the integer (42).  For the
    // default/built-in FIPNUM region set, this will become
    //
    //    ROPR_NUM
    //
    // and that is intentional.  We write this canonical form to the ZACN
    // restart file array.

    auto [base, regset] = decomposeRegionVectorName(this->quantity_);

    if (this->args_.size() == 2) {
        regset = this->args_.back();
        this->args_.erase(this->args_.begin() + 1, this->args_.end());
    }

    this->quantity_ = fmt::format("{:_<5}{}", base, regset);
}

// ===========================================================================
// Class Action::Condition
// ===========================================================================

Condition::Condition(const RestartIO::RstAction::Condition& rst_condition)
    : lhs         { rst_condition.lhs }
    , rhs         { rst_condition.rhs }
    , logic       { rst_condition.logic }
    , cmp         { rst_condition.cmp_op }
    , left_paren  { rst_condition.left_paren }
    , right_paren { rst_condition.right_paren }
    , cmp_string  { comparator_as_string(cmp) }
{
    this->lhs.finalise();
    this->rhs.finalise();
}

Condition::Condition(const std::vector<std::string>& tokens,
                     const KeywordLocation&          location)
{
    std::size_t token_index = 0;

    if (tokens.front() == "(") {
        this->left_paren = true;
        ++token_index;
    }

    this->lhs = Quantity { tokens[token_index] };
    ++token_index;

    while (true) {
        if (token_index >= tokens.size()) {
            break;
        }

        const auto comp = comparator(Parser::tokenType(tokens[token_index]));
        if (comp == Comparator::INVALID) {
            this->lhs.add_arg(tokens[token_index]);
            ++token_index;
        }
        else {
            this->cmp = comp;
            this->cmp_string = comparator_as_string(this->cmp);
            ++token_index;
            break;
        }
    }

    if (token_index >= tokens.size()) {
        throw std::invalid_argument {
            fmt::format("Could not determine right hand side/comparator "
                        "for ACTIONX keyword at {}:{}",
                        location.filename, location.lineno)
        };
    }

    this->rhs = Quantity { tokens[token_index] };
    ++token_index;

    while (true) {
        if (token_index >= tokens.size()) {
            break;
        }

        switch (Parser::tokenType(tokens[token_index])) {
        case TokenType::op_and:
            this->logic = Logical::AND;
            break;

        case TokenType::op_or:
            this->logic = Logical::OR;
            break;

        case TokenType::close_paren:
            this->right_paren = true;
            break;

        default:
            this->rhs.add_arg(tokens[token_index]);
            break;
        }

        ++token_index;
    }

    this->lhs.finalise();
    this->rhs.finalise();
}

bool Condition::operator==(const Condition& data) const
{
    return (this->lhs == data.lhs)
        && (this->rhs == data.rhs)
        && (this->logic == data.logic)
        && (this->cmp == data.cmp)
        && (this->left_paren == data.left_paren)
        && (this->right_paren == data.right_paren)
        && (this->cmp_string == data.cmp_string)
        ;
}

bool Condition::open_paren() const
{
    return this->left_paren && !this->right_paren;
}

bool Condition::close_paren() const
{
    return !this->left_paren && this->right_paren;
}

int Condition::paren_as_int() const
{
    using ParenType = Opm::RestartIO::Helpers
        ::VectorItems::IACN::Value::ParenType;

    if (this->open_paren()) {
        return ParenType::Open;
    }
    else if (this->close_paren()) {
        return ParenType::Close;
    }

    return ParenType::None;
}

int Condition::logic_as_int() const
{
    switch (this->logic) {
    case Logical::END:
        return 0;
    case Logical::AND:
        return 1;
    case Logical::OR:
        return 2;
    default:
        throw std::logic_error("What the f...?");
    }
}

int Condition::comparator_as_int() const
{
    switch (this->cmp) {
    case Comparator::GREATER:
        return 1;
    case Comparator::LESS:
        return 2;
    case Comparator::GREATER_EQUAL:
        return 3;
    case Comparator::LESS_EQUAL:
        return 4;
    case Comparator::EQUAL:
        return 5;
    case Comparator::NOT_EQUAL:
        return 6;
    default:
        throw std::logic_error(fmt::format("Unhandeled value: {} in enum comparison", static_cast<int>(this->cmp)));
    }
}

} // namespace Opm::Action
