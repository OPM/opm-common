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
#include <boost/algorithm/string.hpp>

#include "RawRecord.hpp"
using namespace std;

namespace Opm {
    const char RawRecord::SLASH = '/';
    const char RawRecord::QUOTE = '\'';

    RawRecord::RawRecord() {
    }

    /*
     * It is assumed that after a record is terminated, there is no quote marks
     * in the subsequent comment. This is in accordance with the Eclipse user
     * manual.
     */
    RawRecord::RawRecord(const std::string& singleRecordString) {
        if (isCompleteRecordString(singleRecordString)) {
            setRecordString(singleRecordString);
        } else {
            throw std::invalid_argument("Input string is not a complete record string,"
                    " offending string: " + singleRecordString);
        }

        std::string tokenSeparators = "\t ";
        std::string quoteSeparators = "\'\"";
        char currentChar;
        char tokenStarter;
        std::string currentToken = "";
        for (unsigned i = 0; i < m_sanitizedRecordString.size(); i++) {
            currentChar = m_sanitizedRecordString[i];
            if (stringContains(tokenSeparators, currentChar)) {
                if (stringContains(quoteSeparators, tokenStarter)) {
                    currentToken += currentChar;
                } else {
                    if (currentToken.size() > 0) {
                        m_recordItems.push_back(currentToken);
                        currentToken.clear();
                    }
                    tokenStarter = currentChar;
                }
            } else if (stringContains(quoteSeparators, currentChar)) {
                if (currentChar == tokenStarter) {
                    if (currentToken.size() > 0) {
                        m_recordItems.push_back(currentToken);
                        currentToken.clear();
                    }
                    tokenStarter = '\0';
                } else {
                    tokenStarter = currentChar;
                    currentToken.clear();
                }
            } else {
                currentToken += currentChar;
            }
        }
        if (currentToken.size() > 0) {
            m_recordItems.push_back(currentToken);
            currentToken.clear();
        }
    }

    bool RawRecord::stringContains(std::string collection, char candidate) {
        return std::string::npos != collection.find(candidate);
    }

    void RawRecord::getRecords(std::vector<std::string>& recordItems) {
        recordItems = m_recordItems;
    }

    void RawRecord::getRecordString(std::string& recordString) {
        recordString = m_sanitizedRecordString;
    }

    bool RawRecord::isCompleteRecordString(const std::string& candidateRecordString) {
        unsigned int terminatingSlash = findTerminatingSlash(candidateRecordString);
        return (terminatingSlash < candidateRecordString.size());
    }

    void RawRecord::setRecordString(const std::string& singleRecordString) {
        unsigned terminatingSlash = findTerminatingSlash(singleRecordString);
        m_sanitizedRecordString = singleRecordString.substr(0, terminatingSlash);
        boost::trim(m_sanitizedRecordString);
    }

    unsigned int RawRecord::findTerminatingSlash(const std::string& singleRecordString) {
        unsigned int terminatingSlash = singleRecordString.find_first_of(SLASH);
        unsigned int lastQuotePosition = singleRecordString.find_last_of(QUOTE);

        // Checks lastQuotePosition vs terminatingSlashPosition, 
        // since specifications of WELLS, FILENAMES etc can include slash, but 
        // these are always in quotes (and there are no quotes after record-end).
        if (terminatingSlash < lastQuotePosition && lastQuotePosition < singleRecordString.size()) {
            terminatingSlash = singleRecordString.find_first_of(SLASH, lastQuotePosition);
        }
        return terminatingSlash;
    }

    RawRecord::~RawRecord() {
    }


}
