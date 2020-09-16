/*
  Copyright 2015 Statoil ASA.

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

#ifndef KEYWORD_LOCATION_HPP
#define KEYWORD_LOCATION_HPP

namespace Opm {

class KeywordLocation {
public:
    std::string keyword;
    std::string filename = "<memory string>";
    std::size_t lineno = 0;

    KeywordLocation() = default;
    KeywordLocation(std::string kw, std::string fname, std::size_t lno) :
        keyword(std::move(kw)),
        filename(std::move(fname)),
        lineno(lno)
    {}

    static KeywordLocation serializeObject()
    {
        KeywordLocation result;
        result.keyword = "KW";
        result.filename = "test";
        result.lineno = 1;

        return result;
    }

    bool operator==(const KeywordLocation& data) const {
        return keyword == data.keyword &&
               filename == data.filename &&
               lineno == data.lineno;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(keyword);
        serializer(filename);
        serializer(lineno);
    }
};

}

#endif
