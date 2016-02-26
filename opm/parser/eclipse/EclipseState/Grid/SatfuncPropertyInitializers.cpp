#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

namespace Opm {

    /*
     * See the "Saturation Functions" chapter in the Eclipse Technical
     * Description; there are several alternative families of keywords which
     * can be used to enter relperm and capillary pressure tables.
     *
     * If SWOF and SGOF are specified in the deck it return I
     * If SWFN, SGFN and SOF3 are specified in the deck it return I
     * If keywords are missing or mixed, an error is given.
     */
    enum class SatfuncFamily { none = 0, I = 1, II = 2 };

    static SatfuncFamily getSaturationFunctionFamily( const TableManager& tm ) {
        const TableContainer& swofTables = tm.getSwofTables();
        const TableContainer& sgofTables = tm.getSgofTables();
        const TableContainer& slgofTables = tm.getSlgofTables();
        const TableContainer& sof3Tables = tm.getSof3Tables();
        const TableContainer& swfnTables = tm.getSwfnTables();
        const TableContainer& sgfnTables = tm.getSgfnTables();


        bool family1 = (!sgofTables.empty() || !slgofTables.empty()) && !swofTables.empty();
        bool family2 = !swfnTables.empty() && !sgfnTables.empty() && !sof3Tables.empty();

        if (family1 && family2) {
            throw std::invalid_argument("Saturation families should not be mixed \n"
                                        "Use either SGOF (or SLGOF) and SWOF or SGFN, SWFN and SOF3");
        }

        if (!family1 && !family2) {
            throw std::invalid_argument("Saturations function must be specified using either "
                                        "family 1 or family 2 keywords \n"
                                        "Use either SGOF (or SLGOF) and SWOF or SGFN, SWFN and SOF3" );
        }

        if( family1 ) return SatfuncFamily::I;
        if( family2 ) return SatfuncFamily::II;
        return SatfuncFamily::none;
    }

    enum class limit { min, max };

