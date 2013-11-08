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

    size_t ParserRecord::size() const {
        return m_items.size();
    }

    void ParserRecord::addItem(ParserItemConstPtr item) {
        if (m_itemMap.find(item->name()) == m_itemMap.end()) {
            m_items.push_back(item);
            m_itemMap[item->name()] = item;
        } else
            throw std::invalid_argument("Itemname: " + item->name() + " already exists.");
    }

    ParserItemConstPtr ParserRecord::get(size_t index) const {
        if (index < m_items.size())
            return m_items[ index ];
        else
            throw std::out_of_range("Out of range");
    }

    ParserItemConstPtr ParserRecord::get(const std::string& itemName) const {
        if (m_itemMap.find(itemName) == m_itemMap.end())
            throw std::invalid_argument("Itemname: " + itemName + " does not exist.");
        else {
            std::map<std::string, ParserItemConstPtr>::const_iterator theItem = m_itemMap.find(itemName);
            return (*theItem).second;
        }
    }

    DeckRecordConstPtr ParserRecord::parse(RawRecordPtr rawRecord) const {
        DeckRecordPtr deckRecord(new DeckRecord());
        for (size_t i = 0; i < size(); i++) {
            ParserItemConstPtr parserItem = get(i);
            DeckItemConstPtr deckItem = parserItem->scan(rawRecord);
            deckRecord->addItem(deckItem);
        }

        return deckRecord;
    }

    bool ParserRecord::equal(const ParserRecord& other) const {
        bool equal = true;
        if (size() == other.size()) {
           size_t itemIndex = 0;
           while (true) {
               if (itemIndex == size())
                   break;
               {
                   ParserItemConstPtr item = get(itemIndex);
                   ParserItemConstPtr otherItem = other.get(itemIndex);
                   
                   if (!item->equal(*otherItem)) {
                       equal = false;
                       break;
                   }
               }
               itemIndex++;
            }
        } else
            equal = false;
        return equal;
    }

}
