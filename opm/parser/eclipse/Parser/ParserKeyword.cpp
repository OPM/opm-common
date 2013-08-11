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

#include <string>
#include <stdexcept>



#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserConst.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>


namespace Opm {


    ParserKeyword::ParserKeyword(const std::string& name) {
        commonInit(name);
        m_keywordSizeType = UNDEFINED;
    }


    ParserKeyword::ParserKeyword(const char * name) {
        commonInit(name);
        m_keywordSizeType = UNDEFINED;
    }


    ParserKeyword::ParserKeyword(const std::string& name, size_t fixedKeywordSize) {
        commonInit(name);
        m_keywordSizeType = FIXED;
        m_fixedSize = fixedKeywordSize;
    }
    
    
    ParserKeyword::ParserKeyword(const std::string& name , const std::string& sizeKeyword , const std::string& sizeItem) {
        commonInit(name);
        initSizeKeyword( sizeKeyword , sizeItem );
    }

    
    ParserKeyword::ParserKeyword(const Json::JsonObject& jsonConfig) {
        if (jsonConfig.has_item("name")) {
            commonInit(jsonConfig.get_string("name"));
        } else
            throw std::invalid_argument("Json object is missing name: property");

        if (jsonConfig.has_item("size")) {
            Json::JsonObject sizeObject = jsonConfig.get_item("size");
            
            if (sizeObject.is_number()) {
                m_fixedSize = (size_t) sizeObject.as_int( );
                m_keywordSizeType = FIXED;
            } else {
                std::string sizeKeyword = sizeObject.get_string("keyword");
                std::string sizeItem = sizeObject.get_string("item");
                initSizeKeyword( sizeKeyword , sizeItem);
            }
        } else
            m_keywordSizeType = UNDEFINED;

        if (jsonConfig.has_item("items")) 
            addItems( jsonConfig );
    }

    
    void ParserKeyword::initSizeKeyword( const std::string& sizeKeyword, const std::string& sizeItem) {
        m_keywordSizeType = OTHER;
        m_sizeKeyword = std::pair<std::string , std::string>(sizeKeyword , sizeItem);
    }

    
    void ParserKeyword::commonInit(const std::string& name) {
        if (name.length() > ParserConst::maxKeywordLength)
            throw std::invalid_argument("Given keyword name is too long - max 8 characters.");
        
        for (unsigned int i = 0; i < name.length(); i++)
            if (islower(name[i]))
                throw std::invalid_argument("Keyword must be all upper case - mixed case not allowed:" + name);
        
        m_name = name;
        m_record = ParserRecordPtr(new ParserRecord);
    }


    void ParserKeyword::addItems( const Json::JsonObject& jsonConfig) {
        const Json::JsonObject itemsConfig = jsonConfig.get_item("items");
        if (itemsConfig.is_array()) {
            size_t num_items = itemsConfig.size();
            for (size_t i=0; i < num_items; i++) {
                const Json::JsonObject itemConfig = itemsConfig.get_array_item( i );
                
                if (itemConfig.has_item("value_type")) {
                    ParserValueTypeEnum valueType = ParserValueTypeEnumFromString( itemConfig.get_string("value_type") );
                    switch( valueType ) {
                    case INT:
                        {
                            ParserIntItemConstPtr item = ParserIntItemConstPtr(new ParserIntItem( itemConfig ));
                            m_record->addItem( item );
                        }
                        break;
                    case STRING:
                        {
                            ParserStringItemConstPtr item = ParserStringItemConstPtr(new ParserStringItem( itemConfig ));
                            m_record->addItem( item );
                        }
                        break;
                    case FLOAT:
                        {
                            ParserDoubleItemConstPtr item = ParserDoubleItemConstPtr(new ParserDoubleItem( itemConfig ));
                            m_record->addItem( item );
                        }
                        break;
                    default:
                        throw std::invalid_argument("Not implemented.");
                    }
                } else
                    throw std::invalid_argument("Json config object missing \"value_type\": ... item");            
            }
        } else
            throw std::invalid_argument("The items: object must be an array");            
    }

    ParserRecordPtr ParserKeyword::getRecord() {
        return m_record;
    }


    const std::string& ParserKeyword::getName() const {
        return m_name;
    }

    DeckKeywordPtr ParserKeyword::parse(RawKeywordConstPtr rawKeyword) const {
      DeckKeywordPtr keyword(new DeckKeyword(getName()));
        for (size_t i = 0; i < rawKeyword->size(); i++) {
            DeckRecordConstPtr deckRecord = m_record->parse(rawKeyword->getRecord(i));
            keyword->addRecord(deckRecord);
        }
        
        return keyword;
    }

    size_t ParserKeyword::getFixedSize() const {
        if (!hasFixedSize()) {
            throw std::logic_error("This parser keyword does not have a fixed size!");
        }
        return m_fixedSize;
    }

    bool ParserKeyword::hasFixedSize() const {
        return m_keywordSizeType == FIXED;
    }

    enum ParserKeywordSizeEnum ParserKeyword::getSizeType() const {
        return m_keywordSizeType;
    }

    const std::pair<std::string,std::string>& ParserKeyword::getSizePair() const {
        return m_sizeKeyword;
    }

}
