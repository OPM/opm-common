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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

namespace {
    struct name_eq {
        name_eq( const std::string& x ) : name( x ) {}
        const std::string& name;
        bool operator()( const ParserItem& x ) const {
            return x.name() == this->name;
        }
    };
}

    ParserRecord::ParserRecord()
        : m_dataRecord( false )
    {
    }

    size_t ParserRecord::size() const {
        return m_items.size();
    }

    bool ParserRecord::slashTerminatedRecords() const {
        return this->slash_terminated_records;
    }

    void ParserRecord::addItem( ParserItem item ) {
        if (m_dataRecord)
            throw std::invalid_argument("Record is already marked as DataRecord - can not add items");

        auto itr = std::find_if( this->m_items.begin(),
                                 this->m_items.end(),
                                 name_eq( item.name() ) );

        if( itr != this->m_items.end() )
            throw std::invalid_argument("Itemname: " + item.name() + " already exists.");

        if (item.parseRaw())
            this->slash_terminated_records = false;

        this->m_items.push_back( std::move( item ) );
    }

    void ParserRecord::addDataItem( ParserItem item ) {
        if (m_items.size() > 0)
            throw std::invalid_argument("Record already contains items - can not add Data Item");

        this->addItem( std::move( item) );
        m_dataRecord = true;
    }



    std::vector< ParserItem >::const_iterator ParserRecord::begin() const {
        return m_items.begin();
    }


    std::vector< ParserItem >::const_iterator ParserRecord::end() const {
        return m_items.end();
    }


    bool ParserRecord::hasDimension() const {
        return std::any_of( this->begin(), this->end(),
                    []( const ParserItem& x ) { return x.hasDimension(); } );
    }



    void ParserRecord::applyUnitsToDeck( Deck& deck, DeckRecord& deckRecord ) const {
        for( const auto& parser_item : *this ) {
            if( !parser_item.hasDimension() ) continue;

            auto& deckItem = deckRecord.getItem( parser_item.name() );
            for (size_t idim = 0; idim < parser_item.numDimensions(); idim++) {
                auto activeDimension  = deck.getActiveUnitSystem().getNewDimension( parser_item.getDimension(idim) );
                auto defaultDimension = deck.getDefaultUnitSystem().getNewDimension( parser_item.getDimension(idim) );
                deckItem.push_backDimension( activeDimension , defaultDimension );
            }

            /*
              A little special casing ... the UDAValue elements in the deck must
              carry their own dimension. Observe that the
              ParserItem::setSizeType() method guarantees that UDA data is
              scalar, i.e. this special case loop can be simpler than the
              general code in the block above.
            */
            if (parser_item.dataType() == type_tag::uda && deckItem.size() > 0) {
                auto uda = deckItem.get<UDAValue>(0);
                if (deckItem.defaultApplied(0))
                    uda.set_dim( deck.getDefaultUnitSystem().getNewDimension( parser_item.getDimension(0)));
                else
                    uda.set_dim( deck.getActiveUnitSystem().getNewDimension( parser_item.getDimension(0)));
            }
        }
    }


    const ParserItem& ParserRecord::get(size_t index) const {
        return this->m_items.at( index );
    }

    bool ParserRecord::hasItem( const std::string& name ) const {
        return std::any_of( this->m_items.begin(),
                            this->m_items.end(),
                            name_eq( name ) );
    }

    const ParserItem& ParserRecord::get( const std::string& name ) const {
        auto itr = std::find_if( this->m_items.begin(),
                                 this->m_items.end(),
                                 name_eq( name ) );

        if( itr == this->m_items.end() )
            throw std::out_of_range( "No item '" + name + "'" );

        return *itr;
    }

    DeckRecord ParserRecord::parse(const ParseContext& parseContext , ErrorGuard& errors , RawRecord& rawRecord, const std::string& keyword, const std::string& filename) const {
        std::vector< DeckItem > items;
        items.reserve( this->size() + 20 );
        for( const auto& parserItem : *this )
            items.emplace_back( parserItem.scan( rawRecord ) );

        if (rawRecord.size() > 0) {
            std::string msg = "The RawRecord for keyword \""  + keyword + "\" in file\"" + filename + "\" contained " +
                std::to_string(rawRecord.size()) +
                " too many items according to the spec. RawRecord was: " + rawRecord.getRecordString();
            parseContext.handleError(ParseContext::PARSE_EXTRA_DATA , msg, errors);
        }

        return { std::move( items ) };
    }

    bool ParserRecord::equal(const ParserRecord& other) const {
        bool equal_ = true;
        if (size() == other.size()) {
           size_t itemIndex = 0;
           while (true) {
               if (itemIndex == size())
                   break;
               {
                   const auto& item = get(itemIndex);
                   const auto& otherItem = other.get(itemIndex);

                   if (item != otherItem ) {
                       equal_ = false;
                       break;
                   }
               }
               itemIndex++;
            }
        } else
            equal_ = false;
        return equal_;
    }

    bool ParserRecord::isDataRecord() const {
        return m_dataRecord;
    }

    bool ParserRecord::operator==( const ParserRecord& rhs ) const {
        return this->equal( rhs );
    }

    bool ParserRecord::operator!=( const ParserRecord& rhs ) const {
        return !( *this == rhs );
    }

    std::ostream& operator<<( std::ostream& stream, const ParserRecord& rec ) {
        stream << "  ParserRecord { " << std::endl;

        for( const auto& item : rec )
            stream << "      " << item << std::endl;

        return stream << "    }";
    }

}
