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
#define	RECORD_HPP

#include <string>
#include <vector>


namespace Opm {

    class RawRecord {
    public:
        static const char SLASH;
        static const char QUOTE;
        static const std::string SEPARATORS;

        RawRecord();
        RawRecord(const std::string& singleRecordString);
        void getRecordString(std::string& recordString);
        void getRecords(std::vector<std::string>& recordItems);
        static bool isCompleteRecordString(const std::string& candidateRecordString);
        virtual ~RawRecord();
    private:
        std::string m_sanitizedRecordString;
        std::vector<std::string> m_recordItems;
        void setRecordString(const std::string& singleRecordString);
        void splitSingleRecordString();
        void processSeparatorCharacter(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processQuoteCharacters(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processNonSpecialCharacters(std::string& currentToken, const char& currentChar);
        bool charIsSeparator(char candidate);
        static unsigned int findTerminatingSlash(const std::string& singleRecordString);
    };
    typedef boost::shared_ptr<RawRecord> RawRecordPtr;
}

#endif	/* RECORD_HPP */

