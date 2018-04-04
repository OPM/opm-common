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

#include <iosfwd>
#include <string>
#include <vector>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Utility/Typetools.hpp>

namespace Json {
    class JsonObject;
}

namespace Opm {

    class RawRecord;

    class ParserItem {
    public:
        enum class item_size { ALL, SINGLE };
        static item_size   size_from_string( const std::string& );
        static std::string string_from_size( item_size );

        explicit ParserItem( const std::string& name );
        ParserItem( const std::string& name, item_size );
        template< typename T >
        ParserItem( const std::string& item_name, T val ) :
            ParserItem( item_name, item_size::SINGLE, std::move( val ) ) {}
        ParserItem( const std::string& name, item_size, int defaultValue );
        ParserItem( const std::string& name, item_size, double defaultValue );
        ParserItem( const std::string& name, item_size, std::string defaultValue );

        explicit ParserItem(const Json::JsonObject& jsonConfig);

        void push_backDimension( const std::string& );
        const std::string& getDimension(size_t index) const;
        bool hasDimension() const;
        size_t numDimensions() const;
        const std::string& name() const;
        item_size sizeType() const;
        std::string getDescription() const;
        bool scalar() const;
        void setDescription(std::string helpText);

        template< typename T > void setDefault( T );
        /* set type without a default value. will reset dimension etc. */
        template< typename T > void setType( T );
        template< typename T > void setType( T , bool raw);
        bool parseRaw() const;
        bool hasDefault() const;
        template< typename T > const T& getDefault() const;

        bool operator==( const ParserItem& ) const;
        bool operator!=( const ParserItem& ) const;

        DeckItem scan( RawRecord& rawRecord ) const;
        const std::string className() const;
        std::string createCode() const;
        std::ostream& inlineClass(std::ostream&, const std::string& indent) const;
        std::string inlineClassInit(const std::string& parentClass,
                                    const std::string* defaultValue = nullptr ) const;

    private:
        double dval;
        int ival;
        std::string sval;
        bool raw_string = false;
        std::vector< std::string > dimensions;

        std::string m_name;
        item_size m_sizeType;
        std::string m_description;

        type_tag type = type_tag::unknown;
        bool m_defaultSet;

        template< typename T > T& value_ref();
        template< typename T > const T& value_ref() const;
        friend std::ostream& operator<<( std::ostream&, const ParserItem& );
    };

std::ostream& operator<<( std::ostream&, const ParserItem::item_size& );

}

#endif
