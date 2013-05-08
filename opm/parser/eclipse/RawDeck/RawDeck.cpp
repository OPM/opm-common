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
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <stdexcept>
#include "RawDeck.hpp"
#include "RawConsts.hpp"

namespace Opm {

    RawDeck::RawDeck(RawParserKWsConstPtr rawParserKWs) {
        m_rawParserKWs = rawParserKWs;
    }

    /// Iterate through list of RawKeywords in search for the specified string.
    /// O(n), not using map or hash because the keywords are not unique, 
    /// and the order matters. Returns first matching keyword.

    RawKeywordConstPtr RawDeck::getKeyword(const std::string& keyword) const {
        for (std::list<RawKeywordConstPtr>::const_iterator it = m_keywords.begin(); it != m_keywords.end(); it++) {
            if ((*it)->getKeyword() == keyword) {
                return (*it);
            }
        }
        throw std::invalid_argument("Keyword not found, keyword: " + keyword);
    }

    bool RawDeck::hasKeyword(const std::string& keyword) const {
        for (std::list<RawKeywordConstPtr>::const_iterator it = m_keywords.begin(); it != m_keywords.end(); it++) {
            if ((*it)->getKeyword() == keyword) {
                return true;
            }
        }
        return false;
    }

    /// The main data reading function, reads one and one keyword into the RawDeck
    /// If the INCLUDE keyword is found, the specified include file is inline read into the RawDeck.
    /// The data is read into a keyword, record by record, until the fixed number of records specified
    /// in the RawParserKW is met, or till a slash on a separate line is found.

    void RawDeck::readDataIntoDeck(const std::string& path) {
        boost::filesystem::path dataFolderPath = verifyValidInputPath(path);
        {
            std::ifstream inputstream;
            Logger::info("Initializing from file: " + path);
            inputstream.open(path.c_str());

            std::string line;
            RawKeywordPtr currentRawKeyword;
            while (std::getline(inputstream, line)) {
                std::string keywordString;
                if (currentRawKeyword == NULL) {
                    if (RawKeyword::tryParseKeyword(line, keywordString)) {
                        currentRawKeyword = RawKeywordPtr(new RawKeyword(keywordString));
                        if (isKeywordFinished(currentRawKeyword)) {
                            addKeyword(currentRawKeyword, dataFolderPath);
                            currentRawKeyword.reset();
                        }
                    }
                } else if (currentRawKeyword != NULL && RawKeyword::lineContainsData(line)) {
                    currentRawKeyword->addRawRecordString(line);
                    if (isKeywordFinished(currentRawKeyword)) {
                        addKeyword(currentRawKeyword, dataFolderPath);
                        currentRawKeyword.reset();
                    }
                } else if (currentRawKeyword != NULL && RawKeyword::lineTerminatesKeyword(line)) {
                    if (!currentRawKeyword->isPartialRecordStringEmpty()) {
                        Logger::error("Reached keyword terminator slash, but there is non-terminated data on current keyword. "
                                "Adding terminator, but records should be terminated by slash in data file");
                        currentRawKeyword->addRawRecordString(std::string(1, Opm::RawConsts::slash));
                    }
                    addKeyword(currentRawKeyword, dataFolderPath);
                    currentRawKeyword.reset();
                }
            }
            inputstream.close();
        }
    }

    void RawDeck::addKeyword(RawKeywordConstPtr keyword, const boost::filesystem::path& dataFolderPath) {
        if (keyword->getKeyword() == Opm::RawConsts::include) {
            std::string includeFileString = keyword->getRecords().front()->getItems().front();
            boost::filesystem::path pathToIncludedFile(dataFolderPath);
            pathToIncludedFile /= includeFileString;

            readDataIntoDeck(pathToIncludedFile.string());
        } else {
            m_keywords.push_back(keyword);
        }
    }

    boost::filesystem::path RawDeck::verifyValidInputPath(const std::string& inputPath) {
        Logger::info("Verifying path: " + inputPath);
        boost::filesystem::path pathToInputFile(inputPath);
        if (!boost::filesystem::is_regular_file(pathToInputFile)) {
            Logger::error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
        return pathToInputFile.parent_path();
    }

    unsigned int RawDeck::getNumberOfKeywords() const {
        return m_keywords.size();
    }

    /// Checks if the current keyword being read is finished, based on the number of records
    /// specified for the current keyword type in the RawParserKW class.

    bool RawDeck::isKeywordFinished(RawKeywordConstPtr rawKeyword) {
        if (m_rawParserKWs->keywordExists(rawKeyword->getKeyword())) {
            return rawKeyword->getNumberOfRecords() == m_rawParserKWs->getFixedNumberOfRecords(rawKeyword->getKeyword());
        }
        return false;
    }

    /// Operator overload to write the content of the RawDeck to an ostream

    std::ostream& operator<<(std::ostream& os, const RawDeck& deck) {
        for (std::list<RawKeywordConstPtr>::const_iterator keyword = deck.m_keywords.begin(); keyword != deck.m_keywords.end(); keyword++) {
            os << (*keyword)->getKeyword() << "                -- Keyword\n";

            std::list<RawRecordConstPtr> records = (*keyword)->getRecords();
            for (std::list<RawRecordConstPtr>::const_iterator record = records.begin(); record != records.end(); record++) {
                std::deque<std::string> recordItems = (*record)->getItems();

                for (size_t i=0; i<recordItems.size(); i++) {
                    os << recordItems[i] << " ";
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
