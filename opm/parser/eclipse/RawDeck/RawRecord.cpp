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

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <deque>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>

#include <opm/parser/eclipse/Utility/Stringview.hpp>

using namespace Opm;
using namespace std;

namespace Opm {

    static inline std::string::const_iterator findTerminatingSlash( const std::string& rec ) {

        if( rec.back() == RawConsts::slash ) return rec.end() - 1;

        /* possible fast path: no terminating slash in record */
        const auto rslash = std::find( rec.rbegin(), rec.rend(), RawConsts::slash );
        if( rslash == rec.rend() ) return rec.end();

        /* heuristic: since slash makes everything to the right of it into a
         * no-op, if there is more than whitespace to its right before newline
         * we guess that this is in fact not the terminating slash but rather
         * some comment or description. Most of the time this is not the case,
         * and it is slower, so avoid doing it if possible.
         */

        auto slash = (rslash + 1).base();
        if( std::find_if_not( slash, rec.end(), RawConsts::is_separator ) == rec.end() )
            return slash;

        /*
         * left-to-right search after last closing quote. Like the previous
         * implementation, assumes there are no quote marks past the
         * terminating slash (as per the Eclipse manual). Search past last
         * quote because slashes can appear in quotes in filenames etc, but
         * will always be quoted.
         */

        const auto quote = std::find( rec.rbegin(), rec.rend(), RawConsts::quote );
        const auto begin = quote == rec.rend() ? rec.begin() : (quote + 1).base();
        return std::find( begin, rec.end(), RawConsts::slash );
    }

    static inline std::string::const_iterator first_nonspace (
            std::string::const_iterator begin,
            std::string::const_iterator end ) {
        return std::find_if_not( begin, end, RawConsts::is_separator );
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
            } else if( *current == RawConsts::slash ) {
                /* some records are break the optimistic algorithm of
                 * findTerminatingSlash and contain multiple trailing slashes
                 * with nothing inbetween. The first occuring one is the actual
                 * terminator and we simply ignore everything following it.
                 */
                break;
            } else {
                auto token_end = std::find_if( current, record.end(), RawConsts::is_separator );
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

    static inline bool even_quotes( const std::string& str ) {
        return std::count( str.begin(), str.end(), RawConsts::quote ) % 2 == 0;
    }

    static inline std::string&& trim_record( std::string& str ) {
        auto end = findTerminatingSlash( str );
        /* type hack to work around findTerminatingSlash returning a const_iterator */
        std::string::iterator mut_end( str.begin() );
        std::advance( mut_end, std::distance< decltype( end ) >( mut_end, end ) );
        str.erase( mut_end, str.end() );
        return std::move( str );
    }

    RawRecord::RawRecord(std::string&& singleRecordString,
                         const std::string& fileName,
                         const std::string& keywordName) :
        m_sanitizedRecordString( trim_record( singleRecordString ) ),
        m_recordItems( splitSingleRecordString( m_sanitizedRecordString ) ),
        m_fileName(fileName),
        m_keywordName(keywordName)
    {

        if( !even_quotes( singleRecordString ) )
            throw std::invalid_argument(
                "Input string is not a complete record string, "
                "offending string: " + singleRecordString
            );
    }

    const std::string& RawRecord::getFileName() const {
        return m_fileName;
    }

    const std::string& RawRecord::getKeywordName() const {
        return m_keywordName;
    }

    void RawRecord::push_front(std::string tok ) {
        this->expanded_items.push_back( tok );
        string_view record { this->expanded_items.back().begin(), this->expanded_items.back().end() };
        this->m_recordItems.push_front( record );
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

    const std::string& RawRecord::getRecordString() const {
        return m_sanitizedRecordString;
    }

    bool RawRecord::isTerminatedRecordString(const std::string& candidateRecordString) {
        const auto terminatingSlash = findTerminatingSlash(candidateRecordString);
        bool hasTerminatingSlash = terminatingSlash != candidateRecordString.end();
        return hasTerminatingSlash && even_quotes( candidateRecordString );
    }

}
