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
#include <iostream>
#include "RawDeck.hpp"

namespace Opm {

    RawDeck::RawDeck() {
    }
    
    /*
     * Iterate through list of RawKeywords in search for the specified string.
     * O(n), not using map or hash because the keywords are not unique, 
     * and the order matters. Returns first matching keyword.
     */
    RawKeywordPtr RawDeck::getKeyword(const std::string& keyword) {
        for(std::list<RawKeywordPtr>::iterator it = m_keywords.begin(); it != m_keywords.end(); it++) {
            if ((*it)->getKeyword() == keyword) {
                return (*it);
            }
        }
        return RawKeywordPtr(new RawKeyword());
    }

    /*
     * Read data into deck, from specified path.
     * Throws invalid_argument exception if path not valid.
     */
    void RawDeck::readDataIntoDeck(const std::string& path) {
        checkInputFile(path);
        std::ifstream inputstream;
        Logger::info("Initializing from file: " + path);
        inputstream.open(path.c_str());

        std::string line;
        std::string keywordString;
        RawKeywordPtr currentRawKeyword;
        while (std::getline(inputstream, line)) {
            if (RawKeyword::tryParseKeyword(line, keywordString)) {
                currentRawKeyword = RawKeywordPtr(new RawKeyword(keywordString));
                m_keywords.push_back(currentRawKeyword);
            } else if (currentRawKeyword != NULL) {
                addRawRecordStringToRawKeyword(line, currentRawKeyword);
            }
        }
        inputstream.close();
    }

    void RawDeck::addRawRecordStringToRawKeyword(const std::string& recordCandidate, RawKeywordPtr currentRawKeyword) {
        if (looksLikeData(recordCandidate)) {
            currentRawKeyword->addRawRecordString(recordCandidate);
        }
    }

    bool RawDeck::looksLikeData(const std::string& line) {
        if (line.substr(0, 2) == "--") {
            Logger::debug("COMMENT LINE   <" + line + ">");
            return false;
        } else if (boost::algorithm::trim_copy(line).length() == 0) {
            Logger::debug("EMPTY LINE     <" + line + ">");
            return false;
        } else if (line.substr(0, 1) == "/") {
            Logger::debug("END OF RECORD  <" + line + ">");
            return false;
        } else {
            Logger::debug("LOOKS LIKE DATA<" + line + ">");
            return true;
        }
    }

    void RawDeck::checkInputFile(const std::string& inputPath) {
        boost::filesystem::path pathToInputFile(inputPath);
        if (!boost::filesystem::is_regular_file(pathToInputFile)) {
            Logger::error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
    }

    unsigned int RawDeck::getNumberOfKeywords() {
        return m_keywords.size();
    }

    RawDeck::~RawDeck() {
    }
}
