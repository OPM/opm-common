/*
  Copyright 2014 Andreas Lauser

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
#ifndef ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP
#define ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP

#include <vector>
#include <string>
#include <exception>
#include <memory>
#include <limits>
#include <algorithm>
#include <cmath>

#include <cassert>

#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/GridPropertyInitializers.hpp>

namespace Opm {

// forward definitions
class Deck;
class EclipseState;
class EnptvdTable;
class ImptvdTable;


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class EndpointInitializer
    : public GridPropertyBaseInitializer<double>
{
public:
    EndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : m_deck(deck)
        , m_eclipseState(eclipseState)
    { }

    enum SaturationFunctionFamily { noFamily = 0, FamilyI = 1, FamilyII = 2};

    /*
      See the "Saturation Functions" chapter in the Eclipse Technical
      Description; there are several alternative families of keywords
      which can be used to enter relperm and capillary pressure
      tables.
    */


protected:

    /*
      The method here goes through the saturation function tables
      Either family I (SWOF,SGOF) or family II (SWFN, SGFN and SOF3)
      must be specified. Other keyword alternatives like SLGOF, SOF2
      and SGWFN and the two dimensional saturation tables
      are currently not supported.

      ** Must be fixed. **
      */


    void findSaturationEndpoints( ) const {

        auto tabdims = m_eclipseState.getTabdims();
        size_t numSatTables = tabdims->getNumSatTables();
        m_minWaterSat.resize( numSatTables , 0 );
        m_maxWaterSat.resize( numSatTables , 0 );
        m_minGasSat.resize( numSatTables , 0 );
        m_maxGasSat.resize( numSatTables , 0 );

        switch (getSaturationFunctionFamily()) {
        case SaturationFunctionFamily::FamilyI:
        {
            const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
            const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
            assert(swofTables.size() == numSatTables);
            assert(swofTables.size() == numSatTables);
            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                m_minWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().front();
                m_maxWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().back();

                m_minGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().front();
                m_maxGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().back();
            }
            break;

        }
        case SaturationFunctionFamily::FamilyII:
        {
            const std::vector<SwfnTable>& swfnTables = m_eclipseState.getSwfnTables();
            const std::vector<SgfnTable>& sgfnTables = m_eclipseState.getSgfnTables();
            assert(swfnTables.size() == numSatTables);
            assert(sgfnTables.size() == numSatTables);
            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                m_minWaterSat[tableIdx] = swfnTables[tableIdx].getSwColumn().front();
                m_maxWaterSat[tableIdx] = swfnTables[tableIdx].getSwColumn().back();

                m_minGasSat[tableIdx] = sgfnTables[tableIdx].getSgColumn().front();
                m_maxGasSat[tableIdx] = sgfnTables[tableIdx].getSgColumn().back();
            }
            break;
        }
        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }
    }


    void findCriticalPoints( ) const {

        auto tabdims = m_eclipseState.getTabdims();
        int numSatTables = tabdims->getNumSatTables();

        m_criticalWaterSat.resize( numSatTables , 0 );
        m_criticalGasSat.resize( numSatTables , 0 );
        m_criticalOilOGSat.resize( numSatTables , 0 );
        m_criticalOilOWSat.resize( numSatTables , 0 );

        switch (getSaturationFunctionFamily()) {
        case SaturationFunctionFamily::FamilyI:
        {
            const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
            const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();

            for (int tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                // find the critical water saturation
                int numRows = swofTables[tableIdx].numRows();
                const auto &krwCol = swofTables[tableIdx].getKrwColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (krwCol[rowIdx] > 0.0) {
                        double Sw = 0.0;
                        if (rowIdx > 0)
                            Sw = swofTables[tableIdx].getSwColumn()[rowIdx - 1];
                        m_criticalWaterSat[tableIdx] = Sw;
                        break;
                    }
                }

                // find the critical gas saturation
                numRows = sgofTables[tableIdx].numRows();
                const auto &krgCol = sgofTables[tableIdx].getKrgColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (krgCol[rowIdx] > 0.0) {
                        double Sg = 0.0;
                        if (rowIdx > 0)
                            Sg = sgofTables[tableIdx].getSgColumn()[rowIdx - 1];
                        m_criticalGasSat[tableIdx] = Sg;
                        break;
                    }
                }

                // find the critical oil saturation of the oil-gas system
                numRows = sgofTables[tableIdx].numRows();
                const auto &kroOGCol = sgofTables[tableIdx].getKrogColumn();
                for (int rowIdx = numRows - 1; rowIdx >= 0; --rowIdx) {
                    if (kroOGCol[rowIdx] > 0.0) {
                        double Sg = sgofTables[tableIdx].getSgColumn()[rowIdx + 1];
                        m_criticalOilOGSat[tableIdx] = 1 - Sg;
                        break;
                    }
                }

                // find the critical oil saturation of the water-oil system
                numRows = swofTables[tableIdx].numRows();
                const auto &kroOWCol = swofTables[tableIdx].getKrowColumn();
                for (int rowIdx = numRows - 1; rowIdx >= 0; --rowIdx) {
                    if (kroOWCol[rowIdx] > 0.0) {
                        double Sw = swofTables[tableIdx].getSwColumn()[rowIdx + 1];
                        m_criticalOilOWSat[tableIdx] = 1 - Sw;
                        break;
                    }
                }
            }
            break;

        }
        case SaturationFunctionFamily::FamilyII: {
            const std::vector<SwfnTable>& swfnTables = m_eclipseState.getSwfnTables();
            const std::vector<SgfnTable>& sgfnTables = m_eclipseState.getSgfnTables();
            const std::vector<Sof3Table>& sof3Tables = m_eclipseState.getSof3Tables();

            for (int tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                // find the critical water saturation
                int numRows = swfnTables[tableIdx].numRows();
                const auto &krwCol = swfnTables[tableIdx].getKrwColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (krwCol[rowIdx] > 0.0) {
                        double Sw = 0.0;
                        if (rowIdx > 0)
                            Sw = swfnTables[tableIdx].getSwColumn()[rowIdx - 1];
                        m_criticalWaterSat[tableIdx] = Sw;
                        break;
                    }
                }

                // find the critical gas saturation
                numRows = sgfnTables[tableIdx].numRows();
                const auto &krgCol = sgfnTables[tableIdx].getKrgColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (krgCol[rowIdx] > 0.0) {
                        double Sg = 0.0;
                        if (rowIdx > 0)
                            Sg = sgfnTables[tableIdx].getSgColumn()[rowIdx - 1];
                        m_criticalGasSat[tableIdx] = Sg;
                        break;
                    }
                }

                // find the critical oil saturation of the oil-gas system
                numRows = sof3Tables[tableIdx].numRows();
                const auto &kroOGCol = sof3Tables[tableIdx].getKrogColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (kroOGCol[rowIdx] > 0.0) {
                        double So = sof3Tables[tableIdx].getSoColumn()[rowIdx - 1];
                        m_criticalOilOGSat[tableIdx] = So;
                        break;
                    }
                }

                // find the critical oil saturation of the water-oil system
                numRows = sof3Tables[tableIdx].numRows();
                const auto &kroOWCol = sof3Tables[tableIdx].getKrowColumn();
                for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                    if (kroOWCol[rowIdx] > 0.0) {
                        double So = sof3Tables[tableIdx].getSoColumn()[rowIdx - 1];
                        m_criticalOilOWSat[tableIdx] = So;
                        break;
                    }
                }
            }
            break;

        }
        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }


    }

    void findVerticalPoints( ) const {

        auto tabdims = m_eclipseState.getTabdims();
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

        switch (getSaturationFunctionFamily()) {
        case SaturationFunctionFamily::FamilyI:
        {
            const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
            const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                // find the maximum output values of the oil-gas system
                m_maxPcog[tableIdx] = sgofTables[tableIdx].getPcogColumn().front();
                m_maxKrg[tableIdx] = sgofTables[tableIdx].getKrgColumn().back();

                m_krgr[tableIdx] = sgofTables[tableIdx].getKrgColumn().front();
                m_krwr[tableIdx] = swofTables[tableIdx].getKrwColumn().front();

                // find the oil relperm which corresponds to the critical water saturation
                const auto &krwCol = swofTables[tableIdx].getKrwColumn();
                const auto &krowCol = swofTables[tableIdx].getKrowColumn();
                for (size_t rowIdx = 0; rowIdx < krwCol.size(); ++rowIdx) {
                    if (krwCol[rowIdx] > 0.0) {
                        m_krorw[tableIdx] = krowCol[rowIdx - 1];
                        break;
                    }
                }

                // find the oil relperm which corresponds to the critical gas saturation
                const auto &krgCol = sgofTables[tableIdx].getKrgColumn();
                const auto &krogCol = sgofTables[tableIdx].getKrogColumn();
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
                m_maxPcow[tableIdx] = swofTables[tableIdx].getPcowColumn().front();
                m_maxKro[tableIdx] = swofTables[tableIdx].getKrowColumn().front();
                m_maxKrw[tableIdx] = swofTables[tableIdx].getKrwColumn().back();
            }
            break;
        }
        case SaturationFunctionFamily::FamilyII: {
            const std::vector<SwfnTable>& swfnTables = m_eclipseState.getSwfnTables();
            const std::vector<SgfnTable>& sgfnTables = m_eclipseState.getSgfnTables();
            const std::vector<Sof3Table>& sof3Tables = m_eclipseState.getSof3Tables();

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                // find the maximum output values of the oil-gas system
                m_maxPcog[tableIdx] = sgfnTables[tableIdx].getPcogColumn().back();
                m_maxKrg[tableIdx] = sgfnTables[tableIdx].getKrgColumn().back();

                // find the minimum output values of the relperm
                m_krgr[tableIdx] = sgfnTables[tableIdx].getKrgColumn().front();
                m_krwr[tableIdx] = swfnTables[tableIdx].getKrwColumn().front();

                // find the oil relperm which corresponds to the critical water saturation
                const double OilSatAtcritialWaterSat = 1.0 - m_criticalWaterSat[tableIdx] - m_minGasSat[tableIdx];
                m_krorw[tableIdx] = sof3Tables[tableIdx].evaluate("KROW", OilSatAtcritialWaterSat);

                // find the oil relperm which corresponds to the critical gas saturation
                const double OilSatAtCritialGasSat = 1.0 - m_criticalGasSat[tableIdx] - m_minWaterSat[tableIdx];
                m_krorg[tableIdx] = sof3Tables[tableIdx].evaluate("KROG", OilSatAtCritialGasSat);

                // find the maximum output values of the water-oil system. the maximum oil
                // relperm is possibly wrong because we have two oil relperms in a threephase
                // system. the documentation is very ambiguos here, though: it says that the
                // oil relperm at the maximum oil saturation is scaled according to maximum
                // specified the KRO keyword. the first part of the statement points at
                // scaling the resultant threephase oil relperm, but then the gas saturation
                // is not taken into account which means that some twophase quantity must be
                // scaled.
                m_maxPcow[tableIdx] = swfnTables[tableIdx].getPcowColumn().front();
                m_maxKro[tableIdx] = sof3Tables[tableIdx].getKrowColumn().back();
                m_maxKrw[tableIdx] = swfnTables[tableIdx].getKrwColumn().back();
            }
            break;
        }

        default:
            throw std::domain_error("No valid saturation keyword family specified");
        }

    }
    // The saturation function family.
    // If SWOF and SGOF are specified in the deck it return FamilyI
    // If SWFN, SGFN and SOF3 are specified in the deck it return FamilyII
    // If keywords are missing or mixed, an error is given.
    const SaturationFunctionFamily getSaturationFunctionFamily() const{

        const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
        const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
        const std::vector<SwfnTable>& swfnTables = m_eclipseState.getSwfnTables();
        const std::vector<SgfnTable>& sgfnTables = m_eclipseState.getSgfnTables();
        const std::vector<Sof3Table>& sof3Tables = m_eclipseState.getSof3Tables();

        bool family1 = !sgofTables.empty() && !swofTables.empty();
        bool family2 = !swfnTables.empty() && !sgfnTables.empty() && !sof3Tables.empty();

        if (family1 && family2) {
            throw std::invalid_argument("Saturation families should not be mixed \n"
                                        "Use either SGOF and SWOF or SGFN, SWFN and SOF3");
        }

        if (!family1 && !family2) {
            throw std::invalid_argument("Saturations function must be specified using either "
                                        "family 1 or family 2 keywords \n"
                                        "Use either SGOF and SWOF or SGFN, SWFN and SOF3" );
        }

        if (family1 && !family2)
            return SaturationFunctionFamily::FamilyI;
        else if (family2 && !family1)
            return SaturationFunctionFamily::FamilyII;
        return SaturationFunctionFamily::noFamily; // no family or two families
    }


    template <class TableType>
    double selectValue(const std::vector<TableType>& depthTables,
                       int tableIdx,
                       const std::string& columnName,
                       double cellDepth,
                       double fallbackValue,
                       bool useOneMinusTableValue) const {
        double value = fallbackValue;

        if (tableIdx >= 0) {
            if (tableIdx >= static_cast<int>(depthTables.size()))
                throw std::invalid_argument("Not enough tables!");

            // evaluate the table at the cell depth
            value = depthTables[tableIdx].evaluate(columnName, cellDepth);

            if (!std::isfinite(value))
                // a column can be fully defaulted. In this case, eval() returns a NaN
                // and we have to use the data from saturation tables
                value = fallbackValue;
            else if (useOneMinusTableValue)
                value = 1 - value;
        }

        return value;
    }

    const Deck& m_deck;
    const EclipseState& m_eclipseState;

    mutable std::vector<double> m_criticalGasSat;
    mutable std::vector<double> m_criticalWaterSat;
    mutable std::vector<double> m_criticalOilOWSat;
    mutable std::vector<double> m_criticalOilOGSat;

    mutable std::vector<double> m_minGasSat;
    mutable std::vector<double> m_maxGasSat;
    mutable std::vector<double> m_minWaterSat;
    mutable std::vector<double> m_maxWaterSat;

    mutable std::vector<double> m_maxPcow;
    mutable std::vector<double> m_maxPcog;
    mutable std::vector<double> m_maxKrw;
    mutable std::vector<double> m_krwr;
    mutable std::vector<double> m_maxKro;
    mutable std::vector<double> m_krorw;
    mutable std::vector<double> m_krorg;
    mutable std::vector<double> m_maxKrg;
    mutable std::vector<double> m_krgr;
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SatnumEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    SatnumEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& ) const = 0;


    void satnumApply( std::vector<double>& values,
                      const std::string& columnName,
                      const std::vector<double>& fallbackValues,
                      bool useOneMinusTableValue) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto satnum = this->m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();


        satnum->checkLimits(1 , numSatTables);

        // All table lookup assumes three-phase model
        assert( m_eclipseState.getNumPhases() == 3 );

        this->findSaturationEndpoints( );
        this->findCriticalPoints( );
        this->findVerticalPoints( );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        bool useEnptvd = this->m_deck.hasKeyword("ENPTVD");
        const auto& enptvdTables = this->m_eclipseState.getEnptvdTables();
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
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ImbnumEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    ImbnumEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& ) const = 0;

    void imbnumApply( std::vector<double>& values,
                      const std::string& columnName,
                      const std::vector<double>& fallBackValues ,
                      bool useOneMinusTableValue) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto imbnum = this->m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );
        this->findCriticalPoints( );
        this->findVerticalPoints( );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        bool useImptvd = this->m_deck.hasKeyword("IMPTVD");
        const auto& imptvdTables = this->m_eclipseState.getImptvdTables();
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
};


