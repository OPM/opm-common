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

namespace Opm {


    ParserKeyword::ParserKeyword(const std::string& name) {
        setKeywordName(name);
        m_keywordSizeType = UNDEFINED;
    }


    ParserKeyword::ParserKeyword(const char * name) {
        setKeywordName(name);
        m_keywordSizeType = UNDEFINED;
    }


    ParserKeyword::ParserKeyword(const std::string& name, size_t fixedKeywordSize) {
        setKeywordName(name);
        m_keywordSizeType = FIXED;
        m_fixedSize = fixedKeywordSize;
    }
    
    
    ParserKeyword::ParserKeyword(const Json::JsonObject& jsonConfig) {
        setKeywordName(jsonConfig.get_string("name"));
        if (jsonConfig.has_item("size")) {
            m_fixedSize = (size_t) jsonConfig.get_int("size");
            m_keywordSizeType = FIXED;
        } else
            m_keywordSizeType = UNDEFINED;
    }


    void ParserKeyword::setKeywordName(const std::string& name) {
        if (name.length() > ParserConst::maxKeywordLength)
            throw std::invalid_argument("Given keyword name is too long - max 8 characters.");

        for (unsigned int i = 0; i < name.length(); i++)
            if (islower(name[i]))
                throw std::invalid_argument("Keyword must be all upper case - mixed case not allowed:" + name);

        m_name = name;
    }

    void ParserKeyword::setRecord(ParserRecordConstPtr record) {
        m_record = record;
    }

    ParserRecordConstPtr ParserKeyword::getRecord() {
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
}
