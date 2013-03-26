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
#include <iostream>

namespace Opm {
    RawKeyword::RawKeyword() {
    }

    RawKeyword::RawKeyword(const std::string& keyword) {
        setKeyword(keyword);
    }

    void RawKeyword::setKeyword(const std::string& keyword) {
        m_keyword = boost::algorithm::trim_right_copy(keyword);
        if (!isValidKeyword(m_keyword)) {
            throw std::invalid_argument("Not a valid keyword:" + keyword);
        } else if (m_keyword.size() > 8) {
            throw std::invalid_argument("Too long keyword:" + keyword);
        } else if (boost::algorithm::trim_left_copy(m_keyword) != m_keyword) {
            throw std::invalid_argument("Illegal whitespace start of keyword:" + keyword);
        }
    }

    void RawKeyword::addRawRecordString(const std::string& partialRecordString) {
        m_partialRecordString += partialRecordString;
        if (RawRecord::isCompleteRecordString(m_partialRecordString)) {
            RawRecord record(m_partialRecordString);
            m_records.push_back(record);
            m_partialRecordString.clear();
        }
    }

    unsigned int RawKeyword::getNumberOfRecords() {
        return m_records.size();
    }

    void RawKeyword::getRecords(std::list<RawRecord>& records) {
        records = m_records;
    }

    std::string RawKeyword::getKeyword() {
        return m_keyword;
    }

    bool RawKeyword::tryGetValidKeyword(const std::string& keywordCandidate, std::string& result) {
        result = boost::trim_right_copy(keywordCandidate.substr(0, 8));
        if (isValidKeyword(result)) {
            Logger::debug("KEYWORD     <" + keywordCandidate + ">");
            return true;
        }
        return false;
    }

    bool RawKeyword::isValidKeyword(const std::string& keywordCandidate) {
        std::string keywordRegex = "^[A-Z]{1,8}$";
        int status;
        regex_t re;
        regmatch_t rm;
        if (regcomp(&re, keywordRegex.c_str(), REG_EXTENDED) != 0) {
            throw std::runtime_error("Unable to compile regular expression for keyword! Expression: " + keywordRegex);
        }

        status = regexec(&re, keywordCandidate.c_str(), 1, &rm, 0);
        regfree(&re);

        if (status == 0) {
            return true;
        }
        return false;
    }

    RawKeyword::~RawKeyword() {
    }
}

