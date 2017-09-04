#ifndef SUNBEAM_WELL_HPP
#define SUNBEAM_WELL_HPP

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "converters.hpp"


namespace Opm {
    class Well;
}

namespace well {
    namespace py = boost::python;
    using namespace Opm;

    py::list completions( const Well& w) ;
    std::string status( const Well& w, size_t timestep );
    std::string preferred_phase( const Well& w );

    void export_Well();

}

#endif //SUNBEAM_WELL_HPP
