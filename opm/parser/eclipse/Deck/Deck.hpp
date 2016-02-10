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

#include <map>
#include <memory>
#include <vector>
#include <string>

namespace Opm {

    class DeckKeyword;
    class UnitSystem;

    class Deck {
    public:
        Deck();
        bool hasKeyword(std::shared_ptr< const DeckKeyword > keyword) const;
        bool hasKeyword( const std::string& keyword ) const;
        void addKeyword( std::shared_ptr< const DeckKeyword > keyword);
        std::shared_ptr< const DeckKeyword > getKeyword(const std::string& keyword , size_t index) const;
        std::shared_ptr< const DeckKeyword > getKeyword(const std::string& keyword) const;
        std::shared_ptr< const DeckKeyword > getKeyword(size_t index) const;

        size_t getKeywordIndex(std::shared_ptr< const DeckKeyword > keyword) const;


        size_t numKeywords(const std::string& keyword) const;
        const std::vector<std::shared_ptr< const DeckKeyword >>& getKeywordList(const std::string& keyword) const;
        size_t size() const;
        void initUnitSystem();
        std::shared_ptr<UnitSystem> getDefaultUnitSystem() const;
        std::shared_ptr<UnitSystem> getActiveUnitSystem()  const;
        std::vector<std::shared_ptr< const DeckKeyword >>::const_iterator begin() const;
        std::vector<std::shared_ptr< const DeckKeyword >>::const_iterator end() const;


        template <class Keyword>
        bool hasKeyword() const {
            return hasKeyword( Keyword::keywordName );
        }

        template <class Keyword>
        std::shared_ptr< const DeckKeyword > getKeyword(size_t index) const {
            return getkeyword( Keyword::keywordName , index );
        }

        template <class Keyword>
        std::shared_ptr< const DeckKeyword > getKeyword() const {
            return getKeyword( Keyword::keywordName );
        }

        template <class Keyword>
        const std::vector<std::shared_ptr< const DeckKeyword >>& getKeywordList() const {
            return getKeywordList( Keyword::keywordName );
        }

    protected:
        std::shared_ptr<UnitSystem> m_defaultUnits;
        std::shared_ptr<UnitSystem> m_activeUnits;

    private:
        std::vector<std::shared_ptr< const DeckKeyword >> m_emptyList;
        std::vector<std::shared_ptr< const DeckKeyword >> m_keywordList;
        std::map<std::string, std::vector<std::shared_ptr< const DeckKeyword >> > m_keywordMap;
        std::map<const DeckKeyword *, size_t> m_keywordIndex;
    };

    typedef std::shared_ptr<Deck> DeckPtr;
    typedef std::shared_ptr<const Deck> DeckConstPtr;
}
#endif  /* DECK_HPP */

