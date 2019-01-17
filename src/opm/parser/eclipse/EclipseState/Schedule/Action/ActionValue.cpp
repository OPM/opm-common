#include "ActionValue.hpp"



namespace Opm {


namespace {

bool eval_cmp_scalar(double lhs, TokenType op, double rhs) {
    switch (op) {

    case TokenType::op_eq:
        return lhs == rhs;

    case TokenType::op_ge:
        return lhs >= rhs;

    case TokenType::op_le:
        return lhs <= rhs;

    case TokenType::op_ne:
        return lhs != rhs;

    case TokenType::op_gt:
        return lhs > rhs;

    case TokenType::op_lt:
        return lhs < rhs;

    default:
        throw std::invalid_argument("Incorrect operator type - expected comparison");
    }
}

}


ActionValue::ActionValue(double value) :
    scalar_value(value),
    is_scalar(true)
{ }


double ActionValue::scalar() const {
    if (!this->is_scalar)
        throw std::invalid_argument("This value node represents a well list and can not be evaluated in scalar context");

    return this->scalar_value;
}


void ActionValue::add_well(const std::string& well, double value) {
    if (this->is_scalar)
        throw std::invalid_argument("This value node has been created as a scalar node - can not add well variables");

    this->well_values.emplace_back(well, value);
}


bool ActionValue::eval_cmp_wells(TokenType op, double rhs, WellSet& matching_wells) const {
    bool ret_value = false;
    for (const auto& pair : this->well_values) {
        const std::string& well = pair.first;
        const double value = pair.second;

        if (eval_cmp_scalar(value, op, rhs)) {
            matching_wells.add(well);
            ret_value = true;
        }
    }
    return ret_value;
}


bool ActionValue::eval_cmp(TokenType op, const ActionValue& rhs, WellSet& matching_wells) const {
    if (op == TokenType::number ||
        op == TokenType::ecl_expr ||
        op == TokenType::open_paren ||
        op == TokenType::close_paren ||
        op == TokenType::op_and ||
        op == TokenType::op_or ||
        op == TokenType::end ||
        op == TokenType::error)
        throw std::invalid_argument("Invalid operator");

    if (!rhs.is_scalar)
        throw std::invalid_argument("The right hand side must be a scalar value");

    if (this->is_scalar)
        return eval_cmp_scalar(this->scalar(), op, rhs.scalar());

    return this->eval_cmp_wells(op, rhs.scalar(), matching_wells);
}

}
