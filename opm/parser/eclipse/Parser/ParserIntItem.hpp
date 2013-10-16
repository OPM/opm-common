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
        ParserIntItem(const Json::JsonObject& jsonConfig);

        DeckItemConstPtr scan(RawRecordPtr rawRecord) const;
        bool equal(const ParserIntItem& other) const;
        void inlineNew(std::ostream& os) const;
        
        void setDefault(int defaultValue);
        int getDefault() const {
            return m_default;
        }

    private:
        int  m_default;
    };

    typedef boost::shared_ptr<const ParserIntItem> ParserIntItemConstPtr;
    typedef boost::shared_ptr<ParserIntItem> ParserIntItemPtr;

    
    template<class T> 
    class ParserXItem : public ParserItem{
    public:
        ParserXItem(const std::string& itemName) { };
    };
}

#endif  /* PARSERINTITEM_HPP */

