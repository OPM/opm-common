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
namespace fs = boost::filesystem;
#include <boost/algorithm/string.hpp>

#include "Parser.hpp"

namespace Opm {

    Parser::Parser() {
        m_keywordRawData = new KeywordRawData();
    }

    Parser::Parser(const std::string &path) {
        m_keywordRawData = new KeywordRawData();
        m_dataFilePath = path;
    }

    void Parser::parse(const std::string &path) {
        m_logger.setLogLevel(Opm::Logger::DEBUG);

        fs::path pathToCheck(path);
        if (!fs::is_regular_file(pathToCheck)) {
            m_logger.error("Unable to open file with path: " + path);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + path);
        }

        std::ifstream file;
        initInputStream(path, file);
        createKeywordAndRawData(file);
        file.close();
    }

    void Parser::parse() {
        parse(m_dataFilePath);
    }

    int Parser::getNumberOfKeywords() {
        return m_keywordRawData -> numberOfKeywords();
    }

    void Parser::createKeywordAndRawData(std::ifstream& inputstream) {
        EclipseDeck deck;
        std::string line;
        std::string currentKeyword = "";
        std::list<std::string> currentDataBlob;
        while (std::getline(inputstream, line)) {
            if (isKeyword(line)) {
                if (currentKeyword != "") {
                    m_keywordRawData->addKeywordDataBlob(line, currentDataBlob);
                }
                currentDataBlob = std::list<std::string>();
                currentKeyword = line;
            } else {
                addDataToBlob(line, currentDataBlob);
            }
        }
        if (currentKeyword != "") {
            m_keywordRawData->addKeywordDataBlob(line, currentDataBlob);
        }
    }

    void Parser::addDataToBlob(const std::string& line, std::list<std::string>& currentDataBlob) {
        if (looksLikeData(line)) {
            currentDataBlob.push_back(line);
        }
    }

    bool Parser::looksLikeData(const std::string& line) {
        if (line.substr(0, 2) == "--") {
            m_logger.debug("COMMENT LINE   <" + line + ">");
            return false;
        } else if (boost::algorithm::trim_copy(line).length() == 0) {
            m_logger.debug("EMPTY LINE     <" + line + ">");
            return false;
        } else {
            m_logger.debug("LOOKS LIKE DATA<" + line + ">");
            return true;
        }
    }

    bool Parser::isKeyword(const std::string& line) {
        std::string keywordRegex = "^[A-Z]{1,8}$";
        int status;
        regex_t re;
        regmatch_t rm;
        if (regcomp(&re, keywordRegex.c_str(), REG_EXTENDED) != 0) {
            m_logger.error("Unable to compile regular expression for keyword! Expression: " + keywordRegex);
            throw std::runtime_error("Unable to compile regular expression for keyword! Expression: " + keywordRegex);
        }

        status = regexec(&re, line.c_str(), 1, &rm, 0);
        regfree(&re);

        if (status == 0) {
            m_logger.debug("KEYWORD LINE   <" + line + ">");
            return true;
        }
        return false;
    }

    void Parser::initInputStream(const std::string &path, std::ifstream& file) {
        m_logger.info("Initializing from file: " + path);
        file.open(path.c_str());
    }

    Parser::~Parser() {
        delete m_keywordRawData;
    }
} // namespace Opm
