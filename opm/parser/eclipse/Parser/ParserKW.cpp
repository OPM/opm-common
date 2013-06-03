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

#include <opm/parser/eclipse/Parser/ParserConst.hpp>
#include <opm/parser/eclipse/Parser/ParserRecordSize.hpp>
#include <opm/parser/eclipse/Parser/ParserKW.hpp>

namespace Opm {

    ParserKW::ParserKW(const std::string& name) {
        m_name = name;
    }

    ParserKW::ParserKW(const std::string& name, ParserRecordSizeConstPtr recordSize) {
        if (name.length() > ParserConst::maxKWLength)
            throw std::invalid_argument("Given keyword name is too long - max 8 characters.");

        for (unsigned int i = 0; i < name.length(); i++)
            if (islower(name[i]))
                throw std::invalid_argument("Keyword must be all upper case - mixed case not allowed:" + name);

        m_name = name;
        this->recordSize = recordSize;
    }

    void ParserKW::setRecord(ParserRecordConstPtr record) {
        m_record = record;
    }

    ParserRecordConstPtr ParserKW::getRecord() {
        return m_record;
    }
    
    const std::string& ParserKW::getName() const {
        return m_name;
    }

    DeckKWPtr ParserKW::parse(RawKeywordPtr rawKeyword) {
        DeckKWPtr keyword(new DeckKW(getName()));
        if (m_record != NULL) {
            for (size_t i=0; i<rawKeyword->size(); i++) {
                DeckRecordConstPtr deckRecord = m_record->parse(rawKeyword->getRecord(i));
                keyword->addRecord(deckRecord);
            }
        }
        else 
            throw std::logic_error("Unable to parse rawKeyword, because the ParserKW's record is not set!");
        
        return keyword;
    }
}
