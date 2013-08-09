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

#ifndef PARSERDOUBLEITEM_HPP
#define PARSERDOUBLEITEM_HPP

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>


namespace Opm {

    class ParserDoubleItem : public ParserItem {
    public:
        ParserDoubleItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        ParserDoubleItem(const std::string& itemName, ParserItemSizeEnum sizeType, double defaultValue);
        ParserDoubleItem( const Json::JsonObject& jsonConfig);

        DeckItemConstPtr scan(size_t expectedItems , RawRecordPtr rawRecord) const;
        DeckItemConstPtr scan(RawRecordPtr rawRecord) const;
        
        double getDefault() const {
            return m_default;
        }

    private:
        DeckItemConstPtr scan__(size_t expectedItems , bool scanAll , RawRecordPtr rawRecord) const;
        double  m_default;
    };

    typedef boost::shared_ptr<const ParserDoubleItem> ParserDoubleItemConstPtr;
    typedef boost::shared_ptr<ParserDoubleItem> ParserDoubleItemPtr;
}

#endif  /* PARSERINTITEM_HPP */

