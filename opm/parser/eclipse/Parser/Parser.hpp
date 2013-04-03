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

#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <map>
#include <fstream>
#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Logger.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>
#include <opm/parser/eclipse/Parser/ParserKW.hpp>

namespace Opm {

    class Parser {
    public:
        Parser();
        RawDeckPtr parse(const std::string &path);
        virtual   ~Parser();

        void       addKW(ParserKWConstPtr parserKW);

    private:
        //Logger   m_logger;
      std::map<std::string,ParserKWConstPtr> keywords;
    };
    
    typedef boost::shared_ptr<Parser> ParserPtr;
} // namespace Opm
#endif  /* PARSER_H */

