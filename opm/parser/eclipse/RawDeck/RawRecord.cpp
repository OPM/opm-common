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
    const std::string RawRecord::SEPARATORS = "\t ";

    RawRecord::RawRecord() {
    }

    /*
     * It is assumed that after a record is terminated, there is no quote marks
     * in the subsequent comment. This is in accordance with the Eclipse user
     * manual.
     * 
     * If a "non-complete" record string is supplied, an invalid_argument
     * exception is thrown.
     * 
     */
    RawRecord::RawRecord(const std::string& singleRecordString) {
        if (isCompleteRecordString(singleRecordString)) {
            setRecordString(singleRecordString);
        } else {
            throw std::invalid_argument("Input string is not a complete record string,"
                    " offending string: " + singleRecordString);
        }
        splitSingleRecordString();
    }

    const std::vector<std::string>& RawRecord::getRecords() const {
        return m_recordItems;
    }

    const std::string& RawRecord::getRecordString() const {
       return m_sanitizedRecordString;
    }

    bool RawRecord::isCompleteRecordString(const std::string& candidateRecordString) {
        unsigned int terminatingSlash = findTerminatingSlash(candidateRecordString);
        bool hasTerminatingSlash = (terminatingSlash < candidateRecordString.size());
        int numberOfQuotes = std::count(candidateRecordString.begin(), candidateRecordString.end(), QUOTE);
        bool hasEvenNumberOfQuotes = (numberOfQuotes % 2) == 0;
        return hasTerminatingSlash && hasEvenNumberOfQuotes;
    }

    void RawRecord::splitSingleRecordString() {
        char currentChar;
        char tokenStartCharacter;
        std::string currentToken = "";
        for (unsigned i = 0; i < m_sanitizedRecordString.size(); i++) {
            currentChar = m_sanitizedRecordString[i];
            if (charIsSeparator(currentChar)) {
                processSeparatorCharacter(currentToken, currentChar, tokenStartCharacter);
            } else if (currentChar == QUOTE) {
                processQuoteCharacters(currentToken, currentChar, tokenStartCharacter);
            } else {
                processNonSpecialCharacters(currentToken, currentChar);
            }
        }
        if (currentToken.size() > 0) {
            m_recordItems.push_back(currentToken);
            currentToken.clear();
        }
    }

    void RawRecord::processSeparatorCharacter(std::string& currentToken, const char& currentChar, char& tokenStartCharacter) {
        if (tokenStartCharacter == QUOTE) {
            currentToken += currentChar;
        } else {
            if (currentToken.size() > 0) {
                m_recordItems.push_back(currentToken);
                currentToken.clear();
            }
            tokenStartCharacter = currentChar;
        }
    }

    void RawRecord::processQuoteCharacters(std::string& currentToken, const char& currentChar, char& tokenStartCharacter) {
        if (currentChar == tokenStartCharacter) {
            if (currentToken.size() > 0) {
                m_recordItems.push_back("'" + currentToken + "'"); //Adding quotes, not sure what is best.
                currentToken.clear();
            }
            tokenStartCharacter = '\0';
        } else {
            tokenStartCharacter = currentChar;
            currentToken.clear();
        }
    }

    void RawRecord::processNonSpecialCharacters(std::string& currentToken, const char& currentChar) {
        currentToken += currentChar;
    }

    bool RawRecord::charIsSeparator(char candidate) {
        return std::string::npos != SEPARATORS.find(candidate);
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
