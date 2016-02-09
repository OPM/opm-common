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

    class DeckView {
        public:
            typedef std::vector<std::shared_ptr< const DeckKeyword >>::const_iterator const_iterator;

            bool hasKeyword( const DeckKeyword& keyword ) const;
            bool hasKeyword( const std::string& keyword ) const;
            template< class Keyword >
            bool hasKeyword() const {
                return hasKeyword( Keyword::keywordName );
            }

            std::shared_ptr< const DeckKeyword > getKeyword( const std::string& keyword, size_t index ) const;
            std::shared_ptr< const DeckKeyword > getKeyword( const std::string& keyword ) const;
            std::shared_ptr< const DeckKeyword > getKeyword( size_t index ) const;
            template< class Keyword >
            std::shared_ptr< const DeckKeyword > getKeyword() const {
                return getKeyword( Keyword::keywordName );
            }
            template< class Keyword >
            std::shared_ptr< const DeckKeyword > getKeyword( size_t index ) const {
                return getKeyword( Keyword::keywordName, index );
            }

            const std::vector< std::shared_ptr< const DeckKeyword > >& getKeywordList( const std::string& keyword ) const;
            template< class Keyword >
            const std::vector< std::shared_ptr< const DeckKeyword > >& getKeywordList() const {
                return getKeywordList( Keyword::keywordName );
            }

            size_t count(const std::string& keyword) const;
            size_t size() const;

            const_iterator begin() const;
            const_iterator end() const;

        protected:
            void add( std::shared_ptr< const DeckKeyword >, const_iterator, const_iterator );

            DeckView() = default;
            DeckView( const_iterator first, const_iterator last );
            DeckView( std::pair< const_iterator, const_iterator > );

        private:
            const_iterator first;
            const_iterator last;
            std::map< std::string, std::vector< std::shared_ptr< const DeckKeyword > > > keywordMap;

    };

    class Deck : private DeckView {
        public:
            using DeckView::const_iterator;
            using DeckView::hasKeyword;
            using DeckView::getKeyword;
            using DeckView::getKeywordList;
            using DeckView::count;
            using DeckView::size;
            using DeckView::begin;
            using DeckView::end;

            void addKeyword( std::shared_ptr< const DeckKeyword > keyword );

            void initUnitSystem();
            std::shared_ptr< UnitSystem > getDefaultUnitSystem() const;
            std::shared_ptr< UnitSystem > getActiveUnitSystem()  const;

        private:
            std::vector< std::shared_ptr< const DeckKeyword > > keywordList;
            std::shared_ptr<UnitSystem> m_activeUnits;
    };

    typedef std::shared_ptr<Deck> DeckPtr;
    typedef std::shared_ptr<const Deck> DeckConstPtr;
}
#endif  /* DECK_HPP */

