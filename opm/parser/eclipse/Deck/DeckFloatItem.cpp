/*
  Copyright 2013 Statoil ASA.

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

#include <opm/parser/eclipse/Deck/DeckFloatItem.hpp>

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <iostream>
#include <cmath>

namespace Opm {

    float DeckFloatItem::getRawFloat(size_t index) const {
        if (index < m_data.size()) {
            return m_data[index];
        } else
            throw std::out_of_range("Out of range, index must be lower than " + boost::lexical_cast<std::string>(m_data.size()));
    }

    size_t DeckFloatItem::size() const {
        return m_data.size();
    }

    float DeckFloatItem::getSIFloat(size_t index) const {
        assertSIData();
        {
            if (index < m_data.size()) {
                return m_SIdata[index];
            } else
                throw std::out_of_range("Out of range, index must be lower than " + boost::lexical_cast<std::string>(m_data.size()));
        }
    }

    const std::vector<float>& DeckFloatItem::getSIFloatData() const {
        assertSIData();
        return m_SIdata;
    }

    void DeckFloatItem::assertSIData() const {
        if (m_dimensions.size() > 0) {
            if (m_SIdata.size() > 0) {
                // we already converted this item to SI!
                return;
            }
            m_SIdata.resize( m_data.size() );
            if (m_dimensions.size() == 1) {
                float SIfactor = m_dimensions[0]->getSIScaling();
                std::transform( m_data.begin() , m_data.end() , m_SIdata.begin() , std::bind1st(std::multiplies<float>(),SIfactor));
            } else {
                for (size_t index=0; index < m_data.size(); index++) {
                    size_t dimIndex = (index % m_dimensions.size());
                    float SIfactor = m_dimensions[dimIndex]->getSIScaling();
                    m_SIdata[index] = m_data[index] * SIfactor;
                }
            }
        } else
            throw std::invalid_argument("No dimension has been set for item:" + name() + " can not ask for SI data");
    }

    void DeckFloatItem::push_back(std::deque<float> data , size_t items) {
        for (size_t i=0; i<items; i++) {
            m_data.push_back(data[i]);
        }
        m_valueStatus |= DeckValue::SET_IN_DECK;
    }

    void DeckFloatItem::push_back(std::deque<float> data) {
        push_back( data  , data.size() );
        m_valueStatus |= DeckValue::SET_IN_DECK;
    }

    void DeckFloatItem::push_back(float data) {
        m_data.push_back( data );
        m_valueStatus |= DeckValue::SET_IN_DECK;
    }


    void DeckFloatItem::push_backMultiple(float value, size_t numValues) {
        for (size_t i = 0; i < numValues; i++)
            m_data.push_back( value );
        m_valueStatus |= DeckValue::SET_IN_DECK;
    }


    void DeckFloatItem::push_backDefault(float data) {
        m_data.push_back( data );
        m_valueStatus |= DeckValue::DEFAULT;
    }


    void DeckFloatItem::push_backDimension(std::shared_ptr<const Dimension> activeDimension , std::shared_ptr<const Dimension> defaultDimension) {
        if (m_valueStatus == DeckValue::DEFAULT)
            m_dimensions.push_back( defaultDimension );
        else
            m_dimensions.push_back( activeDimension );
    }

}
