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
#ifndef ECLIPSE_GRIDPROPERTY_INITIALIZERS_HPP
#define ECLIPSE_GRIDPROPERTY_INITIALIZERS_HPP

#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>

#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <algorithm>
#include <cmath>

#include <cassert>

/*
  This classes initialize GridProperty objects. Most commonly, they just get a constant
  value but some properties (e.g, some of these related to endpoint scaling) need more
  complex schemes like table lookups.
*/
namespace Opm {

// forward definitions
class Deck;
class EclipseState;
class EnptvdTable;
class ImptvdTable;

template <class ValueType>
class GridPropertyBaseInitializer
{
protected:
    GridPropertyBaseInitializer()
    { }

public:
    virtual void apply(std::vector<ValueType>& values,
                       const std::string& propertyName) const = 0;
};

template <class ValueType>
class GridPropertyConstantInitializer
    : public GridPropertyBaseInitializer<ValueType>
{
public:
    GridPropertyConstantInitializer(const ValueType& value)
        : m_value(value)
    { }

    void apply(std::vector<ValueType>& values,
               const std::string& /*propertyName*/) const
    {
        std::fill(values.begin(), values.end(), m_value);
    }

private:
    ValueType m_value;
};

template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class GridPropertyEndpointTableLookupInitializer
    : public GridPropertyBaseInitializer<double>
{
public:
    GridPropertyEndpointTableLookupInitializer(const Deck& deck, const EclipseState& eclipseState)
        : m_deck(deck)
        , m_eclipseState(eclipseState)
    { }

    void apply(std::vector<double>& values,
               const std::string& propertyName) const
    {
        auto eclipseGrid = m_eclipseState.getEclipseGrid();

        // calculate drainage and imbibition saturation table of each cell
        std::vector<int> satnumData(values.size(), 1);
        if (m_eclipseState.hasIntGridProperty("SATNUM"))
            satnumData = m_eclipseState.getIntGridProperty("SATNUM")->getData();

        // TODO (?): IMBNUM[XYZ]-?
        std::vector<int> imbnumData(satnumData);
        if (m_eclipseState.hasIntGridProperty("IMBNUM"))
            imbnumData = m_eclipseState.getIntGridProperty("IMBNUM")->getData();

        assert(satnumData.size() == values.size());
        assert(imbnumData.size() == values.size());

        // calculate drainage and imbibition saturation table of each cell
        std::vector<int> endnumData(values.size(), 1);
        if (m_eclipseState.hasIntGridProperty("ENDNUM"))
            satnumData = m_eclipseState.getIntGridProperty("ENDNUM")->getData();

        // create the SWOF tables
        const std::vector<SwofTable>& swofTables = m_eclipseState.getSwofTables();
        int numSatTables = swofTables.size();

        // create the SGOF tables
        const std::vector<SgofTable>& sgofTables = m_eclipseState.getSgofTables();
        assert(swofTables.size() == sgofTables.size());

        // find the critical saturations for each table
        std::vector<double> criticalGasSat(numSatTables, 0.0);
        std::vector<double> criticalWaterSat(numSatTables, 0.0);
        std::vector<double> criticalOilOWSat(numSatTables, 0.0);
        std::vector<double> criticalOilOGSat(numSatTables, 0.0);

        std::vector<double> minGasSat(numSatTables, 1.0);
        std::vector<double> maxGasSat(numSatTables, 0.0);
        std::vector<double> minWaterSat(numSatTables, 1.0);
        std::vector<double> maxWaterSat(numSatTables, 0.0);
        for (int tableIdx = 0; tableIdx < numSatTables; ++tableIdx) {
            minWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().front();
            maxWaterSat[tableIdx] = swofTables[tableIdx].getSwColumn().back();

            minGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().front();
            maxGasSat[tableIdx] = sgofTables[tableIdx].getSgColumn().back();

            // find the critical water saturation
            int numRows = swofTables[tableIdx].numRows();
            const auto &krwCol = swofTables[tableIdx].getKrwColumn();
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                if (krwCol[rowIdx] > 0.0) {
                    double Sw = 0.0;
                    if (rowIdx > 0)
                        Sw = swofTables[tableIdx].getSwColumn()[rowIdx - 1];
                    criticalWaterSat[tableIdx] = Sw;
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
                    criticalGasSat[tableIdx] = Sg;
                    break;
                }
            }

            // find the critical oil saturation of the oil-gas system
            numRows = sgofTables[tableIdx].numRows();
            const auto &kroOGCol = sgofTables[tableIdx].getKrogColumn();
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                if (kroOGCol[rowIdx] == 0.0) {
                    double Sg = sgofTables[tableIdx].getSgColumn()[rowIdx];
                    criticalOilOGSat[tableIdx] = 1 - Sg - minWaterSat[tableIdx];
                    break;
                }
            }

            // find the critical oil saturation of the water-oil system
            numRows = swofTables[tableIdx].numRows();
            const auto &kroOWCol = swofTables[tableIdx].getKrowColumn();
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx) {
                if (kroOWCol[rowIdx] == 0.0) {
                    double Sw = swofTables[tableIdx].getSwColumn()[rowIdx];
                    criticalOilOWSat[tableIdx] = 1 - Sw - minGasSat[tableIdx];
                    break;
                }
            }
        }

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
            int satTableIdx = satnumData.at(cellIdx) - 1;
            int imbTableIdx = imbnumData.at(cellIdx) - 1;
            int endNum = endnumData.at(cellIdx) - 1;

            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));

            assert(0 <= satTableIdx && satTableIdx < numSatTables);
            assert(0 <= imbTableIdx && imbTableIdx < numSatTables);

            // the SGL keyword family
            if (propertyName.find("SGL") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SGCO",
                                              cellDepth,
                                              minGasSat[satTableIdx]);
            else if (propertyName.find("ISGL") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SGCO",
                                              cellDepth,
                                              minGasSat[imbTableIdx]);

            // the SWL keyword family
            else if (propertyName.find("SWL") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWCO",
                                              cellDepth,
                                              minWaterSat[satTableIdx]);
            else if (propertyName.find("ISWL") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SWCO",
                                              cellDepth,
                                              minWaterSat[imbTableIdx]);

            // the SGU keyword family
            else if (propertyName.find("SGU") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SGMAX",
                                              cellDepth,
                                              maxGasSat[satTableIdx]);
            else if (propertyName.find("ISGU") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SGMAX",
                                              cellDepth,
                                              maxGasSat[imbTableIdx]);

            // the SWU keyword family
            else if (propertyName.find("SWU") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWCO",
                                              cellDepth,
                                              maxWaterSat[satTableIdx],
                                              /*useOneMinusTableValue=*/true);
            else if (propertyName.find("ISWU") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWCO",
                                              cellDepth,
                                              maxWaterSat[imbTableIdx],
                                              /*useOneMinusTableValue=*/true);

            // the SGCR keyword family
            else if (propertyName.find("SGCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SGCRIT",
                                              cellDepth,
                                              criticalGasSat[satTableIdx]);
            else if (propertyName.find("ISGCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SGCRIT",
                                              cellDepth,
                                              criticalGasSat[imbTableIdx]);

            // the SWCR keyword family
            else if (propertyName.find("SWCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SWCRIT",
                                              cellDepth,
                                              criticalWaterSat[satTableIdx]);
            else if (propertyName.find("ISWCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SWCRIT",
                                              cellDepth,
                                              criticalWaterSat[imbTableIdx]);

            // the SOGCR keyword family
            else if (propertyName.find("SOGCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SOGCRIT",
                                              cellDepth,
                                              criticalOilOGSat[satTableIdx]);
            else if (propertyName.find("ISOGCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SOGCRIT",
                                              cellDepth,
                                              criticalOilOGSat[imbTableIdx]);

            // the SOWCR keyword family
            else if (propertyName.find("SOWCR") == 0)
                values[cellIdx] = selectValue(enptvdTables,
                                              (useEnptvd && endNum >= 0) ? endNum : -1,
                                              "SOWCRIT",
                                              cellDepth,
                                              criticalOilOWSat[satTableIdx]);
            else if (propertyName.find("ISOWCR") == 0)
                values[cellIdx] = selectValue(imptvdTables,
                                              (useImptvd && endNum >= 0) ? endNum : -1,
                                              "SOWCRIT",
                                              cellDepth,
                                              criticalOilOWSat[imbTableIdx]);
        }
    }

private:
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
};
}

#endif
