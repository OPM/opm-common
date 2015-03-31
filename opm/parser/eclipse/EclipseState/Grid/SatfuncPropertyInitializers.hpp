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

    /*
      See the "Saturation Functions" chapter in the Eclipse Technical
      Description; there are several alternative families of keywords
      which can be used to enter relperm and capillary pressure
      tables.
    */


    void apply(std::vector<double>& values,
               const std::string& propertyName) const
    {
        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        auto tabdims = m_eclipseState.getTabdims();
        int numSatTables = tabdims->getNumSatTables();
        const std::vector<int>& satnumData = m_eclipseState.getIntGridProperty("SATNUM")->getData();
        const std::vector<int>& imbnumData = m_eclipseState.getIntGridProperty("IMBNUM")->getData();
        const std::vector<int>& endnumData = m_eclipseState.getIntGridProperty("ENDNUM")->getData();

        assert(satnumData.size() == values.size());
        assert(imbnumData.size() == values.size());


        findSaturationEndpoints( );
        findCriticalPoints( );

        // acctually assign the defaults. if the ENPVD keyword was specified in the deck,
        // this currently cannot be done because we would need the Z-coordinate of the
        // cell and we would need to know how the simulator wants to interpolate between
        // sampling points. Both of these are outside the scope of opm-parser, so we just
        // assign a NaN in this case...
        bool useEnptvd = m_deck.hasKeyword("ENPTVD");
        bool useImptvd = m_deck.hasKeyword("IMPTVD");
        const auto& enptvdTables = m_eclipseState.getEnptvdTables();
        const auto& imptvdTables = m_eclipseState.getImptvdTables();
        for (size_t cellIdx = 0; cellIdx < satnumData.size(); ++cellIdx) {
            int satTableIdx = satnumData[cellIdx] - 1;
            int imbTableIdx = imbnumData[cellIdx] - 1;
            int endNum = endnumData[cellIdx] - 1;

            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));

            assert(0 <= satTableIdx && satTableIdx < numSatTables);
            assert(0 <= imbTableIdx && imbTableIdx < numSatTables);


            // the SWU keyword family
            if (propertyName.find("SWU") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWMAX",
                                              cellDepth,
                                              m_maxWaterSat[satTableIdx],
                                              /*useOneMinusTableValue=*/true);
            else if (propertyName.find("ISWU") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWMAX",
                                              cellDepth,
                                              m_maxWaterSat[imbTableIdx],
                                              /*useOneMinusTableValue=*/true);

            // the SGCR keyword family
            else if (propertyName.find("SGCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SGCRIT",
                                              cellDepth,
                                              m_criticalGasSat[satTableIdx]);
            else if (propertyName.find("ISGCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SGCRIT",
                                              cellDepth,
                                              m_criticalGasSat[imbTableIdx]);

            // the SWCR keyword family
            else if (propertyName.find("SWCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWCRIT",
                                              cellDepth,
                                              m_criticalWaterSat[satTableIdx]);
            else if (propertyName.find("ISWCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SWCRIT",
                                              cellDepth,
                                              m_criticalWaterSat[imbTableIdx]);

            // the SOGCR keyword family
            else if (propertyName.find("SOGCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SOGCRIT",
                                              cellDepth,
                                              m_criticalOilOGSat[satTableIdx]);
            else if (propertyName.find("ISOGCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SOGCRIT",
                                              cellDepth,
                                              m_criticalOilOGSat[imbTableIdx]);

            // the SOWCR keyword family
            else if (propertyName.find("SOWCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SOWCRIT",
                                              cellDepth,
                                              m_criticalOilOWSat[satTableIdx]);
            else if (propertyName.find("ISOWCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SOWCRIT",
                                              cellDepth,
                                              m_criticalOilOWSat[imbTableIdx]);
        }
    }


protected:

    /*
      The method here goes through the SWOF and SGOF tables to
      determine the critical saturations of the various
      phases. The code in question has a hard assumption that
      relperm properties is entered using the SGOF and SWOF
      keywords, however other keyword combinations can be used -
      and then this will break.

      ** Must be fixed. **
      */


    void findSaturationEndpoints( ) const {

        const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
        const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
        auto tabdims = m_eclipseState.getTabdims();
        int numSatTables = tabdims->getNumSatTables();

        if (swofTables.size() == numSatTables) {
            assert(swofTables.size() == sgofTables.size());

            m_minWaterSat.resize( numSatTables , 0 );
            m_maxWaterSat.resize( numSatTables , 0 );
            m_minGasSat.resize( numSatTables , 0 );
            m_maxGasSat.resize( numSatTables , 0 );

            for (int tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
                m_minWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().front();
                m_maxWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().back();

                m_minGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().front();
                m_maxGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().back();
            }
        } else
            throw std::domain_error("Hardcoded assumption absout saturation keyword family has failed");
    }


    void findCriticalPoints( ) const {

        const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
        const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
        auto tabdims = m_eclipseState.getTabdims();
        int numSatTables = tabdims->getNumSatTables();

        m_criticalWaterSat.resize( numSatTables , 0 );
        m_criticalGasSat.resize( numSatTables , 0 );
        m_criticalOilOGSat.resize( numSatTables , 0 );
        m_criticalOilOWSat.resize( numSatTables , 0 );

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
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                if (kroOGCol[rowIdx] == 0.0) {
                    double Sg = sgofTables[tableIdx].getSgColumn()[rowIdx];
                    m_criticalOilOGSat[tableIdx] = 1 - Sg - m_minWaterSat[tableIdx];
                    break;
                }
            }

            // find the critical oil saturation of the water-oil system
            numRows = swofTables[tableIdx].numRows();
            const auto &kroOWCol = swofTables[tableIdx].getKrowColumn();
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                if (kroOWCol[rowIdx] == 0.0) {
                    double Sw = swofTables[tableIdx].getSwColumn()[rowIdx];
                    m_criticalOilOWSat[tableIdx] = 1 - Sw - m_minGasSat[tableIdx];
                    break;
                }
            }
        }
    }



    template <class TableType>
    double selectValue(const std::vector<TableType>& depthTables,
                       int tableIdx,
                       const std::string& columnName,
                       double cellDepth,
                       double fallbackValue,
                       bool useOneMinusTableValue = false) const {
        double value = fallbackValue;

        if (tableIdx >= 0) {
            if (tableIdx >= static_cast<int>(depthTables.size()))
                throw std::invalid_argument("Not enough tables!");

            // evaluate the table at the cell depth
            value = depthTables[tableIdx].evaluate(columnName, cellDepth);

            if (!std::isfinite(value))
                // a column can be fully defaulted. In this case, eval() returns a NaN
                // and we have to use the data from SWOF/SGOF
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
};





template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SGLEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    SGLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto satnum = this->m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();


        satnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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


            values[cellIdx] = selectValue(enptvdTables,
                                          (useEnptvd && endNum >= 0) ? endNum : -1,
                                          "SGCO",
                                          cellDepth,
                                          this->m_minGasSat[satTableIdx]);
        }
    }
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISGLEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    ISGLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto imbnum = this->m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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

            values[cellIdx] = selectValue(imptvdTables,
                                          (useImptvd && endNum >= 0) ? endNum : -1,
                                          "SGCO",
                                          cellDepth,
                                          this->m_minGasSat[imbTableIdx]);
        }
    }
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SGUEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    SGUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto satnum = this->m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();


        satnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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


            values[cellIdx] = selectValue(enptvdTables,
                                          (useEnptvd && endNum >= 0) ? endNum : -1,
                                          "SGMAX",
                                          cellDepth,
                                          this->m_maxGasSat[satTableIdx]);
        }
    }
};