/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SGLEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SGLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SGCO" , this->m_minGasSat , false);
    }
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISGLEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISGLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SGCO" , this->m_minGasSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SGUEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SGUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SGMAX" , this->m_maxGasSat, false);
    }
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISGUEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISGUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SGMAX" , this->m_maxGasSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SWLEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SWLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SWCO" , this->m_minWaterSat , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISWLEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISWLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SWCO" , this->m_minWaterSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SWUEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SWUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SWMAX" , this->m_maxWaterSat , true);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISWUEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISWUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SWMAX" , this->m_maxWaterSat , true);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SGCREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SGCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SGCRIT" , this->m_criticalGasSat , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISGCREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISGCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SGCRIT" , this->m_criticalGasSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SOWCREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SOWCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SOWCRIT", this->m_criticalOilOWSat , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISOWCREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISOWCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SOWCRIT" , this->m_criticalOilOWSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SOGCREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SOGCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SOGCRIT" , this->m_criticalOilOGSat , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISOGCREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISOGCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SOGCRIT" , this->m_criticalOilOGSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SWCREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    SWCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "SWCRIT" , this->m_criticalWaterSat , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISWCREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    ISWCREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "SWCRIT" , this->m_criticalWaterSat , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class PCWEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    PCWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "PCW" , this->m_maxPcow , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IPCWEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IPCWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IPCW" , this->m_maxPcow , false);
    }
};


