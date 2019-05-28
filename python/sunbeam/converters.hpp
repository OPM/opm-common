#ifndef SUNBEAM_CONVERTERS_HPP
#define SUNBEAM_CONVERTERS_HPP

#include <sstream>
#include <pybind11/pybind11.h>

namespace py = pybind11;

template< typename T >
py::list iterable_to_pylist( const T& v ) {
    py::list l;
    for( const auto& x : v ) l.append( x );
    return l;
}

template< typename T >
std::string str( const T& t ) {
    std::stringstream stream;
    stream << t;
    return stream.str();
}

#endif //SUNBEAM_CONVERTERS_HPP
