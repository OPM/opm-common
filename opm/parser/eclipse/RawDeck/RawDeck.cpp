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
#include <stdexcept>
#include "RawDeck.hpp"
#include "RawConsts.hpp"

namespace Opm {

    RawDeck::RawDeck(RawParserKWsConstPtr rawParserKWs) {
        m_rawParserKWs = rawParserKWs;
    }

    void RawDeck::addKeyword(RawKeywordConstPtr keyword) {
        m_keywords.push_back(keyword);
    }

    RawKeywordConstPtr RawDeck::getKeyword(size_t index) const {
        if (index < m_keywords.size())
            return m_keywords[index];
        else
            throw std::range_error("Index out of range");
    }

    size_t RawDeck::size() const {
        return m_keywords.size();
    }

    /// Checks if the current keyword being read is finished, based on the number of records
    /// specified for the current keyword type in the RawParserKW class.

    bool RawDeck::isKeywordFinished(RawKeywordConstPtr rawKeyword) {
        if (m_rawParserKWs->keywordExists(rawKeyword->getKeywordName())) {
            return rawKeyword->size() == m_rawParserKWs->getFixedNumberOfRecords(rawKeyword->getKeywordName());
        }
        return false;
    }

    /// Operator overload to write the content of the RawDeck to an ostream
    std::ostream& operator<<(std::ostream& os, const RawDeck& deck) {
        for (size_t i = 0; i < deck.size(); i++) {
            RawKeywordConstPtr keyword = deck.getKeyword(i);
            os << keyword->getKeywordName() << "                -- Keyword\n";
            for (size_t i = 0; i < keyword->size(); i++) {
                RawRecordConstPtr rawRecord = keyword->getRecord(i);
                for (size_t j = 0; j < rawRecord->size(); j++) {
                    os << rawRecord->getItem(j) << " ";
                }
                os << " /                -- Data\n";
            }
            os << "\n";
        }
        return os;
    }

    RawDeck::~RawDeck() {
    }
}
