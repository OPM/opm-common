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

#include <map>
#include <opm/parser/eclipse/Deck/KeywordContainer.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

namespace Opm {

    KeywordContainer::KeywordContainer() {
    }

    bool KeywordContainer::hasKeyword(const std::string& keyword) const {
        return (m_keywordMap.find(keyword) != m_keywordMap.end());
    }

    size_t KeywordContainer::size() const {
        return m_keywordList.size();
    }
    
    void KeywordContainer::addKeyword(DeckKeywordConstPtr keyword) {
        m_keywordList.push_back(keyword);
        
        if (!hasKeyword(keyword->name())) {
            m_keywordMap[keyword->name()] = std::vector<DeckKeywordConstPtr>();
        }
        
        {
            std::vector<DeckKeywordConstPtr>& keywordList = m_keywordMap[keyword->name()];
            keywordList.push_back(keyword);
        }
    }
    
    DeckKeywordConstPtr KeywordContainer::getKeyword(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordConstPtr>& keywordList = m_keywordMap.find(keyword)->second;
            return keywordList.back();
        }
        else
            throw std::invalid_argument("Keyword: " + keyword + " is not found in the container");
    }


}
