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
    }

    Parser::Parser(const std::string &path) {
        m_dataFilePath = path;
    }
    
    void Parser::parse() {
        parse(m_dataFilePath);
    }
    
    void Parser::parse(const std::string &path) {
        checkInputFile(path);
        std::ifstream file;
        initInputStream(path, file);
        
        readKeywordAndDataTokens(file);
        
        file.close();
    }

    int Parser::getNumberOfKeywords() {
        return m_keywordRawDatas.size();
    }

    void Parser::readKeywordAndDataTokens(std::ifstream& inputstream) {
        std::string line;
        KeywordDataToken currentKeywordDataToken;
        while (std::getline(inputstream, line)) {
            if (isKeyword(line)) {
                currentKeywordDataToken = KeywordDataToken(line);
                m_keywordRawDatas.push_back(currentKeywordDataToken);
            } else {
                addDataToDataToken(line, currentKeywordDataToken);
            }
        }
    }

    void Parser::addDataToDataToken(const std::string& line, KeywordDataToken& currentKeywordDataToken) {
        if (currentKeywordDataToken.getKeyword() != "" && looksLikeData(line)) {
            currentKeywordDataToken.addDataElement(line);
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

        std::string trimmedRight = boost::trim_right_copy(line);
        status = regexec(&re, trimmedRight.c_str(), 1, &rm, 0);
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

    void Parser::checkInputFile(const std::string& inputPath) {
        fs::path pathToInputFile(inputPath);
        if (!fs::is_regular_file(pathToInputFile)) {
            m_logger.error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
    }

    Parser::~Parser() {
    }
} // namespace Opm
