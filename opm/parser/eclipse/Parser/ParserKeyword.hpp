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
#ifndef PARSER_KEYWORD_H
#define PARSER_KEYWORD_H

#include <string>
#include <iostream>

#include <memory>
#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>


namespace Opm {

    class ParserKeyword {
    public:
        ParserKeyword(const char * name , ParserKeywordSizeEnum sizeType = SLASH_TERMINATED , ParserKeywordActionEnum action = INTERNALIZE);
        ParserKeyword(const std::string& name , ParserKeywordSizeEnum sizeType = SLASH_TERMINATED , ParserKeywordActionEnum action = INTERNALIZE);
        ParserKeyword(const std::string& name , size_t fixedKeywordSize,ParserKeywordActionEnum action = INTERNALIZE);
        ParserKeyword(const std::string& name , const std::string& sizeKeyword , const std::string& sizeItem, ParserKeywordActionEnum action = INTERNALIZE , bool isTableCollection = false);
        ParserKeyword(const Json::JsonObject& jsonConfig);
        
        
        static bool validName(const std::string& name);
        static bool wildCardName(const std::string& name);
        bool matches(const std::string& keyword) const;
        bool hasDimension() const;
        ParserRecordPtr getRecord() const;
        const std::string& getName() const;
        ParserKeywordActionEnum getAction() const;
        size_t getFixedSize() const;
        bool hasFixedSize() const;
        bool isTableCollection() const;


        size_t numItems() const;
        
        DeckKeywordPtr parse(RawKeywordConstPtr rawKeyword) const;
        enum ParserKeywordSizeEnum getSizeType() const;
        const std::pair<std::string,std::string>& getSizeDefinitionPair() const;
        void addItem( ParserItemConstPtr item );
        void addDataItem( ParserItemConstPtr item );
        bool isDataKeyword() const;
        bool equal(const ParserKeyword& other) const;
        void inlineNew(std::ostream& os , const std::string& lhs, const std::string& indent) const;
    private:
        std::pair<std::string,std::string> m_sizeDefinitionPair;
        std::string m_name;
        ParserRecordPtr m_record;
        enum ParserKeywordSizeEnum m_keywordSizeType;
        size_t m_fixedSize;
        bool m_isDataKeyword;
        bool m_isTableCollection;
        ParserKeywordActionEnum m_action;

        static bool validNameStart(const std::string& name);
        void initData( const Json::JsonObject& jsonConfig );
        void initSize( const Json::JsonObject& jsonConfig );
        void initSizeKeyword( const std::string& sizeKeyword, const std::string& sizeItem);
        void initSizeKeyword(const Json::JsonObject& sizeObject);
        void commonInit(const std::string& name, ParserKeywordSizeEnum sizeType , ParserKeywordActionEnum action);
        void addItems( const Json::JsonObject& jsonConfig);
        void addTableItems();
        static void initItemDimension( ParserDoubleItemPtr item, const Json::JsonObject itemConfig);
    };
    typedef std::shared_ptr<ParserKeyword> ParserKeywordPtr;
    typedef std::shared_ptr<const ParserKeyword> ParserKeywordConstPtr;
}

#endif
