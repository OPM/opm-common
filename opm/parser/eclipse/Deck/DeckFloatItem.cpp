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
#include <opm/parser/eclipse/Units/Dimension.hpp>

#include <algorithm>
#include <iostream>
#include <cmath>

namespace Opm {

    float DeckFloatItem::getRawFloat(size_t index) const {
        assertSize(index);

        return m_data[index];
    }

    size_t DeckFloatItem::size() const {
        return m_data.size();
    }

    float DeckFloatItem::getSIFloat(size_t index) const {
        assertSize(index);
        assertSIData();

        return m_SIdata[index];
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

            for (size_t index=0; index < m_data.size(); index++) {
                size_t dimIndex = (index % m_dimensions.size());
                m_SIdata[index] = m_dimensions[dimIndex]->convertRawToSi(m_data[index]);
            }
        } else
            throw std::invalid_argument("No dimension has been set for item:" + name() + " can not ask for SI data");
    }

    void DeckFloatItem::push_back(std::deque<float> data , size_t items) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for (size_t i=0; i<items; i++) {
            m_data.push_back(data[i]);
            m_dataPointDefaulted.push_back(false);
        }
    }

    void DeckFloatItem::push_back(std::deque<float> data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        push_back( data  , data.size() );
        m_dataPointDefaulted.push_back(false);
    }

    void DeckFloatItem::push_back(float data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        m_data.push_back( data );
        m_dataPointDefaulted.push_back(false);
    }



    void DeckFloatItem::push_backMultiple(float value, size_t numValues) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for (size_t i = 0; i < numValues; i++) {
            m_data.push_back( value );
            m_dataPointDefaulted.push_back(false);
        }
    }


    void DeckFloatItem::push_backDefault(float data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        m_data.push_back( data );
        m_dataPointDefaulted.push_back(true);
    }

    void DeckFloatItem::push_backDummyDefault() {
        if (m_dataPointDefaulted.size() != 0)
            throw std::logic_error("Pseudo defaults can only be specified for empty items");

        m_dataPointDefaulted.push_back(true);
    }


    void DeckFloatItem::push_backDimension(std::shared_ptr<const Dimension> activeDimension , std::shared_ptr<const Dimension> defaultDimension) {
        if (m_dataPointDefaulted.empty() || m_dataPointDefaulted.back())
            m_dimensions.push_back( defaultDimension );
        else
            m_dimensions.push_back( activeDimension );
    }

}
