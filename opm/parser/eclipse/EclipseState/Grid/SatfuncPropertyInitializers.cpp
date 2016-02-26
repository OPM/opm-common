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

namespace Opm {

    void EndpointInitializer::findSaturationEndpoints( const EclipseState& m_eclipseState ) const {
        auto tables = m_eclipseState.getTableManager();
        auto tabdims = tables->getTabdims();
        size_t numSatTables = tabdims->getNumSatTables();
        m_minWaterSat.resize( numSatTables , 0 );
        m_maxWaterSat.resize( numSatTables , 0 );
        m_minGasSat.resize( numSatTables , 0 );
        m_maxGasSat.resize( numSatTables , 0 );

        switch (getSaturationFunctionFamily( m_eclipseState )) {
        case SaturationFunctionFamily::FamilyI:
        {
            const TableContainer& swofTables = tables->getSwofTables();
            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                const SwofTable& swofTable = swofTables.getTable<SwofTable>(tableIdx);
                m_minWaterSat[tableIdx] = swofTable.getSwColumn().front();
                m_maxWaterSat[tableIdx] = swofTable.getSwColumn().back();
            }

            {
                const TableContainer& sgofTables  = tables->getSgofTables();
                const TableContainer& slgofTables = tables->getSlgofTables();

                if (!sgofTables.empty()) {
                    for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                        const SgofTable& sgofTable = sgofTables.getTable<SgofTable>( tableIdx );
                        m_minGasSat[tableIdx] = sgofTable.getSgColumn().front();
                        m_maxGasSat[tableIdx] = sgofTable.getSgColumn().back();
                    }
                }
                else {
                    assert(!slgofTables.empty());
                    for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                        const SlgofTable& slgofTable = slgofTables.getTable<SlgofTable>( tableIdx );
                        m_minGasSat[tableIdx] = 1.0 - slgofTable.getSlColumn().back();
                        m_maxGasSat[tableIdx] = 1.0 - slgofTable.getSlColumn().front();
                    }
                }
            }

