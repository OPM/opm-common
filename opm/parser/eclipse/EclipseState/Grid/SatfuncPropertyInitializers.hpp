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
        size_t numSatTables = tabdims->getNumSatTables();

        if (swofTables.size() == numSatTables) {
            assert(swofTables.size() == sgofTables.size());

            m_minWaterSat.resize( numSatTables , 0 );
            m_maxWaterSat.resize( numSatTables , 0 );
            m_minGasSat.resize( numSatTables , 0 );
            m_maxGasSat.resize( numSatTables , 0 );

            for (size_t tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
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

    void findVerticalPoints( ) const {

        const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
        const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
        auto tabdims = m_eclipseState.getTabdims();
        int numSatTables = tabdims->getNumSatTables();

        m_maxPcog.resize( numSatTables , 0 );
        m_maxPcow.resize( numSatTables , 0 );
        m_maxKrg.resize( numSatTables , 0 );
        m_krgr.resize( numSatTables , 0 );
        m_maxKro.resize( numSatTables , 0 );
        m_krorw.resize( numSatTables , 0 );
        m_krorg.resize( numSatTables , 0 );
        m_maxKrw.resize( numSatTables , 0 );
        m_krwr.resize( numSatTables , 0 );

        for (int tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
            // find the maximum output values of the oil-gas system
            m_maxPcog[tableIdx] = sgofTables[tableIdx].getPcogColumn().front();
            m_maxKrg[tableIdx] = sgofTables[tableIdx].getKrgColumn().back();

            m_krgr[tableIdx] = sgofTables[tableIdx].getKrgColumn().front();
            m_krwr[tableIdx] = swofTables[tableIdx].getKrwColumn().front();

            // find the oil relperm which corresponds to the critical water saturation
            const auto &krwCol = swofTables[tableIdx].getKrwColumn();
            const auto &krowCol = swofTables[tableIdx].getKrowColumn();
            for (int rowIdx = 0; rowIdx < krwCol.size(); ++rowIdx) {
                if (krwCol[rowIdx] == 0.0) {
                    m_krorw[tableIdx] = krowCol[rowIdx - 1];
                    break;
                }
            }

            // find the oil relperm which corresponds to the critical gas saturation
            const auto &krgCol = sgofTables[tableIdx].getKrgColumn();
            const auto &krogCol = sgofTables[tableIdx].getKrogColumn();
            for (int rowIdx = 0; rowIdx < krgCol.size(); ++rowIdx) {
                if (krgCol[rowIdx] == 0.0) {
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
        this->satnumApply(values , "SWCRIT" , this->m_criticalOilOWSat , false);
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
