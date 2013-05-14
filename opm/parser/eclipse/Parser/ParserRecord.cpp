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

#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>

namespace Opm {
    
    ParserRecord::ParserRecord() {
        
    }
    
    size_t ParserRecord::size() {
        return m_items.size();
    }

    void ParserRecord::addItem( ParserItemConstPtr item ) {
        if (m_itemMap.find( item->name() ) == m_itemMap.end()) {
            m_items.push_back( item );
            m_itemMap[item->name()]  = item;
        } else
            throw std::invalid_argument("Itemname: " + item->name() + " already exists.");
    }


    ParserItemConstPtr ParserRecord::get(size_t index) {
        if (index < m_items.size())
            return m_items[ index ];
        else
            throw std::out_of_range("Out of range");
    }


    ParserItemConstPtr ParserRecord::get(const std::string& itemName) {
        if (m_itemMap.find( itemName ) == m_itemMap.end())
            throw std::invalid_argument("Itemname: " + itemName + " does not exist.");
        else
            return m_itemMap[ itemName ];
    }

    
}
