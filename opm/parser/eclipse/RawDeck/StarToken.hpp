/*
  Copyright 2013 Statoil ASA.

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

#ifndef STAR_TOKEN_HPP
#define STAR_TOKEN_HPP

#include <string>
#include <iostream>

#include <boost/lexical_cast.hpp>

namespace Opm {
    bool isStarToken(const std::string& token,
                           std::string& countString,
                           std::string& valueString);

    template <class T>
    T readValueToken(const std::string& valueString) {
        try {
            return boost::lexical_cast<T>(valueString);
        }
        catch (boost::bad_lexical_cast&) {
            throw std::invalid_argument("Unable to convert string '" + valueString + "' to typeid: " + typeid(T).name());
        }
    }

    template <>
    inline double readValueToken<double>(const std::string& valueString) {
        // Eclipse supports Fortran syntax for specifying exponents of floating point
        // numbers ('D' and 'E', e.g., 1.234d5) while C++ only supports the 'e' (e.g.,
        // 1.234e5). the quick fix is to replace 'D' by 'E' and 'd' by 'e' before trying
        // to convert the string into a floating point value. This may not be the most
        // performant thing to do, but it is probably fast enough.
        std::string vs(valueString);
        std::replace(vs.begin(), vs.end(), 'D', 'E');
        std::replace(vs.begin(), vs.end(), 'd', 'e');

        try {
            return boost::lexical_cast<double>(vs);
        }
        catch (boost::bad_lexical_cast&) {
            throw std::invalid_argument("Unable to convert string '" + valueString + "' to double");
        }
    }

    template <>
    inline float readValueToken<float>(const std::string& valueString) {
        return readValueToken<double>(valueString);
    }

    template <>
    inline std::string readValueToken<std::string>(const std::string& valueString) {
        if (valueString.size() > 0 && valueString[0] == '\'') {
            if (valueString.size() < 2 || valueString[valueString.size() - 1] != '\'')
                throw std::invalid_argument("Unable to parse string '" + valueString + "' as a string token");
            return valueString.substr(1, valueString.size() - 2);
        }
        else
            return valueString;
    }


class StarToken {
public:
    StarToken(const std::string& token)
    {
        if (!isStarToken(token, m_countString, m_valueString))
            throw std::invalid_argument("Token \""+token+"\" is not a repetition specifier");
        init_(token);
    }

    StarToken(const std::string& token, const std::string& countStr, const std::string& valueStr)
        : m_countString(countStr)
        , m_valueString(valueStr)
    {
        init_(token);
    }

    size_t count() const {
        return m_count;
    }

    bool hasValue() const {
        return !m_valueString.empty();
    }

    // returns the coubt as rendered in the deck. note that this might be different
    // than just converting the return value of count() to a string because an empty
    // count is interpreted as 1...
    const std::string& countString() const {
        return m_countString;
    }

    // returns the value as rendered in the deck. note that this might be different
    // than just converting the return value of value() to a string because values
    // might have different representations in the deck (e.g. strings can be
    // specified with and without quotes and but spaces are only allowed using the
    // first representation.)
    const std::string& valueString() const {
        return m_valueString;
    }

private:
    // internal initialization method. the m_countString and m_valueString attributes
    // must be set before calling this method.
    void init_(const std::string& token) {
        // special-case the interpretation of a lone star as "1*" but do not
        // allow constructs like "*123"...
        if (m_countString == "") {
            if (m_valueString != "")
                // TODO: decorate the deck with a warning instead?
                throw std::invalid_argument("Not specifying a count also implies not specifying a value. Token: \'" + token + "\'.");

            // TODO: since this is explicitly forbidden by the documentation it might
            // be a good idea to decorate the deck with a warning?
            m_count = 1;
        }
        else {
            m_count = boost::lexical_cast<int>(m_countString);

            if (m_count == 0)
                // TODO: decorate the deck with a warning instead?
                throw std::invalid_argument("Specifing zero repetitions is not allowed. Token: \'" + token + "\'.");
        }
    }

    ssize_t m_count;
    std::string m_countString;
    std::string m_valueString;
};
}


#endif
