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
#include <fnmatch.h>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQSet.hpp>

namespace Opm {

UDQScalar::UDQScalar(double value) :
    m_value(value),
    m_defined(true)
{}

bool UDQScalar::defined() const {
    return this->m_defined;
}

double UDQScalar::value() const {
    if (!this->m_defined)
        throw std::invalid_argument("UDQSCalar: Value not defined");

    return this->m_value;
}

void UDQScalar::assign(double value) {
    this->m_value = value;
    this->m_defined = true;
}

void UDQScalar::operator-=(const UDQScalar& rhs) {
    if (this->m_defined && rhs.m_defined)
        this->m_value -= rhs.m_value;
    else
        this->m_defined = false;
}

void UDQScalar::operator-=(double rhs) {
    if (this->m_defined)
        this->m_value -= rhs;
}

void UDQScalar::operator/=(const UDQScalar& rhs) {
    if (this->m_defined && rhs.m_defined)
        this->m_value /= rhs.m_value;
    else
        this->m_defined = false;
}

void UDQScalar::operator/=(double rhs) {
    if (this->m_defined)
        this->m_value /= rhs;
}

void UDQScalar::operator+=(const UDQScalar& rhs) {
    if (this->m_defined && rhs.m_defined)
        this->m_value += rhs.m_value;
    else
        this->m_defined = false;
}

void UDQScalar::operator+=(double rhs) {
    if (this->m_defined)
        this->m_value += rhs;
}

void UDQScalar::operator*=(const UDQScalar& rhs) {
    if (this->m_defined && rhs.m_defined)
        this->m_value *= rhs.m_value;
    else
        this->m_defined = false;
}

void UDQScalar::operator*=(double rhs) {
    if (this->m_defined)
        this->m_value *= rhs;
}


UDQScalar::operator bool() const {
    return this->m_defined;
}


const std::string& UDQSet::name() const {
    return this->m_name;
}

UDQSet::UDQSet(const std::string& name, UDQVarType var_type, std::size_t size) :
    m_name(name),
    m_var_type(var_type)
{
    this->values.resize(size);
}

UDQSet::UDQSet(const std::string& name, std::size_t size) :
    m_name(name)
{
    this->values.resize(size);
}

UDQSet UDQSet::scalar(const std::string& name, double scalar_value)
{
    UDQSet us(name, UDQVarType::SCALAR, 1);
    us.assign(scalar_value);
    return us;
}

UDQSet UDQSet::empty(const std::string& name)
{
    return UDQSet(name, 0);
}


UDQSet UDQSet::field(const std::string& name, double scalar_value)
{
    UDQSet us(name, UDQVarType::FIELD_VAR, 1);
    us.assign(scalar_value);
    return us;
}


UDQSet UDQSet::wells(const std::string& name, const std::vector<std::string>& wells) {
    UDQSet us(name, UDQVarType::WELL_VAR, wells.size());

    std::size_t index = 0;
    for (const auto& well : wells) {
        us.wgname_index[well] = index;
        index += 1;
    }

    return us;
}

UDQSet UDQSet::wells(const std::string& name, const std::vector<std::string>& wells, double scalar_value) {
    UDQSet us = UDQSet::wells(name, wells);
    us.assign(scalar_value);
    return us;
}


UDQSet UDQSet::groups(const std::string& name, const std::vector<std::string>& groups) {
    UDQSet us(name, UDQVarType::GROUP_VAR, groups.size());

    std::size_t index = 0;
    for (const auto& group : groups) {
        us.wgname_index[group] = index;
        index += 1;
    }

    return us;
}

UDQSet UDQSet::groups(const std::string& name, const std::vector<std::string>& groups, double scalar_value) {
    UDQSet us = UDQSet::groups(name, groups);
    us.assign(scalar_value);
    return us;
}



std::size_t UDQSet::size() const {
    return this->values.size();
}


void UDQSet::assign(const std::string& wgname, double value) {
    if (wgname.find('*') == std::string::npos) {
        std::size_t index = this->wgname_index.at(wgname);
        UDQSet::assign(index, value);
    } else {
        int flags = 0;
        for (const auto& pair : this->wgname_index) {
            if (fnmatch(wgname.c_str(), pair.first.c_str(), flags) == 0)
                UDQSet::assign(pair.second, value);
        }
    }
}

void UDQSet::assign(double value) {
    for (auto& v : this->values)
        v.assign(value);
}

void UDQSet::assign(std::size_t index, double value) {
    auto& scalar = this->values[index];
    scalar.assign(value);
}


UDQVarType UDQSet::var_type() const {
    return this->m_var_type;
}

std::vector<std::string> UDQSet::wgnames() const {
    std::vector<std::string> names;
    for (const auto& index_pair : this->wgname_index)
        names.push_back(index_pair.first);
    return names;
}

/************************************************************************/


void UDQSet::operator+=(const UDQSet& rhs) {
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


void UDQSet::operator*=(const UDQSet& rhs) {
    if (this->size() != rhs.size())
        throw std::logic_error("Incompatible size  UDQSet operator*");

    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] *= rhs[index];
}

void UDQSet::operator*=(double rhs) {
    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] *= rhs;
}

