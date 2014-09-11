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
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>



namespace Opm {

    bool isStarToken(const std::string& token,
                           std::string& countString,
                           std::string& valueString) {
        // find first character which is not a digit
        size_t pos = 0;
        for (; pos < token.length(); ++pos)
            if (!std::isdigit(token[pos]))
                break;

        // if no such character exists or if this character is not a star, the token is
        // not a "star token" (i.e. it is not a "repeat this value N times" token.
        if (pos >= token.size() || token[pos] != '*')
            return false;
        // Quote from the Eclipse Reference Manual: "An asterisk by
        // itself is not sufficent". However, our experience is that
        // Eclipse accepts such tokens and we therefore interpret "*"
        // as "1*".
        //
        // Tokens like "*12" are recognized as a star token
        // here, but we will throw in the code which uses
        // StarToken<T>. (Because Eclipse does not seem to
        // accept these and we would stay as closely to the spec as
        // possible.)
        else if (pos == 0) {
            countString = "";
            valueString = token.substr(pos + 1);
            return true;
        }

        // if a star is prefixed by an unsigned integer N, then this should be
        // interpreted as "repeat value after star N times"
        try {
            boost::lexical_cast<int>(token.substr(0, pos));
        }
        catch (...) {
            // the lexical cast may fail as the number of digits may be too large...
            return false;
        }

        countString = token.substr(0, pos);
        valueString = token.substr(pos + 1);
        return true;
    }
}
