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


#ifndef PARSERINTITEM_HPP
#define PARSERINTITEM_HPP

#include <memory>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>


namespace Opm {

    class ParserIntItem : public ParserItem {
    public:
        ParserIntItem(const std::string& itemName);
        ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        ParserIntItem(const std::string& itemName, int defaultValue);
        ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType, int defaultValue);
        explicit ParserIntItem(const Json::JsonObject& jsonConfig);

        DeckItemPtr scan(RawRecordPtr rawRecord) const;
        bool equal(const ParserItem& other) const;

        std::string createCode() const;
	void inlineClass(std::ostream& os, const std::string& indent) const;
        std::string inlineClassInit(const std::string& parentClass) const;

        int  getDefault() const;
        bool hasDefault() const;
        void setDefault(int defaultValue);
    private:
        int  m_default;
    };

    typedef std::shared_ptr<const ParserIntItem> ParserIntItemConstPtr;
    typedef std::shared_ptr<ParserIntItem> ParserIntItemPtr;
}

#endif  /* PARSERINTITEM_HPP */

