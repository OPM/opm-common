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


#include <stdexcept>
#include <string>
#include <algorithm>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>


namespace Opm {

    DeckRecord::DeckRecord() {

    }

    size_t DeckRecord::size() const {
        return m_items.size();
    }

    void DeckRecord::addItem(DeckItemPtr deckItem) {
        const auto& name = deckItem->name();
        const auto eq = [&name]( const std::shared_ptr< DeckItem >& e ) {
            return e->name() == name;
        };

        if( std::any_of( m_items.begin(), m_items.end(), eq ) )
            throw std::invalid_argument(
                    "Item with name: "
                    + name
                    + " already exists in DeckRecord");

        m_items.push_back(deckItem);
    }

    DeckItemPtr DeckRecord::getItem(size_t index) const {
        if (index < m_items.size())
            return m_items[index];
        else
            throw std::range_error("Index out of range.");
    }


    bool DeckRecord::hasItem(const std::string& name) const {
        const auto eq = [&name]( const std::shared_ptr< DeckItem >& e ) {
            return e->name() == name;
        };

        return std::any_of( m_items.begin(), m_items.end(), eq );
    }

    
    DeckItemPtr DeckRecord::getItem(const std::string& name) const {
        const auto eq = [&name]( const std::shared_ptr< DeckItem >& e ) {
            return e->name() == name;
        };

        auto item = std::find_if( m_items.begin(), m_items.end(), eq );
        if( item == m_items.end() )
            throw std::invalid_argument("Itemname: " + name + " does not exist.");

        return *item;
    }


    DeckItemPtr DeckRecord::getDataItem() const {
        if (m_items.size() == 1)
            return getItem(0);
        else
            throw std::range_error("Not a data keyword ?");
    }

}


