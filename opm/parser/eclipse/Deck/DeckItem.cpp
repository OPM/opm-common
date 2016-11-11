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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>

#include <boost/algorithm/string.hpp>

#include <stdexcept>

namespace Opm {

template< typename T >
std::vector< T >& DeckItem::value_ref() {
    return const_cast< std::vector< T >& >(
            const_cast< const DeckItem& >( *this ).value_ref< T >()
         );
}

template<>
const std::vector< int >& DeckItem::value_ref< int >() const {
    if( this->type != get_type< int >() )
        throw std::invalid_argument( "Item of wrong type." );

    return this->ival;
}

template<>
const std::vector< double >& DeckItem::value_ref< double >() const {
    if( this->type != get_type< double >() )
        throw std::invalid_argument( "Item of wrong type." );

    return this->dval;
}

template<>
const std::vector< std::string >& DeckItem::value_ref< std::string >() const {
    if( this->type != get_type< std::string >() )
        throw std::invalid_argument( "Item of wrong type." );

    return this->sval;
}

DeckItem::DeckItem( const std::string& nm ) : item_name( nm ) {}

DeckItem::DeckItem( const std::string& nm, int, size_t hint ) :
    type( get_type< int >() ),
    item_name( nm )
{
    this->ival.reserve( hint );
    this->defaulted.reserve( hint );
}

DeckItem::DeckItem( const std::string& nm, double, size_t hint ) :
    type( get_type< double >() ),
    item_name( nm )
{
    this->dval.reserve( hint );
    this->defaulted.reserve( hint );
}

DeckItem::DeckItem( const std::string& nm, std::string, size_t hint ) :
    type( get_type< std::string >() ),
    item_name( nm )
{
    this->sval.reserve( hint );
    this->defaulted.reserve( hint );
}

const std::string& DeckItem::name() const {
    return this->item_name;
}

bool DeckItem::defaultApplied( size_t index ) const {
    return this->defaulted.at( index );
}

bool DeckItem::hasValue( size_t index ) const {
    switch( this->type ) {
        case type_tag::integer: return this->ival.size() > index;
        case type_tag::fdouble: return this->dval.size() > index;
        case type_tag::string:  return this->sval.size() > index;
        default: throw std::logic_error( "Type not set." );
    }
}

size_t DeckItem::size() const {
    switch( this->type ) {
        case type_tag::integer: return this->ival.size();
        case type_tag::fdouble: return this->dval.size();
        case type_tag::string:  return this->sval.size();
        default: throw std::logic_error( "Type not set." );
    }
}

template< typename T >
const T& DeckItem::get( size_t index ) const {
    return this->value_ref< T >().at( index );
}

template< typename T >
const std::vector< T >& DeckItem::getData() const {
    return this->value_ref< T >();
}

template< typename T >
void DeckItem::push( T x, size_t n ) {
    auto& val = this->value_ref< T >();

    val.insert( val.end(), n, x );
    this->defaulted.insert( this->defaulted.end(), n, false );
}

void DeckItem::push_back( int x, size_t n ) {
    this->push( x, n );
}

void DeckItem::push_back( double x, size_t n ) {
    this->push( x, n );
}

void DeckItem::push_back( std::string x, size_t n ) {
    this->push( std::move( x ), n );
}

template< typename T >
void DeckItem::push_default( T x ) {
    auto& val = this->value_ref< T >();
    if( this->defaulted.size() != val.size() )
        throw std::logic_error("To add a value to an item, "
                "no 'pseudo defaults' can be added before");

    val.push_back( std::move( x ) );
    this->defaulted.push_back( true );
}

void DeckItem::push_backDefault( int x ) {
    this->push_default( x );
}

void DeckItem::push_backDefault( double x ) {
    this->push_default( x );
}

void DeckItem::push_backDefault( std::string x ) {
    this->push_default( std::move( x ) );
}


void DeckItem::push_backDummyDefault() {
    if( !this->defaulted.empty() )
        throw std::logic_error("Pseudo defaults can only be specified for empty items");

    this->defaulted.push_back( true );
}

std::string DeckItem::getTrimmedString( size_t index ) const {
    return boost::algorithm::trim_copy(
               this->value_ref< std::string >().at( index )
           );
}

double DeckItem::getSIDouble( size_t index ) const {
    return this->getSIDoubleData().at( index );
}

const std::vector< double >& DeckItem::getSIDoubleData() const {
    const auto& raw = this->value_ref< double >();
    // we already converted this item to SI?
    if( !this->SIdata.empty() ) return this->SIdata;

    if( this->dimensions.empty() )
        throw std::invalid_argument("No dimension has been set for item'"
                                    + this->name()
                                    + "'; can not ask for SI data");

    /*
     * This is an unobservable state change - SIData is lazily converted to
     * SI units, so externally the object still behaves as const
     */
    const auto dim_size = dimensions.size();
    const auto sz = raw.size();
    this->SIdata.resize( sz );

    for( size_t index = 0; index < sz; index++ ) {
        const auto dimIndex = index % dim_size;
        this->SIdata[ index ] = this->dimensions[ dimIndex ]
                                .convertRawToSi( raw[ index ] );
    }

    return this->SIdata;
}

void DeckItem::push_backDimension( const Dimension& active,
                                    const Dimension& def ) {
    const auto& ds = this->value_ref< double >();
    const bool dim_inactive = ds.empty()
                            || this->defaultApplied( ds.size() - 1 );

    this->dimensions.push_back( dim_inactive ? def : active );
}

type_tag DeckItem::getType() const {
    return this->type;
}

/*
 * Explicit template instantiations. These must be manually maintained and
 * updated with changes in DeckItem so that code is emitted.
 */

template const int& DeckItem::get< int >( size_t ) const;
template const double& DeckItem::get< double >( size_t ) const;
template const std::string& DeckItem::get< std::string >( size_t ) const;

template const std::vector< int >& DeckItem::getData< int >() const;
template const std::vector< double >& DeckItem::getData< double >() const;
template const std::vector< std::string >& DeckItem::getData< std::string >() const;

}
