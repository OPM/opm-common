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

#include <opm/io/eclipse/rst/udq.hpp>

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
        this->numbered_selector.empty())
    {
        values.assign(this->value);
    }
    else if (! this->input_selector.empty()) {
        values.assign(this->input_selector[0], this->value);
    }
    else {
        this->assignEnumeration(this->numbered_selector, values);
    }
}

void UDQAssign::AssignRecord::eval(WGNameMatcher matcher, UDQSet& values) const
{
    if (!this->numbered_selector.empty()) {
        throw std::logic_error {
            "UDQ assignment with well/group pattern "
            "matching is not supported for enumerated UDQ sets"
        };
    }

    for (const auto& wgname : matcher(this->input_selector)) {
        values.assign(wgname, this->value);
    }
}

void UDQAssign::AssignRecord::eval(ItemMatcher matcher, UDQSet& values) const
{
    if (!this->numbered_selector.empty()) {
        throw std::logic_error {
            "UDQ assignment with enumerated item matching "
            "is not supported for explicitly enumerated UDQ sets"
        };
    }

    this->assignEnumeration(matcher(this->input_selector), values);
}

bool UDQAssign::AssignRecord::operator==(const AssignRecord& data) const
{
    return (this->input_selector == data.input_selector)
        && (this->numbered_selector == data.numbered_selector)
        && (this->report_step == data.report_step)
        && (this->value == data.value);
}

void UDQAssign::AssignRecord::
assignEnumeration(const VEnumItems& items, UDQSet& values) const
{
    for (const auto& item : items) {
        for (const auto& number : item.numbers) {
            values.assign(item.name, number, this->value);
        }
    }
}

// ---------------------------------------------------------------------------

UDQAssign::UDQAssign(const std::string& keyword,
                     const VString&     input_selector,
                     const double       value,
                     const std::size_t  report_step)
    : m_keyword (keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(input_selector, value, report_step);
}

UDQAssign::UDQAssign(const std::string& keyword,
                     const VEnumItems&  selector,
                     double             value,
                     std::size_t        report_step)
    : m_keyword(keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(selector, value, report_step);
}

UDQAssign::UDQAssign(const std::string& keyword,
                     VEnumItems&&       selector,
                     double             value,
                     std::size_t        report_step)
    : m_keyword(keyword)
    , m_var_type(UDQ::varType(keyword))
{
    this->add_record(std::move(selector), value, report_step);
}

UDQAssign::UDQAssign(const std::string&       keyword,
                     const RestartIO::RstUDQ& assignRst,
                     const std::size_t        report_step)
    : m_keyword  { keyword }
    , m_var_type { assignRst.category }
{
    this->add_record(assignRst, report_step);
}

UDQAssign UDQAssign::serializationTestObject()
{
    UDQAssign result;
    result.m_keyword = "test";
    result.m_var_type = UDQVarType::CONNECTION_VAR;
    result.records.emplace_back(std::vector<std::string>{"test1"}, 1.0, 0);

    // Class-template argument deduction for the vector element type.
    result.records.emplace_back(std::vector { UDQSet::EnumeratedItems::serializationTestObject() }, 2.71828, 42);

    return result;
}

void UDQAssign::add_record(const VString&    input_selector,
                           const double      value,
                           const std::size_t report_step)
{
    this->records.emplace_back(input_selector, value, report_step);
}

void UDQAssign::add_record(const VEnumItems& selector,
                           const double      value,
                           const std::size_t report_step)
{
    this->records.emplace_back(selector, value, report_step);
}

void UDQAssign::add_record(VEnumItems&&      selector,
                           const double      value,
                           const std::size_t report_step)
{
    this->records.emplace_back(std::move(selector), value, report_step);
}

