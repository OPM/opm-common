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

#include <boost/lexical_cast.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>


namespace Opm {

    ParserIntItem::ParserIntItem(const std::string& itemName) : ParserItem(itemName) {
        m_default = defaultInt();
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType) : ParserItem(itemName, sizeType) {
        m_default = defaultInt();
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType, int defaultValue) : ParserItem(itemName, sizeType) {
        m_default = defaultValue;
        m_defaultSet = true;
    }

    ParserIntItem::ParserIntItem(const Json::JsonObject& jsonConfig) : ParserItem(jsonConfig)
    {
        if (jsonConfig.has_item("default")) 
            setDefault( jsonConfig.get_int("default") );
        else
            m_default = defaultInt();
    }

    void ParserIntItem::setDefault(int defaultValue) {
        m_default = defaultValue;
        m_defaultSet = true;
    }

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!
    DeckItemConstPtr ParserIntItem::scan(RawRecordPtr rawRecord) const {
        DeckIntItemPtr deckItem(new DeckIntItem(name()));

        bool scanAll = (sizeType() == ALL);
        bool defaultActive;
        std::deque<int> intsPreparedForDeckItem = readFromRawRecord(rawRecord, scanAll, m_default, defaultActive);

        if (scanAll)
            deckItem->push_back(intsPreparedForDeckItem);
        else {
            deckItem->push_back(intsPreparedForDeckItem.front());
            intsPreparedForDeckItem.pop_front();
            pushBackToRecord(rawRecord, intsPreparedForDeckItem, defaultActive);
        }
        return deckItem;
    }


    bool ParserIntItem::equal(const ParserIntItem& other) const
    {
        if (ParserItem::equal(other) && (getDefault() == other.getDefault()))
            return true;
        else
            return false;
    }


  void ParserIntItem::inlineNew(std::ostream& os) const {
        os << "new ParserIntItem(" << "\"" << name() << "\"" << "," << ParserItemSizeEnum2String( sizeType() );
        if (m_defaultSet)
            os << "," << getDefault();
        os << ")";
    }
}
