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

#include <opm/parser/eclipse/Deck/UDAValue.hpp>


namespace Opm {

UDAValue::UDAValue(double value):
    numeric_value(true),
    double_value(value)
{
}


UDAValue::UDAValue() :
    UDAValue(0)
{}

UDAValue::UDAValue(const std::string& value):
    numeric_value(false),
    string_value(value)
{
}


template<>
bool UDAValue::is<double>() const {
    return this->numeric_value;
}


template<>
bool UDAValue::is<std::string>() const {
  return !this->numeric_value;
}


template<>
double UDAValue::get() const {
    if (this->numeric_value)
        return this->double_value;

    throw std::invalid_argument("UDAValue does not hold a numerical value");
}

template<>
std::string UDAValue::get() const {
    if (!this->numeric_value)
        return this->string_value;

    throw std::invalid_argument("UDAValue does not hold a string value");
}


void UDAValue::set_dim(const Dimension& dim) const {
    this->dim = dim;
}

const Dimension& UDAValue::get_dim() const {
    return this->dim;
}


bool UDAValue::operator==(const UDAValue& other) const {
    if (this->numeric_value != other.numeric_value)
        return false;

    if (this->numeric_value)
        return (this->double_value == other.double_value);

    return this->string_value == other.string_value;
}


bool UDAValue::operator!=(const UDAValue& other) const {
    return !(*this == other);
}

}