            break;
        }
        case SaturationFunctionFamily::FamilyII:
        {
            const TableContainer& swfnTables = tables->getSwfnTables();
            const TableContainer& sgfnTables = tables->getSgfnTables();
            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                const SwfnTable& swfnTable = swfnTables.getTable<SwfnTable>(tableIdx);
                const SgfnTable& sgfnTable = sgfnTables.getTable<SgfnTable>(tableIdx);

                m_minWaterSat[tableIdx] = swfnTable.getSwColumn().front();
                m_maxWaterSat[tableIdx] = swfnTable.getSwColumn().back();

                m_minGasSat[tableIdx] = sgfnTable.getSgColumn().front();
                m_maxGasSat[tableIdx] = sgfnTable.getSgColumn().back();
            }
            break;
        }
        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }
    }

    void EndpointInitializer::findCriticalPoints( const EclipseState& m_eclipseState ) const {
        auto tables = m_eclipseState.getTableManager();
        auto tabdims = tables->getTabdims();
        size_t numSatTables = tabdims->getNumSatTables();

        m_criticalWaterSat.resize( numSatTables , 0 );
        m_criticalGasSat.resize( numSatTables , 0 );
        m_criticalOilOGSat.resize( numSatTables , 0 );
        m_criticalOilOWSat.resize( numSatTables , 0 );

        switch (getSaturationFunctionFamily( m_eclipseState )) {
        case SaturationFunctionFamily::FamilyI:
        {
            const TableContainer& swofTables = tables->getSwofTables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                // find the critical water saturation
                const SwofTable& swofTable = swofTables.getTable<SwofTable>( tableIdx );
                int numRows = swofTable.numRows();
                const auto &krwCol = swofTable.getKrwColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (krwCol[rowIdx] > 0.0) {
                        double Sw = 0.0;
                        if (rowIdx > 0)
                            Sw = swofTable.getSwColumn()[rowIdx - 1];
                        m_criticalWaterSat[tableIdx] = Sw;
                        break;
                    }
                }

                // find the critical oil saturation of the water-oil system
                const auto &kroOWCol = swofTable.getKrowColumn();
                for (int rowIdx = numRows - 1; rowIdx >= 0; --rowIdx) {
                    if (kroOWCol[rowIdx] > 0.0) {
                        double Sw = swofTable.getSwColumn()[rowIdx + 1];
                        m_criticalOilOWSat[tableIdx] = 1 - Sw;
                        break;
                    }
                }
            }

            {
                const TableContainer& sgofTables = tables->getSgofTables();
                const TableContainer& slgofTables = tables->getSlgofTables();

                if (!sgofTables.empty()) {
                    for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                        const SgofTable& sgofTable = sgofTables.getTable<SgofTable>( tableIdx );
                        // find the critical gas saturation
                        int numRows = sgofTable.numRows();
                        const auto &krgCol = sgofTable.getKrgColumn();
                        for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                            if (krgCol[rowIdx] > 0.0) {
                                double Sg = 0.0;
                                if (rowIdx > 0)
                                    Sg = sgofTable.getSgColumn()[rowIdx - 1];
                                m_criticalGasSat[tableIdx] = Sg;
                                break;
                            }
                        }

                        // find the critical oil saturation of the oil-gas system
                        const auto &kroOGCol = sgofTable.getKrogColumn();
                        for (int rowIdx = numRows - 1; rowIdx >= 0; --rowIdx) {
                            if (kroOGCol[rowIdx] > 0.0) {
                                double Sg = sgofTable.getSgColumn()[rowIdx + 1];
                                m_criticalOilOGSat[tableIdx] = 1 - Sg;
                                break;
                            }
                        }
                    }
                }
                else {
                    assert(!slgofTables.empty());
                    for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                        const SlgofTable& slgofTable = slgofTables.getTable<SlgofTable>( tableIdx );
                        // find the critical gas saturation
                        int numRows = slgofTable.numRows();
                        const auto &krgCol = slgofTable.getKrgColumn();
                        for (int rowIdx = numRows - 1; rowIdx >= 0; -- rowIdx) {
                            if (krgCol[rowIdx] > 0.0) {
                                assert(rowIdx < numRows - 1);
                                m_criticalGasSat[tableIdx] =
                                    1.0 - slgofTable.getSlColumn()[rowIdx + 1];
                                break;
                            }
                        }

                        // find the critical oil saturation of the oil-gas system
                        const auto &kroOGCol = slgofTable.getKrogColumn();
                        for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                            if (kroOGCol[rowIdx] > 0.0) {
                                m_criticalOilOGSat[tableIdx] =
                                    slgofTable.getSlColumn()[rowIdx + 1];
                                break;
                            }
                        }
                    }
                }

                break;
            }
        }

        case SaturationFunctionFamily::FamilyII: {
            const TableContainer& swfnTables = tables->getSwfnTables();
            const TableContainer& sgfnTables = tables->getSgfnTables();
            const TableContainer& sof3Tables = tables->getSof3Tables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                {
                    const SwfnTable& swfnTable = swfnTables.getTable<SwfnTable>( tableIdx );
                    // find the critical water saturation
                    size_t numRows = swfnTable.numRows();
                    const auto &krwCol = swfnTable.getKrwColumn();
                    for (size_t rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                        if (krwCol[rowIdx] > 0.0) {
                            double Sw = 0.0;
                            if (rowIdx > 0)
                                Sw = swfnTable.getSwColumn()[rowIdx - 1];
                            m_criticalWaterSat[tableIdx] = Sw;
                            break;
                        }
                    }
                }

                {
                    const SgfnTable& sgfnTable = sgfnTables.getTable<SgfnTable>( tableIdx );
                    // find the critical gas saturation
                    size_t numRows = sgfnTable.numRows();
                    const auto &krgCol = sgfnTable.getKrgColumn();
                    for (size_t rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                        if (krgCol[rowIdx] > 0.0) {
                            double Sg = 0.0;
                            if (rowIdx > 0)
                                Sg = sgfnTable.getSgColumn()[rowIdx - 1];
                            m_criticalGasSat[tableIdx] = Sg;
                            break;
                        }
                    }
                }

                // find the critical oil saturation of the oil-gas system
                {
                    const Sof3Table& sof3Table = sof3Tables.getTable<Sof3Table>( tableIdx );
                    size_t numRows = sof3Table.numRows();
                    const auto &kroOGCol = sof3Table.getKrogColumn();
                    for (size_t rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                        if (kroOGCol[rowIdx] > 0.0) {
                            double So = sof3Table.getSoColumn()[rowIdx - 1];
                            m_criticalOilOGSat[tableIdx] = So;
                            break;
                        }
                    }

                    // find the critical oil saturation of the water-oil system
                    const auto &kroOWCol = sof3Table.getKrowColumn();
                    for (size_t rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                        if (kroOWCol[rowIdx] > 0.0) {
                            double So = sof3Table.getSoColumn()[rowIdx - 1];
                            m_criticalOilOWSat[tableIdx] = So;
                            break;
                        }
                    }
                }
            }
            break;

        }
        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }


    }

    void EndpointInitializer::findVerticalPoints( const EclipseState& m_eclipseState ) const {
        auto tables = m_eclipseState.getTableManager();
        auto tabdims = tables->getTabdims();
        size_t numSatTables = tabdims->getNumSatTables();

        m_maxPcog.resize( numSatTables , 0 );
        m_maxPcow.resize( numSatTables , 0 );
        m_maxKrg.resize( numSatTables , 0 );
        m_krgr.resize( numSatTables , 0 );
        m_maxKro.resize( numSatTables , 0 );
        m_krorw.resize( numSatTables , 0 );
        m_krorg.resize( numSatTables , 0 );
        m_maxKrw.resize( numSatTables , 0 );
        m_krwr.resize( numSatTables , 0 );

        switch (getSaturationFunctionFamily( m_eclipseState )) {
        case SaturationFunctionFamily::FamilyI:
        {
            const TableContainer& swofTables = tables->getSwofTables();
            const TableContainer& sgofTables = tables->getSgofTables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                const SwofTable& swofTable = swofTables.getTable<SwofTable>(tableIdx);
                const SgofTable& sgofTable = sgofTables.getTable<SgofTable>(tableIdx);
                // find the maximum output values of the oil-gas system
                m_maxPcog[tableIdx] = sgofTable.getPcogColumn().front();
                m_maxKrg[tableIdx] = sgofTable.getKrgColumn().back();

                m_krgr[tableIdx] = sgofTable.getKrgColumn().front();
                m_krwr[tableIdx] = swofTable.getKrwColumn().front();

                // find the oil relperm which corresponds to the critical water saturation
                const auto &krwCol = swofTable.getKrwColumn();
                const auto &krowCol = swofTable.getKrowColumn();
                for (size_t rowIdx = 0; rowIdx < krwCol.size(); ++rowIdx) {
                    if (krwCol[rowIdx] > 0.0) {
                        m_krorw[tableIdx] = krowCol[rowIdx - 1];
                        break;
                    }
                }

                // find the oil relperm which corresponds to the critical gas saturation
                const auto &krgCol = sgofTable.getKrgColumn();
                const auto &krogCol = sgofTable.getKrogColumn();
                for (size_t rowIdx = 0; rowIdx < krgCol.size(); ++rowIdx) {
                    if (krgCol[rowIdx] > 0.0) {
                        m_krorg[tableIdx] = krogCol[rowIdx - 1];
                        break;
                    }
                }

                // find the maximum output values of the water-oil system. the maximum oil
                // relperm is possibly wrong because we have two oil relperms in a threephase
                // system. the documentation is very ambiguos here, though: it says that the
                // oil relperm at the maximum oil saturation is scaled according to maximum
                // specified the KRO keyword. the first part of the statement points at
                // scaling the resultant threephase oil relperm, but then the gas saturation
                // is not taken into account which means that some twophase quantity must be
                // scaled.
                m_maxPcow[tableIdx] = swofTable.getPcowColumn().front();
                m_maxKro[tableIdx] = swofTable.getKrowColumn().front();
                m_maxKrw[tableIdx] = swofTable.getKrwColumn().back();
            }
            break;
        }
        case SaturationFunctionFamily::FamilyII: {
            const TableContainer& swfnTables = tables->getSwfnTables();
            const TableContainer& sgfnTables = tables->getSgfnTables();
            const TableContainer& sof3Tables = tables->getSof3Tables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                const Sof3Table& sof3Table = sof3Tables.getTable<Sof3Table>( tableIdx );
                const SgfnTable& sgfnTable = sgfnTables.getTable<SgfnTable>( tableIdx );
                const SwfnTable& swfnTable = swfnTables.getTable<SwfnTable>( tableIdx );

                // find the maximum output values of the oil-gas system
                m_maxPcog[tableIdx] = sgfnTable.getPcogColumn().back();
                m_maxKrg[tableIdx] = sgfnTable.getKrgColumn().back();

                // find the minimum output values of the relperm
                m_krgr[tableIdx] = sgfnTable.getKrgColumn().front();
                m_krwr[tableIdx] = swfnTable.getKrwColumn().front();

                // find the oil relperm which corresponds to the critical water saturation
                const double OilSatAtcritialWaterSat = 1.0 - m_criticalWaterSat[tableIdx] - m_minGasSat[tableIdx];
                m_krorw[tableIdx] = sof3Table.evaluate("KROW", OilSatAtcritialWaterSat);

                // find the oil relperm which corresponds to the critical gas saturation
                const double OilSatAtCritialGasSat = 1.0 - m_criticalGasSat[tableIdx] - m_minWaterSat[tableIdx];
                m_krorg[tableIdx] = sof3Table.evaluate("KROG", OilSatAtCritialGasSat);

                // find the maximum output values of the water-oil system. the maximum oil
                // relperm is possibly wrong because we have two oil relperms in a threephase
                // system. the documentation is very ambiguos here, though: it says that the
                // oil relperm at the maximum oil saturation is scaled according to maximum
                // specified the KRO keyword. the first part of the statement points at
                // scaling the resultant threephase oil relperm, but then the gas saturation
                // is not taken into account which means that some twophase quantity must be
                // scaled.
                m_maxPcow[tableIdx] = swfnTable.getPcowColumn().front();
                m_maxKro[tableIdx] = sof3Table.getKrowColumn().back();
                m_maxKrw[tableIdx] = swfnTable.getKrwColumn().back();
            }
            break;
        }

        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }

    }

    EndpointInitializer::SaturationFunctionFamily
    EndpointInitializer::getSaturationFunctionFamily( const EclipseState& m_eclipseState ) const {
        auto tables = m_eclipseState.getTableManager();
        const TableContainer& swofTables = tables->getSwofTables();
        const TableContainer& sgofTables = tables->getSgofTables();
        const TableContainer& slgofTables = tables->getSlgofTables();
        const TableContainer& sof3Tables = tables->getSof3Tables();
        const TableContainer& swfnTables = tables->getSwfnTables();
        const TableContainer& sgfnTables = tables->getSgfnTables();


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

        if (family1 && !family2)
            return SaturationFunctionFamily::FamilyI;
        else if (family2 && !family1)
            return SaturationFunctionFamily::FamilyII;
        return SaturationFunctionFamily::noFamily; // no family or two families
    }

    double EndpointInitializer::selectValue(const TableContainer& depthTables,
                                            int tableIdx,
                                            const std::string& columnName,
                                            double cellDepth,
                                            double fallbackValue,
                                            bool useOneMinusTableValue) const {
        double value = fallbackValue;

        if (tableIdx >= 0) {
            const SimpleTable& table = depthTables.getTable( tableIdx );
            if (tableIdx >= static_cast<int>(depthTables.size()))
                throw std::invalid_argument("Not enough tables!");

            // evaluate the table at the cell depth
            value = table.evaluate(columnName, cellDepth);

            if (!std::isfinite(value))
                // a column can be fully defaulted. In this case, eval() returns a NaN
                // and we have to use the data from saturation tables
                value = fallbackValue;
            else if (useOneMinusTableValue)
                value = 1 - value;
        }

        return value;
    }

    void SatnumEndpointInitializer::satnumApply(
            std::vector<double>& values,
            const std::string& columnName,
            const std::vector<double>& fallbackValues,
            const Deck& m_deck,
            const EclipseState& m_eclipseState,
            bool useOneMinusTableValue
            ) const
    {
        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        auto tables = m_eclipseState.getTableManager();
        auto tabdims = tables->getTabdims();
        auto satnum = m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();


        satnum->checkLimits(1 , numSatTables);

        // All table lookup assumes three-phase model
        assert( m_eclipseState.getNumPhases() == 3 );

        this->findSaturationEndpoints( m_eclipseState );
        this->findCriticalPoints( m_eclipseState );
        this->findVerticalPoints( m_eclipseState );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        bool useEnptvd = m_deck.hasKeyword("ENPTVD");
        const auto& enptvdTables = tables->getEnptvdTables();
        for (size_t cellIdx = 0; cellIdx < eclipseGrid->getCartesianSize(); cellIdx++) {
            int satTableIdx = satnum->iget( cellIdx ) - 1;
            int endNum = endnum->iget( cellIdx ) - 1;
            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));


            values[cellIdx] = this->selectValue(enptvdTables,
                                                (useEnptvd && endNum >= 0) ? endNum : -1,
                                                columnName ,
                                                cellDepth,
                                                fallbackValues[satTableIdx],
                                                useOneMinusTableValue);
        }
    }


    void ImbnumEndpointInitializer::imbnumApply(
            std::vector<double>& values,
            const std::string& columnName,
            const std::vector<double>& fallBackValues ,
            const Deck& m_deck,
            const EclipseState& m_eclipseState,
            bool useOneMinusTableValue
            ) const
    {
        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        auto tables = m_eclipseState.getTableManager();
        auto imbnum = m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = m_eclipseState.getIntGridProperty("ENDNUM");

        auto tabdims = tables->getTabdims();
        int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( m_eclipseState );
        this->findCriticalPoints( m_eclipseState );
        this->findVerticalPoints( m_eclipseState );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        bool useImptvd = m_deck.hasKeyword("IMPTVD");
        const TableContainer& imptvdTables = tables->getImptvdTables();
        for (size_t cellIdx = 0; cellIdx < eclipseGrid->getCartesianSize(); cellIdx++) {
            int imbTableIdx = imbnum->iget( cellIdx ) - 1;
            int endNum = endnum->iget( cellIdx ) - 1;
            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));

            values[cellIdx] = this->selectValue(imptvdTables,
                                                (useImptvd && endNum >= 0) ? endNum : -1,
                                                columnName,
                                                cellDepth,
                                                fallBackValues[imbTableIdx],
                                                useOneMinusTableValue);
        }
    }

}
