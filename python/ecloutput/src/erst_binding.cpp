
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "pybind11/numpy.h"


#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>
#include <src/opm/io/eclipse/ERst.cpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      

#include <src/opm/common/OpmLog/TimerLog.cpp>
#include <src/opm/common/OpmLog/CounterLog.cpp>
#include <src/opm/common/OpmLog/EclipsePRTLog.cpp>  


namespace py = pybind11;

class ERstTmp : public Opm::EclIO::ERst {
    
public:
    ERstTmp(const std::string& filename): Opm::EclIO::ERst(filename) {};

    py::array_t<int> getRstIntegerNumpy(const std::string& name, int reportStepNumber) {
        std::vector<int> tmp=getRst<int>(name, reportStepNumber);
        return py::array(py::dtype("i"), {tmp.size()}, {}, &tmp[0]); 
    };

    py::array_t<float> getRstFloatNumpy(const std::string& name, int reportStepNumber) {
        std::vector<float> tmp=getRst<float>(name, reportStepNumber);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);  
    };

    py::array_t<double> getRstDoubleNumpy(const std::string& name, int reportStepNumber) {
        std::vector<double> tmp=getRst<double>(name, reportStepNumber);
        return py::array(py::dtype("d"), {tmp.size()}, {}, &tmp[0]);  
    };
    
};


PYBIND11_MODULE(erst_bind, m) {

    py::class_<ERstTmp>(m, "ERstBind")
        .def(py::init<const std::string &>())
        .def("hasReportStepNumber", &ERstTmp::hasReportStepNumber)   
        .def("loadReportStepNumber", &ERstTmp::loadReportStepNumber)   
        .def("listOfReportStepNumbers", &ERstTmp::listOfReportStepNumbers)   
        .def("listOfRstArrays", &ERstTmp::listOfRstArrays)  
        
        .def("getInteArrayNumpy", &ERstTmp::getRstIntegerNumpy)   
        .def("getRealArrayNumpy", &ERstTmp::getRstFloatNumpy)   
        .def("getDoubArrayNumpy", &ERstTmp::getRstDoubleNumpy) 
        
        .def("getInteArray", (const std::vector<int>& (ERstTmp::*)(const std::string&, int)) &ERstTmp::getRst<int>)   
        .def("getLogiArray", (const std::vector<bool>& (ERstTmp::*)(const std::string&, int)) &ERstTmp::getRst<bool>)   
        .def("getDoubArray", (const std::vector<double>& (ERstTmp::*)(const std::string&, int)) &ERstTmp::getRst<double>)   
        .def("getRealArray", (const std::vector<float>& (ERstTmp::*)(const std::string&, int)) &ERstTmp::getRst<float>)   
        .def("getCharArray", (const std::vector<std::string>& (ERstTmp::*)(const std::string&, int)) &ERstTmp::getRst<std::string>);   
}
