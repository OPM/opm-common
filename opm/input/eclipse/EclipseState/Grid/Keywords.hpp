/*
  Copyright 2020 Equinor AS.

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

#ifndef OPM_KEYWORDS_HPP
#define OPM_KEYWORDS_HPP

#include <optional>
#include <string>

namespace Opm {
namespace Fieldprops {
namespace keywords {

template <typename T>
struct keyword_info {
    std::optional<std::string> unit = std::nullopt;
    std::optional<T> scalar_init = std::nullopt;
    bool multiplier = false;
    bool top = false;
    bool global = false;
    // TODO: checking whether mutable can be removed
    std::size_t num_value = 1;


    bool operator==(const keyword_info& other) const {
        return this->unit == other.unit &&
               this->scalar_init == other.scalar_init &&
               this->multiplier == other.multiplier &&
               this->top == other.top &&
               this->global == other.global &&
               this->num_value == other.num_value;
    }

    keyword_info<T>& init(T init_value) {
        this->scalar_init = init_value;
        return *this;
    }

    keyword_info<T>& unit_string(const std::string& unit_string) {
        this->unit = unit_string;
        return *this;
    }

    keyword_info<T>& distribute_top(bool dtop) {
        this->top = dtop;
        return *this;
    }

    keyword_info<T>& mult(bool m) {
        this->multiplier = m;
        return *this;
    }

    keyword_info<T>& global_kw(bool g) {
        this->global = g;
        return *this;
    }

    const keyword_info<T>& num_value_per_cell(const std::size_t n) const {
        this->num_value = n;
        return *this;
    }
};

} // end namespace Keywords
} // end namespace Fieldprops
} //end namespace Opm

#endif //OPM_KEYWORDS_HPP
