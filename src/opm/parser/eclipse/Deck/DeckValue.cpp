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

#include <stdexcept>

#include <opm/parser/eclipse/Deck/DeckValue.hpp>

namespace Opm {

DeckValue::DeckValue():
    DeckValue(0)
{}

DeckValue::DeckValue(int value):
    is_int(true),
    is_double(false),
    int_value(value)
{}

DeckValue::DeckValue(double value):
    is_int(false),
    is_double(true),
    double_value(value)
{}

DeckValue::DeckValue(const std::string& value):
    is_int(false),
    is_double(false),
    string_value(value)
{}

template<>
int DeckValue::get() const {
    if (!is_int)
        throw std::invalid_argument("DeckValue does not hold an integer value");

    return this->int_value;
}

template<>
double DeckValue::get() const {
    if (!is_double)
        throw std::invalid_argument("DeckValue does not hold a double value");

    return this->double_value;
}

template<>
std::string DeckValue::get() const {
    if (!is_numeric())
        return this->string_value;

    throw std::invalid_argument("DeckValue does not hold a string value");
}

template<>
bool DeckValue::is<int>() const {
    return is_int;
}

template<>
bool DeckValue::is<double>() const {
    return is_double;
}


template<>
bool DeckValue::is<std::string>() const {
    return !is_numeric();
}

bool DeckValue::is_numeric() const {
    return (is_int or is_double);
}

void DeckValue::reset(int value) {
  this->is_int = true;
  this->is_double = false;
  this->int_value = value;
}

void DeckValue::reset(double value) {
  this->is_int = false;
  this->is_double = true;
  this->double_value = value;
}

void DeckValue::reset(const std::string& value) {
  this->is_int = false;
  this->is_double = false;
  this->string_value = value;
}

}

