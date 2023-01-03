/*
  Copyright 2020 Equinor ASA.

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

#ifndef OPM_UTILITY_STRING_HPP
#define OPM_UTILITY_STRING_HPP

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace Opm {

template<typename T, typename U>
U& uppercase( const T& src, U& dst );

template< typename T >
typename std::decay< T >::type uppercase( T&& x ) {
    typename std::decay< T >::type t( std::forward< T >( x ) );
    return uppercase( t, t );
}

template<typename T>
std::string ltrim_copy(const T& s);

template<typename T>
std::string rtrim_copy(const T& s);

template<typename T>
std::string trim_copy(const T& s);

template<typename T>
void replaceAll(T& data, const T& toSearch, const T& replace);

std::vector<std::string> split_string(const std::string& input,
                                      char delimiter);

std::vector<std::string> split_string(const std::string& input,
                                      const std::string& delimiters);

std::string format_double(double d);

std::optional<double> try_parse_double(const std::string& token);

}
#endif //OPM_UTILITY_STRING_HPP
