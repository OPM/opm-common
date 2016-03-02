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
#include <vector>
#include <deque>

#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>

#include <opm/parser/eclipse/Utility/Stringview.hpp>

using namespace Opm;
using namespace std;

namespace Opm {

    static inline unsigned int findTerminatingSlash( const std::string& singleRecordString ) {
        unsigned int terminatingSlash = singleRecordString.find_first_of(RawConsts::slash);
        unsigned int lastQuotePosition = singleRecordString.find_last_of(RawConsts::quote);

        // Checks lastQuotePosition vs terminatingSlashPosition,
        // since specifications of WELLS, FILENAMES etc can include slash, but
        // these are always in quotes (and there are no quotes after record-end).
        if (terminatingSlash < lastQuotePosition && lastQuotePosition < singleRecordString.size()) {
            terminatingSlash = singleRecordString.find_first_of(RawConsts::slash, lastQuotePosition);
        }
        return terminatingSlash;
    }

    static inline bool is_whitespace( char ch ) {
        return ch == '\t' || ch == ' ';
    }

    static inline std::string::const_iterator first_nonspace (
            std::string::const_iterator begin,
            std::string::const_iterator end ) {
        return std::find_if_not( begin, end, is_whitespace );
    }

    static std::deque< string_view > splitSingleRecordString( const std::string& record ) {

        std::deque< string_view > dst;

        for( auto current = first_nonspace( record.begin(), record.end() );
                current != record.end();
                current = first_nonspace( current, record.end() ) )
        {
            if( *current == RawConsts::quote ) {
                auto quote_end = std::find( current + 1, record.end(), RawConsts::quote ) + 1;
                dst.push_back( { current, quote_end } );
                current = quote_end;
            } else {
                auto token_end = std::find_if( current, record.end(), is_whitespace );
                dst.push_back( { current, token_end } );
                current = token_end;
            }
        }

        return dst;
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
    RawRecord::RawRecord(const std::string& singleRecordString, const std::string& fileName, const std::string& keywordName) : m_fileName(fileName), m_keywordName(keywordName){
        if (isTerminatedRecordString(singleRecordString)) {
            setRecordString(singleRecordString);
            this->m_recordItems = splitSingleRecordString( this->m_sanitizedRecordString );
        } else {
            throw std::invalid_argument("Input string is not a complete record string,"
                    " offending string: " + singleRecordString);
        }
    }

    const std::string& RawRecord::getFileName() const {
        return m_fileName;
    }

    const std::string& RawRecord::getKeywordName() const {
        return m_keywordName;
    }

    string_view RawRecord::pop_front() {
        auto front = m_recordItems.front();

        this->m_recordItems.pop_front();
        return front;
    }

    void RawRecord::push_front(std::string tok ) {
        this->expanded_items.push_back( tok );
        string_view record { this->expanded_items.back().begin(), this->expanded_items.back().end() };
        this->m_recordItems.push_front( record );
    }

    size_t RawRecord::size() const {
        return m_recordItems.size();
    }

    void RawRecord::dump() const {
        std::cout << "RecordDump: ";
        for (size_t i = 0; i < m_recordItems.size(); i++) {
            std::cout
                << this->m_recordItems[i] << "/"
                << getItem( i ) << " ";
        }
        std::cout << std::endl;
    }


    string_view RawRecord::getItem(size_t index) const {
        return this->m_recordItems.at( index );
    }

    const std::string& RawRecord::getRecordString() const {
        return m_sanitizedRecordString;
    }

    bool RawRecord::isTerminatedRecordString(const std::string& candidateRecordString) {
        unsigned int terminatingSlash = findTerminatingSlash(candidateRecordString);
        bool hasTerminatingSlash = (terminatingSlash < candidateRecordString.size());
        int numberOfQuotes = std::count(candidateRecordString.begin(), candidateRecordString.end(), RawConsts::quote);
        bool hasEvenNumberOfQuotes = (numberOfQuotes % 2) == 0;
        return hasTerminatingSlash && hasEvenNumberOfQuotes;
    }

    void RawRecord::setRecordString(const std::string& singleRecordString) {
        unsigned terminatingSlash = findTerminatingSlash(singleRecordString);
        m_sanitizedRecordString = singleRecordString.substr(0, terminatingSlash);
        boost::trim(m_sanitizedRecordString);
    }
}
