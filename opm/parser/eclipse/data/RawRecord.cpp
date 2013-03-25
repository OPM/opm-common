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
        sanitizeInputString(singleRecordString);
    }

    void RawRecord::getRecord(std::string& recordString) {
        recordString = m_sanitizedRecordString;
    }

    bool RawRecord::isCompleteRecordString(const std::string& candidateRecordString) {
        unsigned int terminatingSlash = findTerminatingSlash(candidateRecordString);
        return (terminatingSlash < candidateRecordString.size());
    }

    void RawRecord::sanitizeInputString(const std::string& singleRecordString) {
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
