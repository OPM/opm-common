#ifndef SUNBEAM_GROUP_HPP
#define SUNBEAM_GROUP_HPP

#include <boost/python.hpp>
#include "converters.hpp"


namespace Opm {
    class Group;
}

namespace group {

    namespace py = boost::python;
    using namespace Opm;

    py::list wellnames( const Group& g, size_t timestep );
    void export_Group();

}

#endif //SUNBEAM_GROUP_HPP
