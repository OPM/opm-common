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

#ifndef PARSERFLOATITEM_HPP
#define PARSERFLOATITEM_HPP

#include <memory>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>


namespace Opm {

    class ParserFloatItem : public ParserItem
    {
    public:
        ParserFloatItem(const std::string& itemName);
        ParserFloatItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        ParserFloatItem(const std::string& itemName, float defaultValue);
        ParserFloatItem(const std::string& itemName, ParserItemSizeEnum sizeType, float defaultValue);
        explicit ParserFloatItem( const Json::JsonObject& jsonConfig);

        size_t numDimensions() const;
        bool hasDimension() const;
        void push_backDimension(const std::string& dimension);
        const std::string& getDimension(size_t index) const;
        bool equalDimensions(const ParserItem& other) const;

        DeckItemPtr scan(RawRecordPtr rawRecord) const;
        bool equal(const ParserItem& other) const;
        void inlineNew(std::ostream& os) const;
        void setDefault(float defaultValue);
        float getDefault() const;
        size_t dimensionSize() const;

    private:
        float m_default;
        std::vector<std::string> m_dimensions;
    };

    typedef std::shared_ptr<const ParserFloatItem> ParserFloatItemConstPtr;
    typedef std::shared_ptr<ParserFloatItem> ParserFloatItemPtr;
}

#endif // PARSERFLOATITEM_HPP
