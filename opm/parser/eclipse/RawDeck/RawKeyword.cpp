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
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

namespace Opm {


    RawKeyword::RawKeyword(const std::string& name, Raw::KeywordSizeEnum sizeType , const std::string& filename, size_t lineNR) {
        if (sizeType == Raw::SLASH_TERMINATED || sizeType == Raw::UNKNOWN) {
            commonInit(name,filename,lineNR);
            m_sizeType = sizeType;
        } else
            throw std::invalid_argument("Error - invalid sizetype on input");
    }


    RawKeyword::RawKeyword(const std::string& name , const std::string& filename, size_t lineNR , size_t inputSize, bool isTableCollection ) {
        commonInit(name,filename,lineNR);
        if (isTableCollection) {
            m_sizeType = Raw::TABLE_COLLECTION;
            m_numTables = inputSize;
        } else {
            m_sizeType = Raw::FIXED;
            m_fixedSize = inputSize;
            if (m_fixedSize == 0)
                m_isFinished = true;
            else
                m_isFinished = false;
        }
    }


    void RawKeyword::commonInit(const std::string& name , const std::string& filename, size_t lineNR) {
        setKeywordName( name );
        m_filename = filename;
        m_lineNR = lineNR;
        m_isFinished = false;
        m_currentNumTables = 0;
    }


    const std::string& RawKeyword::getKeywordName() const {
        return m_name;
    }

    size_t RawKeyword::size() const {
        return m_records.size();
    }

    static inline bool isTerminator( const std::string& line ) {
        auto fst = std::find_if_not( line.begin(), line.end(),
                RawConsts::is_separator );
        return fst != line.end() && *fst == RawConsts::slash;
    }

    /// Important method, being repeatedly called. When a record is terminated,
    /// it is added to the list of records, and a new record is started.

    void RawKeyword::addRawRecordString(const std::string& partialRecordString) {
        m_partialRecordString += " " + partialRecordString;


        if( m_sizeType != Raw::FIXED && isTerminator( m_partialRecordString ) ) {
            if (m_sizeType == Raw::TABLE_COLLECTION) {
                m_currentNumTables += 1;
                if (m_currentNumTables == m_numTables) {
                    m_isFinished = true;
                    m_partialRecordString.clear();
                    return;
                }
            } else if( m_sizeType != Raw::UNKNOWN ) {
                m_isFinished = true;
                m_partialRecordString.clear();
                return;
            }
        }

        if( m_isFinished ) return;

        if( RawRecord::isTerminatedRecordString( partialRecordString ) ) {
            m_records.emplace_back( std::move( m_partialRecordString ), m_filename, m_name );
            m_partialRecordString.clear();

            if( m_sizeType == Raw::FIXED && m_records.size() == m_fixedSize )
                m_isFinished = true;
        }
    }


    const RawRecord& RawKeyword::getFirstRecord() const {
        return *m_records.begin();
    }

    static inline std::string uppercase( std::string&& str ) {
        std::transform( str.begin(), str.end(), str.begin(),
            []( char c ) { return std::toupper( c ); } );
        return str;
    }

    bool RawKeyword::isKeywordPrefix(const std::string& line, std::string& keyword ) {
        // make the keyword string ALL_UPPERCASE because Eclipse seems
        // to be case-insensitive (although this is one of its
        // undocumented features...)
        keyword = uppercase( ParserKeyword::getDeckName( line ) );

        return isValidKeyword( keyword );
    }

    bool RawKeyword::isValidKeyword(const std::string& keywordCandidate) {
        return ParserKeyword::validDeckName(keywordCandidate);
    }

    void RawKeyword::setKeywordName(const std::string& name) {
        m_name = boost::algorithm::trim_right_copy(name);
        if (!isValidKeyword(m_name)) {
            throw std::invalid_argument("Not a valid keyword:" + name);
        } else if (m_name.size() > Opm::RawConsts::maxKeywordLength) {
            throw std::invalid_argument("Too long keyword:" + name);
        } else if (boost::algorithm::trim_left_copy(m_name) != m_name) {
            throw std::invalid_argument("Illegal whitespace start of keyword:" + name);
        }
    }

    bool RawKeyword::isPartialRecordStringEmpty() const {
        return m_partialRecordString.size() == 0;
    }


    void RawKeyword::finalizeUnknownSize() {
        if (m_sizeType == Raw::UNKNOWN)
            m_isFinished = true;
        else
            throw std::invalid_argument("Fatal error finalizing keyword:" + m_name + " Only RawKeywords with UNKNOWN size can be explicitly finalized.");
    }


    bool RawKeyword::isFinished() const {
        return m_isFinished;
    }

    const std::string& RawKeyword::getFilename() const {
        return m_filename;
    }

    size_t RawKeyword::getLineNR() const {
        return m_lineNR;
    }

    RawKeyword::const_iterator RawKeyword::begin() const {
        return this->m_records.begin();
    }

    RawKeyword::const_iterator RawKeyword::end() const {
        return this->m_records.end();
    }

    RawKeyword::iterator RawKeyword::begin() {
        return this->m_records.begin();
    }

    RawKeyword::iterator RawKeyword::end() {
        return this->m_records.end();
    }


    Raw::KeywordSizeEnum RawKeyword::getSizeType() const {
        return m_sizeType;
    }
}

