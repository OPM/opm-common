
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>


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


class EclModInitTmp : public EclModInit {
    
public:
    EclModInitTmp(const std::string& filename): EclModInit(filename) {};

    py::array_t<int> getIntegerParamNumpy(const std::string& name) {
        std::vector<int> tmp=getParam<int>(name);
        return py::array(py::dtype("i"), {tmp.size()}, {}, &tmp[0]); 
    };

    py::array_t<float> getFloatParamNumpy(const std::string& name) {
        std::vector<float> tmp=getParam<float>(name);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]); 
    };
    
};



PYBIND11_MODULE(eminit_bind, m) {

    py::class_<std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>>>(m, "EclEntry2")
        .def(py::init<>())
        .def("__len__", [](const std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v) { return v.size(); })
        .def("__getitem__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v, int item) { return v[item];})
        .def("__iter__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType>> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());
    
    py::class_<EclModInitTmp>(m, "EclModInitBind")
        .def(py::init<const std::string &>())
        .def("hasInitReportStep", &EclModInitTmp::hasInitReportStep)   
        .def("getListOfParameters", &EclModInitTmp::getListOfParameters)   
        .def("hasParameter", &EclModInitTmp::hasParameter)  
        
        .def("getRealParamNumpy", &EclModInitTmp::getFloatParamNumpy)   
        .def("getInteParamNumpy", &EclModInitTmp::getIntegerParamNumpy)   

        .def("getRealParam", (const std::vector<float>& (EclModInitTmp::*)(const std::string&)) &EclModInitTmp::getParam<float>)   
        .def("getInteParam", (const std::vector<int>& (EclModInitTmp::*)(const std::string&)) &EclModInitTmp::getParam<int>)   
        .def("getNumberOfActiveCells", &EclModInitTmp::getNumberOfActiveCells)   
        .def("resetFilter", &EclModInitTmp::resetFilter)   
        .def("gridDims", &EclModInitTmp::gridDims)   
        .def("setDepthfwl", &EclModInitTmp::setDepthfwl)   
        .def("addHCvolFilter", &EclModInitTmp::addHCvolFilter)   
        .def("addFilterRealParam", (void (EclModInitTmp::*)(std::string, std::string, float)) &EclModInitTmp::addFilter<float>)   
        .def("addFilterInteParam", (void (EclModInitTmp::*)(std::string, std::string, int)) &EclModInitTmp::addFilter<int>)   
        .def("addFilterRealParam2", (void (EclModInitTmp::*)(std::string, std::string, float, float)) &EclModInitTmp::addFilter<float>)   
        .def("addFilterInteParam2", (void (EclModInitTmp::*)(std::string, std::string, int, int)) &EclModInitTmp::addFilter<int>);   

}


