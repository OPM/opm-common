/*
  Copyright 2019 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <opm/common/utility/shmatch.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm {

UDQScalar::UDQScalar(const double value, const std::size_t num)
    : m_num(num)
{
    this->assign(value);
}

UDQScalar::UDQScalar(const std::string& wgname, const std::size_t num)
    : m_wgname(wgname)
    , m_num   (num)
{}

bool UDQScalar::defined() const
{
    return this->m_value.has_value();
}

double UDQScalar::get() const
{
    if (!this->defined()) {
        throw std::invalid_argument {
            fmt::format("UDQSCalar: Value not defined wgname = {}, num = {}",
                        this->m_wgname, this->m_num)
        };
    }

    return *this->m_value;
}

void UDQScalar::assign(const std::optional<double>& value)
{
    if (value.has_value()) {
        this->assign(*value);
    }
    else {
        this->m_value = std::nullopt;
    }
}

void UDQScalar::assign(const double value)
{
    if (std::isfinite(value)) {
        this->m_value = value;
    }
    else {
        this->m_value = std::nullopt;
    }
}

void UDQScalar::operator-=(const UDQScalar& rhs) {
    if (this->defined() && rhs.defined())
        this->assign(*this->m_value - *rhs.m_value);
    else
        this->m_value = std::nullopt;
}

void UDQScalar::operator-=(double rhs) {
    if (this->defined())
        this->assign(*this->m_value - rhs);
}

void UDQScalar::operator/=(const UDQScalar& rhs) {
    if (this->defined() && rhs.defined())
        this->assign(*this->m_value / *rhs.m_value);
    else
        this->m_value = std::nullopt;
}

void UDQScalar::operator/=(double rhs) {
    if (this->defined())
        this->assign(*this->m_value / rhs);
}

void UDQScalar::operator+=(const UDQScalar& rhs) {
    if (this->defined() && rhs.defined())
        this->assign(*this->m_value + *rhs.m_value);
    else
        this->m_value = std::nullopt;
}

void UDQScalar::operator+=(double rhs) {
    if (this->defined())
        this->assign(*this->m_value + rhs);
}

void UDQScalar::operator*=(const UDQScalar& rhs) {
    if (this->defined() && rhs.defined())
        this->assign(*this->m_value * *rhs.m_value);
    else
        this->m_value = std::nullopt;
}

void UDQScalar::operator*=(double rhs) {
    if (this->defined())
        this->assign(*this->m_value * rhs);
}

UDQScalar::operator bool() const
{
    return this->defined();
}

bool UDQScalar::operator==(const UDQScalar& other) const
{
    return (this->m_value == other.m_value)
        && (this->m_wgname == other.m_wgname)
        && (this->m_num == other.m_num);
}

// ------------------------------------------------------------------------
// UDQSet Implementation Below Separator
// ------------------------------------------------------------------------

bool UDQSet::EnumeratedWellItems::operator==(const EnumeratedWellItems& that) const
{
    return (this->well == that.well)
        && (this->numbers == that.numbers);
}

UDQSet::EnumeratedWellItems
UDQSet::EnumeratedWellItems::serializationTestObject()
{
    return { "PROD01", std::vector<std::size_t>{ 17, 29 } };
}

const std::string& UDQSet::name() const
{
    return this->m_name;
}

void UDQSet::name(const std::string& name) {
    this->m_name = name;
}

UDQSet::UDQSet(const std::string& name,
               const UDQVarType   var_type)
    : m_name    (name)
    , m_var_type(var_type)
{
    this->values.resize(1);
}

UDQSet::UDQSet(const std::string&              name,
               const UDQVarType                var_type,
               const std::vector<std::string>& wgnames)
    : m_name    (name)
    , m_var_type(var_type)
{
    for (const auto& wgname : wgnames)
        this->values.emplace_back(wgname);
}

UDQSet::UDQSet(const std::string&                      name,
               const UDQVarType                        var_type,
               const std::vector<EnumeratedWellItems>& items)
    : m_name    (name)
    , m_var_type(var_type)
{
    for (const auto& item : items) {
        for (const auto& number : item.numbers) {
            this->values.emplace_back(item.well, number);
        }
    }
}

UDQSet::UDQSet(const std::string& name,
               const UDQVarType   var_type,
               const std::size_t  size)
    : m_name    (name)
    , m_var_type(var_type)
{
    this->values.resize(size);
}

UDQSet::UDQSet(const std::string& name, const std::size_t size)
    : m_name(name)
{
    this->values.resize(size);
}

UDQSet UDQSet::scalar(const std::string& name, const double scalar_value)
{
    UDQSet us(name, UDQVarType::SCALAR);
    us.assign(scalar_value);
    return us;
}

UDQSet UDQSet::scalar(const std::string&           name,
                      const std::optional<double>& scalar_value)
{
    UDQSet us(name, UDQVarType::SCALAR);
    us.assign(scalar_value);
    return us;
}

UDQSet UDQSet::empty(const std::string& name)
{
    return { name, std::size_t{0} };
}

UDQSet UDQSet::field(const std::string& name, double scalar_value)
{
    UDQSet us(name, UDQVarType::FIELD_VAR);
    us.assign(scalar_value);
    return us;
}

UDQSet UDQSet::wells(const std::string&              name,
                     const std::vector<std::string>& wells)
{
    return { name, UDQVarType::WELL_VAR, wells };
}

UDQSet UDQSet::wells(const std::string&              name,
                     const std::vector<std::string>& wells,
                     const double                    scalar_value)
{
    UDQSet us = UDQSet::wells(name, wells);
    us.assign(scalar_value);
    return us;
}

UDQSet UDQSet::groups(const std::string&              name,
                      const std::vector<std::string>& groups)
{
    return { name, UDQVarType::GROUP_VAR, groups };
}

UDQSet UDQSet::groups(const std::string&              name,
                      const std::vector<std::string>& groups,
                      const double                    scalar_value)
{
    UDQSet us = UDQSet::groups(name, groups);
    us.assign(scalar_value);
    return us;
}


bool UDQSet::has(const std::string& name) const
{
    return std::any_of(this->values.begin(), this->values.end(),
                       [&name](const UDQScalar& value)
                       {
                           return value.wgname() == name;
                       });
}

std::size_t UDQSet::size() const
{
    return this->values.size();
}

void UDQSet::assign(const std::string& wgname, const double value)
{
    bool assigned = false;
    for (auto& udq_value : this->values) {
        if (shmatch(wgname, udq_value.wgname())) {
            udq_value.assign(value);
            assigned = true;
        }
    }

    if (! assigned) {
        throw std::out_of_range {
            fmt::format("No well/group matching: {}", wgname)
        };
    }
}

void UDQSet::assign(const std::size_t index, const std::optional<double>& value)
{
    this->values[index].assign(value);
}

void UDQSet::assign(const std::string&           wgname,
                    const std::optional<double>& value)
{
    bool assigned = false;
    for (auto& udq_value : this->values) {
        if (shmatch(wgname, udq_value.wgname())) {
            udq_value.assign(value);
            assigned = true;
        }
    }

    if (! assigned) {
        throw std::out_of_range {
            fmt::format("No well/group matching: {}", wgname)
        };
    }
}

void UDQSet::assign(const std::string&           wgname,
                    const std::size_t            number,
                    const std::optional<double>& value)
{
    auto assigned = false;

    for (auto& udq : this->values) {
        if ((udq.number() == number) && shmatch(wgname, udq.wgname())) {
            udq.assign(value);
            assigned = true;
        }
    }

    if (! assigned) {
        throw std::out_of_range {
            fmt::format("No segment {} in well matching '{}'", number, wgname)
        };
    }
}

void UDQSet::assign(double value)
{
    for (auto& v : this->values)
        v.assign(value);
}

void UDQSet::assign(const std::optional<double>& value)
{
    for (auto& v : this->values)
        v.assign(value);
}

void UDQSet::assign(std::size_t index, const double value)
{
    this->values[index].assign(value);
}

UDQVarType UDQSet::var_type() const
{
    return this->m_var_type;
}

std::vector<std::string> UDQSet::wgnames() const
{
    auto names = std::vector<std::string> {};
    names.reserve(this->values.size());

    std::transform(this->values.begin(), this->values.end(), std::back_inserter(names),
                   [](const UDQScalar& value) { return value.wgname(); });

    return names;
}

// ------------------------------------------------------------------------

void UDQSet::operator+=(const UDQSet& rhs)
{
    if (this->size() != rhs.size())
        throw std::logic_error("Incompatible size in UDQSet operator+");

    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] += rhs[index];
}

void UDQSet::operator+=(double rhs) {
    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] += rhs;
}

void UDQSet::operator-=(double rhs) {
    *(this) += (-rhs);
}

void UDQSet::operator-=(const UDQSet& rhs) {
    *(this) += (rhs * -1.0);
}

void UDQSet::operator*=(const UDQSet& rhs)
{
    if (this->size() != rhs.size()) {
        throw std::logic_error("Incompatible size  UDQSet operator*");
    }

    for (std::size_t index = 0; index < this->size(); ++index) {
        this->values[index] *= rhs[index];
    }
}

void UDQSet::operator*=(double rhs)
{
    for (std::size_t index = 0; index < this->size(); ++index) {
        this->values[index] *= rhs;
    }
}

void UDQSet::operator/=(const UDQSet& rhs)
{
    if (this->size() != rhs.size()) {
        throw std::logic_error("Incompatible size  UDQSet operator/");
    }

    for (std::size_t index = 0; index < this->size(); ++index) {
        this->values[index] /= rhs[index];
    }
}

void UDQSet::operator/=(double rhs)
{
    for (std::size_t index = 0; index < this->size(); ++index) {
        this->values[index] /= rhs;
    }
}

std::vector<double> UDQSet::defined_values() const
{
    std::vector<double> dv;

    for (const auto& v : this->values) {
        if (v) {
            dv.push_back(v.get());
        }
    }

    return dv;
}

std::size_t UDQSet::defined_size() const
{
    return std::count_if(this->values.begin(), this->values.end(),
                         [](const UDQScalar& value)
                         {
                             return value.defined();
                         });
}

const UDQScalar& UDQSet::operator[](std::size_t index) const
{
    if (index >= this->size()) {
        throw std::out_of_range("Index out of range in UDQset::operator[]");
    }

    return this->values[index];
}

const UDQScalar& UDQSet::operator[](const std::string& wgname) const
{
    auto value_iter = std::find_if(this->values.begin(), this->values.end(),
                                   [&wgname](const UDQScalar& value)
                                   {
                                       return value.wgname() == wgname;
                                   });

    if (value_iter == this->values.end()) {
        throw std::out_of_range("No such well/group: " + wgname);
    }

    return *value_iter;
}

const UDQScalar&
UDQSet::operator()(const std::string& well,
                   const std::size_t  item) const
{
    auto value_iter = std::find_if(this->values.begin(), this->values.end(),
                                   [&well, item](const UDQScalar& value)
                                   {
                                       return (value.number() == item)
                                           && (value.wgname() == well);
                                   });

    if (value_iter == this->values.end()) {
        throw std::out_of_range {
            fmt::format("No such well/item: {}/{}", well, item)
        };
    }

    return *value_iter;
}

std::vector<UDQScalar>::const_iterator UDQSet::begin() const
{
    return this->values.begin();
}

std::vector<UDQScalar>::const_iterator UDQSet::end() const
{
    return this->values.end();
}

// ----------------------------------------------------------------

UDQScalar operator+(const UDQScalar& lhs, const UDQScalar& rhs)
{
    UDQScalar sum = lhs;
    sum += rhs;
    return sum;
}

UDQScalar operator+(const UDQScalar& lhs, double rhs)
{
    UDQScalar sum = lhs;
    sum += rhs;
    return sum;
}

UDQScalar operator+(double lhs, const UDQScalar& rhs)
{
    UDQScalar sum = rhs;
    sum += lhs;
    return sum;
}

UDQScalar operator-(const UDQScalar& lhs, const UDQScalar& rhs)
{
    UDQScalar sum = lhs;
    sum -= rhs;
    return sum;
}

UDQScalar operator-(const UDQScalar& lhs, double rhs)
{
    UDQScalar sum = lhs;
    sum -= rhs;
    return sum;
}

UDQScalar operator-(double lhs, const UDQScalar& rhs)
{
    UDQScalar sum = rhs;
    sum -= lhs;
    return sum;
}

UDQScalar operator*(const UDQScalar& lhs, const UDQScalar& rhs)
{
    UDQScalar sum = lhs;
    sum *= rhs;
    return sum;
}

UDQScalar operator*(const UDQScalar& lhs, double rhs)
{
    UDQScalar sum = lhs;
    sum *= rhs;
    return sum;
}

UDQScalar operator*(double lhs, const UDQScalar& rhs)
{
    UDQScalar sum = rhs;
    sum *= lhs;
    return sum;
}

UDQScalar operator/(const UDQScalar& lhs, const UDQScalar& rhs)
{
    UDQScalar sum = lhs;
    sum /= rhs;
    return sum;
}

UDQScalar operator/(const UDQScalar& lhs, double rhs)
{
    UDQScalar sum = lhs;
    sum /= rhs;
    return sum;
}

UDQScalar operator/(double lhs, const UDQScalar& rhs)
{
    UDQScalar result = rhs;
    if (result) {
        result.assign(lhs / result.get());
    }

    return result;
}

// -----------------------------------------------------------------

namespace {

bool is_scalar(const UDQSet& udq_set)
{
    const auto vtype = udq_set.var_type();

    return (vtype == UDQVarType::SCALAR)
        || (vtype == UDQVarType::FIELD_VAR);
}

// If one result set is scalar and the other represents a set of
// wells/groups, the scalar result is promoted to a set of the right type.
//
// This function is quite subconscious about FIELD / SCALAR.
std::pair<UDQSet, UDQSet> udq_cast(const UDQSet& lhs, const UDQSet& rhs)
{
    if ((lhs.var_type() == rhs.var_type()) ||
        (is_scalar(lhs) && is_scalar(rhs)))
    {
        return { lhs, rhs };
    }

    if (is_scalar(lhs)) {
        if (rhs.var_type() == UDQVarType::WELL_VAR) {
            return { UDQSet::wells(lhs.name(), rhs.wgnames(), lhs[0].get()), rhs };
        }

        if (rhs.var_type() == UDQVarType::GROUP_VAR) {
            return { UDQSet::groups(lhs.name(), rhs.wgnames(), lhs[0].get()), rhs };
        }
    }

    if (is_scalar(rhs)) {
        if (lhs.var_type() == UDQVarType::WELL_VAR) {
            return { lhs, UDQSet::wells(rhs.name(), lhs.wgnames(), rhs[0].get()) };
        }

        if (lhs.var_type() == UDQVarType::GROUP_VAR) {
            return { lhs, UDQSet::groups(rhs.name(), lhs.wgnames(), rhs[0].get()) };
        }
    }

    throw std::logic_error {
        fmt::format("Type/size mismatch when combining UDQs "
                    "{}(size={}, type={}) and "
                    "{}(size={}, type={})",
                    lhs.name(), lhs.size(), UDQ::typeName(lhs.var_type()),
                    rhs.name(), rhs.size(), UDQ::typeName(rhs.var_type()))
    };
}

} // Anonymous namespace

UDQSet operator+(const UDQSet& lhs, const UDQSet& rhs)
{
    auto [left,right] = udq_cast(lhs, rhs);
    left += right;
    return left;
}

UDQSet operator+(const UDQSet& lhs, double rhs)
{
    UDQSet sum = lhs;
    sum += rhs;
    return sum;
}

UDQSet operator+(double lhs, const UDQSet& rhs)
{
    UDQSet sum = rhs;
    sum += lhs;
    return sum;
}

UDQSet operator-(const UDQSet& lhs, const UDQSet& rhs)
{
    auto [left,right] = udq_cast(lhs, rhs);
    left -= right;
    return left;
}

UDQSet operator-(const UDQSet& lhs, double rhs)
{
    UDQSet sum = lhs;
    sum -= rhs;
    return sum;
}

UDQSet operator-(double lhs, const UDQSet& rhs)
{
    UDQSet sum = rhs;
    sum -= lhs;
    return sum;
}

UDQSet operator*(const UDQSet& lhs, const UDQSet& rhs)
{
    auto [left,right] = udq_cast(lhs, rhs);
    left *= right;
    return left;
}

UDQSet operator*(const UDQSet& lhs, double rhs)
{
    UDQSet prod = lhs;
    prod *= rhs;
    return prod;
}

UDQSet operator*(double lhs, const UDQSet& rhs)
{
    UDQSet sum = rhs;
    sum *= lhs;
    return sum;
}

UDQSet operator/(const UDQSet& lhs, const UDQSet& rhs)
{
    auto [left, right] = udq_cast(lhs, rhs);
    left /= right;
    return left;
}

UDQSet operator/(const UDQSet& lhs, double rhs)
{
    UDQSet frac = lhs;
    frac /= rhs;
    return frac;
}

UDQSet operator/(double lhs, const UDQSet& rhs)
{
    UDQSet result = rhs;

    for (std::size_t index = 0; index < rhs.size(); ++index) {
        const auto& elm = rhs[index];
        if (elm) {
            result.assign(index, lhs / elm.get());
        }
    }

    return result;
}

bool UDQSet::operator==(const UDQSet& other) const
{
    return (this->m_name == other.m_name)
        && (this->m_var_type == other.m_var_type)
        && (this->values == other.values);
}

} // namespace Opm
