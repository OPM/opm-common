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

#ifndef DECK_HPP
#define DECK_HPP

#include <vector>
#include <memory>

//#include <opm/parser/eclipse/Deck/KeywordContainer.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

    class Deck {
    public:
        Deck();
        bool hasKeyword(DeckKeywordConstPtr keyword) const;
        bool hasKeyword( const std::string& keyword ) const;
        void addKeyword( DeckKeywordConstPtr keyword);
        DeckKeywordConstPtr getKeyword(const std::string& keyword , size_t index) const;
        DeckKeywordConstPtr getKeyword(const std::string& keyword) const;
        DeckKeywordConstPtr getKeyword(size_t index) const;

        size_t getKeywordIndex(DeckKeywordConstPtr keyword) const;


        size_t numKeywords(const std::string& keyword) const;
        const std::vector<DeckKeywordConstPtr>& getKeywordList(const std::string& keyword) const;
        size_t size() const;
        void initUnitSystem();
        std::shared_ptr<UnitSystem> getDefaultUnitSystem() const;
        std::shared_ptr<UnitSystem> getActiveUnitSystem()  const;
        std::vector<DeckKeywordConstPtr>::const_iterator begin() const;
        std::vector<DeckKeywordConstPtr>::const_iterator end() const;

    private:
        std::shared_ptr<UnitSystem> m_defaultUnits;
        std::shared_ptr<UnitSystem> m_activeUnits;

        std::vector<DeckKeywordConstPtr> m_keywordList;
        std::map<std::string, std::vector<DeckKeywordConstPtr> > m_keywordMap;
        std::map<const DeckKeyword *, size_t> m_keywordIndex;
    };

    typedef std::shared_ptr<Deck> DeckPtr;
    typedef std::shared_ptr<const Deck> DeckConstPtr;
}
#endif  /* DECK_HPP */