void UDQAssign::add_record(const RestartIO::RstUDQ& assignRst,
                           const std::size_t        report_step)
{
    if (assignRst.name != this->m_keyword) {
        throw std::invalid_argument {
            fmt::format("ASSIGN UDQ '{}' must not attempt to include "
                        "information for unrelated UDQ '{}' from restart file.",
                        this->m_keyword, assignRst.name)
        };
    }

    switch (assignRst.category) {
    case UDQVarType::SCALAR:
    case UDQVarType::FIELD_VAR:
        this->add_record(assignRst.entityNames(),
                         assignRst.scalarValue(), report_step);
        break;

    case UDQVarType::WELL_VAR:
    case UDQVarType::GROUP_VAR:
        this->add_well_or_group_records(assignRst, report_step);
        break;

    case UDQVarType::SEGMENT_VAR:
        this->add_segment_records(assignRst, report_step);
        break;

    default:
        break;
    }
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

UDQSet UDQAssign::eval(const VString& wgnames) const
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

UDQSet UDQAssign::eval(const VEnumItems& items) const
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

UDQSet UDQAssign::eval(const VString& wgNames, WGNameMatcher matcher) const
{
    if ((this->m_var_type != UDQVarType::WELL_VAR) &&
        (this->m_var_type != UDQVarType::GROUP_VAR))
    {
        throw std::invalid_argument {
            fmt::format("ASSIGN UDQ '{}': eval(vector<string>) not "
                        "yet implemented for variables of type {}",
                        this->m_keyword, UDQ::typeName(this->m_var_type))
        };
    }

    auto us = UDQSet { this->m_keyword, this->m_var_type, wgNames };

    for (const auto& record : this->records) {
        record.eval(matcher, us);
    }

    return us;
}

UDQSet UDQAssign::eval(const VEnumItems& items, ItemMatcher matcher) const
{
    if (this->m_var_type != UDQVarType::SEGMENT_VAR) {
        throw std::invalid_argument {
            fmt::format("ASSIGN UDQ '{}': eval(vector<items>) not "
                        "yet implemented for variables of type {}",
                        this->m_keyword, UDQ::typeName(this->m_var_type))
        };
    }

    auto us = UDQSet { this->m_keyword, this->m_var_type, items };

    for (const auto& record : this->records) {
        record.eval(matcher, us);
    }

    return us;
}

bool UDQAssign::operator==(const UDQAssign& data) const
{
    return (this->keyword() == data.keyword())
        && (this->var_type() == data.var_type())
        && (this->records == data.records);
}

// ---------------------------------------------------------------------------
// Private member functions below separator
// ---------------------------------------------------------------------------

void UDQAssign::add_well_or_group_records(const RestartIO::RstUDQ& assignRst,
                                          const std::size_t        report_step)
{
    const auto& wgnames = assignRst.entityNames();
    const auto& nameIdx = assignRst.nameIndex();
    const auto n = assignRst.numEntities();

    // Note: We intentionally allocate a single selector string and reuse
    // that for every add_record() call.  The loop here guarantees that we
    // handle the case of different values for well or group, albeit at the
    // cost of the 'records' data member being larger than necessary if all
    // entities do have the same value.
    auto selector = std::vector<std::string>(1);

    for (auto i = 0*n; i < n; ++i) {
        for (const auto& subValuePair : assignRst[i]) {
            selector.front() = wgnames[nameIdx[i]];
            this->add_record(selector, subValuePair.second, report_step);
        }
    }
}

void UDQAssign::add_segment_records(const RestartIO::RstUDQ& assignRst,
                                    const std::size_t        report_step)
{
    const auto& wgnames = assignRst.entityNames();
    const auto& nameIdx = assignRst.nameIndex();
    const auto n = assignRst.numEntities();

    // Note: We intentionally allocate a single EnumeratedItems structure
    // and reuse that for every add_record() call.  The loop here guarantees
    // that we handle the case of different values for every segment for
    // every MS well, albeit at the cost of the 'records' data member being
    // larger than necessary if all entities do have the same value.
    auto selector = std::vector<UDQSet::EnumeratedItems>(1);
    auto& items = selector.front();
    items.numbers.assign(1, std::size_t{0});

    for (auto i = 0*n; i < n; ++i) {
        items.name = wgnames[nameIdx[i]];

        for (const auto& [segIx, value] : assignRst[i]) {
            // +1 since segIx is a zero-based segment number.
            items.numbers.front() = segIx + 1;
            this->add_record(selector, value, report_step);
        }
    }
}

} // namespace Opm
