#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaultFace.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>

#include <pybind11/stl.h>
#include "converters.hpp"
#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {
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

    py::array cellVolumeAll( const EclipseGrid& grid)
    {
        std::vector<double> cellVol;
        std::array<int, 3> dims = grid.getNXYZ();        
        size_t nCells = dims[0]*dims[1]*dims[2];
        cellVol.reserve(nCells);
        
        for (size_t n = 0; n < nCells; n++)
            cellVol.push_back(grid.getCellVolume(n));
        
        return convert::numpy_array(cellVol);
    }

    py::array cellVolumeMask( const EclipseGrid& grid, std::vector<int>& mask)
    {
        std::array<int, 3> dims = grid.getNXYZ();        
        size_t nCells = dims[0]*dims[1]*dims[2];
    
        if (nCells != mask.size()) 
            throw std::logic_error("size of input mask doesn't match size of grid");
        
        std::vector<double> cellVol(nCells, 0.0);

        for (size_t n = 0; n < nCells; n++)
            if (mask[n]==1)
                cellVol[n] = grid.getCellVolume(n);
                
        return convert::numpy_array(cellVol);
    }
    
    double cellDepth1G( const EclipseGrid& grid, size_t glob_idx) {
      return grid.getCellDepth(glob_idx);
    }

    double cellDepth3( const EclipseGrid& grid, size_t i_idx, size_t j_idx, size_t k_idx) {
      return grid.getCellDepth(i_idx, j_idx, k_idx);
    }
    
    py::array cellDepthAll( const EclipseGrid& grid)
    {
        std::vector<double> cellDepth;
        std::array<int, 3> dims = grid.getNXYZ();        
        size_t nCells = dims[0]*dims[1]*dims[2];
        cellDepth.reserve(nCells);
        
        for (size_t n = 0; n < nCells; n++)
            cellDepth.push_back(grid.getCellDepth(n));
        
        return convert::numpy_array(cellDepth);
    }

    py::array cellDepthMask( const EclipseGrid& grid, std::vector<int>& mask)
    {
        std::array<int, 3> dims = grid.getNXYZ();        
        size_t nCells = dims[0]*dims[1]*dims[2];
    
        if (nCells != mask.size()) 
            throw std::logic_error("size of input mask doesn't match size of grid");
        
        std::vector<double> cellDepth(nCells, 0.0);

        for (size_t n = 0; n < nCells; n++)
            if (mask[n]==1)
                cellDepth[n] = grid.getCellDepth(n);
                
        return convert::numpy_array(cellDepth);
    }
}

void python::common::export_EclipseGrid(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_< EclipseGrid >( module, "EclipseGrid", EclipseGrid_docstring )
        .def( "_getXYZ", &getXYZ, EclipseGrid_getXYZ_docstring )
        .def_property_readonly("nx", &EclipseGrid::getNX, EclipseGrid_nx_docstring )
        .def_property_readonly("ny", &EclipseGrid::getNY, EclipseGrid_ny_docstring )
        .def_property_readonly("nz", &EclipseGrid::getNZ, EclipseGrid_nz_docstring )
        .def_property_readonly( "nactive", &getNumActive, EclipseGrid_nactive_docstring )
        .def_property_readonly( "cartesianSize", &getCartesianSize, EclipseGrid_cartesianSize_docstring )
        .def("globalIndex", &getGlobalIndex, py::arg("i"), py::arg("j"), py::arg("k"), EclipseGrid_globalIndex_docstring)
        .def( "getIJK", &getIJK, py::arg("g"), EclipseGrid_getIJK_docstring )
        .def( "getCellVolume", &cellVolume1G, py::arg("g"), EclipseGrid_getCellVolume1G_docstring )
        .def( "getCellVolume", &cellVolume3, py::arg("i"), py::arg("j"), py::arg("k"), EclipseGrid_getCellVolume3_docstring )
        .def( "getCellVolume", &cellVolumeAll, EclipseGrid_getCellVolumeAll_docstring )
        .def( "getCellVolume", &cellVolumeMask, py::arg("mask"), EclipseGrid_getCellVolumeMask_docstring )
        .def( "getCellDepth", &cellDepth1G, py::arg("g"), EclipseGrid_getCellDepth1G_docstring )
        .def( "getCellDepth", &cellDepth3, py::arg("i"), py::arg("j"), py::arg("k"), EclipseGrid_getCellDepth3_docstring )
        .def( "getCellDepth", &cellDepthAll, EclipseGrid_getCellDepthAll_docstring )
        .def( "getCellDepth", &cellDepthMask, py::arg("mask"), EclipseGrid_getCellDepthMask_docstring )
        ;

}