void UDQSet::operator/=(const UDQSet& rhs) {
    if (this->size() != rhs.size())
        throw std::logic_error("Incompatible size  UDQSet operator/");

    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] /= rhs[index];
}

void UDQSet::operator/=(double rhs) {
    for (std::size_t index = 0; index < this->size(); index++)
        this->values[index] /= rhs;
}


std::vector<double> UDQSet::defined_values() const {
    std::vector<double> dv;
    for (const auto& v : this->values) {
        if (v)
            dv.push_back(v.value());
    }
    return dv;
}


std::size_t UDQSet::defined_size() const {
    std::size_t dsize = 0;
    for (const auto& v : this->values) {
        if (v)
            dsize += 1;
    }
    return dsize;
}

const UDQScalar& UDQSet::operator[](std::size_t index) const {
    if (index >= this->size())
        throw std::out_of_range("Index out of range in UDQset::operator[]");
    return this->values[index];
}

const UDQScalar& UDQSet::operator[](const std::string& well) const {
    std::size_t index = this->wgname_index.at(well);
    return this->operator[](index);
}


std::vector<UDQScalar>::const_iterator UDQSet::begin() const {
    return this->values.begin();
}

std::vector<UDQScalar>::const_iterator UDQSet::end() const {
    return this->values.end();
}

/*****************************************************************/
UDQScalar operator+(const UDQScalar&lhs, const UDQScalar& rhs) {
    UDQScalar sum = lhs;
    sum += rhs;
    return sum;
}

UDQScalar operator+(const UDQScalar&lhs, double rhs) {
    UDQScalar sum = lhs;
    sum += rhs;
    return sum;
}

UDQScalar operator+(double lhs, const UDQScalar& rhs) {
    UDQScalar sum = rhs;
    sum += lhs;
    return sum;
}

UDQScalar operator-(const UDQScalar&lhs, const UDQScalar& rhs) {
    UDQScalar sum = lhs;
    sum -= rhs;
    return sum;
}

UDQScalar operator-(const UDQScalar&lhs, double rhs) {
    UDQScalar sum = lhs;
    sum -= rhs;
    return sum;
}

UDQScalar operator-(double lhs, const UDQScalar& rhs) {
    UDQScalar sum = rhs;
    sum -= lhs;
    return sum;
}

UDQScalar operator*(const UDQScalar&lhs, const UDQScalar& rhs) {
    UDQScalar sum = lhs;
    sum *= rhs;
    return sum;
}

UDQScalar operator*(const UDQScalar&lhs, double rhs) {
    UDQScalar sum = lhs;
    sum *= rhs;
    return sum;
}

UDQScalar operator*(double lhs, const UDQScalar& rhs) {
    UDQScalar sum = rhs;
    sum *= lhs;
    return sum;
}

UDQScalar operator/(const UDQScalar&lhs, const UDQScalar& rhs) {
    UDQScalar sum = lhs;
    sum /= rhs;
    return sum;
}

UDQScalar operator/(const UDQScalar&lhs, double rhs) {
    UDQScalar sum = lhs;
    sum /= rhs;
    return sum;
}


UDQScalar operator/(double lhs, const UDQScalar& rhs) {
    UDQScalar result = rhs;
    if (result)
        result.assign(lhs / result.value());

    return result;
}


UDQSet operator+(const UDQSet&lhs, const UDQSet& rhs) {
    UDQSet sum = lhs;
    sum += rhs;
    return sum;
}

UDQSet operator+(const UDQSet&lhs, double rhs) {
    UDQSet sum = lhs;
    sum += rhs;
    return sum;
}

UDQSet operator+(double lhs, const UDQSet& rhs) {
    UDQSet sum = rhs;
    sum += lhs;
    return sum;
}

UDQSet operator-(const UDQSet&lhs, const UDQSet& rhs) {
    UDQSet sum = lhs;
    sum -= rhs;
    return sum;
}

UDQSet operator-(const UDQSet&lhs, double rhs) {
    UDQSet sum = lhs;
    sum -= rhs;
    return sum;
}

UDQSet operator-(double lhs, const UDQSet& rhs) {
    UDQSet sum = rhs;
    sum -= lhs;
    return sum;
}

UDQSet operator*(const UDQSet&lhs, const UDQSet& rhs) {
    UDQSet sum = lhs;
    sum *= rhs;
    return sum;
}

UDQSet operator*(const UDQSet&lhs, double rhs) {
    UDQSet sum = lhs;
    sum *= rhs;
    return sum;
}

UDQSet operator*(double lhs, const UDQSet& rhs) {
    UDQSet sum = rhs;
    sum *= lhs;
    return sum;
}

UDQSet operator/(const UDQSet&lhs, const UDQSet& rhs) {
    UDQSet sum = lhs;
    sum /= rhs;
    return sum;
}

UDQSet operator/(const UDQSet&lhs, double rhs) {
    UDQSet sum = lhs;
    sum /= rhs;
    return sum;
}

UDQSet operator/(double lhs, const UDQSet&rhs) {
    UDQSet result = rhs;
    for (std::size_t index = 0; index < rhs.size(); index++) {
        const auto& elm = rhs[index];
        if (elm)
            result.assign(index, lhs / elm.value());
    }
    return result;
}

}
