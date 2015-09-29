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
#include <set>

#ifdef HAVE_REGEX
#include <regex>
#else
#include <boost/regex.hpp>
#endif

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/EclipseState/Util/RecordVector.hpp>


namespace Opm {
    class ParserKeyword;
    typedef std::shared_ptr<ParserKeyword> ParserKeywordPtr;
    typedef std::shared_ptr<const ParserKeyword> ParserKeywordConstPtr;

    class ParserKeyword {
    public:
        ParserKeyword(const std::string& name ,
                      const std::string& sizeKeyword ,
                      const std::string& sizeItem,
                      bool isTableCollection = false);
        explicit ParserKeyword(const std::string& name);
        explicit ParserKeyword(const Json::JsonObject& jsonConfig);

        void setFixedSize( size_t keywordSize);
        void setSizeType( ParserKeywordSizeEnum sizeType );
        void setTableCollection(bool isTableCollection);
        void initSizeKeyword( const std::string& sizeKeyword, const std::string& sizeItem);


        typedef std::set<std::string> DeckNameSet;
        typedef std::set<std::string> SectionNameSet;


        static std::string getDeckName(const std::string& rawString);
        static bool validInternalName(const std::string& name);
        static bool validDeckName(const std::string& name);
        bool hasMatchRegex() const;
        void setMatchRegex(const std::string& deckNameRegexp);
        bool matches(const std::string& deckKeywordName) const;
        bool hasDimension() const;
        void addRecord(std::shared_ptr<ParserRecord> record);
        void addDataRecord(std::shared_ptr<ParserRecord> record);
        ParserRecordPtr getRecord(size_t recordIndex) const;
        std::vector<ParserRecordPtr>::const_iterator recordBegin() const;
        std::vector<ParserRecordPtr>::const_iterator recordEnd() const;
        const std::string className() const;
        const std::string& getName() const;
        size_t getFixedSize() const;
        bool hasFixedSize() const;
        bool isTableCollection() const;
        std::string getDescription() const;
        void setDescription(const std::string &description);

        bool hasMultipleDeckNames() const;
        void clearDeckNames();
        void addDeckName( const std::string& deckName );
        DeckNameSet::const_iterator deckNamesBegin() const;
        DeckNameSet::const_iterator deckNamesEnd() const;

        void clearValidSectionNames();
        void addValidSectionName(const std::string& sectionName);
        bool isValidSection(const std::string& sectionName) const;
        SectionNameSet::const_iterator validSectionNamesBegin() const;
        SectionNameSet::const_iterator validSectionNamesEnd() const;

        DeckKeywordPtr parse(RawKeywordConstPtr rawKeyword) const;
        enum ParserKeywordSizeEnum getSizeType() const;
        const std::pair<std::string,std::string>& getSizeDefinitionPair() const;
        bool isDataKeyword() const;
        bool equal(const ParserKeyword& other) const;

        std::string createDeclaration(const std::string& indent) const;
        std::string createDecl() const;
        std::string createCode() const;
        void applyUnitsToDeck(std::shared_ptr<const Deck> deck , std::shared_ptr<const DeckKeyword> deckKeyword) const;
    private:
        std::pair<std::string,std::string> m_sizeDefinitionPair;
        std::string m_name;
        DeckNameSet m_deckNames;
        DeckNameSet m_validSectionNames;
        std::string m_matchRegexString;
#ifdef HAVE_REGEX
        std::regex m_matchRegex;
#else
        boost::regex m_matchRegex;
#endif
        RecordVector<ParserRecordPtr> m_records;
        enum ParserKeywordSizeEnum m_keywordSizeType;
        size_t m_fixedSize;
        bool m_isTableCollection;
        std::string m_Description;

        static bool validNameStart(const std::string& name);
        void initDeckNames( const Json::JsonObject& jsonConfig );
        void initSectionNames( const Json::JsonObject& jsonConfig );
        void initMatchRegex( const Json::JsonObject& jsonObject );
        void initData( const Json::JsonObject& jsonConfig );
        void initSize( const Json::JsonObject& jsonConfig );
        void initSizeKeyword(const Json::JsonObject& sizeObject);
        void commonInit(const std::string& name, ParserKeywordSizeEnum sizeType);
        void addItems( const Json::JsonObject& jsonConfig);
        void initDoubleItemDimension( ParserDoubleItemPtr item, const Json::JsonObject itemConfig);
        void initFloatItemDimension( ParserFloatItemPtr item, const Json::JsonObject itemConfig);
    };
}

#endif
