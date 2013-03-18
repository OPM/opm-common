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


#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
using std::ifstream;

#include "Parser.hpp"

Parser::Parser() {
}

Parser::Parser(const std::string &path) {
    m_dataFilePath = path;
}

EclipseDeck Parser::parse() {
    EclipseDeck deck;
    m_logger.debug("Initializing inputstream from file: " + m_dataFilePath);

    ifstream inputstream;
    inputstream.open(m_dataFilePath.c_str());

    if (!inputstream.is_open()) {
        m_logger.debug("ERROR: unable to open file");
        return deck;
    }

    std::string line;
    while (!inputstream.eof()) {
        std::getline(inputstream, line);
        if (line.substr(0, 2) == "--") {
            m_logger.debug("COMMENT LINE   < " + line + ">");
        }
        else if (boost::algorithm::trim_copy(line).length() == 0) {
            m_logger.debug("EMPTY LINE     <" + line + ">");
        }
        else if (line.substr(0, 1) != " " && boost::algorithm::to_upper_copy(line) == line) {
            deck.addKeyword(line);
            m_logger.debug("KEYWORD LINE   <" + line + ">");
        }
        else {
            m_logger.debug("SOMETHING ELSE <" + line + ">");
        }
    }
    inputstream.close();
    return deck;
}

Parser::~Parser() {
}

