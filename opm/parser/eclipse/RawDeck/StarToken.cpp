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

#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>



namespace Opm {

    bool tokenContainsStar(const std::string& token) {
        size_t pos = token.find('*');
        if (pos == std::string::npos)
            return false;
        else {
            if (pos == 0) return true;
            try {
                boost::lexical_cast<int>(token.substr(0, pos));
                return true;
            }
            catch (boost::bad_lexical_cast&) {
                return false;
            }
        }
    }

  
    template <>
    int readValueToken(const std::string& valueToken) {
        try {
            return boost::lexical_cast<int>(valueToken);
        }
        catch (boost::bad_lexical_cast&) {
            throw std::invalid_argument("Unable to parse string" + valueToken + " to an integer");
        }
    }
    

    template <>
    double readValueToken(const std::string& valueToken) {
        try {
            return boost::lexical_cast<double>(valueToken);
        }
        catch (boost::bad_lexical_cast&) {
            throw std::invalid_argument("Unable to parse string " + valueToken + " to a double");
        }
    }


    template <>
    std::string readValueToken(const std::string& valueToken) {
        return valueToken;
    }


    
}
