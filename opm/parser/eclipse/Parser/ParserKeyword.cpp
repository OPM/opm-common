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
#include <opm/parser/eclipse/Parser/ParserFloatItem.hpp>

#include <boost/algorithm/string.hpp>

namespace Opm {

    void ParserKeyword::commonInit(const std::string& name, ParserKeywordSizeEnum sizeType , ParserKeywordActionEnum action) {
        m_isDataKeyword = false;
        m_isTableCollection = false;
        m_name = name;
        m_action = action;
        m_record = ParserRecordPtr(new ParserRecord);
        m_keywordSizeType = sizeType;
        m_Description = "";

        m_deckNames.insert(m_name);
    }

    ParserKeyword::ParserKeyword(const std::string& name, ParserKeywordSizeEnum sizeType, ParserKeywordActionEnum action) {
        if (!(sizeType == SLASH_TERMINATED || sizeType == UNKNOWN)) {
            throw std::invalid_argument("Size type " + ParserKeywordSizeEnum2String(sizeType) + " can not be set explicitly.");
        }
        commonInit(name, sizeType , action);
    }

    ParserKeyword::ParserKeyword(const std::string& name, size_t fixedKeywordSize, ParserKeywordActionEnum action) {
        commonInit(name, FIXED , action);
        m_fixedSize = fixedKeywordSize;
    }

    ParserKeyword::ParserKeyword(const std::string& name, const std::string& sizeKeyword, const std::string& sizeItem, ParserKeywordActionEnum action, bool _isTableCollection) {
        commonInit(name, OTHER_KEYWORD_IN_DECK , action);
        m_isTableCollection = _isTableCollection;
        initSizeKeyword(sizeKeyword, sizeItem);
    }

    void ParserKeyword::clearDeckNames() {
        m_deckNames.clear();
    }

    void ParserKeyword::addDeckName( const std::string& deckName ) {
        m_deckNames.insert(deckName);
    }

    bool ParserKeyword::hasDimension() const {
        return m_record->hasDimension();
    }

    bool ParserKeyword::isTableCollection() const {
        return m_isTableCollection;
    }

    std::string ParserKeyword::getDescription() const {
        return m_Description;
    }

    void ParserKeyword::setDescription(const std::string& description) {
        m_Description = description;
    }

    void ParserKeyword::initSize(const Json::JsonObject& jsonConfig) {
        // The number of record has been set explicitly with the size: keyword
        if (jsonConfig.has_item("size")) {
            Json::JsonObject sizeObject = jsonConfig.get_item("size");

            if (sizeObject.is_number()) {
                m_fixedSize = (size_t) sizeObject.as_int();
                m_keywordSizeType = FIXED;
            } else
                initSizeKeyword(sizeObject);

        } else
            if (jsonConfig.has_item("num_tables")) {
            Json::JsonObject numTablesObject = jsonConfig.get_item("num_tables");

            if (!numTablesObject.is_object())
                throw std::invalid_argument("The num_tables key must point to a {} object");

            initSizeKeyword(numTablesObject);
            m_isTableCollection = true;
        } else {
            if (jsonConfig.has_item("items"))
                // The number of records is undetermined - the keyword will be '/' terminated.
                m_keywordSizeType = SLASH_TERMINATED;
            else {
                m_keywordSizeType = FIXED;
                if (jsonConfig.has_item("data"))
                    m_fixedSize = 1;
                else
                    m_fixedSize = 0;
            }
        }
    }

    ParserKeyword::ParserKeyword(const Json::JsonObject& jsonConfig) {
        ParserKeywordActionEnum action = INTERNALIZE;

        if (jsonConfig.has_item("action"))
            action = ParserKeywordActionEnumFromString(jsonConfig.get_string("action"));

        if (jsonConfig.has_item("name")) {
            ParserKeywordSizeEnum sizeType = UNKNOWN;
            commonInit(jsonConfig.get_string("name"), sizeType , action);
        } else
            throw std::invalid_argument("Json object is missing name: property");

        if (jsonConfig.has_item("deck_names") || jsonConfig.has_item("deck_name_regex") )
            // if either the deck names or the regular expression for deck names are
            // explicitly specified, we do not implicitly add the contents of the 'name'
            // item to the deck names...
            clearDeckNames();

        initSize(jsonConfig);
        initDeckNames(jsonConfig);
        initSectionNames(jsonConfig);
        initMatchRegex(jsonConfig);

        if (jsonConfig.has_item("items"))
            addItems(jsonConfig);

        if (jsonConfig.has_item("data"))
            initData(jsonConfig);

        if (jsonConfig.has_item("description")) {
            m_Description = jsonConfig.get_string("description");
        }

        if ((m_keywordSizeType == FIXED && m_fixedSize == 0) || (m_action != INTERNALIZE))
            return;
        else {
            if (numItems() == 0)
                throw std::invalid_argument("Json object for keyword: " + jsonConfig.get_string("name") + " is missing items specifier");
        }
    }

