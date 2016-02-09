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

#include <algorithm>
#include <vector>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

    bool DeckView::hasKeyword( const DeckKeyword& keyword ) const {
        auto key = this->keywordMap.find( keyword.name() );

        if( key == this->keywordMap.end() ) return false;

        for( auto& ptr : key->second )
            if( ptr.get() == &keyword ) return true;

        return false;
    }

    bool DeckView::hasKeyword( const std::string& keyword ) const {
        return this->keywordMap.find( keyword ) != this->keywordMap.end();
    }

    std::shared_ptr< const DeckKeyword > DeckView::getKeyword( const std::string& keyword, size_t index ) const {
        if( !this->hasKeyword( keyword ) )
            throw std::invalid_argument("Keyword " + keyword + " not in deck.");

        return this->getKeywordList( keyword ).at( index );
    }

    std::shared_ptr< const DeckKeyword > DeckView::getKeyword( const std::string& keyword ) const {
        if( !this->hasKeyword( keyword ) )
            throw std::invalid_argument("Keyword " + keyword + " not in deck.");

        return this->getKeywordList( keyword ).back();
    }

    std::shared_ptr< const DeckKeyword > DeckView::getKeyword( size_t index ) const {
        if( index >= this->size() )
            throw std::out_of_range("Keyword index " + std::to_string( index ) + " is out of range.");

        return *( this->begin() + index );
    }

    size_t DeckView::count( const std::string& keyword ) const {
        if( !this->hasKeyword( keyword ) ) return 0;

        return this->getKeywordList( keyword ).size();
   }

    static std::vector< std::shared_ptr< const DeckKeyword > > empty_kw_list;

    const std::vector< std::shared_ptr< const DeckKeyword > >& DeckView::getKeywordList( const std::string& keyword ) const {
        if( !hasKeyword( keyword ) ) return empty_kw_list;

        return this->keywordMap.find( keyword )->second;
    }

    size_t DeckView::size() const {
        return std::distance( this->begin(), this->end() );
    }

    DeckView::const_iterator DeckView::begin() const {
        return this->first;
    }

    DeckView::const_iterator DeckView::end() const {
        return this->last;
    }

    DeckView::DeckView( const_iterator first, const_iterator last ) :
        first( first ), last( last )
    {
        for( auto itr = this->first; itr != this->last; ++itr )
            this->keywordMap[ (*itr)->name() ].push_back( *itr );
    }

    DeckView::DeckView( std::pair< const_iterator, const_iterator > limits ) :
        DeckView( limits.first, limits.second )
    {}

    void DeckView::add( std::shared_ptr< const DeckKeyword > kw, const_iterator f, const_iterator l ) {
        this->keywordMap[ kw->name() ].push_back( kw );
        this->first = f;
        this->last = l;
    }

    void Deck::addKeyword( std::shared_ptr< const DeckKeyword > keyword ) {
        this->keywordList.push_back( keyword );

        auto first = this->keywordList.begin();
        auto last = this->keywordList.end();

        this->add( this->keywordList.back(), first, last );
    }

    void Deck::initUnitSystem() {
        if (hasKeyword("FIELD"))
            m_activeUnits = std::shared_ptr<UnitSystem>( UnitSystem::newFIELD() );
        else
            m_activeUnits = this->getDefaultUnitSystem();
    }

    std::shared_ptr< UnitSystem > Deck::getDefaultUnitSystem() const {
        return std::shared_ptr<UnitSystem>( UnitSystem::newMETRIC() );
    }

    std::shared_ptr< UnitSystem > Deck::getActiveUnitSystem() const {
        return this->m_activeUnits;
    }


}
