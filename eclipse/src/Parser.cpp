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

#include <stdexcept>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/algorithm/string.hpp>

#include "Parser.hpp"

namespace Opm {

    Parser::Parser() {
    }

    Parser::Parser(const std::string &path) {
        m_dataFilePath = path;
    }

    EclipseDeck Parser::parse(const std::string &path) {
        fs::path pathToCheck(path);
        if (!fs::is_regular_file(pathToCheck)) {
            throw std::invalid_argument("Given path is not a valid file-path, path: " + path);
        }
        
        ifstream file;
        initInputStream(path, file);
        EclipseDeck deck = doFileParsing(file);
        file.close();
        return deck;
    }
    
    EclipseDeck Parser::parse() {
        return parse(m_dataFilePath);
    }
    
    EclipseDeck Parser::doFileParsing(ifstream& inputstream) {
        EclipseDeck deck;
        std::string line;
        while (std::getline(inputstream, line)) {
            if (line.substr(0, 2) == "--") {
                m_logger.debug("COMMENT LINE   < " + line + ">");
            } else if (boost::algorithm::trim_copy(line).length() == 0) {
                m_logger.debug("EMPTY LINE     <" + line + ">");
            } else if (line.substr(0, 1) != " " && boost::algorithm::to_upper_copy(line) == line) {
                deck.addKeyword(line);
                m_logger.debug("KEYWORD LINE   <" + line + ">");
            } else {
                m_logger.debug("SOMETHING ELSE <" + line + ">");
            }
        }
        return deck;
    }
    
    void Parser::initInputStream(const std::string &path, ifstream& file) {
        m_logger.debug("Initializing from file: " + path);
        file.open(path.c_str());
    }

    Parser::~Parser() {
    }
} // namespace Opm
