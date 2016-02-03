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

#include "DeckKeyword.hpp"
#include "DeckRecord.hpp"
#include "DeckItem.hpp"

namespace Opm {

    DeckKeyword::DeckKeyword(const std::string& keywordName) {
        m_knownKeyword = true;
        m_keywordName = keywordName;
        m_isDataKeyword = false;
        m_fileName = "";
        m_lineNumber = -1;
    }

    DeckKeyword::DeckKeyword(const std::string& keywordName, bool knownKeyword) {
        m_knownKeyword = knownKeyword;
        m_keywordName = keywordName;
        m_isDataKeyword = false;
        m_fileName = "";
        m_lineNumber = -1;
    }

    void DeckKeyword::setLocation(const std::string& fileName, int lineNumber) {
        m_fileName = fileName;
        m_lineNumber = lineNumber;
    }

    std::shared_ptr<const ParserKeyword> DeckKeyword::getParserKeyword() const {
        if (!m_parserKeyword)
            throw std::invalid_argument("No ParserKeyword object available");
        return m_parserKeyword;
    }

    bool DeckKeyword::hasParserKeyword() const {
        return static_cast<bool>(m_parserKeyword);
    }

    const std::string& DeckKeyword::getFileName() const {
        return m_fileName;
    }

    int DeckKeyword::getLineNumber() const {
        return m_lineNumber;
    }

    void DeckKeyword::setParserKeyword(std::shared_ptr<const ParserKeyword> &parserKeyword) {
        m_parserKeyword = parserKeyword;
    }

    void DeckKeyword::setDataKeyword(bool isDataKeyword_) {
        m_isDataKeyword = isDataKeyword_;
    }

    bool DeckKeyword::isDataKeyword() const {
        return m_isDataKeyword;
    }


    const std::string& DeckKeyword::name() const {
        return m_keywordName;
    }

    size_t DeckKeyword::size() const {
        return m_recordList.size();
    }

    bool DeckKeyword::isKnown() const {
        return m_knownKeyword;
    }

    void DeckKeyword::addRecord(DeckRecordConstPtr record) {
        m_recordList.push_back(record);
    }

    std::vector<DeckRecordConstPtr>::const_iterator DeckKeyword::begin() const {
        return m_recordList.begin();
    }

    std::vector<DeckRecordConstPtr>::const_iterator DeckKeyword::end() const {
        return m_recordList.end();
    }

    DeckRecordConstPtr DeckKeyword::getRecord(size_t index) const {
        if (index < m_recordList.size()) {
            return m_recordList[index];
        } else
            throw std::range_error("Index out of range");
    }


    DeckRecordConstPtr DeckKeyword::getDataRecord() const {
        if (m_recordList.size() == 1)
            return getRecord(0);
        else
            throw std::range_error("Not a data keyword ?");
    }


    size_t DeckKeyword::getDataSize() const {
        DeckRecordConstPtr record = getDataRecord();
        DeckItemConstPtr item = record->getDataItem();
        return item->size();
    }


    const std::vector<int>& DeckKeyword::getIntData() const {
        DeckRecordConstPtr record = getDataRecord();
        DeckItemConstPtr item = record->getDataItem();
        return item->getIntData();
    }


    const std::vector<std::string>& DeckKeyword::getStringData() const {
        DeckRecordConstPtr record = getDataRecord();
        DeckItemConstPtr item = record->getDataItem();
        return item->getStringData();
    }


    const std::vector<double>& DeckKeyword::getRawDoubleData() const {
        DeckRecordConstPtr record = getDataRecord();
        DeckItemConstPtr item = record->getDataItem();
        return item->getRawDoubleData();
    }

    const std::vector<double>& DeckKeyword::getSIDoubleData() const {
        DeckRecordConstPtr record = getDataRecord();
        DeckItemPtr item = record->getDataItem();
        return item->getSIDoubleData();
    }

}

