#ifndef OPM_TYPETOOLS_HPP
#define OPM_TYPETOOLS_HPP
#include <opm/parser/eclipse/Deck/UDAValue.hpp>

namespace Opm {

enum class type_tag {
    unknown = 0,
    integer = 1,
    string  = 2,
    fdouble = 3,
    uda     = 4,
};

inline std::string tag_name( type_tag x ) {
    switch( x ) {
        case type_tag::integer: return "int";
        case type_tag::string:  return "std::string";
        case type_tag::fdouble: return "double";
        case type_tag::uda:     return "UDAValue";
        case type_tag::unknown: return "unknown";
    }
    return "unknown";
}

template< typename T > type_tag get_type();

template<> inline type_tag get_type< int >() {
    return type_tag::integer;
}

template<> inline type_tag get_type< double >() {
    return type_tag::fdouble;
}

template<> inline type_tag get_type< std::string >() {
    return type_tag::string;
}

template<> inline type_tag get_type<UDAValue>() {
  return type_tag::uda;
}

}

#endif //OPM_TYPETOOLS_HPP
