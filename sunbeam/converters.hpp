#ifndef SUNBEAM_CONVERTERS_HPP
#define SUNBEAM_CONVERTERS_HPP

#include <exception>
#include <vector>

#include <boost/python.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <datetime.h>

namespace py = boost::python;


/*
 * boost.python lacks converters for a lot of types we use, or we need to
 * fine-tune the behaviour a little bit. They're often for one specific
 * instantiation or function, but they have in common that they're not really
 * reflected much in C++ code for other things than conversion registries or
 * single instantiations.
 *
 * a converter is a struct with the static method
 *  `PyObject* convert( const type& )`
 * which returns a Python C-api object either by invoking the C api directly or
 * going via boost types. It is the job of the conversion function to make sure
 * resources and ownership is handled and converted properly.
 */


/* boost.posix_time(y, m, d) -> datetime.date(y, m, d) */
struct ptime_to_python_datetime {
    static PyObject* convert( const boost::posix_time::ptime& pt ) {
        const auto& date = pt.date();
        return PyDate_FromDate( int( date.year() ),
                                int( date.month() ),
                                int( date.day() ) );

    }
};

/*
 * Plenty of sunbeam's types are essentially dictionaries, and we want to map
 * directly from some C++ function to __getitem__. However, C++ associative
 * containers throw std::out_of_range if the key is not present, and in python
 * you'd expect a KeyError for this. Boost.python is not able to map between
 * these two types so we register our own exception with boost. When this
 * exception is thrown it will raise a corresponding KeyError in python.
 */
struct key_error : public std::out_of_range {
    static void translate( const key_error& e ) {
        PyErr_SetString( PyExc_KeyError, e.what() );
    }

    using std::out_of_range::out_of_range;
};

/*
 * opm returns plenty of container or container-like types. In python, this
 * rigid structure isn't as interesting and we often want a quick way of obtain
 * a first-class python list of our values. This can either be achieved by
 * registering it with vector_indexing_suite, which is perfect when the value
 * is already a C++ by-value or by-ref vector of some type with value
 * semantics. However, sometimes C++ returns a set where we in python would
 * probably rather use a list, or C++ returns a type that doesnt't play as nice
 * with boost.python (see functions that return a vector of non-owning pointers
 * for instance). In these cases we directly construct a python list with what
 * is essentially a C++-backed list comprehension. It works on anything that
 * supports C++' for-each syntax.
 *
 * Requires that whatever's being mapped is registered with a C++ -> python
 * converter.
 */
template< typename T >
py::list iterable_to_pylist( const T& v ) {
    py::list l;
    for( const auto& x : v ) l.append( x );
    return l;
}

/*
 * Return value modifiers in boost python are verbose and clunky, so they're
 * aliased for some comfort.
 */
using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;

/*
 * boost isn't that good at figuring out to when to return properties by
 * reference, and unlike .def()'s can't take the return value modifier as its
 * third argument because it possibly expects a setter there. This is easily
 * fixed by manually constructing the py::function, but the syntax is a
 * handful. mkref takes any C++ function and creates a boost.python function
 * that returns by (internal) reference and can be easily used with
 * .add_property.
 */
template< typename F >
auto mkref( F f ) -> decltype( py::make_function( f, ref() ) ) {
    return py::make_function( f, ref() );
}

/*
 * boost doesn't really like functions that returns strings by reference,
 * becuase it's somewhat difficult to map onto python's owning, immutable
 * strings. Since string data (and similar) typically are trivial and
 * uninteresting to preserve references, mkcopy() takes any function that
 * returns by const ref and makes sure the returned python object copies it and
 * owns the new instance.
 */
template< typename F >
auto mkcopy( F f ) -> decltype( py::make_function( f, copy() ) ) {
    return py::make_function( f, copy() );
}

#endif //SUNBEAM_CONVERTERS_HPP
