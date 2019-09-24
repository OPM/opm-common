/*
  Copyright 2014 Statoil ASA.

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

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {


namespace {

template<typename T>
void set_element(const std::size_t i,
                 std::vector<T>& data,
                 std::vector<bool>& defaulted,
                 const T value,
                 const bool def = false) {
    data[i] = value;
    defaulted[i] = def;
}



}


    template< typename T >
    static std::function< std::vector< T >( size_t ) > constant( T val ) {
        return [=]( size_t size ) { return std::vector< T >( size, val ); };
    }

    template< typename T >
    static std::function< void( const std::vector<bool>&, std::vector< T >& ) > noop() {
        return []( const std::vector<bool>&, std::vector< T >& ) { return; };
    }

    template< typename T >
    GridPropertySupportedKeywordInfo< T >::GridPropertySupportedKeywordInfo(
            const std::string& name,
            std::function< std::vector< T >( size_t ) > init,
            std::function< void( const std::vector<bool>& defaulted, std::vector< T >& ) > post,
            const std::string& dimString,
            bool defaultInitializable ) :
        m_keywordName( name ),
        m_initializer( init ),
        m_postProcessor( post ),
        m_dimensionString( dimString ),
        m_defaultInitializable ( defaultInitializable )
    {}

    template< typename T >
    GridPropertySupportedKeywordInfo< T >::GridPropertySupportedKeywordInfo(
            const std::string& name,
            std::function< std::vector< T >( size_t ) > init,
            const std::string& dimString,
            bool defaultInitializable ) :
        m_keywordName( name ),
        m_initializer( init ),
        m_postProcessor( noop< T >() ),
        m_dimensionString( dimString ),
        m_defaultInitializable ( defaultInitializable )
    {}

    template< typename T >
    GridPropertySupportedKeywordInfo< T >::GridPropertySupportedKeywordInfo(
            const std::string& name,
            const T defaultValue,
            const std::string& dimString,
            bool defaultInitializable ) :
        m_keywordName( name ),
        m_initializer( constant( defaultValue ) ),
        m_postProcessor( noop< T >() ),
        m_dimensionString( dimString ),
        m_defaultInitializable ( defaultInitializable )
    {}

    template< typename T >
    GridPropertySupportedKeywordInfo< T >::GridPropertySupportedKeywordInfo(
            const std::string& name,
            const T defaultValue,
            std::function< void( const std::vector<bool>&, std::vector< T >& ) > post,
            const std::string& dimString,
            bool defaultInitializable ) :
        m_keywordName( name ),
        m_initializer( constant( defaultValue ) ),
        m_postProcessor( post ),
        m_dimensionString( dimString ),
        m_defaultInitializable ( defaultInitializable )
    {}

    template< typename T >
    const std::string& GridPropertySupportedKeywordInfo< T >::getKeywordName() const {
        return this->m_keywordName;
    }

    template< typename T >
    const std::string& GridPropertySupportedKeywordInfo< T >::getDimensionString() const {
        return this->m_dimensionString;
    }

    template< typename T >
    const std::function< std::vector< T >( size_t ) >& GridPropertySupportedKeywordInfo< T >::initializer() const {
        return this->m_initializer;
    }

    template< typename T >
    const std::function< void( const std::vector<bool>&, std::vector< T >& ) >&
    GridPropertySupportedKeywordInfo< T >::postProcessor() const
    {
        return this->m_postProcessor;
    }

    template<typename T>
    bool GridPropertySupportedKeywordInfo< T >::isDefaultInitializable() const {
        return m_defaultInitializable;
    }

    template< typename T >
    GridProperty< T >::GridProperty( size_t nx, size_t ny, size_t nz, const SupportedKeywordInfo& kwInfo ) :
        m_nx( nx ),
        m_ny( ny ),
        m_nz( nz ),
        m_kwInfo( kwInfo ),
        m_data(nx*ny*nz),
        m_defaulted( nx * ny * nz ),
        m_hasRunPostProcessor( false )
    {
        m_data.assign( kwInfo.initializer()(nx*ny*nz));
        m_defaulted.assign( std::vector<bool>(nx*ny*nz, true));
    }

    template< typename T >
    size_t GridProperty< T >::getCartesianSize() const {
        return m_data.size();
    }

    template< typename T >
    size_t GridProperty< T >::getNX() const {
        return m_nx;
    }


    template< typename T >
    size_t GridProperty< T >::getNY() const {
        return m_ny;
    }

    template< typename T >
    size_t GridProperty< T >::getNZ() const {
        return m_nz;
    }

    template< typename T >
    bool GridProperty<T>::deckAssigned() const {
        return this->assigned;
    }

    template< typename T >
    std::vector< bool > GridProperty< T >::wasDefaulted() const {
        return this->m_defaulted.data();
    }

    template< typename T >
    std::vector< T > GridProperty< T >::getData() const {
        return m_data.data();
    }

    template< typename T >
    void GridProperty< T >::assignData(const std::vector<T>& data) {
        this->m_data.assign(data);
    }


    template< typename T >
    void GridProperty< T >::multiplyWith( const GridProperty< T >& other ) {
        if ((m_nx == other.m_nx) && (m_ny == other.m_ny) && (m_nz == other.m_nz)) {
            auto data = this->m_data.data();
            auto other_data = other.m_data.data();
            for (size_t g=0; g < data.size(); g++)
                data[g] *= other_data[g];

            this->m_data.assign(data);
        } else
            throw std::invalid_argument("Size mismatch between properties in mulitplyWith.");
    }


    template< typename T >
    void GridProperty< T >::multiplyValueAtIndex(size_t index, T factor) {
        auto data = this->m_data.data();
        data[index] *= factor;
        this->m_data.assign(data);
    }



    template< typename T >
    void GridProperty< T >::maskedSet( T value, const std::vector< bool >& mask ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        for (size_t g = 0; g < getCartesianSize(); g++) {
            if (mask[g])
                set_element(g, data, defaulted, value);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
        this->assigned = true;
    }

    template< typename T >
    void GridProperty< T >::maskedMultiply( T value, const std::vector<bool>& mask ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        for (size_t g = 0; g < getCartesianSize(); g++) {
            if (mask[g])
                set_element(g, data, defaulted, value * data[g]);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::maskedAdd( T value, const std::vector<bool>& mask ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        for (size_t g = 0; g < getCartesianSize(); g++) {
            if (mask[g])
                set_element(g, data, defaulted, value + data[g]);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::maskedCopy( const GridProperty< T >& other, const std::vector< bool >& mask) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();
        const auto other_data = other.m_data.data();
        const auto other_defaulted = other.m_defaulted.data();

        for (size_t g = 0; g < getCartesianSize(); g++) {
            if (mask[g])
                set_element(g, data, defaulted, other_data[g], other_defaulted[g]);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
        this->assigned = other.deckAssigned();
    }

    template< typename T >
    void GridProperty< T >::initMask( T value, std::vector< bool >& mask ) const {
        const auto data = this->m_data.data();
        mask.resize(getCartesianSize());
        for (size_t g = 0; g < getCartesianSize(); g++) {
            if (data[g] == value)
                mask[g] = true;
            else
                mask[g] = false;
        }
    }

    template<>
    void GridProperty<int>::loadFromDeckKeyword( const DeckKeyword& deckKeyword ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();
        const auto& deckItem = getDeckItem(deckKeyword);
        const auto size = deckItem.size();
        const auto& deck_data = deckItem.getData<int>();

        for (size_t dataPointIdx = 0; dataPointIdx < size; ++dataPointIdx) {
            if (!deckItem.defaultApplied(dataPointIdx))
                set_element(dataPointIdx, data, defaulted, deck_data[dataPointIdx]);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
        this->assigned = true;
    }


    template<>
    void GridProperty<double>::loadFromDeckKeyword( const DeckKeyword& deckKeyword ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();
        const auto& deckItem = getDeckItem(deckKeyword);
        const auto size = deckItem.size();
        const auto& deck_data = deckItem.getSIDoubleData();

        for (size_t dataPointIdx = 0; dataPointIdx < size; ++dataPointIdx) {
            if (!deckItem.defaultApplied(dataPointIdx))
                set_element(dataPointIdx, data, defaulted, deck_data[dataPointIdx]);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
        this->assigned = true;
    }


    template<>
    void GridProperty<int>::loadFromDeckKeyword( const Box& inputBox, const DeckKeyword& deckKeyword) {
        if (inputBox.isGlobal())
            loadFromDeckKeyword( deckKeyword );
        else {
            auto data = this->m_data.data();
            auto defaulted = this->m_defaulted.data();
            const auto& deckItem = getDeckItem(deckKeyword);
            const auto& deck_data = deckItem.getData<int>();
            const std::vector<size_t>& indexList = inputBox.getIndexList();
            if (indexList.size() == deckItem.size()) {
                for (size_t sourceIdx = 0; sourceIdx < indexList.size(); sourceIdx++) {
                    size_t targetIdx = indexList[sourceIdx];
                    if (sourceIdx < deckItem.size()
                        && !deckItem.defaultApplied(sourceIdx))
                        {
                            set_element(targetIdx, data, defaulted, deck_data[sourceIdx]);
                        }
                }
            } else {
                std::string boxSize = std::to_string(static_cast<long long>(indexList.size()));
                std::string keywordSize = std::to_string(static_cast<long long>(deckItem.size()));

                throw std::invalid_argument("Size mismatch: Box:" + boxSize + "  DeckKeyword:" + keywordSize);
            }
            this->m_data.assign(data);
            this->m_defaulted.assign(defaulted);
        }
    }

    template<>
    void GridProperty<double>::loadFromDeckKeyword( const Box& inputBox, const DeckKeyword& deckKeyword) {
        if (inputBox.isGlobal())
            loadFromDeckKeyword( deckKeyword );
        else {
            auto data = this->m_data.data();
            auto defaulted = this->m_defaulted.data();
            const auto& deckItem = getDeckItem(deckKeyword);
            const auto& deck_data = deckItem.getSIDoubleData();
            const std::vector<size_t>& indexList = inputBox.getIndexList();
            if (indexList.size() == deckItem.size()) {
                for (size_t sourceIdx = 0; sourceIdx < indexList.size(); sourceIdx++) {
                    size_t targetIdx = indexList[sourceIdx];
                    if (sourceIdx < deckItem.size()
                        && !deckItem.defaultApplied(sourceIdx))
                    {
                        set_element(targetIdx, data, defaulted, deck_data[sourceIdx]);
                    }
                }
            } else {
                std::string boxSize = std::to_string(static_cast<long long>(indexList.size()));
                std::string keywordSize = std::to_string(static_cast<long long>(deckItem.size()));

                throw std::invalid_argument("Size mismatch: Box:" + boxSize + "  DeckKeyword:" + keywordSize);
            }
            this->m_data.assign(data);
            this->m_defaulted.assign(defaulted);
        }
    }

    template< typename T >
    void GridProperty< T >::copyFrom( const GridProperty< T >& src, const Box& inputBox ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();
        const auto other_data = src.m_data.data();
        const auto other_defaulted = src.m_defaulted.data();

        if (inputBox.isGlobal()) {
            for (size_t i = 0; i < src.getCartesianSize(); ++i)
                set_element(i, data, defaulted, other_data[i], other_defaulted[i]);
        } else {
            for (const auto& i : inputBox.getIndexList())
                set_element(i, data, defaulted, other_data[i], other_defaulted[i]);
        }

        this->assigned = src.deckAssigned();
        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::maxvalue( T value, const Box& inputBox ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        if (inputBox.isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                set_element(i, data, defaulted, std::min(value, data[i]));
        } else {
            for (const auto& i : inputBox.getIndexList())
                set_element(i, data, defaulted, std::min(value, data[i]));
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::minvalue( T value, const Box& inputBox ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        if (inputBox.isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                set_element(i, data, defaulted, std::max(value, data[i]));
        } else {
            for (const auto& i : inputBox.getIndexList())
                set_element(i, data, defaulted, std::max(value, data[i]));
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::scale( T scaleFactor, const Box& inputBox ) {
        auto data = this->m_data.data();
        if (inputBox.isGlobal()) {
            for (size_t i = 0; i < data.size(); ++i)
                data[i] *= scaleFactor;
        } else {
            const std::vector<size_t>& indexList = inputBox.getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                data[targetIndex] *= scaleFactor;
            }
        }
        this->m_data.assign(data);
    }

    template< typename T >
    void GridProperty< T >::add( T shiftValue, const Box& inputBox ) {
        auto data = this->m_data.data();
        if (inputBox.isGlobal()) {
            for (size_t i = 0; i < data.size(); ++i)
                data[i] += shiftValue;
        } else {
            const std::vector<size_t>& indexList = inputBox.getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                data[targetIndex] += shiftValue;
            }
        }
        this->m_data.assign(data);
    }

    template< typename T >
    void GridProperty< T >::setScalar( T value, const Box& inputBox ) {
        auto data = this->m_data.data();
        auto defaulted = this->m_defaulted.data();

        if (inputBox.isGlobal()) {
            std::fill(data.begin(), data.end(), value);
            defaulted.assign(defaulted.size(), false);
        } else {
            for (const auto& i : inputBox.getIndexList())
                set_element(i, data, defaulted, value);
        }

        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
        this->assigned = true;
    }

    template< typename T >
    const std::string& GridProperty< T >::getKeywordName() const {
        return m_kwInfo.getKeywordName();
    }

    template< typename T >
    const typename GridProperty< T >::SupportedKeywordInfo&
    GridProperty< T >::getKeywordInfo() const {
        return m_kwInfo;
    }

    template< typename T >
    void GridProperty< T >::runPostProcessor() {
        if( this->m_hasRunPostProcessor ) return;
        this->m_hasRunPostProcessor = true;

        auto defaulted = this->m_defaulted.data();
        auto data = this->m_data.data();
        this->m_kwInfo.postProcessor()( defaulted, data );
        this->m_data.assign(data);
        this->m_defaulted.assign(defaulted);
    }

    template< typename T >
    void GridProperty< T >::checkLimits( T min, T max ) const {
        auto data = this->m_data.data();
        for (size_t g=0; g < data.size(); g++) {
            T value = data[g];
            if ((value < min) || (value > max))
                throw std::invalid_argument("Property element " + std::to_string( value) + " in " + getKeywordName() + " outside valid limits: [" + std::to_string(min) + ", " + std::to_string(max) + "]");
        }
    }

    template< typename T  >
    const DeckItem& GridProperty< T >::getDeckItem( const DeckKeyword& deckKeyword ) {
        if (deckKeyword.size() != 1)
            throw std::invalid_argument("Grid properties can only have a single record (keyword "
                                        + deckKeyword.name() + ")");
        if (deckKeyword.getRecord(0).size() != 1)
            // this is an error of the definition of the ParserKeyword (most likely in
            // the corresponding JSON file)
            throw std::invalid_argument("Grid properties may only exhibit a single item  (keyword "
                                        + deckKeyword.name() + ")");

        const auto& deckItem = deckKeyword.getRecord(0).getItem(0);

        if (deckItem.size() > m_data.size())
            throw std::invalid_argument("Size mismatch when setting data for:" + getKeywordName()
                                        + " keyword size: " + std::to_string( deckItem.size() )
                                        + " input size: " + std::to_string( m_data.size()) );

        return deckItem;
    }

template<>
bool GridProperty<int>::containsNaN( ) const {
    throw std::logic_error("Only <double> and can be meaningfully queried for nan");
}

template<>
bool GridProperty<double>::containsNaN( ) const {
    bool return_value = false;
    auto data = this->m_data.data();
    size_t size = data.size();
    size_t index = 0;
    while (true) {
        if (std::isnan(data[index])) {
            return_value = true;
            break;
        }

        index++;
        if (index == size)
            break;
    }
    return return_value;
}

template<>
const std::string& GridProperty<int>::getDimensionString() const {
    throw std::logic_error("Only <double> grid properties have dimension");
}

template<>
const std::string& GridProperty<double>::getDimensionString() const {
    return m_kwInfo.getDimensionString();
}


template<typename T>
std::vector<T> GridProperty<T>::compressedCopy(const EclipseGrid& grid) const {
    auto data = this->m_data.data();
    if (grid.allActive())
        return data;
    else {
        return grid.compressedVector( data );
    }
}



template<typename T>
std::vector<size_t> GridProperty<T>::cellsEqual(T value, const std::vector<int>& activeMap) const {
    std::vector<size_t> cells;
    auto data = this->m_data.data();
    for (size_t active_index = 0; active_index < activeMap.size(); active_index++) {
        size_t global_index = activeMap[ active_index ];
        if (data[global_index] == value)
            cells.push_back( active_index );
    }
    return cells;
}


template<typename T>
std::vector<size_t> GridProperty<T>::indexEqual(T value) const {
    std::vector<size_t> index_list;
    auto data = this->m_data.data();
    for (size_t index = 0; index < data.size(); index++) {
        if (data[index] == value)
            index_list.push_back( index );
    }
    return index_list;
}


template<typename T>
std::vector<size_t> GridProperty<T>::cellsEqual(T value, const EclipseGrid& grid, bool active) const {
    if (active)
        return cellsEqual( value , grid.getActiveMap());
    else
        return indexEqual( value );
}

std::vector< double > temperature_lookup( size_t size,
                                          const TableManager* tables,
                                          const EclipseGrid* grid,
                                          const GridProperties<int>* ig_props ) {

    if (tables->hasTables("RTEMPVD")) {
        const std::vector< int >& eqlNum = ig_props->getKeyword("EQLNUM").getData();

        const auto& rtempvdTables = tables->getRtempvdTables();
        std::vector< double > values( size, 0 );

        for (size_t cellIdx = 0; cellIdx < eqlNum.size(); ++ cellIdx) {
            int cellEquilRegionIdx = eqlNum[cellIdx] - 1; // EQLNUM contains fortran-style indices!
            const RtempvdTable& rtempvdTable = rtempvdTables.getTable<RtempvdTable>(cellEquilRegionIdx);
            double cellDepth = std::get<2>(grid->getCellCenter(cellIdx));
            values[cellIdx] = rtempvdTable.evaluate("Temperature", cellDepth);
        }

        return values;
    } else
        return std::vector< double >( size, tables->rtemp( ) );
}

}

template class Opm::GridPropertySupportedKeywordInfo< int >;
template class Opm::GridPropertySupportedKeywordInfo< double >;

template class Opm::GridProperty< int >;
template class Opm::GridProperty< double >;
