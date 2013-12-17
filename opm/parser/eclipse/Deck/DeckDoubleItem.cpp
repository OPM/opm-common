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

namespace Opm {

    double DeckDoubleItem::getRawDouble(size_t index) const {
        if (index < m_data.size()) {
            return m_data[index];
        } else
            throw std::out_of_range("Out of range, index must be lower than " + boost::lexical_cast<std::string>(m_data.size()));
    }
    
    const std::vector<double>& DeckDoubleItem::getRawDoubleData() const {
        return m_data;
    }


    void DeckDoubleItem::assertSIData() {
        if (m_dimensions.size() > 0) {
            m_SIdata.resize( m_data.size() );
            if (m_dimensions.size() == 1) {
                double SIfactor = m_dimensions[0]->getSIScaling();
                std::transform( m_data.begin() , m_data.end() , m_SIdata.begin() , std::bind1st(std::multiplies<double>(),SIfactor));
            } else {
                for (size_t index=0; index < m_data.size(); index++) {
                    size_t dimIndex = (index % m_dimensions.size());
                    double SIfactor = m_dimensions[dimIndex]->getSIScaling();
                    m_SIdata[index] = m_data[index] * SIfactor;
                }
            }
        } else
            throw std::invalid_argument("No dimension has been set for item:" + name() + " can not ask for SI data");
    }
    


    double DeckDoubleItem::getSIDouble(size_t index) {
        assertSIData();
        {
            if (index < m_data.size()) {
                return m_SIdata[index];
            } else
                throw std::out_of_range("Out of range, index must be lower than " + boost::lexical_cast<std::string>(m_data.size()));
        }
    }
    
    const std::vector<double>& DeckDoubleItem::getSIDoubleData() {
        assertSIData();
        return m_SIdata;
    }


    void DeckDoubleItem::push_back(std::deque<double> data , size_t items) {
        for (size_t i=0; i<items; i++) {
            m_data.push_back(data[i]);
        }
    }


    void DeckDoubleItem::push_back(std::deque<double> data) {
        push_back( data  , data.size() );
    }

    void DeckDoubleItem::push_back(double data) {
        m_data.push_back( data );
    }


    void DeckDoubleItem::push_backDefault(double data) {
        m_data.push_back( data );
        m_defaultApplied = true;
    }
    
    
    void DeckDoubleItem::push_backMultiple(double value, size_t numValues) {
        for (size_t i = 0; i < numValues; i++) 
            m_data.push_back( value );
    }


    size_t DeckDoubleItem::size() const {
        return m_data.size();
    }

    void DeckDoubleItem::push_backDimension(std::shared_ptr<const Dimension> activeDimension , std::shared_ptr<const Dimension> defaultDimension) {
        if (m_defaultApplied)
            m_dimensions.push_back( defaultDimension );
        else
            m_dimensions.push_back( activeDimension );
    }
    

}

