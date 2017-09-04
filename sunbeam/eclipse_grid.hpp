#ifndef SUNBEAM_ECLIPSE_GRID_HPP
#define SUNBEAM_ECLIPSE_GRID_HPP

#include <boost/python.hpp>


namespace Opm {
    class EclipseGrid;
}

namespace eclipse_grid {
    namespace py = boost::python;
    using namespace Opm;

    py::tuple getXYZ( const EclipseGrid& grid );
    void export_EclipseGrid();

}

#endif //SUNBEAM_ECLIPSE_GRID_HPP
