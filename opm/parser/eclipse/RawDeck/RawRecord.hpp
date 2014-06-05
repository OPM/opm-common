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

#ifndef RECORD_HPP
#define RECORD_HPP

#include <string>
#include <deque>


namespace Opm {

    /// Class representing the lowest level of the Raw datatypes, a record. A record is simply
    /// a vector containing the record elements, represented as strings. Some logic is present
    /// to handle special elements in a record string, particularly with quote characters.

    class RawRecord {
    public:
        RawRecord(const std::string& singleRecordString, const std::string& fileName = "", const std::string& keywordName = "");
        
        std::string pop_front();
        void push_front(std::string token);
        size_t size() const;

        const std::string& getRecordString() const;
        const std::string& getItem(size_t index) const;
        const std::string& getFileName() const;
        const std::string& getKeywordName() const;

        static bool isTerminatedRecordString(const std::string& candidateRecordString);
        virtual ~RawRecord();
        void dump() const;
        
    private:
        std::string m_sanitizedRecordString;
        std::deque<std::string> m_recordItems;
        const std::string m_fileName;
        const std::string m_keywordName;
        
        void setRecordString(const std::string& singleRecordString);
        void splitSingleRecordString();
        void processSeparatorCharacter(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processQuoteCharacters(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processNonSpecialCharacters(std::string& currentToken, const char& currentChar);
        bool charIsSeparator(char candidate);
        static unsigned int findTerminatingSlash(const std::string& singleRecordString);
    };
    typedef std::shared_ptr<RawRecord> RawRecordPtr;
    typedef std::shared_ptr<const RawRecord> RawRecordConstPtr;

}

#endif  /* RECORD_HPP */

