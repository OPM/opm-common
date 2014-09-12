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

#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>

namespace Opm {

    std::string DeckStringItem::getString(size_t index) const {
        assertSize(index);

        return m_data[index];
    }

    std::string DeckStringItem::getTrimmedString(size_t index) const {
        assertSize(index);

        return boost::algorithm::trim_copy(m_data[index]);
    }

    const std::vector<std::string>& DeckStringItem::getStringData() const {
        return m_data;
    }



    void DeckStringItem::push_back(std::deque<std::string> data, size_t items) {
        for (size_t i=0; i<items; i++) {
            m_data.push_back(data[i]);
            m_dataPointDefaulted.push_back(false);
        }
    }


    void DeckStringItem::push_back(std::deque<std::string> data) {
        push_back(data, data.size());
        m_dataPointDefaulted.push_back(false);
    }


    void DeckStringItem::push_back(const std::string& data ) {
        m_data.push_back( data );
        m_dataPointDefaulted.push_back(false);
    }


  
    void DeckStringItem::push_backMultiple(std::string value, size_t numValues) {
        for (size_t i = 0; i < numValues; i++) {
            m_data.push_back( value );
            m_dataPointDefaulted.push_back(false);
        }
    }


    void DeckStringItem::push_backDefault(std::string data) {
        m_data.push_back( data );
        m_dataPointDefaulted.push_back(true);
    }
    
    

    size_t DeckStringItem::size() const {
        return m_data.size();
    }

}

