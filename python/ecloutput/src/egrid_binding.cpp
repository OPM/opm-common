
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "pybind11/numpy.h"

#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/io/eclipse/EGrid.cpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      


namespace py = pybind11;

class EGridTmp : public Opm::EclIO::EGrid {
    
public:
    EGridTmp(const std::string& filename): Opm::EclIO::EGrid(filename) {};
    
    std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> getCellCornersTmp(int globindex) {
        std::vector<double> X(8, 0.0);     
        std::vector<double> Y(8, 0.0);     
        std::vector<double> Z(8, 0.0);

        getCellCorners(ijk_from_global_index(globindex),X,Y,Z);    
       
        return std::make_tuple(X,Y,Z);
    }

    std::tuple<py::array_t<double>, py::array_t<double>, py::array_t<double>> getCellCornersTmpNumpy(int globindex) {
        std::vector<double> X(8, 0.0);     
        std::vector<double> Y(8, 0.0);     
        std::vector<double> Z(8, 0.0);

        getCellCorners(ijk_from_global_index(globindex),X,Y,Z);    
        
        py::array_t<double> X_NP = py::array(py::dtype("d"), {X.size()}, {}, &X[0]); 
        py::array_t<double> Y_NP = py::array(py::dtype("d"), {Y.size()}, {}, &Y[0]); 
        py::array_t<double> Z_NP = py::array(py::dtype("d"), {Z.size()}, {}, &Z[0]); 
        
        return std::make_tuple(X_NP,Y_NP,Z_NP);
    }
};


PYBIND11_MODULE(egrid_bind, m) {
        
    py::class_<EGridTmp>(m, "EGridBind")
        .def(py::init<const std::string &>())
        .def("activeCells", &EGridTmp::activeCells)   
        .def("totalNumberOfCells", &EGridTmp::totalNumberOfCells)   
        .def("global_index", &EGridTmp::global_index)   
        .def("active_index", &EGridTmp::active_index)   
        .def("ijk_from_global_index", &EGridTmp::ijk_from_global_index)   
        .def("ijk_from_active_index", &EGridTmp::ijk_from_active_index)   
        .def("dimension", &EGridTmp::dimension)   
        .def("getCellCorners", &EGridTmp::getCellCornersTmp)   
        .def("getCellCornersNumpy", &EGridTmp::getCellCornersTmpNumpy);   
        
}


