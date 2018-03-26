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

#include <ostream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>


namespace Opm {

namespace {

template< typename T > const T& default_value();
template<> const int& default_value< int >() {
    static const int value = -1;
    return value;
}

template<> const double& default_value< double >() {
    static constexpr auto value = std::numeric_limits< double >::quiet_NaN();
    return value;
}

template<> const std::string& default_value< std::string >() {
    static const std::string value = "";
    return value;
}

type_tag get_type_json( const std::string& str ) {
    if( str == "INT" )       return type_tag::integer;
    if( str == "DOUBLE" )    return type_tag::fdouble;
    if( str == "STRING" )    return type_tag::string;
    if( str == "RAW_STRING") return type_tag::string;
    throw std::invalid_argument( str + " cannot be converted to enum 'tag'" );
}


}

ParserItem::item_size ParserItem::size_from_string( const std::string& str ) {
    if( str == "ALL" )   return item_size::ALL;
    if( str == "SINGLE") return item_size::SINGLE;
    throw std::invalid_argument( str + " can not be converted "
                                 "to enum 'item_size'" );
}

std::string ParserItem::string_from_size( ParserItem::item_size sz ) {
    switch( sz ) {
        case item_size::ALL:    return "ALL";
        case item_size::SINGLE: return "SINGLE";
    }

    throw std::logic_error( "Fatal error; should not be reachable" );
}

template<> const int& ParserItem::value_ref< int >() const {
    if( this->type != get_type< int >() )
        throw std::invalid_argument( "Wrong type." );
    return this->ival;
}

template<> const double& ParserItem::value_ref< double >() const {
    if( this->type != get_type< double >() )
        throw std::invalid_argument( "Wrong type." );
    return this->dval;
}

template<> const std::string& ParserItem::value_ref< std::string >() const {
    if( this->type != get_type< std::string >() )
        throw std::invalid_argument( "Wrong type." );
    return this->sval;
}

template< typename T >
T& ParserItem::value_ref() {
    return const_cast< T& >(
                const_cast< const ParserItem& >( *this ).value_ref< T >()
            );
}

ParserItem::ParserItem( const std::string& itemName ) :
    ParserItem( itemName, ParserItem::item_size::SINGLE )
{}

ParserItem::ParserItem( const std::string& itemName,
                        ParserItem::item_size p_sizeType ) :
    m_name( itemName ),
    m_sizeType( p_sizeType ),
    m_defaultSet( false )
{}

ParserItem::ParserItem( const std::string& itemName, item_size sz, int val ) :
    ival( val ),
    m_name( itemName ),
    m_sizeType( sz ),
    type( get_type< int >() ),
    m_defaultSet( true )
{}

ParserItem::ParserItem( const std::string& itemName,
                        item_size sz,
                        double val ) :
    dval( val ),
    m_name( itemName ),
    m_sizeType( sz ),
    type( get_type< double >() ),
    m_defaultSet( true )
{}

ParserItem::ParserItem( const std::string& itemName,
                        item_size sz,
                        std::string val ) :
    sval( std::move( val ) ),
    m_name( itemName ),
    m_sizeType( sz ),
    type( get_type< std::string >() ),
    m_defaultSet( true )
{}

ParserItem::ParserItem( const Json::JsonObject& json ) :
    m_name( json.get_string( "name" ) ),
    m_sizeType( json.has_item( "size_type" )
              ? ParserItem::size_from_string( json.get_string( "size_type" ) )
              : ParserItem::item_size::SINGLE ),
    m_description( json.has_item( "description" )
                 ? json.get_string( "description" )
                 : "" ),
    type( get_type_json( json.get_string( "value_type" ) ) ),
    m_defaultSet( false )
{
    if( json.has_item( "dimension" ) ) {
        const auto& dim = json.get_item( "dimension" );

        if( dim.is_string() ) {
            this->push_backDimension( dim.as_string() );
        }
        else if( dim.is_array() ) {
            for( size_t i = 0; i < dim.size(); ++i )
                this->push_backDimension( dim.get_array_item( i ).as_string() );
        }
        else {
            throw std::invalid_argument(
                "The 'dimension' attribute must be a string or list of strings"
            );
        }
    }
    if (json.get_string("value_type") == "RAW_STRING")
        this->raw_string = true;

    if( !json.has_item( "default" ) ) return;

    switch( this->type ) {
        case type_tag::integer:
            this->setDefault( json.get_int( "default" ) );
            break;

        case type_tag::fdouble:
            this->setDefault( json.get_double( "default" ) );
            break;

        case type_tag::string:
            this->setDefault( json.get_string( "default" ) );
            break;

        default:
            throw std::logic_error( "Item of unknown type." );
    }
}

template< typename T >
void ParserItem::setDefault( T val ) {
    if( this->type != type_tag::fdouble && this->m_sizeType == item_size::ALL )
        throw std::invalid_argument( "The size type ALL can not be combined "
                                     "with an explicit default value." );

    this->value_ref< T >() = std::move( val );
    this->m_defaultSet = true;
}

template< typename T >
void ParserItem::setType( T) {
    this->type = get_type< T >();
}

template< typename T >
void ParserItem::setType( T, bool raw ) {
    this->type = get_type< T >();
    this->raw_string = raw;
}


bool ParserItem::hasDefault() const {
    return this->m_defaultSet;
}


template< typename T >
const T& ParserItem::getDefault() const {
    if( get_type< T >() != this->type )
        throw std::invalid_argument( "Wrong type." );

    if( !this->hasDefault() && this->m_sizeType == item_size::ALL )
        return default_value< T >();

    if( !this->hasDefault() )
        throw std::invalid_argument( "No default value available for item "
                                     + this->name() );

    return this->value_ref< T >();
}

bool ParserItem::hasDimension() const {
    if( this->type != type_tag::fdouble )
        return false;

    return !this->dimensions.empty();
}

size_t ParserItem::numDimensions() const {
    if( this->type != type_tag::fdouble ) return 0;
    return this->dimensions.size();
}

const std::string& ParserItem::getDimension( size_t index ) const {
    if( this->type != type_tag::fdouble )
        throw std::invalid_argument("Item is not double.");

    return this->dimensions.at( index );
}

void ParserItem::push_backDimension( const std::string& dim ) {
    if( this->type != type_tag::fdouble )
        throw std::invalid_argument( "Invalid type, does not have dimension." );

    if( this->sizeType() == item_size::SINGLE && this->dimensions.size() > 0 ) {
        throw std::invalid_argument(
            "Internal error: "
            "cannot add more than one dimension to an item of size 1" );
    }

    this->dimensions.push_back( dim );
}

