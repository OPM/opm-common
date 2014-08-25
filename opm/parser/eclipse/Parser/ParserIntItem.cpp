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


    ParserIntItem::ParserIntItem(const std::string& itemName) : ParserItem(itemName) 
    {
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType_) : ParserItem(itemName, sizeType_) 
    {
    }

    ParserIntItem::ParserIntItem(const std::string& itemName, int defaultValue) : ParserItem(itemName) 
    {
        setDefault(defaultValue);
    }


    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType_, int defaultValue) : ParserItem(itemName, sizeType_) 
    {
        setDefault(defaultValue);
    }

    ParserIntItem::ParserIntItem(const Json::JsonObject& jsonConfig) : ParserItem(jsonConfig)
    {
        if (jsonConfig.has_item("default")) 
            setDefault( jsonConfig.get_int("default") );
    }


    void ParserIntItem::setDefault(int defaultValue) {
        if (sizeType() == ALL)
            throw std::invalid_argument("The size type ALL can not be combined with an explicit default value");
        m_defaultSet = true;
        m_default = defaultValue;
    }

    int ParserIntItem::getDefault() const {
        if (m_defaultSet) 
            return m_default;
        else
            throw std::invalid_argument("Tried get default from parser item " + name() + " No default has been configured");
    }

    DeckItemPtr ParserIntItem::scan(RawRecordPtr rawRecord) const {
        return ParserItemScan<ParserIntItem,DeckIntItem,int>(this , rawRecord);
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
