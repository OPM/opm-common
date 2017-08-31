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
    int getNumActive( const EclipseGrid& grid );
    int getCartesianSize( const EclipseGrid& grid );
    int getGlobalIndex( const EclipseGrid& grid, int i, int j, int k );
    py::tuple getIJK( const EclipseGrid& grid, int g );
    double cellVolume1G( const EclipseGrid& grid, size_t glob_idx);
    double cellVolume3( const EclipseGrid& grid, size_t i_idx, size_t j_idx, size_t k_idx);

    void export_EclipseGrid();

}

#endif //SUNBEAM_ECLIPSE_GRID_HPP
