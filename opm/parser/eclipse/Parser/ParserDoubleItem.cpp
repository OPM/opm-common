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
            ParserItemSizeEnum sizeType_) :
            ParserItem(itemName, sizeType_)
    {
        // use NaN as 'default default'. (Keep in mind that in the deck it can be queried
        // using deckItem->defaultApplied(idx) if an item was defaulted or not...
        m_default = std::numeric_limits<double>::quiet_NaN();
    }

    ParserDoubleItem::ParserDoubleItem(const std::string& itemName) : ParserItem(itemName)
    {
        m_default = std::numeric_limits<double>::quiet_NaN();
    }


    ParserDoubleItem::ParserDoubleItem(const std::string& itemName, double defaultValue) : ParserItem(itemName)
    {
        setDefault( defaultValue );
    }


    ParserDoubleItem::ParserDoubleItem(const std::string& itemName, ParserItemSizeEnum sizeType_, double defaultValue) : ParserItem(itemName, sizeType_)
    {
        setDefault( defaultValue );
    }


    double ParserDoubleItem::getDefault() const {
        return m_default;
    }
    

    void ParserDoubleItem::setDefault(double defaultValue) {
        m_default = defaultValue;
        m_defaultSet = true;
    }



    ParserDoubleItem::ParserDoubleItem(const Json::JsonObject& jsonConfig) :
            ParserItem(jsonConfig)
    {
        m_default = std::numeric_limits<double>::quiet_NaN();
        if (jsonConfig.has_item("default")) 
            setDefault( jsonConfig.get_double("default") );
    }

    bool ParserDoubleItem::equal(const ParserItem& other) const
    {
        return (ParserItemEqual<ParserDoubleItem>(this , other) && equalDimensions(other));    }


    bool ParserDoubleItem::equalDimensions(const ParserItem& other) const {
        bool equal_=false;
        if (other.numDimensions() == numDimensions()) {
            equal_ = true;
            for (size_t idim=0; idim < numDimensions(); idim++) {
                if (other.getDimension(idim) != getDimension(idim))
                    equal_ = false;
            }
        } 
        return equal_;
    }

    void ParserDoubleItem::push_backDimension(const std::string& dimension) {
        if ((sizeType() == SINGLE) && (m_dimensions.size() > 0))
            throw std::invalid_argument("Internal error - can not add more than one dimension to a Item os size 1");
        
        m_dimensions.push_back( dimension );
    }

    bool ParserDoubleItem::hasDimension() const {
        return (m_dimensions.size() > 0);
    }

    size_t ParserDoubleItem::numDimensions() const {
        return m_dimensions.size();
    }

    const std::string& ParserDoubleItem::getDimension(size_t index) const {
        if (index < m_dimensions.size())
            return m_dimensions[index];
        else
            throw std::invalid_argument("Invalid index ");
    }


    DeckItemPtr ParserDoubleItem::scan(RawRecordPtr rawRecord) const {
        return ParserItemScan<ParserDoubleItem,DeckDoubleItem,double>(this , rawRecord);
    }
    
    void ParserDoubleItem::inlineNew(std::ostream& os) const {
        os << "new ParserDoubleItem(" << "\"" << name() << "\"" << "," << ParserItemSizeEnum2String( sizeType() );
        if (m_defaultSet)
            os << "," << getDefault();
        os << ")";
    }

}

