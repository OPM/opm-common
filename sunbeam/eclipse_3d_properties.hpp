#ifndef SUNBEAM_ESCLIPSE_3D_PROPERTIES_HPP
#define SUNBEAM_ESCLIPSE_3D_PROPERTIES_HPP

#include <boost/python.hpp>
#include "converters.hpp"

namespace Opm {
    class Eclipse3DProperties;
}

namespace eclipse_3d_properties {
    namespace py = boost::python;
    using namespace Opm;

    py::list getitem( const Eclipse3DProperties& p, const std::string& kw);
    bool contains( const Eclipse3DProperties& p, const std::string& kw);
    py::list regions( const Eclipse3DProperties& p, const std::string& kw);

    void export_Eclipse3DProperties();

}

#endif //SUNBEAM_ESCLIPSE_3D_PROPERTIES_HPP
