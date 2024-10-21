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
#include <fmt/format.h>

#include <ostream>
#include <unordered_set>

#include <opm/input/eclipse/Deck/UDAValue.hpp>

namespace {

bool is_udq_blacklist(const std::string& keyword)
{
    static const auto udq_blacklistkw = std::unordered_set<std::string> {
        "SUMTHIN", "SUMMARY", "RUNSUM",
    };

    return udq_blacklistkw.find(keyword) != udq_blacklistkw.end();
}

bool is_udq(const std::string& keyword)
{
    // Does 'keyword' match one of the patterns
    //   AU*, BU*, CU*, FU*, GU*, RU*, SU*, or WU*?
    using sz_t = std::string::size_type;

    return (keyword.size() > sz_t{1})
        && (keyword[1] == 'U')
        && ! is_udq_blacklist(keyword)
        && (keyword.find_first_of("WGFCRBSA") == sz_t{0});
}

} // Anonymous namespace

namespace Opm {

UDAValue::UDAValue(double value):
    double_value(value)
{
}

UDAValue::UDAValue(double value, const Dimension& dim_):
    double_value(value),
    dim(dim_)
{
}

UDAValue::UDAValue(const Dimension& dim_):
    UDAValue(0, dim_)
{
}

UDAValue::UDAValue() :
    UDAValue(0)
{}

UDAValue::UDAValue(const std::string& value):
    string_value(value)
{
    if (! is_udq(value)) {
        std::string msg = fmt::format("Input error: Cannot create UDA value from string '{}' - neither float nor a valid UDQ name.", value);
        throw std::invalid_argument(msg);
    }
}

UDAValue::UDAValue(const std::string& value, const Dimension& dim_):
    string_value(value),
    dim(dim_)
{
    if (! is_udq(value)) {
        std::string msg = fmt::format("Input error: Cannot create UDA value from string '{}' - neither float nor a valid UDQ name.", value);
        throw std::invalid_argument(msg);
    }
}

UDAValue UDAValue::serializationTestObject()
{
    UDAValue result;
    result.double_value = 1.0;
    result.string_value = "test";
    result.dim = Dimension::serializationTestObject();

    return result;
}


void UDAValue::assert_numeric() const {
    if (!this->is_numeric()) {
        std::string msg = fmt::format("Internal error: The support for use of UDQ/UDA is not complete in opm/flow. The string: '{}' must be numeric", this->string_value);
        this->assert_numeric(msg);
    }
}


void UDAValue::assert_numeric(const std::string& error_msg) const {
    if (this->is_numeric())
        return;

    throw std::invalid_argument(error_msg);
}
double UDAValue::epsilonLimit() const {
        return 1.E-20;
}

template<>
bool UDAValue::is<double>() const {
    return this->is_numeric();
}


template<>
bool UDAValue::is<std::string>() const {
  return !this->is_numeric();
}


template<>
double UDAValue::get() const {
    this->assert_numeric();
    return *this->double_value;
}


double UDAValue::getSI() const {
    this->assert_numeric();
    return this->dim.convertRawToSi(*this->double_value);
}


void UDAValue::update(double value) {
    this->double_value = value;
}

void UDAValue::update(const std::string& value) {
    this->double_value = std::nullopt;
    this->string_value = value;
}



template<>
std::string UDAValue::get() const {
    if (!this->is_numeric())
        return this->string_value;

    throw std::invalid_argument("UDAValue does not hold a string value");
}

bool UDAValue::zero() const {
    this->assert_numeric();
    return (this->double_value == 0.0);
}

const Dimension& UDAValue::get_dim() const {
    return this->dim;
}

void UDAValue::set_dim(const Dimension& new_dim) {
    this->dim = new_dim;
}

bool UDAValue::operator==(const UDAValue& other) const {
    if (this->double_value != other.double_value)
        return false;

    if (this->dim != other.dim)
        return false;

    return this->string_value == other.string_value;
}


bool UDAValue::operator!=(const UDAValue& other) const {
    return !(*this == other);
}

std::ostream& operator<<( std::ostream& stream, const UDAValue& uda_value ) {
    if (uda_value.is<double>())
        stream << uda_value.get<double>();
    else
        stream << "'" << uda_value.get<std::string>() << "'";
    return stream;
}

void UDAValue::update_value(const UDAValue& other) {
    if (other.is<double>()) {
        this->double_value = other.get<double>();
    } else {
        this->string_value = other.get<std::string>();
        this->double_value = std::nullopt;
    }
}

void UDAValue::operator*=(double factor) {
    if (this->is<double>())
        (*this->double_value) *=factor;
    else
        throw std::logic_error(fmt::format("Can not multiply UDA: {} with numeric value", this->get<std::string>()));
}


}
