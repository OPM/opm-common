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
#include <boost/shared_ptr.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>


namespace Opm {

    class ParserKeyword {
    public:
        ParserKeyword(const char * name);
        ParserKeyword(const std::string& name);
        ParserKeyword(const std::string& name, size_t fixedKeywordSize);
        ParserKeyword(const std::string& name , const std::string& sizeKeyword , const std::string& sizeItem);
        ParserKeyword(const Json::JsonObject& jsonConfig);

        
        ParserRecordPtr getRecord();
        const std::string& getName() const;
        size_t getFixedSize() const;
        bool hasFixedSize() const;
        
        DeckKeywordPtr parse(RawKeywordConstPtr rawKeyword) const;
        enum ParserKeywordSizeEnum getSizeType() const;
        const std::pair<std::string,std::string>& getSizeDefinitionPair() const;
    private:
        std::pair<std::string,std::string> m_sizeDefinitionPair;
        std::string m_name;
        ParserRecordPtr m_record;
        enum ParserKeywordSizeEnum m_keywordSizeType;
        size_t m_fixedSize;
        
        void initSizeKeyword( const std::string& sizeKeyword, const std::string& sizeItem);
        void commonInit(const std::string& name);
        void addItems( const Json::JsonObject& jsonConfig);
    };
    typedef boost::shared_ptr<ParserKeyword> ParserKeywordPtr;
    typedef boost::shared_ptr<const ParserKeyword> ParserKeywordConstPtr;
}

#endif