template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISGUEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    ISGUEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto imbnum = this->m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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

            values[cellIdx] = selectValue(imptvdTables,
                                          (useImptvd && endNum >= 0) ? endNum : -1,
                                          "SGMAX",
                                          cellDepth,
                                          this->m_maxGasSat[imbTableIdx]);
        }
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class SWLEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    SWLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto satnum = this->m_eclipseState.getIntGridProperty("SATNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();


        satnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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


            values[cellIdx] = selectValue(enptvdTables,
                                          (useEnptvd && endNum >= 0) ? endNum : -1,
                                          "SWCO",
                                          cellDepth,
                                          this->m_minWaterSat[satTableIdx]);
        }
    }
};



template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class ISWLEndpointInitializer
    : public EndpointInitializer<EclipseState,Deck>
{
public:
    ISWLEndpointInitializer(const Deck& deck, const EclipseState& eclipseState)
        : EndpointInitializer<EclipseState,Deck>( deck , eclipseState )
    { }

    void apply(std::vector<double>& values,
               const std::string& /* propertyname */ ) const
    {
        auto eclipseGrid = this->m_eclipseState.getEclipseGrid();
        auto tabdims = this->m_eclipseState.getTabdims();
        auto imbnum = this->m_eclipseState.getIntGridProperty("IMBNUM");
        auto endnum = this->m_eclipseState.getIntGridProperty("ENDNUM");
        int numSatTables = tabdims->getNumSatTables();

        imbnum->checkLimits(1 , numSatTables);
        this->findSaturationEndpoints( );

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

            values[cellIdx] = selectValue(imptvdTables,
                                          (useImptvd && endNum >= 0) ? endNum : -1,
                                          "SWCO",
                                          cellDepth,
                                          this->m_minWaterSat[imbTableIdx]);
        }
    }
};



}

#endif
