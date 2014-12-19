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

#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>

#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <iostream>
#include <cmath>

namespace Opm {

    double DeckDoubleItem::getRawDouble(size_t index) const {
        assertSize(index);

        return m_data[index];
    }


    const std::vector<double>& DeckDoubleItem::getRawDoubleData() const {
        return m_data;
    }


    void DeckDoubleItem::assertSIData() const {
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



    double DeckDoubleItem::getSIDouble(size_t index) const {
        assertSize(index);
        assertSIData();

        return m_SIdata[index];
    }

    const std::vector<double>& DeckDoubleItem::getSIDoubleData() const {
        assertSIData();

        return m_SIdata;
    }


    void DeckDoubleItem::push_back(std::deque<double> data , size_t items) {
        for (size_t i=0; i<items; i++) {
            m_data.push_back(data[i]);
            m_dataPointDefaulted.push_back(false);
        }
    }


    void DeckDoubleItem::push_back(std::deque<double> data) {
        push_back( data  , data.size() );
        m_dataPointDefaulted.push_back(false);
    }

    void DeckDoubleItem::push_back(double data) {
        m_data.push_back( data );
        m_dataPointDefaulted.push_back(false);
    }

    void DeckDoubleItem::push_backMultiple(double value, size_t numValues) {
        for (size_t i = 0; i < numValues; i++) {
            m_data.push_back( value );
            m_dataPointDefaulted.push_back(false);
        }
    }


    void DeckDoubleItem::push_backDefault(double data) {
        m_data.push_back( data );
        m_dataPointDefaulted.push_back(true);
    }


    size_t DeckDoubleItem::size() const {
        return m_data.size();
    }

    void DeckDoubleItem::push_backDimension(std::shared_ptr<const Dimension> activeDimension , std::shared_ptr<const Dimension> defaultDimension) {
        if (m_dataPointDefaulted.empty() || m_dataPointDefaulted.back())
            m_dimensions.push_back( defaultDimension );
        else
            m_dimensions.push_back( activeDimension );
    }


}

