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
#include <exception>
#include <algorithm>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

namespace Opm {
    Section::Section(DeckConstPtr deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords ) : m_name( startKeyword ) {
        populateKeywords(deck, startKeyword, stopKeywords);
    }

    void Section::populateKeywords(DeckConstPtr deck, const std::string& startKeyword,
                                   const std::vector<std::string>& stopKeywords)
    {
        size_t i;

        for (i=deck->getKeyword(startKeyword)->getDeckIndex(); i<deck->size(); i++) {
            if (std::find(stopKeywords.begin(), stopKeywords.end(), deck->getKeyword(i)->name()) != stopKeywords.end())
                break;
            m_keywords.addKeyword(deck->getKeyword(i));
        }
    }

    const std::string& Section::name() const {
        return m_name;
    }

    bool Section::hasKeyword( const std::string& keyword ) const {
        return m_keywords.hasKeyword(keyword);
    }

    std::vector<DeckKeywordPtr>::iterator Section::begin() {
        return m_keywords.begin();
    }

    std::vector<DeckKeywordPtr>::iterator Section::end() {
        return m_keywords.end();
    }

    DeckKeywordConstPtr Section::getKeyword(const std::string& keyword, size_t index) const {
        return m_keywords.getKeyword(keyword , index);
    }

    DeckKeywordConstPtr Section::getKeyword(const std::string& keyword) const {
        return m_keywords.getKeyword(keyword);
    }
    
    DeckKeywordConstPtr Section::getKeyword(size_t index) const {
        return m_keywords.getKeyword(index);
    }

    bool Section::hasSection(DeckConstPtr deck, const std::string& startKeyword) {
        return deck->hasKeyword(startKeyword);
    }
    
}