/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class PCGEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    PCGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "PCG" , this->m_maxPcog , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IPCGEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IPCGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IPCG" , this->m_maxPcog , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRWEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRW" , this->m_maxKrw , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRWEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRW" , this->m_maxKrw , false);
    }
};


/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRWREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRWREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRWR" , this->m_krwr , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRWREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRWREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRWR" , this->m_krwr , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KROEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KROEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRO" , this->m_maxKro , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKROEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKROEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRO" , this->m_maxKro , false);
    }
};


/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRORWEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRORWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRORW" , this->m_krorw , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRORWEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRORWEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRORW" , this->m_krorw , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRORGEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRORGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRORG" , this->m_krorg , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRORGEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRORGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRORG" , this->m_krorg , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRGEndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRG" , this->m_maxKrg , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRGEndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRGEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRG" , this->m_maxKrg , false);
    }
};

/*****************************************************************/

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class KRGREndpointInitializer
    : public SatnumEndpointInitializer<EclipseState,Deck>
{
public:
    KRGREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : SatnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->satnumApply(values , "KRGR" , this->m_krgr , false);
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class IKRGREndpointInitializer
    : public ImbnumEndpointInitializer<EclipseState,Deck>
{
public:
    IKRGREndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : ImbnumEndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values) const
    {
        this->imbnumApply(values , "IKRGR" , this->m_krgr , false);
    }
};

}

#endif
