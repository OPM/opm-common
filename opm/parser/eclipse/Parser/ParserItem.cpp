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

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

namespace Opm {

    ParserItem::ParserItem(const std::string& itemName, ParserItemSizeEnum sizeType) {
        m_name.assign(itemName);
        m_sizeType = sizeType;
        m_defaultSet = false;
    }

    
    ParserItem::ParserItem(const Json::JsonObject& jsonConfig) {
        if (jsonConfig.has_item("name"))
            m_name = jsonConfig.get_string("name");
        else
            throw std::invalid_argument("Json config object missing \"name\": ... item");

        if (jsonConfig.has_item("size_type")) {
            const std::string sizeTypeString = jsonConfig.get_string("size_type");
            m_sizeType = ParserItemSizeEnumFromString( sizeTypeString );
        } else
            throw std::invalid_argument("Json config object missing \"size_type\": ... item");
    }

    const std::string& ParserItem::name() const {
        return m_name;
    }

    ParserItemSizeEnum ParserItem::sizeType() const {
        return m_sizeType;
    }

    int ParserItem::defaultInt() {
        return 0;
    }

    double ParserItem::defaultDouble() {
        return 0.0;
    }

    std::string ParserItem::defaultString() {
        return "DEFAULT";
    }


    bool ParserItem::equal(const ParserItem& other) const {
        if (typeid(this) == typeid(&other)) {
            if ((name() == other.name()) && (sizeType() == other.sizeType()))
                return true;
            else
                return false;
        }
        else
            return false;
    }

}
