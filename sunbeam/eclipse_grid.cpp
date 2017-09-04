#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultFace.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>

#include "eclipse_grid.hpp"

namespace py = boost::python;
using namespace Opm;

using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;

namespace eclipse_grid {
    py::tuple getXYZ( const EclipseGrid& grid ) {
        return py::make_tuple( grid.getNX(),
                               grid.getNY(),
                               grid.getNZ());
    }
    int getNumActive( const EclipseGrid& grid ) {
        return grid.getNumActive();
    }
    int getCartesianSize( const EclipseGrid& grid ) {
        return grid.getCartesianSize();
    }
    int getGlobalIndex( const EclipseGrid& grid, int i, int j, int k ) {
        return grid.getGlobalIndex(i, j, k);
    }
    py::tuple getIJK( const EclipseGrid& grid, int g ) {
        const auto& ijk = grid.getIJK(g);
        return py::make_tuple(ijk[0], ijk[1], ijk[2]);
    }
    double cellVolume1G( const EclipseGrid& grid, size_t glob_idx) {
      return grid.getCellVolume(glob_idx);
    }
    double cellVolume3( const EclipseGrid& grid, size_t i_idx, size_t j_idx, size_t k_idx) {
      return grid.getCellVolume(i_idx, j_idx, k_idx);
    }


    void export_EclipseGrid() {

        py::class_< EclipseGrid >( "EclipseGrid", py::no_init )
            .def( "_getXYZ",        &getXYZ )
            .def( "nactive",        &getNumActive )
            .def( "cartesianSize",  &getCartesianSize )
            .def( "globalIndex",    &getGlobalIndex )
            .def( "getIJK",         &getIJK )
            .def( "_cellVolume1G",  &cellVolume1G)
            .def( "_cellVolume3",   &cellVolume3)
            ;

    }
}
