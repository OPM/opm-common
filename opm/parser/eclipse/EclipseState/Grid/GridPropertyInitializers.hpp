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

#include <vector>
#include <string>
#include <exception>
#include <memory>
#include <limits>
#include <algorithm>
#include <cmath>

#include <cassert>

#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>



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





// initialize the TEMPI grid property using the temperature vs depth
// table (stemming from the TEMPVD or the RTEMPVD keyword)
template <class EclipseState=Opm::EclipseState,
          class Deck=Opm::Deck>
class GridPropertyTemperatureLookupInitializer
    : public GridPropertyBaseInitializer<double>
{
public:
    GridPropertyTemperatureLookupInitializer(const Deck& deck, const EclipseState& eclipseState)
        : m_deck(deck)
        , m_eclipseState(eclipseState)
    { }

    void apply(std::vector<double>& values,
               const std::string& propertyName) const
    {
        if (propertyName != "TEMPI")
            throw std::logic_error("The TemperatureLookupInitializer can only be used for the initial temperature!");

        if (!m_deck.hasKeyword("EQLNUM")) {
            // if values are defaulted in the TEMPI keyword, but no
            // EQLNUM is specified, you will get NaNs...
            double nan = std::numeric_limits<double>::quiet_NaN();
            std::fill(values.begin(), values.end(), nan);
            return;
        }

        auto eclipseGrid = m_eclipseState.getEclipseGrid();
        const std::vector<RtempvdTable>& rtempvdTables = m_eclipseState.getRtempvdTables();
        const std::vector<int>& eqlNum = m_eclipseState.getIntGridProperty("EQLNUM")->getData();

        for (size_t cellIdx = 0; cellIdx < eqlNum.size(); ++ cellIdx) {
            int cellEquilNum = eqlNum[cellIdx];
            const RtempvdTable& rtempvdTable = rtempvdTables[cellEquilNum];
            double cellDepth = std::get<2>(eclipseGrid->getCellCenter(cellIdx));
            values[cellIdx] = rtempvdTable.evaluate("Temperature", cellDepth);
        }
    }

private:
    const Deck& m_deck;
    const EclipseState& m_eclipseState;
};

}

#endif
