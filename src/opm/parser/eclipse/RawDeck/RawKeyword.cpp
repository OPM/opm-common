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

#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/Utility/String.hpp>

namespace Opm {

    static const std::string emptystr = "";

    RawKeyword::RawKeyword(const string_view& name, Raw::KeywordSizeEnum sizeType , const std::string& filename, size_t lineNR, bool slash_terminated) :
        m_location(filename, lineNR),
        slash_terminated_records(slash_terminated),
        m_partialRecordString( emptystr )
    {
        if (sizeType == Raw::SLASH_TERMINATED || sizeType == Raw::UNKNOWN) {
            commonInit(name.string(),filename,lineNR);
            m_sizeType = sizeType;
        } else
            throw std::invalid_argument("Error - invalid sizetype on input");
    }

    RawKeyword::RawKeyword(const string_view& name , const std::string& filename, size_t lineNR , size_t inputSize, bool slash_terminated, bool isTableCollection ) :
        m_location(filename, lineNR),
        slash_terminated_records(slash_terminated)
    {
        commonInit(name.string(),filename,lineNR);
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

        this->m_is_title = name == "TITLE";
    }


    const std::string& RawKeyword::getKeywordName() const {
        return m_name;
    }

    size_t RawKeyword::size() const {
        return m_records.size();
    }

    static inline bool isTerminator( const string_view& line ) {
        return line.size() == 1 && line.back() == RawConsts::slash;
    }

    /// Important method, being repeatedly called. When a record is terminated,
    /// it is added to the list of records, and a new record is started.

    void RawKeyword::addRawRecordString(const string_view& partialRecordString) {
        if( m_partialRecordString == emptystr )
            m_partialRecordString = partialRecordString;
        else
            m_partialRecordString = { m_partialRecordString.begin(), partialRecordString.end() };


        if( isTerminator( m_partialRecordString ) ) {
            if (m_sizeType == Raw::TABLE_COLLECTION) {
                m_currentNumTables += 1;
                if (m_currentNumTables == m_numTables) {
                    m_isFinished = true;
                    m_partialRecordString = emptystr;
                    return;
                }
            } else if( m_sizeType == Raw::SLASH_TERMINATED) {
                m_isFinished = true;
                m_partialRecordString = emptystr;
                return;
            }
        }

        if( m_isFinished ) return;

        if( this->is_title() ) {

            string_view recstr = m_partialRecordString == ""
                               ? "untitled"
                               : m_partialRecordString;

            m_records.emplace_back( recstr );
            m_partialRecordString = emptystr;
            m_isFinished = true;
            return;
        }

        if( RawRecord::isTerminatedRecordString( partialRecordString ) ) {
            this->m_partialRecordString = string_view{ this->m_partialRecordString.begin(), this->m_partialRecordString.end() - 1 };
            this->terminateRecord(false);
        }
    }


    void RawKeyword::terminateRecord(bool text) {
        this->m_records.emplace_back( this->m_partialRecordString, text );
        m_partialRecordString = emptystr;

        if( m_sizeType == Raw::FIXED && m_records.size() == m_fixedSize )
            m_isFinished = true;
    };

    const RawRecord& RawKeyword::getFirstRecord() const {
        return *m_records.begin();
    }

    bool RawKeyword::isKeywordPrefix(const string_view& line, std::string& keyword ) {
        // make the keyword string ALL_UPPERCASE because Eclipse seems
        // to be case-insensitive (although this is one of its
        // undocumented features...)
        keyword = uppercase( ParserKeyword::getDeckName( line ).string() );

        return isValidKeyword( keyword );
    }

    bool RawKeyword::isValidKeyword(const std::string& keywordCandidate) {
        return ParserKeyword::validDeckName(keywordCandidate);
    }

    void RawKeyword::setKeywordName(const std::string& name) {
        m_name = rtrim_copy(name);
        if (!isValidKeyword(m_name)) {
            throw std::invalid_argument("Not a valid keyword:" + name);
        } else if (m_name.size() > Opm::RawConsts::maxKeywordLength) {
            throw std::invalid_argument("Too long keyword:" + name);
        } else if (ltrim_copy(m_name) != m_name) {
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

    bool RawKeyword::is_title() const {
        return this->m_is_title;
    }

    Raw::KeywordSizeEnum RawKeyword::getSizeType() const {
        return m_sizeType;
    }

    bool RawKeyword::slashTerminatedRecords() const {
        return this->slash_terminated_records;
    }

}

