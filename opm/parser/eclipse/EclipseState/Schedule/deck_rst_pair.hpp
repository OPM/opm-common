/*
  Copyright 2021 EquinorASA.

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
#ifndef DECK_RST_PAIR
#define DECK_RST_PAIR


#include <variant>

namespace Opm {


/*
  This is small template class which is designed to wrap a variable of type T,
  and keep track of whether the assigned value came from the input deck, or from
  a restart file.
*/


template <typename T>
class deck_rst_pair {
public:
    struct rst {
        rst(T v) : value(v) {};

        T value;

        bool operator==(const rst& other) const { return this->value == other.value; }
    };

    struct deck {
        deck (T v) : value(v) {};

        T value;

        bool operator==(const deck& other) const { return this->value == other.value; }
    };


    deck_rst_pair& operator=(const T& illegal_value) = delete;

    deck_rst_pair& operator=(const rst& rst_value) {
        this->value = rst_value;
        return *this;
    }

    deck_rst_pair& operator=(const deck& deck_value) {
        this->value = deck_value;
        return *this;
    }

    bool holds_deck() const {
        return std::holds_alternative<deck>(this->value);
    }

    bool holds_rst() const {
        return std::holds_alternative<rst>(this->value);
    }

    bool empty() const {
        return std::holds_alternative<std::monostate>(this->value);
    }

    template <typename C>
    const T& get() const {
        if (std::holds_alternative<C>( this->value ))
            return std::get<C>(this->value).value;
        throw std::logic_error("Wrong type in deck_rst_pair<T>");
    }


    const T& get() const {
        if (std::holds_alternative<deck>( this->value ))
            return std::get<deck>(this->value).value;
        else if (std::holds_alternative<rst>( this->value ))
            return std::get<rst>(this->value).value;
        else
            throw std::logic_error("Trying to get value from uninitilized deck_rst_pair");
    }


    bool operator==(const deck_rst_pair<T>& other) const {
        return this->value == other.value;
    }

private:
    std::variant<std::monostate, deck, rst> value;
};



}

#endif
