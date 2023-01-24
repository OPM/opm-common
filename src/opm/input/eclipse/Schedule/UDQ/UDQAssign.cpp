/*
  Copyright 2018 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/UDQ/UDQAssign.hpp>

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm {

void UDQAssign::AssignRecord::eval(UDQSet& values) const
{
    if (this->input_selector.empty() &&
        this->rst_selector.empty() &&
        this->numbered_selector.empty())
    {
        values.assign(this->value);
    }
    else if (! this->input_selector.empty()) {
        values.assign(this->input_selector[0], this->value);
    }
    else if (! this->rst_selector.empty()) {
        for (const auto& wgname : this->rst_selector) {
            values.assign(wgname, this->value);
        }
    }
    else {
        for (const auto& item : this->numbered_selector) {
            for (const auto& number : item.numbers) {
                values.assign(item.well, number, this->value);
            }
        }
    }
}

bool UDQAssign::AssignRecord::operator==(const AssignRecord& data) const
{
    return (this->input_selector == data.input_selector)
        && (this->rst_selector == data.rst_selector)
        && (this->numbered_selector == data.numbered_selector)
        && (this->report_step == data.report_step)
        && (this->value == data.value);
}

UDQAssign::UDQAssign(const std::string&              keyword,
                     const std::vector<std::string>& input_selector,
                     const double                    value,
                     const std::size_t               report_step)
    : m_keyword (keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(input_selector, value, report_step);
}

UDQAssign::UDQAssign(const std::string&                     keyword,
                     const std::unordered_set<std::string>& rst_selector,
                     const double                           value,
                     const std::size_t                      report_step)
    : m_keyword (keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(rst_selector, value, report_step);
}

UDQAssign::UDQAssign(const std::string&                              keyword,
                     const std::vector<UDQSet::EnumeratedWellItems>& selector,
                     double                                          value,
                     std::size_t                                     report_step)
    : m_keyword(keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(selector, value, report_step);
}

UDQAssign::UDQAssign(const std::string&                         keyword,
                     std::vector<UDQSet::EnumeratedWellItems>&& selector,
                     double                                     value,
                     std::size_t                                report_step)
    : m_keyword(keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(std::move(selector), value, report_step);
}

UDQAssign UDQAssign::serializationTestObject()
{
    UDQAssign result;
    result.m_keyword = "test";
    result.m_var_type = UDQVarType::CONNECTION_VAR;
    result.records.emplace_back(std::vector<std::string>{"test1"}, 1.0, 0);

    result.records.emplace_back(std::unordered_set<std::string>{ "I-45" }, 3.1415, 3);

    // Class-template argument deduction for the vector element type.
    result.records.emplace_back(std::vector { UDQSet::EnumeratedWellItems::serializationTestObject() }, 2.71828, 42);

    return result;
}

void UDQAssign::add_record(const std::vector<std::string>& input_selector,
                           const double                    value,
                           const std::size_t               report_step)
{
    this->records.emplace_back(input_selector, value, report_step);
}

void UDQAssign::add_record(const std::unordered_set<std::string>& rst_selector,
                           const double                           value,
                           const std::size_t                      report_step)
{
    this->records.emplace_back(rst_selector, value, report_step);
}

void UDQAssign::add_record(const std::vector<UDQSet::EnumeratedWellItems>& selector,
                           const double                                    value,
                           const std::size_t                               report_step)
{
    this->records.emplace_back(selector, value, report_step);
}

void UDQAssign::add_record(std::vector<UDQSet::EnumeratedWellItems>&& selector,
                           const double                               value,
                           const std::size_t                          report_step)
{
    this->records.emplace_back(std::move(selector), value, report_step);
}

const std::string& UDQAssign::keyword() const
{
    return this->m_keyword;
}

UDQVarType UDQAssign::var_type() const
{
    return this->m_var_type;
}

std::size_t UDQAssign::report_step() const
{
    return this->records.back().report_step;
}

UDQSet UDQAssign::eval(const std::vector<std::string>& wgnames) const
{
    if (this->m_var_type == UDQVarType::WELL_VAR) {
        auto ws = UDQSet::wells(this->m_keyword, wgnames);

        for (const auto& record : this->records) {
            record.eval(ws);
        }

        return ws;
    }

    throw std::invalid_argument {
        fmt::format("ASSIGN UDQ '{}': eval(vector<string>) not "
                    "yet implemented for variables of type {}",
                    this->m_keyword, UDQ::typeName(this->m_var_type))
    };
}

UDQSet UDQAssign::eval(const std::vector<UDQSet::EnumeratedWellItems>& items) const
{
    if (this->m_var_type == UDQVarType::SEGMENT_VAR) {
        auto us = UDQSet { this->m_keyword, this->m_var_type, items };

        for (const auto& record : this->records) {
            record.eval(us);
        }

        return us;
    }

    throw std::invalid_argument {
        fmt::format("ASSIGN UDQ '{}': eval(vector<Enumerated Items>) not "
                    "yet implemented for variables of type {}",
                    this->m_keyword, UDQ::typeName(this->m_var_type))
    };
}

UDQSet UDQAssign::eval() const
{
    if ((this->m_var_type == UDQVarType::FIELD_VAR) ||
        (this->m_var_type == UDQVarType::SCALAR))
    {
        return UDQSet::scalar(this->m_keyword, this->records.back().value);
    }

    throw std::invalid_argument {
        fmt::format("ASSIGN UDQ '{}': eval() not yet "
                    "implemented for variables of type {}",
                    this->m_keyword, UDQ::typeName(this->m_var_type))
    };
}

bool UDQAssign::operator==(const UDQAssign& data) const
{
    return (this->keyword() == data.keyword())
        && (this->var_type() == data.var_type())
        && (this->records == data.records);
}

} // namespace Opm
