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

/*
  This classes initialize GridProperty objects. Most commonly, they just get a constant
  value but some properties (e.g, some of these related to endpoint scaling) need more
  complex schemes like table lookups.
*/
namespace Opm {

class Deck;
class EclipseState;

template <class ValueType>
class GridPropertyBaseInitializer
{
protected:
    GridPropertyBaseInitializer()
    { }

public:
    virtual void apply(std::vector<ValueType>& values, const Deck&, const EclipseState& ) const = 0;
};

template <class ValueType>
class GridPropertyConstantInitializer
    : public GridPropertyBaseInitializer<ValueType>
{
public:
    GridPropertyConstantInitializer(const ValueType& value)
        : m_value(value)
    { }

    void apply(std::vector<ValueType>& values, const Deck&, const EclipseState& ) const;

private:
    ValueType m_value;
};

// initialize the TEMPI grid property using the temperature vs depth
// table (stemming from the TEMPVD or the RTEMPVD keyword)
class GridPropertyTemperatureLookupInitializer
    : public GridPropertyBaseInitializer<double>
{
public:
    void apply(std::vector<double>& values, const Deck&, const EclipseState& ) const;
};

template< typename T >
class GridPropertyFunction {
    public:
        GridPropertyFunction() = default;

        GridPropertyFunction( std::shared_ptr< GridPropertyBaseInitializer< T > >,
                              const Deck*,
                              const EclipseState* );

        std::vector< T >& operator()( std::vector< T >& ) const;

    private:
        std::shared_ptr< GridPropertyBaseInitializer< T > > f;
        const Deck* deck = nullptr;
        const EclipseState* es = nullptr;
};

}

#endif