    ParserKeywordPtr ParserKeyword::createFixedSized(const std::string& name,
                                                     size_t fixedKeywordSize,
                                                     ParserKeywordActionEnum action) {
        return ParserKeywordPtr(new ParserKeyword(name, fixedKeywordSize, action));
    }

    ParserKeywordPtr ParserKeyword::createDynamicSized(const std::string& name,
                                                       ParserKeywordSizeEnum sizeType ,
                                                       ParserKeywordActionEnum action) {
        return ParserKeywordPtr(new ParserKeyword(name, sizeType, action));
    }

    ParserKeywordPtr ParserKeyword::createTable(const std::string& name,
                                                const std::string& sizeKeyword,
                                                const std::string& sizeItem,
                                                ParserKeywordActionEnum action,
                                                bool isTableCollection) {
        return ParserKeywordPtr(new ParserKeyword(name, sizeKeyword, sizeItem, action, isTableCollection));
    }

    ParserKeywordPtr ParserKeyword::createFromJson(const Json::JsonObject& jsonConfig) {
        return ParserKeywordPtr(new ParserKeyword(jsonConfig));
    }

    void ParserKeyword::initSizeKeyword(const std::string& sizeKeyword, const std::string& sizeItem) {
        m_sizeDefinitionPair = std::pair<std::string, std::string>(sizeKeyword, sizeItem);
        m_keywordSizeType = OTHER_KEYWORD_IN_DECK;
    }

    void ParserKeyword::initSizeKeyword(const Json::JsonObject& sizeObject) {
        if (sizeObject.is_object()) {
            std::string sizeKeyword = sizeObject.get_string("keyword");
            std::string sizeItem = sizeObject.get_string("item");
            initSizeKeyword(sizeKeyword, sizeItem);
        } else {
            m_keywordSizeType = ParserKeywordSizeEnumFromString( sizeObject.as_string() );
        }
    }


    bool ParserKeyword::validNameStart(const std::string& name) {
        if (name.length() > ParserConst::maxKeywordLength)
            return false;

        if (!isalpha(name[0]))
            return false;

        return true;
    }

    bool ParserKeyword::validInternalName(const std::string& name) {
        if (name.length() < 2)
            return false;
        else if (!std::isalpha(name[0]))
            return false;

        for (size_t i = 1; i < name.length(); i++) {
            char c = name[i];
            if (!isalnum(c) &&
                c != '_')
            {
                return false;
            }
        }

        return true;
    }

    std::string ParserKeyword::getDeckName(const std::string& rawString)
    {
        // only look at the first 8 characters (at most)
        std::string result = rawString.substr(0, 8);

        // remove any white space
        boost::algorithm::trim(result);

        return result;
    }

    bool ParserKeyword::validDeckName(const std::string& name) {
        // make the keyword string ALL_UPPERCASE because Eclipse seems
        // to be case-insensitive (although this is one of its
        // undocumented features...)
        std::string upperCaseName = boost::to_upper_copy(name);

        if (!validNameStart(upperCaseName))
            return false;

        for (size_t i = 1; i < upperCaseName.length(); i++) {
            char c = upperCaseName[i];
            if (!isalnum(c) &&
                c != '-' &&
                c != '_' &&
                c != '+')
            {
                return false;
            }
        }
        return true;
    }

    bool ParserKeyword::hasMultipleDeckNames() const {
        return m_deckNames.size() > 1;
    }

