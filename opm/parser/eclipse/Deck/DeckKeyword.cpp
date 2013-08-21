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

namespace Opm {

    DeckKeyword::DeckKeyword(const std::string& keywordName) {
        m_knownKeyword = true;
        m_keywordName = keywordName;
    }

    std::string DeckKeyword::name() const {
        return m_keywordName;
    }

    size_t DeckKeyword::size() const {
        return m_recordList.size();
    }

    void DeckKeyword::setUnknown() {
        m_knownKeyword = false;
    }
    
    
    bool DeckKeyword::isKnown() const {
        return m_knownKeyword;
    }
    
    void DeckKeyword::addRecord(DeckRecordConstPtr record) {
        m_recordList.push_back(record);
    }

    DeckRecordConstPtr DeckKeyword::getRecord(size_t index) const {
        if (index < m_recordList.size()) {
            return m_recordList[index];
        } else
            throw std::range_error("Index out of range");
    }
}

