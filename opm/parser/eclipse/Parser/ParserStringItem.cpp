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
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>
namespace Opm {

    ParserStringItem::ParserStringItem(const std::string& itemName) : ParserItem(itemName) {
        m_default = defaultString();
    }


    ParserStringItem::ParserStringItem(const std::string& itemName, ParserItemSizeEnum sizeType_) : ParserItem(itemName, sizeType_) {
        m_default = defaultString();
    }

    ParserStringItem::ParserStringItem(const std::string& itemName, ParserItemSizeEnum sizeType_, const std::string& defaultValue) : ParserItem(itemName, sizeType_) {
        setDefault(defaultValue);
    }


    ParserStringItem::ParserStringItem(const std::string& itemName, const std::string& defaultValue) : ParserItem(itemName) {
        setDefault(defaultValue);
    }


    ParserStringItem::ParserStringItem(const Json::JsonObject& jsonConfig) : ParserItem(jsonConfig) {
        if (jsonConfig.has_item("default")) 
            setDefault( jsonConfig.get_string("default") );
        else
            m_default = defaultString();
    }



    void ParserStringItem::setDefault(const std::string& defaultValue) {
        m_default = defaultValue;
        m_defaultSet = true;
    }




    DeckItemPtr ParserStringItem::scan(RawRecordPtr rawRecord) const {
        return ParserItemScan<ParserStringItem,DeckStringItem,std::string>(this , rawRecord);
    }

     
     
    bool ParserStringItem::equal(const ParserItem& other) const
    {
        return ParserItemEqual<ParserStringItem>(this , other);
    }
    
    void ParserStringItem::inlineNew(std::ostream& os) const {
        os << "new ParserStringItem(" << "\"" << name() << "\"" << "," << ParserItemSizeEnum2String( sizeType() );
        if (m_defaultSet)
            os << ",\"" << getDefault() << "\"";
        os << ")";
    }
}
