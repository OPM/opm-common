
#include <pybind11/pybind11.h>


#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>
#include <src/opm/io/eclipse/ERst.cpp>

//#include <src/opm/io/eclipse/OutputStream.cpp>
//#include <src/opm/io/eclipse/EclOutput.cpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>



#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      

//#include <opm/common/ErrorMacros.hpp>
//#include <opm/common/OpmLog/OpmLog.hpp>
#include <src/opm/common/OpmLog/TimerLog.cpp>
#include <src/opm/common/OpmLog/CounterLog.cpp>
#include <src/opm/common/OpmLog/EclipsePRTLog.cpp>  

#include <boost/regex.hpp>


namespace py = pybind11;


PYBIND11_MODULE(erst_bind, m) {

    py::class_<Opm::EclIO::ERst>(m, "ERstBind")
        .def(py::init<const std::string &>())
        .def("hasReportStepNumber", &Opm::EclIO::ERst::hasReportStepNumber)   
        .def("loadReportStepNumber", &Opm::EclIO::ERst::loadReportStepNumber)   
        .def("listOfReportStepNumbers", &Opm::EclIO::ERst::listOfReportStepNumbers)   
        .def("listOfRstArrays", &Opm::EclIO::ERst::listOfRstArrays)   
        .def("getInteArray", (const std::vector<int>& (Opm::EclIO::ERst::*)(const std::string&, int)) &Opm::EclIO::ERst::getRst<int>)   
        .def("getLogiArray", (const std::vector<bool>& (Opm::EclIO::ERst::*)(const std::string&, int)) &Opm::EclIO::ERst::getRst<bool>)   
        .def("getDoubArray", (const std::vector<double>& (Opm::EclIO::ERst::*)(const std::string&, int)) &Opm::EclIO::ERst::getRst<double>)   
        .def("getRealArray", (const std::vector<float>& (Opm::EclIO::ERst::*)(const std::string&, int)) &Opm::EclIO::ERst::getRst<float>)   
        .def("getCharArray", (const std::vector<std::string>& (Opm::EclIO::ERst::*)(const std::string&, int)) &Opm::EclIO::ERst::getRst<std::string>);   
}


