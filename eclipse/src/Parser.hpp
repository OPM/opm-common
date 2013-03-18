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
#define	PARSER_H

#include <string>
#include <fstream>
using std::ifstream;

#include "EclipseDeck.hpp"
#include "Logger.hpp"

namespace Opm {

    class Parser {
    public:
        Parser();
        Parser(const std::string &path);
        EclipseDeck parse();
        EclipseDeck parse(const std::string &path);
        std::string getLog();
        virtual ~Parser();
    private:
        std::string m_dataFilePath;
        Logger m_logger;
        void initInputStream(const std::string &path, ifstream& file);
        EclipseDeck doFileParsing(ifstream& inputstream);
    };
} // namespace Opm
#endif	/* PARSER_H */

