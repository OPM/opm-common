#ifndef SUNBEAM_ECLIPSE_CONFIG_HPP
#define SUNBEAM_ECLIPSE_CONFIG_HPP

#include <boost/python.hpp>


namespace Opm {
}

namespace eclipse_config {
    namespace py = boost::python;
    using namespace Opm;

    void export_EclipseConfig();

}

#endif //SUNBEAM_ECLIPSE_CONFIG_HPP
