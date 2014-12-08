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
#include <string>

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

    void KeywordContainer::addKeyword(DeckKeywordPtr keyword) {
        keyword->setDeckIndex( m_keywordList.size());
        m_keywordList.push_back(keyword);

        if (!hasKeyword(keyword->name())) {
            m_keywordMap[keyword->name()] = std::vector<DeckKeywordPtr>();
        }

        {
            std::vector<DeckKeywordPtr>& keywordList = m_keywordMap[keyword->name()];
            keywordList.push_back(keyword);
        }
    }

    const std::vector<DeckKeywordPtr>&  KeywordContainer::getKeywordList(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordPtr>& keywordList = m_keywordMap.find(keyword)->second;
            return keywordList;
        } else
            throw std::invalid_argument("Keyword: " + keyword + " is not found in the container");
    }


    DeckKeywordPtr KeywordContainer::getKeyword(const std::string& keyword, size_t index) const {
        const std::vector<DeckKeywordPtr>& keywordList = getKeywordList( keyword );
        if (index < keywordList.size())
            return keywordList[index];
        else
            throw std::out_of_range("Keyword index is out of range.");
    }


    DeckKeywordPtr KeywordContainer::getKeyword(const std::string& keyword) const {
        const std::vector<DeckKeywordPtr>& keywordList = getKeywordList( keyword );
        return keywordList.back();
    }

    DeckKeywordPtr KeywordContainer::getKeyword(size_t index) const {
        if (index < m_keywordList.size())
            return m_keywordList[index];
        else
            throw std::out_of_range("Keyword index is out of range.");
    }

    size_t KeywordContainer::numKeywords(const std::string& keyword) const{
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordPtr>& keywordList = getKeywordList( keyword );
            return keywordList.size();
        } else
            return 0;
    }

    std::vector<DeckKeywordPtr>::iterator KeywordContainer::begin() {
        return m_keywordList.begin();
    }

    std::vector<DeckKeywordPtr>::iterator KeywordContainer::end() {
        return m_keywordList.end();
    }
}
