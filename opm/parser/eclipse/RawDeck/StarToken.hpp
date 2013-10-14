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

#define STAR    '*'
#define C_EOS  '\0'

namespace Opm {

    bool tokenContainsStar(const std::string& token);
    
    template <class T>
    T readValueToken(const std::string& valueToken) {
        return 0;
    }

    template <>
    std::string readValueToken(const std::string& valueToken);

    template <>
    double readValueToken(const std::string& valueToken);

    template <>
    int readValueToken(const std::string& valueToken);


template <class T>
class StarToken {

    public:
        StarToken(const std::string& token) {  
            size_t star_pos = token.find( STAR );
            
            if (star_pos != std::string::npos) {
                
                if (token[0] == STAR) 
                m_multiplier = 1;
                else {
                    char * error_ptr;
                    m_multiplier = strtol( token.c_str() , &error_ptr , 10);

                    if (m_multiplier <= 0 || error_ptr[0] != STAR)
                        throw std::invalid_argument("Parsing multiplier from " + token + " failed");
                }
                {
                    const std::string& value_token = token.substr( star_pos + 1);
                    if (value_token.size()) {
                        m_value = readValueToken<T>( value_token );
                        m_hasValue = true;
                        if (star_pos == 0)
                          throw std::invalid_argument("Failed to extract multiplier from token:"  + token);
                    } else {
                        m_hasValue = false;
                    }
                }
            } else
                throw std::invalid_argument("The input token: \'" + token + "\' does not contain a \'*\'.");
        }

        size_t multiplier() const {
            return m_multiplier;
        }        


        T value() const {
            if (!m_hasValue)
                throw std::invalid_argument("The input token did not specify a value ");
            return m_value; 
        }     


        bool hasValue() const {
            return m_hasValue;
        }


 
    private:
        ssize_t m_multiplier;
        T m_value;
        bool m_hasValue;
    };
}


#endif
