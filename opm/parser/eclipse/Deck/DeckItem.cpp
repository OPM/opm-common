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

#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>
#include <opm/parser/eclipse/Deck/DeckFloatItem.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdexcept>
#include <cassert>

namespace Opm {

    DeckItem::DeckItem(const std::string& name_, bool scalar) {
        m_name = name_;
        m_scalar = scalar;
    }

    const std::string& DeckItem::name() const {
        return m_name;
    }

    bool DeckItem::defaultApplied(size_t index) const {
        if (index >= m_dataPointDefaulted.size())
            throw std::out_of_range("Index must be smaller than "
                                    + boost::lexical_cast<std::string>(m_dataPointDefaulted.size())
                                    + " but is "
                                    + boost::lexical_cast<std::string>(index));

        return m_dataPointDefaulted[index];
    }

    void DeckItem::assertSize(size_t index) const {
        if (index >= size())
            throw std::out_of_range("Index must be smaller than "
                                    + boost::lexical_cast<std::string>(size())
                                    + " but is "
                                    + boost::lexical_cast<std::string>(index));
    }


    bool DeckItem::hasValue(size_t index) const {
        if (index < size())
            return true;
        else
            return false;
    }

    template< typename T >
    void DeckTypeItem< T >::push_back( std::deque< T > data, size_t items ) {
        if( m_dataPointDefaulted.size() != this->data.size() )
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for( size_t i = 0; i < items; i++ ) {
            this->data.push_back( data[ i ] );
            m_dataPointDefaulted.push_back( false );
        }
    }

    template< typename T >
    void DeckTypeItem< T >::push_back( std::deque< T > data ) {
        if( m_dataPointDefaulted.size() != this->data.size() )
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        this->push_back( data, data.size() );
        this->m_dataPointDefaulted.push_back( false );
    }

    template< typename T >
    void DeckTypeItem< T >::push_back( T data ) {
        if( m_dataPointDefaulted.size() != this->data.size() )
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        this->data.push_back( data );
        this->m_dataPointDefaulted.push_back( false );
    }

    template< typename T >
    void DeckTypeItem< T >::push_backDefault( T data ) {
        if( m_dataPointDefaulted.size() != this->data.size() )
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        this->data.push_back( data );
        this->m_dataPointDefaulted.push_back(true);
    }

    template< typename T >
    void DeckTypeItem< T >::push_backDummyDefault() {
        if( m_dataPointDefaulted.size() != 0 )
            throw std::logic_error("Pseudo defaults can only be specified for empty items");

        this->m_dataPointDefaulted.push_back( true );
    }

    template< typename T >
    void DeckTypeItem< T >::push_backMultiple( T value, size_t numValues ) {
        if( m_dataPointDefaulted.size() != this->data.size() )
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for( size_t i = 0; i < numValues; i++ ) {
            this->data.push_back( value );
            this->m_dataPointDefaulted.push_back(false);
        }
    }

    template< typename T >
    size_t DeckTypeItem< T >::size() const {
        return this->data.size();
    }

    template< typename T >
    const T& DeckTypeItem< T >::get( size_t index ) const {
        return this->data.at( index );
    }

    template< typename T >
    const std::vector< T >& DeckTypeItem< T >::getData() const {
        return this->data;
    }

    template< typename T >
    const T& DeckSIItem< T >::getSI( size_t index ) const {
        this->assertSIData();
        return this->SIdata.at( index );
    }

    template< typename T >
    const std::vector< T >& DeckSIItem< T >::getSIData() const {
        this->assertSIData();
        return this->SIdata;
    }

    template< typename T >
    void DeckSIItem< T >::push_backDimension(
                std::shared_ptr< const Dimension > activeDimension,
                std::shared_ptr< const Dimension > defaultDimension ) {

        if( this->m_dataPointDefaulted.empty() || this->m_dataPointDefaulted.back() )
            this->dimensions.push_back( defaultDimension );
        else
            this->dimensions.push_back( activeDimension );
    }

    template< typename T >
    void DeckSIItem< T >::assertSIData() const {
        if( this->dimensions.size() <= 0 )
            throw std::invalid_argument("No dimension has been set for item:" + this->name() + " can not ask for SI data");

        // we already converted this item to SI?
        if( this->SIdata.size() > 0 ) return;

        this->SIdata.resize( this->size() );

        for( size_t index = 0; index < this->size(); index++ ) {
            const auto dimIndex = ( index % dimensions.size() );
            this->SIdata[ index ] = this->dimensions[ dimIndex ]
                                          ->convertRawToSi( this->get( index ) );
        }
    }

    template class DeckTypeItem< double >;
    template class DeckTypeItem< float >;
    template class DeckTypeItem< int >;
    template class DeckTypeItem< std::string >;

    template class DeckSIItem< double >;
    template class DeckSIItem< float >;

    int DeckIntItem::getInt( size_t index ) const {
        return this->get( index );
    }

    const std::vector< int >& DeckIntItem::getIntData() const {
        return this->getData();
    }

    const std::string& DeckStringItem::getString( size_t index ) const {
        return this->get( index );
    }

    std::string DeckStringItem::getTrimmedString( size_t index ) const {
        return boost::algorithm::trim_copy( this->get( index ));
    }

    const std::vector< std::string >& DeckStringItem::getStringData() const {
        return this->getData();
    }

    double DeckDoubleItem::getRawDouble( size_t index ) const {
        return this->get( index );
    }

    const std::vector< double >& DeckDoubleItem::getRawDoubleData() const {
        return this->getData();
    }

    double DeckDoubleItem::getSIDouble( size_t index ) const {
        return this->getSI( index );
    }

    const std::vector< double >& DeckDoubleItem::getSIDoubleData() const {
        return this->getSIData();
    }

    float DeckFloatItem::getRawFloat( size_t index ) const {
        return this->get( index );
    }

    float DeckFloatItem::getSIFloat( size_t index ) const {
        return this->getSI( index );
    }

    const std::vector< float >& DeckFloatItem::getSIFloatData() const {
        return this->getSIData();
    }

}
