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

#ifndef DECKITEM_HPP
#define DECKITEM_HPP

#include <string>
#include <vector>
#include <memory>

#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Utility/Typetools.hpp>

namespace Opm {

    class DeckItem {
    public:
        DeckItem() = default;
        DeckItem( const std::string& );

        DeckItem( const std::string&, int, size_t size_hint = 8 );
        DeckItem( const std::string&, double, size_t size_hint = 8 );
        DeckItem( const std::string&, std::string, size_t size_hint = 8 );

        const std::string& name() const;

        // return true if the default value was used for a given data point
        bool defaultApplied( size_t ) const;

        // Return true if the item has a value for the current index;
        // does not differentiate between default values from the
        // config and values which have been set in the deck.
        bool hasValue( size_t ) const;

        // if the number returned by this method is less than what is semantically
        // expected (e.g. size() is less than the number of cells in the grid for
        // keywords like e.g. SGL), then the remaining values are defaulted. The deck
        // creates the defaulted items if all their sizes are fully specified by the
        // keyword, though...
        size_t size() const;

        template< typename T > const T& get( size_t ) const;
        double getSIDouble( size_t ) const;
        std::string getTrimmedString( size_t ) const;

        template< typename T > const std::vector< T >& getData() const;
        const std::vector< double >& getSIDoubleData() const;

        void push_back( int, size_t = 1 );
        void push_back( double, size_t = 1 );
        void push_back( std::string, size_t = 1 );
        void push_backDefault( int );
        void push_backDefault( double );
        void push_backDefault( std::string );
        // trying to access the data of a "dummy default item" will raise an exception
        void push_backDummyDefault();

        void push_backDimension( const Dimension& /* activeDimension */,
                                 const Dimension& /* defaultDimension */);

        type_tag getType() const;

    private:
        std::vector< double > dval;
        std::vector< int > ival;
        std::vector< std::string > sval;

        type_tag type = type_tag::unknown;

        std::string item_name;
        std::vector< bool > defaulted;
        std::vector< Dimension > dimensions;
        mutable std::vector< double > SIdata;

        template< typename T > std::vector< T >& value_ref();
        template< typename T > const std::vector< T >& value_ref() const;
        template< typename T > void push( T, size_t );
        template< typename T > void push_default( T );
    };
}
#endif  /* DECKITEM_HPP */

