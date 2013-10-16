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

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>

namespace Opm
{

    ParserDoubleItem::ParserDoubleItem(const std::string& itemName,
            ParserItemSizeEnum sizeType) :
            ParserItem(itemName, sizeType)
    {
        m_default = defaultDouble();
    }

    ParserDoubleItem::ParserDoubleItem(const std::string& itemName) : ParserItem(itemName)
    {
        m_default = defaultDouble();
    }


    ParserDoubleItem::ParserDoubleItem(const std::string& itemName, double defaultValue) : ParserItem(itemName)
    {
        setDefault( defaultValue );
    }


    ParserDoubleItem::ParserDoubleItem(const std::string& itemName, ParserItemSizeEnum sizeType, double defaultValue) : ParserItem(itemName, sizeType)
    {
        setDefault( defaultValue );
    }


    void ParserDoubleItem::setDefault(double defaultValue) {
        m_default = defaultValue;
        m_defaultSet = true;
    }



    ParserDoubleItem::ParserDoubleItem(const Json::JsonObject& jsonConfig) :
            ParserItem(jsonConfig)
    {
        if (jsonConfig.has_item("default")) 
            setDefault( jsonConfig.get_double("default") );
        else
            m_default = defaultDouble();
    }

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!

//    DeckItemConstPtr ParserDoubleItem::scan(RawRecordPtr rawRecord) const
//    {
//        DeckDoubleItemPtr deckItem(new DeckDoubleItem(name()));
//
//        bool scanAll = (sizeType() == ALL);
//        bool defaultActive;
//        std::deque<double> doublesPreparedForDeckItem = readFromRawRecord(
//                rawRecord, scanAll, m_default, defaultActive);
//
//        if (scanAll)
//            deckItem->push_back(doublesPreparedForDeckItem);
//        else
//            {
//                deckItem->push_back(doublesPreparedForDeckItem.front());
//                doublesPreparedForDeckItem.pop_front();
//                pushBackToRecord(rawRecord, doublesPreparedForDeckItem,
//                        defaultActive);
//            }
//        return deckItem;
//    }

    bool ParserDoubleItem::equal(const ParserDoubleItem& other) const
    {
        if (ParserItem::equal(other) && (getDefault() == other.getDefault()))
            return true;
        else
            return false;
    }

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!
    DeckItemConstPtr ParserDoubleItem::scan(RawRecordPtr rawRecord) const {
        DeckDoubleItemPtr deckItem(new DeckDoubleItem(name()));
        double defaultValue = m_default;

        if (sizeType() == ALL) {  // This can probably not be combined with a default value ....
            // The '*' should be interpreted as a multiplication sign
            while (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<double> st(token);
                    double value = defaultValue;    // This probably does never apply
                    if (st.hasValue())
                        value = st.value();
                    deckItem->push_backMultiple( value , st.multiplier() );
                } else {
                    double value = readValueToken<double>(token);
                    deckItem->push_back(value);
                }
            }
        } else {
            // The '*' should be interpreted as a default indicator
            if (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<double> st(token);
        
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
                    double value = readValueToken<double>(token);
                    deckItem->push_back(value);
                }
            } else
                deckItem->push_backDefault( defaultValue );
        }
        return deckItem;
    }
    
  void ParserDoubleItem::inlineNew(std::ostream& os) const {
        os << "new ParserDoubleItem(" << "\"" << name() << "\"" << "," << ParserItemSizeEnum2String( sizeType() );
        if (m_defaultSet)
            os << "," << getDefault();
        os << ")";
    }

}

