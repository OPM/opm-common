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
#include <iostream>

#include <opm/parser/eclipse/Deck/DeckValue.hpp>

namespace Opm {

DeckValue::DeckValue():
    DeckValue(0)
{}

DeckValue::DeckValue(int value):
    value_enum(DECK_VALUE_INT),
    int_value(value)
{}

DeckValue::DeckValue(double value):
    value_enum(DECK_VALUE_DOUBLE),
    double_value(value)
{}

DeckValue::DeckValue(const std::string& value):
    value_enum(DECK_VALUE_STRING),
    string_value(value)
{}

template<>
int DeckValue::get() const {
    if (value_enum == DECK_VALUE_INT)
        return this->int_value;

    throw std::invalid_argument("DeckValue does not hold an integer value");
}

template<>
double DeckValue::get() const {
    if (value_enum == DECK_VALUE_DOUBLE)
        return this->double_value;
        
    throw std::invalid_argument("DeckValue does not hold a double value");
}

template<>
std::string DeckValue::get() const {
    if (value_enum == DECK_VALUE_STRING)
        return this->string_value;

    throw std::invalid_argument("DeckValue does not hold a string value");
}

template<>
bool DeckValue::is<int>() const {
    return (value_enum == DECK_VALUE_INT);
}

template<>
bool DeckValue::is<double>() const {
    return (value_enum == DECK_VALUE_DOUBLE);
}


template<>
bool DeckValue::is<std::string>() const {
    return (value_enum == DECK_VALUE_STRING);
}


}