    void ParserKeyword::addItem(ParserItemConstPtr item) {
        if (m_isDataKeyword)
            throw std::invalid_argument("Keyword:" + getName() + " is already configured as data keyword - can not add more items.");

        m_record->addItem(item);
    }

    void ParserKeyword::addDataItem(ParserItemConstPtr item) {
        if (m_record->size())
            throw std::invalid_argument("Keyword:" + getName() + " already has items - can not add a data item.");

        if ((m_keywordSizeType == FIXED) && (m_fixedSize == 1U)) {
            addItem(item);
            m_isDataKeyword = true;
        } else
            throw std::invalid_argument("Keyword:" + getName() + ". When calling addDataItem() the keyword must be configured with fixed size == 1.");
    }

    void ParserKeyword::initDeckNames(const Json::JsonObject& jsonObject) {
        if (!jsonObject.has_item("deck_names"))
            return;

        const Json::JsonObject namesObject = jsonObject.get_item("deck_names");
        if (!namesObject.is_array())
            throw std::invalid_argument("The 'deck_names' JSON item needs to be a list (keyword: '"+m_name+"')");

        if (namesObject.size() > 0)
            m_deckNames.clear();

        for (size_t nameIdx = 0; nameIdx < namesObject.size(); ++ nameIdx) {
            const Json::JsonObject nameObject = namesObject.get_array_item(nameIdx);

            if (!nameObject.is_string())
                throw std::invalid_argument("The items of 'deck_names' need to be strings (keyword: '"+m_name+"')");

            addDeckName(nameObject.as_string());
        }
    }

    void ParserKeyword::initSectionNames(const Json::JsonObject& jsonObject) {
        if (!jsonObject.has_item("sections"))
            throw std::invalid_argument("The 'sections' JSON item needs to be defined (keyword: '"+m_name+"')");

        const Json::JsonObject namesObject = jsonObject.get_item("sections");

        if (!namesObject.is_array())
            throw std::invalid_argument("The 'sections' JSON item needs to be a list (keyword: '"+m_name+"')");

        m_validSectionNames.clear();
        for (size_t nameIdx = 0; nameIdx < namesObject.size(); ++ nameIdx) {
            const Json::JsonObject nameObject = namesObject.get_array_item(nameIdx);

            if (!nameObject.is_string())
                throw std::invalid_argument("The items of 'sections' need to be strings (keyword: '"+m_name+"')");

            addValidSectionName(nameObject.as_string());
        }
    }

    void ParserKeyword::initMatchRegex(const Json::JsonObject& jsonObject) {
        if (!jsonObject.has_item("deck_name_regex"))
            return;

        const Json::JsonObject regexStringObject = jsonObject.get_item("deck_name_regex");
        if (!regexStringObject.is_string())
            throw std::invalid_argument("The 'deck_name_regex' JSON item needs to be a string (keyword: '"+m_name+"')");

        setMatchRegex(regexStringObject.as_string());
    }

