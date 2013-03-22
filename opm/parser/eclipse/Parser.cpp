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
#include <iostream>
#include <stdexcept>
#include <regex.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "Parser.hpp"

namespace Opm {

    Parser::Parser() {
    }

    void Parser::parse(const std::string &path, RawDeck& outputDeck) {
        checkInputFile(path);
        std::ifstream file;
        initInputStream(path, file);

        readRawDeck(file, outputDeck);

        file.close();
    }

    void Parser::readRawDeck(std::ifstream& inputstream, RawDeck& outputDeck) {
        std::string line;
        std::string keyword;
        RawKeyword currentRawKeyword;
        while (std::getline(inputstream, line)) {
            if (RawKeyword::tryGetValidKeyword(line, keyword)) {
                currentRawKeyword = RawKeyword(keyword);
                outputDeck.addKeyword(currentRawKeyword);
            }
            else {
                addRawRecordStringToRawKeyword(line, currentRawKeyword);
            }
        }
    }

    void Parser::addRawRecordStringToRawKeyword(const std::string& line, RawKeyword& currentRawKeyword) {
        if (currentRawKeyword.getKeyword() != "" && looksLikeData(line)) {
            currentRawKeyword.addRawRecordString(line);
        }
    }

    bool Parser::looksLikeData(const std::string& line) {
        if (line.substr(0, 2) == "--") {
            m_logger.debug("COMMENT LINE   <" + line + ">");
            return false;
        } else if (boost::algorithm::trim_copy(line).length() == 0) {
            m_logger.debug("EMPTY LINE     <" + line + ">");
            return false;
        } else if (line.substr(0, 1) == "/") {
            m_logger.debug("END OF RECORD  <" + line + ">");
            return false;
        } else {
            m_logger.debug("LOOKS LIKE DATA<" + line + ">");
            return true;
        }
    }

    void Parser::initInputStream(const std::string &path, std::ifstream& file) {
        m_logger.info("Initializing from file: " + path);
        file.open(path.c_str());
    }

    void Parser::checkInputFile(const std::string& inputPath) {
        boost::filesystem::path pathToInputFile(inputPath);
        if (!boost::filesystem::is_regular_file(pathToInputFile)) {
            m_logger.error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
    }

    Parser::~Parser() {
    }
} // namespace Opm
