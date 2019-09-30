/*
  Copyright 2019 Equinor ASA.

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

#ifndef DECK_VALUE_HPP
#define DECK_VALUE_HPP

#include <string>

namespace Opm {

class DeckValue {

    public:
        DeckValue();
        explicit DeckValue(int);
        explicit DeckValue(double);
        explicit DeckValue(const std::string&);

        template<typename T>
        T get() const;

        template<typename T>
        bool is() const;

    private:

        enum class DeckValueEnum {INT, DOUBLE, STRING};

        DeckValueEnum value_enum;
        int int_value;
        double double_value;
        std::string string_value;    

};

}


#endif
