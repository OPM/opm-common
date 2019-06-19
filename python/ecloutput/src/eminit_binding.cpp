
#include <pybind11/pybind11.h>


#include <src/opm/io/eclipse/EclFile.cpp>
#include <src/opm/io/eclipse/EGrid.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/common/utility/numeric/calculateCellVol.cpp> 

#include <examples/EclModInit.cpp> 

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      


namespace py = pybind11;


PYBIND11_MODULE(eminit_bind, m) {

    py::class_<std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>>>(m, "EclEntry2")
        .def(py::init<>())
        .def("__len__", [](const std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v) { return v.size(); })
        .def("__getitem__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v, int item) { return v[item];})
        .def("__iter__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());
    
    py::class_<EclModInit>(m, "EclModInitBind")
        .def(py::init<const std::string &>())
        .def("hasInitReportStep", &EclModInit::hasInitReportStep)   
        .def("getListOfParameters", &EclModInit::getListOfParameters)   
        .def("hasParameter", &EclModInit::hasParameter)   
        .def("getRealParam", (const std::vector<float>& (EclModInit::*)(const std::string&)) &EclModInit::getParam<float>)   
        .def("getInteParam", (const std::vector<int>& (EclModInit::*)(const std::string&)) &EclModInit::getParam<int>)   
        .def("getNumberOfActiveCells", &EclModInit::getNumberOfActiveCells)   
        .def("resetFilter", &EclModInit::resetFilter)   
        .def("gridDims", &EclModInit::gridDims)   
        .def("setDepthfwl", &EclModInit::setDepthfwl)   
        .def("addHCvolFilter", &EclModInit::addHCvolFilter)   
        .def("addFilterRealParam", (void (EclModInit::*)(std::string, std::string, float)) &EclModInit::addFilter<float>)   
        .def("addFilterInteParam", (void (EclModInit::*)(std::string, std::string, int)) &EclModInit::addFilter<int>)   
        .def("addFilterRealParam2", (void (EclModInit::*)(std::string, std::string, float, float)) &EclModInit::addFilter<float>)   
        .def("addFilterInteParam2", (void (EclModInit::*)(std::string, std::string, int, int)) &EclModInit::addFilter<int>);   


        

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}


