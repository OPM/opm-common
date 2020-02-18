#ifndef OPM_UTILITY_STRING_HPP
#define OPM_UTILITY_STRING_HPP

#include <algorithm>
#include <cctype>
#include <string>

namespace Opm {

template< typename T, typename U >
U& uppercase( const T& src, U& dst ) {
    const auto up = []( char c ) { return std::toupper( c ); };
    std::transform( std::begin( src ), std::end( src ), std::begin( dst ), up );
    return dst;
}

template< typename T >
typename std::decay< T >::type uppercase( T&& x ) {
    typename std::decay< T >::type t( std::forward< T >( x ) );
    return uppercase( t, t );
}

template<typename T>
std::string ltrim_copy(const T& s)
{
    auto ret = std::string(s.c_str());

    const auto start = ret.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos)
        return "";

    return ret.substr(start);
}


template<typename T>
std::string rtrim_copy(const T& s)
{
    auto ret = std::string(s.c_str());

    const auto end = ret.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos)
        return "";

    return ret.substr(0, end + 1);
}

template<typename T>
std::string trim_copy(const T& s)
{
    return ltrim_copy( rtrim_copy(s) );
}


template<typename T>
void replaceAll(T& data, const T& toSearch, const T& replace)
{
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while (pos != std::string::npos)
    {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replace);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replace.size());
    }
}

}

#endif //OPM_UTILITY_STRING_HPP