    static std::vector< double > findMinWaterSaturation( const TableManager& tm ) {
        const auto num_tables = tm.getTabdims()->getNumSatTables();
        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto famI = [&swofTables]( int i ) {
            return swofTables.getTable< SwofTable >( i ).getSwColumn().front();
        };

        const auto famII = [&swfnTables]( int i ) {
            return swfnTables.getTable< SwfnTable >( i ).getSwColumn().front();
        };

        switch( getSaturationFunctionFamily( tm ) ) {
            case SatfuncFamily::I: return map( famI, fun::iota( num_tables ) );
            case SatfuncFamily::II: return map( famII, fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    static std::vector< double > findMaxWaterSaturation( const TableManager& tm ) {
        const auto num_tables = tm.getTabdims()->getNumSatTables();
        const auto& swofTables = tm.getSwofTables();
        const auto& swfnTables = tm.getSwfnTables();

        const auto famI = [&swofTables]( int i ) {
            return swofTables.getTable< SwofTable >( i ).getSwColumn().back();
        };

        const auto famII = [&swfnTables]( int i ) {
            return swfnTables.getTable< SwfnTable >( i ).getSwColumn().back();
        };

        switch( getSaturationFunctionFamily( tm ) ) {
            case SatfuncFamily::I: return map( famI, fun::iota( num_tables ) );
            case SatfuncFamily::II: return map( famII, fun::iota( num_tables ) );
            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    static std::vector< double > findMinGasSaturation( const TableManager& tm ) {
        const auto num_tables = tm.getTabdims()->getNumSatTables();
        const auto& sgofTables  = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable< SgofTable >( i ).getSgColumn().front();
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return 1.0 - slgofTables.getTable< SlgofTable >( i ).getSlColumn().back();
        };

        const auto famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable< SgfnTable >( i ).getSgColumn().front();
        };


        switch( getSaturationFunctionFamily( tm ) ) {
            case SatfuncFamily::I:

                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return fun::map( famI_sgof, fun::iota( num_tables ) );
                else
                    return fun::map( famI_slgof, fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return fun::map( famII, fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");

        }

    }

    static std::vector< double > findMaxGasSaturation( const TableManager& tm ) {
        const auto num_tables = tm.getTabdims()->getNumSatTables();
        const auto& sgofTables  = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();
        const auto& sgfnTables = tm.getSgfnTables();

        const auto famI_sgof = [&sgofTables]( int i ) {
            return sgofTables.getTable< SgofTable >( i ).getSgColumn().back();
        };

        const auto famI_slgof = [&slgofTables]( int i ) {
            return 1.0 - slgofTables.getTable< SlgofTable >( i ).getSlColumn().front();
        };

        const auto famII = [&sgfnTables]( int i ) {
            return sgfnTables.getTable< SgfnTable >( i ).getSgColumn().back();
        };


        switch( getSaturationFunctionFamily( tm ) ) {
            case SatfuncFamily::I:
                if( sgofTables.empty() && slgofTables.empty() )
                    throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

                if( !sgofTables.empty() )
                    return fun::map( famI_sgof, fun::iota( num_tables ) );
                else
                    return fun::map( famI_slgof, fun::iota( num_tables ) );

            case SatfuncFamily::II:
                return fun::map( famII, fun::iota( num_tables ) );

            default:
                throw std::domain_error("No valid saturation keyword family specified");
        }
    }


    struct CriticalSat {
        CriticalSat( size_t sz ) : water( sz, 0 ), gas( sz, 0 ),
                                   oil_water( sz, 0 ), oil_gas( sz, 0 )
        {}

        std::vector< double > water;
        std::vector< double > gas;
        std::vector< double > oil_water;
        std::vector< double > oil_gas;
    };

    /*
     * These functions have been ported from an older implementation to instead
     * use std::upper_bound and more from <algorithm> to make code -intent-
     * clearer. This also made some (maybe intentional) details easier to spot.
     * A short discussion:
     *
     * I don't know if not finding any element larger than 0.0 in the tables
     * was ever supposed to happen (or even possible), in which case the vector
     * elements remained at their initial value of 0.0. This behaviour has been
     * preserved, but is now explicit. The original code was also not clear if
     * it was possible to look up columns at index -1 (see critical_water for
     * an example), but the new version is explicit about this. Unfortuately
     * I'm not familiar enough with the maths or internal structure to make
     * more than a guess here, but most of this behaviour should be preserved.
     *
     */

    template< typename T >
    static inline double critical_water( const T& table ) {

        const auto& col = table.getKrwColumn();
        const auto end = col.begin() + table.numRows() + 1;
        const auto critical = std::upper_bound( col.begin(), end, 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == end ) return 0.0;

        return table.getSwColumn()[ index - 1 ];
    }

    static inline double critical_oil_water( const SwofTable& swofTable ) {
        const auto& col = swofTable.getKrowColumn();

        using reverse = std::reverse_iterator< decltype( col.begin() ) >;
        auto rbegin = reverse( col.begin() + swofTable.numRows() + 1 );
        auto rend = reverse( col.begin() );
        const auto critical = std::upper_bound( rbegin, rend, 0.0 );
        const auto index = std::distance( col.begin(), critical.base() - 1 );

        if( critical == rend ) return 0.0;

        return 1 - swofTable.getSwColumn()[ index + 1 ];
    }

    static inline double critical_oil_water( const Sof3Table& sof3Table ) {

        const auto& col = sof3Table.getKrowColumn();
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return sof3Table.getSoColumn()[ index - 1 ];
    }

    template< typename T >
    static inline double critical_gas( const T& table ) {
        const auto& col = table.getKrgColumn();
        const auto end = col.begin() + table.numRows() + 1;
        const auto critical = std::upper_bound( col.begin(), end, 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == end ) return 0.0;

        return table.getSgColumn()[ index - 1 ];
    }

    static inline double critical_oil_gas( const SgofTable& sgofTable ) {
        const auto& col = sgofTable.getKrogColumn();

        using reverse = std::reverse_iterator< decltype( col.begin() ) >;
        auto rbegin = reverse( col.begin() + sgofTable.numRows() + 1 );
        auto rend = reverse( col.begin() );
        const auto critical = std::upper_bound( rbegin, rend, 0.0 );
        const auto index = std::distance( col.begin(), critical.base() - 1 );

        if( critical == rend ) return 0.0;

        return 1 - sgofTable.getSgColumn()[ index + 1 ];
    }

    static inline double critical_gas( const SlgofTable& sgofTable ) {
        const auto& col = sgofTable.getKrgColumn();
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return sgofTable.getSlColumn()[ index - 1 ];
    }

    static inline double critical_oil_gas( const SlgofTable& sgofTable ) {

        const auto& col = sgofTable.getKrogColumn();
        using reverse = std::reverse_iterator< decltype( col.begin() ) >;
        auto rbegin = reverse( col.begin() + sgofTable.numRows() + 1 );
        auto rend = reverse( col.begin() );
        const auto critical = std::upper_bound( rbegin, rend, 0.0 );
        const auto index = std::distance( col.begin(), critical.base() - 1 );

        if( critical == rend ) return 0.0;

        return 1 - sgofTable.getSlColumn()[ index + 1 ];
    }

    static CriticalSat findCriticalPointsI( const TableManager& tm ) {
        const auto numSatTables = tm.getTabdims()->getNumSatTables();

        CriticalSat critical( numSatTables );

        const auto& swofTables = tm.getSwofTables();
        const auto& sgofTables = tm.getSgofTables();
        const auto& slgofTables = tm.getSlgofTables();

        if( sgofTables.empty() && slgofTables.empty() )
            throw std::runtime_error( "Saturation keyword family I requires either sgof or slgof non-empty" );

        for( size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx ) {
            const auto& swofTable = swofTables.getTable< SwofTable >( tableIdx );
            critical.water[ tableIdx ] = critical_water( swofTable );
            critical.oil_water[ tableIdx ] = critical_oil_water( swofTable );

            if( !sgofTables.empty() ) {
                const SgofTable& sgofTable = sgofTables.getTable<SgofTable>( tableIdx );
                critical.gas[ tableIdx ] = critical_gas( sgofTable );
                critical.oil_gas[ tableIdx ] = critical_oil_gas( sgofTable );
            }
            else {
                const SlgofTable& slgofTable = slgofTables.getTable<SlgofTable>( tableIdx );
                critical.gas[ tableIdx ] = critical_gas( slgofTable );
                critical.oil_gas[ tableIdx ] = critical_oil_gas( slgofTable );
            }
        }

        return critical;
    }

    static inline double critical_oil( const Sof3Table& sof3Table, const TableColumn& col ) {
        const auto critical = std::upper_bound( col.begin(), col.end(), 0.0 );
        const auto index = std::distance( col.begin(), critical );

        if( index == 0 || critical == col.end() ) return 0.0;

        return sof3Table.getSoColumn()[ index - 1 ];
    }

    static CriticalSat findCriticalPointsII( const TableManager& tm ) {
        const auto numSatTables = tm.getTabdims()->getNumSatTables();
        CriticalSat critical( numSatTables );

        const TableContainer& swfnTables = tm.getSwfnTables();
        const TableContainer& sgfnTables = tm.getSgfnTables();
        const TableContainer& sof3Tables = tm.getSof3Tables();

        for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {

            const auto& swfnTable = swfnTables.getTable<SwfnTable>( tableIdx );
            const auto& sgfnTable = sgfnTables.getTable<SgfnTable>( tableIdx );

            critical.water[ tableIdx ] = critical_water( swfnTable );
            critical.gas[ tableIdx ] = critical_gas( sgfnTable );

            const auto& sof3Table = sof3Tables.getTable<Sof3Table>( tableIdx );
            critical.oil_gas[ tableIdx ] = critical_oil( sof3Table, sof3Table.getKrogColumn() );
            critical.oil_water[ tableIdx ] = critical_oil( sof3Table, sof3Table.getKrowColumn() );
        }

        return critical;
    }

    static CriticalSat findCriticalPoints( const EclipseState& m_eclipseState ) {
        const auto& tables = *m_eclipseState.getTableManager();

        switch( getSaturationFunctionFamily( tables ) ) {
            case SatfuncFamily::I: return findCriticalPointsI( tables );
            case SatfuncFamily::II: return findCriticalPointsII( tables );
            default: throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    struct VerticalPts {
        VerticalPts( size_t sz ) : maxPcog( sz, 0 ), maxPcow( sz, 0 ), maxKrg( sz, 0 ),
                           krgr( sz, 0 ), maxKro( sz, 0 ), krorw( sz, 0 ),
                           krorg( sz, 0 ), maxKrw( sz, 0 ), krwr( sz, 0 )
        {}

        std::vector< double > maxPcog;
        std::vector< double > maxPcow;
        std::vector< double > maxKrg;
        std::vector< double > krgr;
        std::vector< double > maxKro;
        std::vector< double > krorw;
        std::vector< double > krorg;
        std::vector< double > maxKrw;
        std::vector< double > krwr;
    };

    static inline VerticalPts findVerticalPointsI( const TableManager& tm ) {
        const auto& swofTables = tm.getSwofTables();
        const auto& sgofTables = tm.getSgofTables();

        const auto numSatTables = tm.getTabdims()->getNumSatTables();
        VerticalPts vps( numSatTables );

        for( size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx ) {
            const auto& swofTable = swofTables.getTable<SwofTable>(tableIdx);
            const auto& sgofTable = sgofTables.getTable<SgofTable>(tableIdx);

            // find the maximum output values of the oil-gas system
            vps.maxPcog[ tableIdx ] = sgofTable.getPcogColumn().front();
            vps.maxKrg[ tableIdx ] = sgofTable.getKrgColumn().back();

            vps.krgr[ tableIdx] = sgofTable.getKrgColumn().front();
            vps.krwr[ tableIdx] = swofTable.getKrwColumn().front();

            // find the oil relperm which corresponds to the critical water saturation
            const auto& krwCol = swofTable.getKrwColumn();
            const auto& krowCol = swofTable.getKrowColumn();

            const auto critw = std::upper_bound( krwCol.begin(), krwCol.end(), 0.0 );
            vps.krorw[ tableIdx ] = krowCol[ std::distance( critw, krwCol.end() - 2 ) ];

            // find the oil relperm which corresponds to the critical gas saturation
            const auto &krgCol = sgofTable.getKrgColumn();
            const auto &krogCol = sgofTable.getKrogColumn();

            const auto critg = std::upper_bound( krgCol.begin(), krgCol.end(), 0.0 );
            vps.krorg[ tableIdx ] = krogCol[ std::distance( critg, krgCol.end() - 2 ) ];

            // find the maximum output values of the water-oil system. the maximum oil
            // relperm is possibly wrong because we have two oil relperms in a threephase
            // system. the documentation is very ambiguos here, though: it says that the
            // oil relperm at the maximum oil saturation is scaled according to maximum
            // specified the KRO keyword. the first part of the statement points at
            // scaling the resultant threephase oil relperm, but then the gas saturation
            // is not taken into account which means that some twophase quantity must be
            // scaled.
            vps.maxPcow[ tableIdx ] = swofTable.getPcowColumn().front();
            vps.maxKro[ tableIdx ] = swofTable.getKrowColumn().front();
            vps.maxKrw[ tableIdx ] = swofTable.getKrwColumn().back();
        }

        return vps;
    }

    static inline VerticalPts findVerticalPointsII( const TableManager& tm,
                                            const CriticalSat& crit ) {

        const auto& swfnTables = tm.getSwfnTables();
        const auto& sgfnTables = tm.getSgfnTables();
        const auto& sof3Tables = tm.getSof3Tables();

        const auto numSatTables = tm.getTabdims()->getNumSatTables();
        VerticalPts vps( numSatTables );

        const auto min_water = findMinWaterSaturation( tm );
        const auto min_gas = findMinGasSaturation( tm );

        for( size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx ) {
            const auto& sof3Table = sof3Tables.getTable<Sof3Table>( tableIdx );
            const auto& sgfnTable = sgfnTables.getTable<SgfnTable>( tableIdx );
            const auto& swfnTable = swfnTables.getTable<SwfnTable>( tableIdx );

            // find the maximum output values of the oil-gas system
            vps.maxPcog[tableIdx] = sgfnTable.getPcogColumn().back();
            vps.maxKrg[tableIdx] = sgfnTable.getKrgColumn().back();

            // find the minimum output values of the relperm
            vps.krgr[tableIdx] = sgfnTable.getKrgColumn().front();
            vps.krwr[tableIdx] = swfnTable.getKrwColumn().front();

            // find the oil relperm which corresponds to the critical water saturation
            const double OilSatAtcritialWaterSat = 1.0 - crit.water[tableIdx] - min_gas[tableIdx];
            vps.krorw[tableIdx] = sof3Table.evaluate("KROW", OilSatAtcritialWaterSat);

            // find the oil relperm which corresponds to the critical gas saturation
            const double OilSatAtCritialGasSat = 1.0 - crit.gas[tableIdx] - min_water[tableIdx];
            vps.krorg[tableIdx] = sof3Table.evaluate("KROG", OilSatAtCritialGasSat);

            // find the maximum output values of the water-oil system. the maximum oil
            // relperm is possibly wrong because we have two oil relperms in a threephase
            // system. the documentation is very ambiguos here, though: it says that the
            // oil relperm at the maximum oil saturation is scaled according to maximum
            // specified the KRO keyword. the first part of the statement points at
            // scaling the resultant threephase oil relperm, but then the gas saturation
            // is not taken into account which means that some twophase quantity must be
            // scaled.
            vps.maxPcow[tableIdx] = swfnTable.getPcowColumn().front();
            vps.maxKro[tableIdx] = sof3Table.getKrowColumn().back();
            vps.maxKrw[tableIdx] = swfnTable.getKrwColumn().back();
        }

        return vps;
    }

    static VerticalPts findVerticalPoints( const EclipseState& m_eclipseState,
                                   const CriticalSat& crit ) {

        const auto& tables = *m_eclipseState.getTableManager();

        switch( getSaturationFunctionFamily( tables ) ) {
            case SatfuncFamily::I: return findVerticalPointsI( tables );
            case SatfuncFamily::II: return findVerticalPointsII( tables, crit );
            default: throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    static double selectValue( const TableContainer& depthTables,
                               int tableIdx,
                               const std::string& columnName,
                               double cellDepth,
                               double fallbackValue,
                               bool useOneMinusTableValue) {

        if( tableIdx < 0 ) return fallbackValue;

        const auto& table = depthTables.getTable( tableIdx );

        if( tableIdx >= int( depthTables.size() ) )
            throw std::invalid_argument("Not enough tables!");

        // evaluate the table at the cell depth
        const double value = table.evaluate( columnName, cellDepth );

        // a column can be fully defaulted. In this case, eval() returns a NaN
        // and we have to use the data from saturation tables
        if( !std::isfinite( value ) ) return fallbackValue;
        if( useOneMinusTableValue ) return 1 - value;
        return value;
    }

    static std::vector< double >& satnumApply( std::vector< double >& values,
                                               const std::string& columnName,
                                               const std::vector< double >& fallbackValues,
                                               const Deck& m_deck,
                                               const EclipseState& m_eclipseState,
                                               bool useOneMinusTableValue ) {

        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        auto tables = m_eclipseState.getTableManager();
        auto tabdims = tables->getTabdims();
        auto satnum = m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();

        satnum->checkLimits( 1 , numSatTables );

        // All table lookup assumes three-phase model
        assert( m_eclipseState.getNumPhases() == 3 );

        const auto crit = findCriticalPoints( m_eclipseState );
        const auto vps = findVerticalPoints( m_eclipseState, crit );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        const bool useEnptvd = m_deck.hasKeyword("ENPTVD");
        const auto& enptvdTables = tables->getEnptvdTables();

        for( size_t cellIdx = 0; cellIdx < eclipseGrid->getCartesianSize(); cellIdx++ ) {
            int satTableIdx = satnum->iget( cellIdx ) - 1;
            int endNum = endnum->iget( cellIdx ) - 1;
            double cellDepth = std::get< 2 >( eclipseGrid->getCellCenter( cellIdx ) );


            values[cellIdx] = selectValue(enptvdTables,
                                        (useEnptvd && endNum >= 0) ? endNum : -1,
                                        columnName,
                                        cellDepth,
                                        fallbackValues[ satTableIdx ],
                                        useOneMinusTableValue);
        }

        return values;
    }

    static std::vector< double >& imbnumApply( std::vector< double >& values,
                                               const std::string& columnName,
                                               const std::vector< double >& fallBackValues,
                                               const Deck& m_deck,
                                               const EclipseState& m_eclipseState,
                                               bool useOneMinusTableValue ) {

        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        auto tables = m_eclipseState.getTableManager();
        auto imbnum = m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = m_eclipseState.getIntGridProperty("ENDNUM");

        auto tabdims = tables->getTabdims();
        const int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits( 1 , numSatTables );
        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        const bool useImptvd = m_deck.hasKeyword("IMPTVD");
        const TableContainer& imptvdTables = tables->getImptvdTables();
        for( size_t cellIdx = 0; cellIdx < eclipseGrid->getCartesianSize(); cellIdx++ ) {
            int imbTableIdx = imbnum->iget( cellIdx ) - 1;
            int endNum = endnum->iget( cellIdx ) - 1;
            double cellDepth = std::get< 2 >( eclipseGrid->getCellCenter( cellIdx ) );

            values[cellIdx] = selectValue(imptvdTables,
                                                (useImptvd && endNum >= 0) ? endNum : -1,
                                                columnName,
                                                cellDepth,
                                                fallBackValues[imbTableIdx],
                                                useOneMinusTableValue);
        }

        return values;
    }

    std::vector< double >& SGLEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto min_gas = findMinGasSaturation( *es.getTableManager() );
        return satnumApply( values, "SGCO", min_gas, deck, es, false );
    }

    std::vector< double >& ISGLEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto min_gas = findMinGasSaturation( *es.getTableManager() );
        return imbnumApply( values, "SGCO", min_gas, deck, es, false );
    }

    std::vector< double >& SGUEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto max_gas = findMaxGasSaturation( *es.getTableManager() );
        return satnumApply( values, "SGMAX", max_gas, deck, es, false );
    }

    std::vector< double >& ISGUEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto max_gas = findMaxGasSaturation( *es.getTableManager() );
        return imbnumApply( values, "SGMAX", max_gas, deck, es, false );
    }

    std::vector< double >& SWLEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto min_water = findMinWaterSaturation( *es.getTableManager() );
        return satnumApply( values, "SWCO", min_water, deck, es, false );
    }

    std::vector< double >& ISWLEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto min_water = findMinWaterSaturation( *es.getTableManager() );
        return imbnumApply( values, "SWCO", min_water, deck, es, false );
    }

    std::vector< double >& SWUEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto max_water = findMaxWaterSaturation( *es.getTableManager() );
        return satnumApply( values, "SWMAX", max_water, deck, es, true );
    }

    std::vector< double >& ISWUEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto max_water = findMaxWaterSaturation( *es.getTableManager() );
        return imbnumApply( values, "SWMAX", max_water, deck, es, true);
    }

    std::vector< double >& SGCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return satnumApply( values, "SGCRIT", crit.gas, deck, es, false );
    }

