
#include <pybind11/pybind11.h>

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


PYBIND11_MODULE(egrid_bind, m) {

    py::class_<std::array<int, 3>>(m, "IntArray3")
        .def(py::init<>())
        .def("__len__", [](const std::array<int, 3> &v) { return v.size(); })
        .def("__getitem__", [](const std::array<int, 3> &v, int item) { return v[item]; })
        .def("__iter__", [](std::array<int, 3> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());
        
        
    py::class_<Opm::EclIO::EGrid>(m, "EGridBind")
        .def(py::init<const std::string &>())
        .def("activeCells", &Opm::EclIO::EGrid::activeCells)   
        .def("totalNumberOfCells", &Opm::EclIO::EGrid::totalNumberOfCells)   
        .def("global_index", &Opm::EclIO::EGrid::global_index)   
        .def("active_index", &Opm::EclIO::EGrid::active_index)   
        .def("ijk_from_global_index", &Opm::EclIO::EGrid::ijk_from_global_index)   
        .def("ijk_from_active_index", &Opm::EclIO::EGrid::ijk_from_active_index)   
        .def("dimension", &Opm::EclIO::EGrid::dimension);   
       
       // .def("getCellCorners", &Opm::EclIO::EGrid::getCellCornersForPython);   
  
        
}