    const std::string& ParserItem::name() const {
        return m_name;
    }

    const std::string ParserItem::className() const {
        return m_name;
    }



    ParserItem::item_size ParserItem::sizeType() const {
        return m_sizeType;
    }

    bool ParserItem::scalar() const {
        return this->m_sizeType == item_size::SINGLE;
    }

    std::string ParserItem::getDescription() const {
        return m_description;
    }

    void ParserItem::setDescription(std::string description) {
        m_description = description;
    }

bool ParserItem::operator==( const ParserItem& rhs ) const {
    if( !( this->type           == rhs.type
        && this->m_name         == rhs.m_name
        && this->m_description  == rhs.m_description
        && this->m_sizeType     == rhs.m_sizeType
        && this->m_defaultSet   == rhs.m_defaultSet ) )
            return false;

    if( this->m_defaultSet ) {
        switch( this->type ) {
            case type_tag::integer:
                if( this->ival != rhs.ival ) return false;
                break;

            case type_tag::fdouble:
                if( this->dval != rhs.dval ) return false;
                break;

            case type_tag::string:
                if( this->sval != rhs.sval ) return false;
                break;

            default:
                throw std::logic_error( "Item of unknown type." );
        }
    }

    if( this->type != type_tag::fdouble ) return true;

    return this->dimensions.size() == rhs.dimensions.size()
        && std::equal( this->dimensions.begin(),
                       this->dimensions.end(),
                       rhs.dimensions.begin() );
}

bool ParserItem::operator!=( const ParserItem& rhs ) const {
    return !( *this == rhs );
}

std::string ParserItem::createCode() const {
    std::stringstream stream;
    stream << "ParserItem item(\"" << this->name()
           << "\", ParserItem::item_size::" << this->sizeType();

       if( m_defaultSet ) {
           stream << ", ";
           switch( this->type ) {
               case type_tag::integer:
                   stream << this->getDefault< int >();
                   break;

               case type_tag::fdouble:
                   stream << "double( "
                          << boost::lexical_cast< std::string >
                                ( this->getDefault< double >() )
                          << " )";
                   break;

               case type_tag::string:
                   stream << "\"" << this->getDefault< std::string >() << "\"";
                   break;

               default:
                   throw std::logic_error( "Item of unknown type." );
           }
        }

    if (raw_string)
        stream << " ); item.setType( " << tag_name( this->type ) << "(), true );"; 
    else
        stream << " ); item.setType( " << tag_name( this->type ) << "() );";

    return stream.str();
}

namespace {

template< typename T >
DeckItem scan_item( const ParserItem& p, RawRecord& record ) {
    DeckItem item( p.name(), T(), record.size() );
    bool parse_raw = p.parseRaw();

    if( p.sizeType() == ParserItem::item_size::ALL ) {
        if (parse_raw) {
            while (record.size()) {
                auto token = record.pop_front();
                item.push_back( token.string() );
            }
            return item;
        }

        while( record.size() > 0 ) {
            auto token = record.pop_front();

            std::string countString;
            std::string valueString;

            if( !isStarToken( token, countString, valueString ) ) {
                item.push_back( readValueToken< T >( token ) );
                continue;
            }

            StarToken st(token, countString, valueString);

            if( st.hasValue() ) {
                item.push_back( readValueToken< T >( st.valueString() ), st.count() );
                continue;
            }

            auto value = p.getDefault< T >();
            for (size_t i=0; i < st.count(); i++)
                item.push_backDefault( value );
        }

        return item;
    }

    if( record.size() == 0 ) {
        // if the record was ended prematurely,
        if( p.hasDefault() ) {
            // use the default value for the item, if there is one...
            item.push_backDefault( p.getDefault< T >() );
        } else {
            // ... otherwise indicate that the deck item should throw once the
            // item's data is accessed.
            item.push_backDummyDefault();
        }

        return item;
    }

    if (parse_raw) {
        item.push_back( record.pop_front().string());
        return item;
    }

    // The '*' should be interpreted as a repetition indicator, but it must
    // be preceeded by an integer...
    auto token = record.pop_front();
    std::string countString;
    std::string valueString;
    if( !isStarToken(token, countString, valueString) ) {
        item.push_back( readValueToken<T>( token) );
        return item;
    }

    StarToken st(token, countString, valueString);

    if( st.hasValue() )
        item.push_back(readValueToken< T >( st.valueString()) );
    else if( p.hasDefault() )
        item.push_backDefault( p.getDefault< T >() );
    else
        item.push_backDummyDefault();

    const auto value_start = token.size() - valueString.size();
    // replace the first occurence of "N*FOO" by a sequence of N-1 times
    // "FOO". this is slightly hacky, but it makes it work if the
    // number of defaults pass item boundaries...
    // We can safely make a string_view of one_star because it
    // has static storage
    static const char* one_star = "1*";
    string_view rep = !st.hasValue()
                    ? string_view{ one_star }
                    : string_view{ token.begin() + value_start, token.end() };
    record.prepend( st.count() - 1, rep );

    return item;
}

}


/// Scans the records data according to the ParserItems definition.
/// returns a DeckItem object.
/// NOTE: data are popped from the records deque!
DeckItem ParserItem::scan( RawRecord& record ) const {
    switch( this->type ) {
        case type_tag::integer:
            return scan_item< int >( *this, record );
        case type_tag::fdouble:
            return scan_item< double >( *this, record );
        case type_tag::string:
            return scan_item< std::string >( *this, record );
        default:
            throw std::logic_error( "Fatal error; should not be reachable" );
    }
}

std::ostream& ParserItem::inlineClass( std::ostream& stream, const std::string& indent ) const {
    std::string local_indent = indent + "    ";

    stream << indent << "class " << this->className() << " {" << std::endl
           << indent << "public:" << std::endl
           << local_indent << "static const std::string itemName;" << std::endl;

    if( this->hasDefault() ) {
        stream << local_indent << "static const "
               << tag_name( this->type )
               << " defaultValue;" << std::endl;
    }

    return stream << indent << "};" << std::endl;
}

std::string ParserItem::inlineClassInit(const std::string& parentClass,
                                        const std::string* defaultValue ) const {

    std::stringstream ss;
    ss << "const std::string " << parentClass
       << "::" << this->className()
       << "::itemName = \"" << this->name()
       << "\";" << std::endl;

    if( !this->hasDefault() ) return ss.str();

    auto typestring = tag_name( this->type );

    auto defval = [this]() -> std::string {
        switch( this->type ) {
            case type_tag::integer:
                return std::to_string( this->getDefault< int >() );
            case type_tag::fdouble:
                return std::to_string( this->getDefault< double >() );
            case type_tag::string:
                return "\"" + this->getDefault< std::string >() + "\"";

            default:
                throw std::logic_error( "Fatal error; should not be reachable" );
        }
    };

    ss << "const " << typestring << " "
        << parentClass << "::" << this->className()
        << "::defaultValue = " << (defaultValue ? *defaultValue : defval() )
        << ";" << std::endl;

    return ss.str();
}


std::ostream& operator<<( std::ostream& stream, const ParserItem::item_size& sz ) {
    return stream << ParserItem::string_from_size( sz );
}

std::ostream& operator<<( std::ostream& stream, const ParserItem& item ) {
    stream
        << "ParserItem " << item.name() << " { "
        << "size: " << item.sizeType() << " "
        << "description: '" << item.getDescription() << "' "
        ;

    if( item.hasDefault() ) {
        stream << "default: ";
        switch( item.type ) {
            case type_tag::integer:
                stream << item.getDefault< int >();
                break;

            case type_tag::fdouble:
                stream << item.getDefault< double >();
                break;

            case type_tag::string:
                stream << "'" << item.getDefault< std::string >() << "'";
                break;

            default:
                throw std::logic_error( "Item of unknown type." );
        }

        stream << " ";
    }

    if( !item.hasDimension() )
        stream << "dimensions: none";
    else {
        stream << "dimensions: [ ";
        for( size_t i = 0; i < item.numDimensions(); ++i )
            stream << "'" << item.getDimension( i ) << "' ";
        stream << "]";
    }

    return stream << " }";
}

bool ParserItem::parseRaw( ) const {
    return this->raw_string;
}

template void ParserItem::setDefault( int );
template void ParserItem::setDefault( double );
template void ParserItem::setDefault( std::string );

template void ParserItem::setType( int );
template void ParserItem::setType( double );
template void ParserItem::setType( std::string );
template void ParserItem::setType( std::string , bool);

template const int& ParserItem::getDefault() const;
template const double& ParserItem::getDefault() const;
template const std::string& ParserItem::getDefault() const;

}
