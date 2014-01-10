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
#ifndef PARSER_ITEM_H
#define PARSER_ITEM_H

#include <string>
#include <sstream>
#include <iostream>
#include <deque>

#include <memory>
#include <boost/lexical_cast.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>


namespace Opm {
    
    class ParserItem {
    public:
        ParserItem(const std::string& itemName);
        ParserItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        explicit ParserItem(const Json::JsonObject& jsonConfig);

        virtual void push_backDimension(const std::string& dimension);
        virtual const std::string& getDimension(size_t index) const;
        virtual DeckItemPtr scan(RawRecordPtr rawRecord) const = 0;
        virtual bool hasDimension() const;
        virtual size_t numDimensions() const;
        const std::string& name() const;
        ParserItemSizeEnum sizeType() const;
        std::string getDescription() const;
        void setDescription(std::string helpText);

        static int defaultInt();
        static std::string defaultString();
        static double defaultDouble();
      
        virtual bool equal(const ParserItem& other) const;
        virtual void inlineNew(std::ostream& os) const {}
      
        virtual ~ParserItem() {
        }
    
    protected:
        bool m_defaultSet;



    private:
        std::string m_name;
        ParserItemSizeEnum m_sizeType;
        std::string m_description;
    };

    typedef std::shared_ptr<const ParserItem> ParserItemConstPtr;
    typedef std::shared_ptr<ParserItem> ParserItemPtr;


}

#endif

