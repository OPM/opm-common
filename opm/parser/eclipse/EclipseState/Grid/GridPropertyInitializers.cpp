#include <algorithm>
#include <limits>
#include <vector>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridPropertyInitializers.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {

    template< typename T >
    static std::vector< T >& constf( std::vector< T >& values, T x ) {
        std::fill( values.begin(), values.end(), x );
        return values;
    }

    template< typename T >
    GridPropertyFunction< T >::GridPropertyFunction( T x ) :
        f( nullptr ),
        constant( x )
    {}

    template< typename T >
    GridPropertyFunction< T >::GridPropertyFunction(
            std::vector< T >& (*fn)( std::vector< T >&, const Deck&, const EclipseState& ),
            const Deck& d,
            const EclipseState& state ) :
        f( fn ), deck( &d ), es( &state )
    {}

    template< typename T >
    std::vector< T >& GridPropertyFunction< T >::operator()( std::vector< T >& values ) const {
        if( !this->f ) return constf( values, this->constant );

        return (*this->f)( values, *this->deck, *this->es );
    }

    static std::vector< double >& id( std::vector< double >& values, const Deck&, const EclipseState& ) {
        return values;
    }

    static std::vector< int >& id( std::vector< int >& values, const Deck&, const EclipseState& ) {
        return values;
    }

    template< typename T >
    GridPropertyFunction< T > GridPropertyFunction< T >::identity() {
        return GridPropertyFunction< T >();
    }

    template< typename T >
    GridPropertyFunction< T >::GridPropertyFunction() :
        f( &id )
    {}

    std::vector< double >& temperature_lookup(
            std::vector< double >& values,
            const Deck& deck,
            const EclipseState& eclipseState ) {

        if( !deck.hasKeyword("EQLNUM") ) {
            /* if values are defaulted in the TEMPI keyword, but no
             * EQLNUM is specified, you will get NaNs
             */
            return constf( values, std::numeric_limits< double >::quiet_NaN() );
        }

        auto tables = eclipseState.getTableManager();
        auto eclipseGrid = eclipseState.getEclipseGrid();
        const auto& rtempvdTables = tables->getRtempvdTables();
        const std::vector< int >& eqlNum = eclipseState.getIntGridProperty("EQLNUM")->getData();

        for (size_t cellIdx = 0; cellIdx < eqlNum.size(); ++ cellIdx) {
            int cellEquilNum = eqlNum[cellIdx];
            const RtempvdTable& rtempvdTable = rtempvdTables.getTable<RtempvdTable>(cellEquilNum);
            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));
            values[cellIdx] = rtempvdTable.evaluate("Temperature", cellDepth);
        }

        return values;
    }
}

template class Opm::GridPropertyFunction< int >;
template class Opm::GridPropertyFunction< double >;
