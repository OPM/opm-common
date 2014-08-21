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
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>

namespace Opm {


    ParserIntItem::ParserIntItem(const std::string& itemName) : ParserItem(itemName) {
        m_default = defaultInt();
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType_) : ParserItem(itemName, sizeType_) {
        m_default = defaultInt();
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, int defaultValue) : ParserItem(itemName) {
        setDefault(defaultValue);
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType_, int defaultValue) : ParserItem(itemName, sizeType_) {
        setDefault(defaultValue);
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
    DeckItemPtr ParserIntItem::scan(RawRecordPtr rawRecord) const {
        DeckIntItemPtr deckItem(new DeckIntItem( name() , scalar() ));
        int defaultValue = m_default;

        if (sizeType() == ALL) {  
            // The '*' should be interpreted either as a default indicator in the cases '*' and '5*'
            // or as multiplier in the case: '5*99'. 
            while (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<int> st(token);
                    int value = defaultValue;    
                    if (st.hasValue())
                        value = st.value();
                    deckItem->push_backMultiple( value , st.multiplier() );
                } else {
                    int value = readValueToken<int>(token);
                    deckItem->push_back(value);
                }
            }
        } else {
            // The '*' should be interpreted as a default indicator
            if (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<int> st(token);
        
                    if (st.hasValue()) { // Probably never true
                        deckItem->push_back( st.value() ); 
                        std::string stringValue = boost::lexical_cast<std::string>(st.value());
                        for (size_t i=1; i < st.multiplier(); i++)
                            rawRecord->push_front( stringValue );
                    } else {
                        deckItem->push_backDefault( defaultValue );
                        for (size_t i=1; i < st.multiplier(); i++)
                            rawRecord->push_front( "*" );
                    }
                } else {
                    int value = readValueToken<int>(token);
                    deckItem->push_back(value);
                }
            } else
                deckItem->push_backDefault( defaultValue );
        }
        return deckItem;
    }


    bool ParserIntItem::equal(const ParserItem& other) const
    {
        return ParserItemEqual<ParserIntItem>(this , other);
    }
    

    void ParserIntItem::inlineNew(std::ostream& os) const {
        os << "new ParserIntItem(" << "\"" << name() << "\"" << "," << ParserItemSizeEnum2String( sizeType() );
        if (m_defaultSet)
            os << "," << getDefault();
        os << ")";
    }
}