    void ParserKeyword::addItems(const Json::JsonObject& jsonConfig) {
        const Json::JsonObject itemsConfig = jsonConfig.get_item("items");
        if (itemsConfig.is_array()) {
            size_t num_items = itemsConfig.size();
            for (size_t i = 0; i < num_items; i++) {
                const Json::JsonObject itemConfig = itemsConfig.get_array_item(i);

                if (itemConfig.has_item("value_type")) {
                    ParserValueTypeEnum valueType = ParserValueTypeEnumFromString(itemConfig.get_string("value_type"));
                    switch (valueType) {
                    case INT:
                        {
                            ParserIntItemConstPtr item = ParserIntItemConstPtr(new ParserIntItem(itemConfig));
                            addItem(item);
                        }
                        break;
                    case STRING:
                        {
                            ParserStringItemConstPtr item = ParserStringItemConstPtr(new ParserStringItem(itemConfig));
                            addItem(item);
                        }
                        break;
                    case DOUBLE:
                        {
                            ParserDoubleItemPtr item = ParserDoubleItemPtr(new ParserDoubleItem(itemConfig));
                            initDoubleItemDimension( item , itemConfig );
                            addItem(item);
                        }
                        break;
                    case FLOAT:
                        {
                            ParserFloatItemPtr item = ParserFloatItemPtr(new ParserFloatItem(itemConfig));
                            initFloatItemDimension( item , itemConfig );
                            addItem(item);
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


    void ParserKeyword::initFloatItemDimension( ParserFloatItemPtr item, const Json::JsonObject itemConfig) {
        if (itemConfig.has_item("dimension")) {
            const Json::JsonObject dimensionConfig = itemConfig.get_item("dimension");
            if (dimensionConfig.is_string())
                item->push_backDimension( dimensionConfig.as_string() );
            else if (dimensionConfig.is_array()) {
                for (size_t idim = 0; idim < dimensionConfig.size(); idim++) {
                    Json::JsonObject dimObject = dimensionConfig.get_array_item( idim );
                    item->push_backDimension( dimObject.as_string());
                }
            } else
                throw std::invalid_argument("The dimension: attribute must be a string/list of strings");
        }
    }


    void ParserKeyword::initDoubleItemDimension( ParserDoubleItemPtr item, const Json::JsonObject itemConfig) {
        if (itemConfig.has_item("dimension")) {
            const Json::JsonObject dimensionConfig = itemConfig.get_item("dimension");
            if (dimensionConfig.is_string())
                item->push_backDimension( dimensionConfig.as_string() );
            else if (dimensionConfig.is_array()) {
                for (size_t idim = 0; idim < dimensionConfig.size(); idim++) {
                    Json::JsonObject dimObject = dimensionConfig.get_array_item( idim );
                    item->push_backDimension( dimObject.as_string());
                }
            } else
                throw std::invalid_argument("The dimension: attribute must be a string/list of strings");
        }
    }



    void ParserKeyword::initData(const Json::JsonObject& jsonConfig) {
        m_fixedSize = 1U;
        m_keywordSizeType = FIXED;

        const Json::JsonObject dataConfig = jsonConfig.get_item("data");
        if (dataConfig.has_item("value_type")) {
            ParserValueTypeEnum valueType = ParserValueTypeEnumFromString(dataConfig.get_string("value_type"));
            const std::string itemName(getName());
            bool hasDefault = dataConfig.has_item("default");

            switch (valueType) {
                case INT:
                {
                    ParserIntItemPtr item = ParserIntItemPtr(new ParserIntItem(itemName, ALL));
                    if (hasDefault) {
                        int defaultValue = dataConfig.get_int("default");
                        item->setDefault(defaultValue);
                    }
                    addDataItem(item);
                }
                break;
                case STRING:
                {
                    ParserStringItemPtr item = ParserStringItemPtr(new ParserStringItem(itemName, ALL));
                    if (hasDefault) {
                        std::string defaultValue = dataConfig.get_string("default");
                        item->setDefault(defaultValue);
                    }
                    addDataItem(item);
                }
                break;
                case DOUBLE:
                {
                    ParserDoubleItemPtr item = ParserDoubleItemPtr(new ParserDoubleItem(itemName, ALL));
                    if (hasDefault) {
                        double defaultValue = dataConfig.get_double("default");
                        item->setDefault(defaultValue);
                    }
                    initDoubleItemDimension( item , dataConfig );
                    addDataItem(item);
                }
                break;
                case FLOAT:
                {
                    ParserFloatItemPtr item = ParserFloatItemPtr(new ParserFloatItem(itemName, ALL));
                    if (hasDefault) {
                        double defaultValue = dataConfig.get_double("default");
                        item->setDefault((float) defaultValue);
                    }
                    initFloatItemDimension( item , dataConfig );
                    addDataItem(item);
                }
                break;
                default:
                    throw std::invalid_argument("Not implemented.");
            }
        } else
            throw std::invalid_argument("Json config object missing \"value_type\": ... item");
    }

    ParserRecordPtr ParserKeyword::getRecord() const {
        return m_record;
    }

    ParserKeywordActionEnum ParserKeyword::getAction() const {
        return m_action;
    }

    const std::string& ParserKeyword::getName() const {
        return m_name;
    }

    size_t ParserKeyword::numItems() const {
        return m_record->size();
    }

    void ParserKeyword::clearValidSectionNames() {
        m_validSectionNames.clear();
    }

    void ParserKeyword::addValidSectionName( const std::string& sectionName ) {
        m_validSectionNames.insert(sectionName);
    }

    bool ParserKeyword::isValidSection(const std::string& sectionName) const {
        return m_validSectionNames.size() == 0 || m_validSectionNames.count(sectionName) > 0;
    }

    ParserKeyword::SectionNameSet::const_iterator ParserKeyword::validSectionNamesBegin() const {
        return m_validSectionNames.begin();
    }

    ParserKeyword::SectionNameSet::const_iterator ParserKeyword::validSectionNamesEnd() const  {
        return m_validSectionNames.end();
    }

    ParserKeyword::DeckNameSet::const_iterator ParserKeyword::deckNamesBegin() const {
        return m_deckNames.begin();
    }

    ParserKeyword::DeckNameSet::const_iterator ParserKeyword::deckNamesEnd() const  {
        return m_deckNames.end();
    }

    DeckKeywordPtr ParserKeyword::parse(RawKeywordConstPtr rawKeyword) const {
        if (rawKeyword->isFinished()) {
            DeckKeywordPtr keyword(new DeckKeyword(rawKeyword->getKeywordName()));
            keyword->setLocation(rawKeyword->getFilename(), rawKeyword->getLineNR());
            for (size_t i = 0; i < rawKeyword->size(); i++) {
                DeckRecordConstPtr deckRecord = m_record->parse(rawKeyword->getRecord(i));
                keyword->addRecord(deckRecord);
            }
            keyword->setDataKeyword( isDataKeyword() );
            return keyword;
        } else
            throw std::invalid_argument("Tried to create a deck keyword from an incomplete raw keyword " + rawKeyword->getKeywordName());
    }

    size_t ParserKeyword::getFixedSize() const {
        if (!hasFixedSize())
            throw std::logic_error("The parser keyword "+getName()+" does not have a fixed size!");
        return m_fixedSize;
    }

    bool ParserKeyword::hasFixedSize() const {
        return m_keywordSizeType == FIXED;
    }

    enum ParserKeywordSizeEnum ParserKeyword::getSizeType() const {
        return m_keywordSizeType;
    }

    const std::pair<std::string, std::string>& ParserKeyword::getSizeDefinitionPair() const {
        return m_sizeDefinitionPair;
    }

    bool ParserKeyword::isDataKeyword() const {
        return m_isDataKeyword;
    }

    bool ParserKeyword::hasMatchRegex() const {
        return !m_matchRegexString.empty();
    }

    void ParserKeyword::setMatchRegex(const std::string& deckNameRegexp) {
        try {
#ifdef HAVE_REGEX
            m_matchRegex = std::regex(deckNameRegexp, std::regex::extended);
#else
            m_matchRegex = boost::regex(deckNameRegexp);
#endif
            m_matchRegexString = deckNameRegexp;
        }
        catch (const std::exception &e) {
            std::cerr << "Warning: Malformed regular expression for keyword '" << getName() << "':\n"
                      << "\n"
                      << e.what() << "\n"
                      << "\n"
                      << "Ignoring expression!\n";
        }
    }

    bool ParserKeyword::matches(const std::string& deckKeywordName) const {
        if (!validDeckName(deckKeywordName))
            return false;
        else if (m_deckNames.count(deckKeywordName) > 0)
            return true;
        else if (hasMatchRegex()) {
#ifdef HAVE_REGEX
            return std::regex_match(deckKeywordName, m_matchRegex);
#else
            return boost::regex_match(deckKeywordName, m_matchRegex);
#endif
        }

        return false;
    }

    bool ParserKeyword::equal(const ParserKeyword& other) const {
        // compare the deck names. we don't care about the ordering of the strings.
        if (m_deckNames != other.m_deckNames)
            return false;

        if ((m_name == other.m_name) &&
            (m_matchRegexString == other.m_matchRegexString) &&
            (m_record->equal(*(other.m_record))) &&
            (m_keywordSizeType == other.m_keywordSizeType) &&
            (m_isDataKeyword == other.m_isDataKeyword) &&
            (m_isTableCollection == other.m_isTableCollection) &&
            (m_action == other.m_action)) {
            bool equal_ = false;
            switch (m_keywordSizeType) {
                case FIXED:
                    if (m_fixedSize == other.m_fixedSize)
                        equal_ = true;
                    break;
                case OTHER_KEYWORD_IN_DECK:
                    if ((m_sizeDefinitionPair.first == other.m_sizeDefinitionPair.first) &&
                            (m_sizeDefinitionPair.second == other.m_sizeDefinitionPair.second))
                        equal_ = true;
                    break;
                default:
                    equal_ = true;
                    break;

            }
            return equal_;
        } else
            return false;
    }

    void ParserKeyword::inlineNew(std::ostream& os, const std::string& lhs, const std::string& indent) const {
        {
            const std::string actionString(ParserKeywordActionEnum2String(m_action));
            const std::string sizeString(ParserKeywordSizeEnum2String(m_keywordSizeType));
            switch (m_keywordSizeType) {
                case SLASH_TERMINATED:
                    os << lhs << " = ParserKeyword::createDynamicSized(\"" << m_name << "\"," << sizeString << "," << actionString << ");" << std::endl;
                    break;
                case UNKNOWN:
                    os << lhs << " = ParserKeyword::createDynamicSized(\"" << m_name << "\"," << sizeString << "," << actionString << ");" << std::endl;
                    break;
                case FIXED:
                    os << lhs << " = ParserKeyword::createFixedSized(\"" << m_name << "\",(size_t)" << m_fixedSize << "," << actionString << ");" << std::endl;
                    break;
                case OTHER_KEYWORD_IN_DECK:
                    if (isTableCollection())
                        os << lhs << " = ParserKeyword::createTable(\"" << m_name << "\",\"" << m_sizeDefinitionPair.first << "\",\"" << m_sizeDefinitionPair.second << "\"," << actionString << ", true);" << std::endl;
                    else
                        os << lhs << " = ParserKeyword::createTable(\"" << m_name << "\",\"" << m_sizeDefinitionPair.first << "\",\"" << m_sizeDefinitionPair.second << "\"," << actionString << ");" << std::endl;
                    break;
            }
        }
        os << indent << lhs << "->setDescription(\"" << getDescription() << "\");" << std::endl;

        // add the valid sections for the keyword
        os << indent << lhs << "->clearValidSectionNames();\n";
        for (auto sectionNameIt = m_validSectionNames.begin();
             sectionNameIt != m_validSectionNames.end();
             ++sectionNameIt)
        {
            os << indent << lhs << "->addValidSectionName(\"" << *sectionNameIt << "\");" << std::endl;
        }

        // add the deck names
        os << indent << lhs << "->clearDeckNames();\n";
        for (auto deckNameIt = m_deckNames.begin();
             deckNameIt != m_deckNames.end();
             ++deckNameIt)
        {
            os << indent << lhs << "->addDeckName(\"" << *deckNameIt << "\");" << std::endl;
        }

        // set the deck name match regex
        if (hasMatchRegex())
            os << indent << lhs << "->setMatchRegex(\"" << m_matchRegexString << "\");" << std::endl;

        for (size_t i = 0; i < m_record->size(); i++) {
            const std::string local_indent = indent + "   ";
            ParserItemConstPtr item = m_record->get(i);
            os << local_indent << "ParserItemPtr "<<item->name()<<"item(";
            item->inlineNew(os);
            os << ");" << std::endl;
            os << local_indent << item->name()<<"item->setDescription(\"" << item->getDescription() << "\");" << std::endl;
            for (size_t idim=0; idim < item->numDimensions(); idim++)
                os << local_indent <<item->name()<<"item->push_backDimension(\"" << item->getDimension( idim ) << "\");" << std::endl;
            {
                std::string addItemMethod = "addItem";
                if (m_isDataKeyword)
                    addItemMethod = "addDataItem";

                os << local_indent << lhs << "->" << addItemMethod << "("<<item->name()<<"item);" << std::endl;
            }
        }
    }


    void ParserKeyword::applyUnitsToDeck(std::shared_ptr<const Deck> deck , std::shared_ptr<const DeckKeyword> deckKeyword) const {
        std::shared_ptr<const ParserRecord> parserRecord = getRecord();
        for (size_t index = 0; index < deckKeyword->size(); index++) {
            std::shared_ptr<const DeckRecord> deckRecord = deckKeyword->getRecord(index);
            parserRecord->applyUnitsToDeck( deck , deckRecord);
        }
    }

}