    std::vector< double >& ISGCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return imbnumApply( values, "SGCRIT", crit.gas, deck, es, false );
    }

    std::vector< double >& SOWCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return satnumApply( values, "SOWCRIT", crit.oil_water, deck, es, false );
    }

    std::vector< double >& ISOWCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return imbnumApply( values, "SOWCRIT", crit.oil_water, deck, es, false );
    }

    std::vector< double >& SOGCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return satnumApply( values, "SOGCRIT", crit.oil_gas, deck, es, false );
    }

    std::vector< double >& ISOGCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return imbnumApply( values, "SOGCRIT", crit.oil_gas, deck, es, false );
    }

    std::vector< double >& SWCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return satnumApply( values, "SWCRIT", crit.water, deck, es, false );
    }

    std::vector< double >& ISWCREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        return imbnumApply( values, "SWCRIT", crit.water, deck, es, false );
    }

    std::vector< double >& PCWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "PCW", vps.maxPcow, deck, es, false );
    }

    std::vector< double >& IPCWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IPCW", vps.maxPcow, deck, es, false );
    }

    std::vector< double >& PCGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "PCG", vps.maxPcog, deck, es, false );
    }

    std::vector< double >& IPCGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IPCG", vps.maxPcog, deck, es, false );
    }

    std::vector< double >& KRWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRW", vps.maxKrw, deck, es, false );
    }

    std::vector< double >& IKRWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRW", vps.krwr, deck, es, false );
    }

    std::vector< double >& KRWREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRWR", vps.krwr, deck, es, false );
    }

    std::vector< double >& IKRWREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRWR", vps.krwr, deck, es, false );
    }

    std::vector< double >& KROEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRO", vps.maxKro, deck, es, false );
    }

    std::vector< double >& IKROEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRO", vps.maxKro, deck, es, false );
    }

    std::vector< double >& KRORWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRORW", vps.krorw, deck, es, false );
    }

    std::vector< double >& IKRORWEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRORW", vps.krorw, deck, es, false );
    }

    std::vector< double >& KRORGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRORG", vps.krorg, deck, es, false );
    }

    std::vector< double >& IKRORGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRORG", vps.krorg, deck, es, false );
    }

    std::vector< double >& KRGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRG", vps.maxKrg, deck, es, false );
    }

    std::vector< double >& IKRGEndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRG", vps.maxKrg, deck, es, false );
    }

    std::vector< double >& KRGREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return satnumApply( values, "KRGR", vps.krgr, deck, es, false );
    }

    std::vector< double >& IKRGREndpoint( std::vector< double >& values, const Deck& deck, const EclipseState& es ) {
        const auto crit = findCriticalPoints( es );
        const auto vps = findVerticalPoints( es, crit );

        return imbnumApply( values, "IKRGR", vps.krgr, deck, es, false );
    }

}
