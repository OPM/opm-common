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

    RawDeck::RawDeck(RawParserKWsConstPtr rawParserKWs) {
        m_rawParserKWs = rawParserKWs;
    }

    /*
     * Iterate through list of RawKeywords in search for the specified string.
     * O(n), not using map or hash because the keywords are not unique, 
     * and the order matters. Returns first matching keyword.
     */
    RawKeywordPtr RawDeck::getKeyword(const std::string& keyword) {
        for (std::list<RawKeywordPtr>::iterator it = m_keywords.begin(); it != m_keywords.end(); it++) {
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
        readDataIntoDeck(path, m_keywords);
    }

    void RawDeck::readDataIntoDeck(const std::string& path, std::list<RawKeywordPtr>& keywordList) {
        verifyValidInputPath(path);
        {
            std::ifstream inputstream;
            Logger::info("Initializing from file: " + path);
            inputstream.open(path.c_str());

            std::string line;
            RawKeywordPtr currentRawKeyword;
            bool previousKeywordFinished = true;
            while (std::getline(inputstream, line)) {
                std::string keywordString;

                if (previousKeywordFinished && RawKeyword::tryParseKeyword(line, keywordString)) {
                    currentRawKeyword = RawKeywordPtr(new RawKeyword(keywordString));
                    keywordList.push_back(currentRawKeyword);
                    previousKeywordFinished = isKeywordFinished(currentRawKeyword);
                }
                else if (currentRawKeyword != NULL && RawKeyword::lineContainsData(line)) {
                    currentRawKeyword->addRawRecordString(line);
                    previousKeywordFinished = isKeywordFinished(currentRawKeyword);
                }
                else if (RawKeyword::lineTerminatesKeyword(line)) {
                    previousKeywordFinished = true;
                }
            }
            inputstream.close();
        }
    }

    void RawDeck::verifyValidInputPath(const std::string& inputPath) {
        boost::filesystem::path pathToInputFile(inputPath);
        if (!boost::filesystem::is_regular_file(pathToInputFile)) {
            Logger::error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
    }

    unsigned int RawDeck::getNumberOfKeywords() {
        return m_keywords.size();
    }

    bool RawDeck::isKeywordFinished(RawKeywordPtr rawKeyword) {
        if (m_rawParserKWs->keywordExists(rawKeyword->getKeyword())) {
            return rawKeyword->getNumberOfRecords() == m_rawParserKWs->getFixedNumberOfRecords(rawKeyword->getKeyword());
        }
        return false;
    }

    std::ostream& operator<<(std::ostream& os, const RawDeck& deck) {
        for (std::list<RawKeywordPtr>::const_iterator keyword = deck.m_keywords.begin(); keyword != deck.m_keywords.end(); keyword++) {
            os << (*keyword)->getKeyword() << "                -- Keyword\n";
            
            std::list<RawRecordPtr> records = (*keyword)->getRecords();
            for (std::list<RawRecordPtr>::const_iterator record = records.begin(); record != records.end(); record++) {
                std::vector<std::string> recordItems  = (*record)->getRecords();

                for (std::vector<std::string>::const_iterator recordItem = recordItems.begin(); recordItem != recordItems.end(); recordItem++) {
                    os << (*recordItem) << " ";
                }
                os << " /                -- Data\n";
            }
            os << "\n";
        }
        return os;
    }

    RawDeck::~RawDeck() {
    }
}
