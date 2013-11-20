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
#include <regex.h>
#include <boost/algorithm/string.hpp>
#include "RawKeyword.hpp"
#include "RawConsts.hpp"
#include <iostream>

namespace Opm {


    RawKeyword::RawKeyword(const std::string& name, const std::string& filename, size_t lineNR) {
        commonInit(name,filename,lineNR);
    }


    RawKeyword::RawKeyword(const std::string& name , const std::string& filename, size_t lineNR , size_t inputSize, bool isTableCollection ) {
        commonInit(name,filename,lineNR);
        if (isTableCollection) {
            m_isTableCollection = true;
            m_numTables = inputSize;
        } else {
            m_fixedSizeKeyword = true;
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
        m_fixedSizeKeyword = false;
        m_isTableCollection = false;
        m_currentNumTables = 0;
    }


    const std::string& RawKeyword::getKeywordName() const {
        return m_name;
    }

    size_t RawKeyword::size() const {
        return m_records.size();
    }




    /// Important method, being repeatedly called. When a record is terminated,
    /// it is added to the list of records, and a new record is started.

    void RawKeyword::addRawRecordString(const std::string& partialRecordString) {
        m_partialRecordString += " " + partialRecordString;

        if (!m_fixedSizeKeyword && isTerminator( m_partialRecordString )) {
            if (m_isTableCollection) {
                m_currentNumTables += 1;
                if (m_currentNumTables == m_numTables) {
                    m_isFinished = true;
                    m_partialRecordString.clear();
                }
            } else {
                m_isFinished = true;
                m_partialRecordString.clear();
            }
        }

        if (!m_isFinished) {
            if (RawRecord::isTerminatedRecordString(partialRecordString)) {
                RawRecordPtr record(new RawRecord(m_partialRecordString, m_filename, m_name));
                m_records.push_back(record);
                m_partialRecordString.clear();
                
                if (m_fixedSizeKeyword && (m_records.size() == m_fixedSize))
                    m_isFinished = true;
            }
        }
    }

    bool RawKeyword::isTerminator(std::string line) {
        boost::algorithm::trim_left( line );
        if (line[0] == RawConsts::slash) {
            return true;
        } else
            return false;
    }


    bool RawKeyword::isTableCollection() const {
        return m_isTableCollection;
    }

    RawRecordPtr RawKeyword::getRecord(size_t index) const {
        if (index < m_records.size()) {
            return m_records[index];
        }
        else 
            throw std::range_error("Index out of range");
    }

    bool RawKeyword::tryParseKeyword(const std::string& keywordCandidate, std::string& result) {
        result = boost::trim_right_copy_if(keywordCandidate.substr(0, 8), boost::is_any_of("- \t"));
        if (isValidKeyword(result))
            return true;
        else
            return false;
    }

    bool RawKeyword::isValidKeyword(const std::string& keywordCandidate) {
        std::string keywordRegex = "^[A-Z][A-Z,0-9]{1,7}$";
        int status;
        regex_t regularExpression;
        regmatch_t rm;
        if (regcomp(&regularExpression, keywordRegex.c_str(), REG_EXTENDED) != 0) {
            throw std::runtime_error("Unable to compile regular expression for keyword! Expression: " + keywordRegex);
        }

        status = regexec(&regularExpression, keywordCandidate.c_str(), 1, &rm, 0);
        regfree(&regularExpression);

        if (status == 0) {
            return true;
        }
        return false;
    }


    bool RawKeyword::useLine(std::string line) {
        boost::algorithm::trim_left(line);
        if (line.length()) {
            if (line.substr(0,2) == "--")
                return false;
            else
                return true;
        } else
            return false;
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


    bool RawKeyword::isFinished() const {
        return m_isFinished;
    }

    const std::string& RawKeyword::getFilename() const {
        return m_filename;
    }

    size_t RawKeyword::getLineNR() const {
        return m_lineNR;
    }

}

