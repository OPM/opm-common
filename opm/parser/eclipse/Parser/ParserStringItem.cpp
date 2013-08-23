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

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>

namespace Opm {

    ParserStringItem::ParserStringItem(const std::string& itemName, ParserItemSizeEnum sizeType) : ParserItem(itemName, sizeType) {
        m_default = defaultString();
    }
    

    ParserStringItem::ParserStringItem(const std::string& itemName, ParserItemSizeEnum sizeType, std::string defaultValue) : ParserItem(itemName, sizeType) {
        m_default = defaultValue;
    }


    ParserStringItem::ParserStringItem( const Json::JsonObject& jsonConfig) : ParserItem(jsonConfig) {
        if (jsonConfig.has_item("default"))
            m_default = jsonConfig.get_string("default");
        else
            m_default = defaultString();
    }

    DeckItemConstPtr ParserStringItem::scan(RawRecordPtr rawRecord) const {
        if (sizeType() == SINGLE) {
            return scan__(1U, false, rawRecord);
        } else if (sizeType() == ALL) {
            return scan__(0, true, rawRecord);
        } else
            throw std::invalid_argument("Unsupported size type, only support SINGLE and ALL. Use scan( numTokens , rawRecord) instead ");
    }

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!

    DeckItemConstPtr ParserStringItem::scan__(size_t expectedItems, bool scanAll, RawRecordPtr rawRecord) const {
        if (sizeType() == SINGLE && expectedItems > 1) {
            throw std::invalid_argument("Can only ask for one item when sizeType == SINGLE");
        }

        {
            DeckStringItemPtr deckItem(new DeckStringItem(name()));
            
            if ((expectedItems > 0) || scanAll) {
                bool defaultActive;
                std::vector<std::string> stringsPreparedForDeckItem = readFromRawRecord(rawRecord, scanAll, m_default, defaultActive);
                if (scanAll)
                    deckItem->push_back(stringsPreparedForDeckItem);
                else if (stringsPreparedForDeckItem.size() >= expectedItems) {
                    deckItem->push_back(stringsPreparedForDeckItem, expectedItems);
                    
                    if (stringsPreparedForDeckItem.size() > expectedItems)
                        pushBackToRecord(rawRecord, stringsPreparedForDeckItem, expectedItems, defaultActive);
                } else {
                    std::string preparedStrings = boost::lexical_cast<std::string>(stringsPreparedForDeckItem.size());
                    std::string parserSizeValue = boost::lexical_cast<std::string>(expectedItems);
                    throw std::invalid_argument("The number of parsed strings (" + preparedStrings + ") did not correspond to the expected number of items:(" + parserSizeValue + ")");
                }
            }

            return deckItem;
        }
    }
}
