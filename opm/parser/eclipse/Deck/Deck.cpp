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

#include <vector>
#include <iostream>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

    Deck::Deck() {
    }

    bool Deck::hasKeyword(DeckKeywordConstPtr keyword) const {
        return (m_keywordIndex.find( const_cast<DeckKeyword *>(keyword.get()) ) != m_keywordIndex.end());
    }


    bool Deck::hasKeyword(const std::string& keyword) const {
        return (m_keywordMap.find(keyword) != m_keywordMap.end());
    }

    void Deck::addKeyword( DeckKeywordConstPtr keyword) {
        m_keywordIndex[ keyword.get() ] = m_keywordList.size();
        m_keywordList.push_back(keyword);

        if (!hasKeyword(keyword->name())) {
            m_keywordMap[keyword->name()] = std::vector<DeckKeywordConstPtr>();
        }

        {
            std::vector<DeckKeywordConstPtr>& keywordList = m_keywordMap[keyword->name()];
            keywordList.push_back(keyword);
        }
    }

    size_t Deck::getKeywordIndex(DeckKeywordConstPtr keyword) const {
        auto iter = m_keywordIndex.find( const_cast<DeckKeyword *>(keyword.get()));
        if (iter == m_keywordIndex.end())
            throw std::invalid_argument("Keyword " + keyword->name() + " not in deck.");

        return (*iter).second;
    }


    DeckKeywordConstPtr Deck::getKeyword(const std::string& keyword, size_t index) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordConstPtr>& keywordList = getKeywordList( keyword );
            if (index < keywordList.size())
                return keywordList[index];
            else
                throw std::out_of_range("Keyword " + keyword + ":" + std::to_string( index ) + " not in deck.");
        } else
            throw std::invalid_argument("Keyword " + keyword + " not in deck.");
    }

    DeckKeywordConstPtr Deck::getKeyword(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordConstPtr>& keywordList = getKeywordList( keyword );
            return keywordList.back();
        } else
            throw std::invalid_argument("Keyword " + keyword + " not in deck.");
    }

    DeckKeywordConstPtr Deck::getKeyword(size_t index) const {
        if (index < m_keywordList.size())
            return m_keywordList[index];
        else
            throw std::out_of_range("Keyword index " + std::to_string( index ) + " is out of range.");
    }

    size_t Deck::numKeywords(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordConstPtr>& keywordList = getKeywordList( keyword );
            return keywordList.size();
        } else
            return 0;
    }

    const std::vector<DeckKeywordConstPtr>& Deck::getKeywordList(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            const std::vector<DeckKeywordConstPtr>& keywordList = m_keywordMap.find(keyword)->second;
            return keywordList;
        } else
            return m_emptyList;
    }

    std::vector<DeckKeywordConstPtr>::const_iterator Deck::begin() const {
        return m_keywordList.begin();
    }

    std::vector<DeckKeywordConstPtr>::const_iterator Deck::end() const {
        return m_keywordList.end();
    }


    size_t Deck::size() const {
        return m_keywordList.size();
    }

    void Deck::initUnitSystem() {
        m_defaultUnits = std::shared_ptr<UnitSystem>( UnitSystem::newMETRIC() );
        if (hasKeyword("FIELD"))
            m_activeUnits = std::shared_ptr<UnitSystem>( UnitSystem::newFIELD() );
        else
            m_activeUnits = m_defaultUnits;
    }

    std::shared_ptr<UnitSystem> Deck::getActiveUnitSystem() const {
        return m_activeUnits;
    }


    std::shared_ptr<UnitSystem> Deck::getDefaultUnitSystem() const {
        return m_defaultUnits;
    }

}

