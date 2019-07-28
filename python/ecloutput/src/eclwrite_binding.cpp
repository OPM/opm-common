#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>


#include <src/opm/io/eclipse/EclOutput.cpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>


namespace py = pybind11;

class EclOutputNew : public Opm::EclIO::EclOutput {
    
public:
    EclOutputNew(const std::string& filename, const bool formatted): 
       Opm::EclIO::EclOutput(filename, formatted, std::ios::out) {}; 
};

class EclOutputApp : public Opm::EclIO::EclOutput {
    
public:
    EclOutputApp(const std::string& filename, const bool formatted): 
       Opm::EclIO::EclOutput(filename, formatted, std::ios_base::app) {}; 
};


PYBIND11_MODULE(eclwrite_bind, m) {


    py::class_<EclOutputNew>(m, "EclWriteNewBind")
        .def(py::init<const std::string &, const bool>())
        .def("writeInteger", (void (EclOutputNew::*)(const std::string&, const std::vector<int>&)) &EclOutputNew::write)
        .def("writeFloat", (void (EclOutputNew::*)(const std::string&, const std::vector<float>&)) &EclOutputNew::write)
        .def("writeDouble", (void (EclOutputNew::*)(const std::string&, const std::vector<double>&)) &EclOutputNew::write)
        .def("writeString", (void (EclOutputNew::*)(const std::string&, const std::vector<std::string>&)) &EclOutputNew::write)
        .def("writeBool", (void (EclOutputNew::*)(const std::string&, const std::vector<bool>&)) &EclOutputNew::write)
         .def("message", &EclOutputNew::message);   

    py::class_<EclOutputApp>(m, "EclWriteAppBind")
        .def(py::init<const std::string &, const bool>())
        .def("writeInteger", (void (EclOutputApp::*)(const std::string&, const std::vector<int>&)) &EclOutputApp::write)
        .def("writeFloat", (void (EclOutputApp::*)(const std::string&, const std::vector<float>&)) &EclOutputApp::write)
        .def("writeDouble", (void (EclOutputApp::*)(const std::string&, const std::vector<double>&)) &EclOutputApp::write)
        .def("writeString", (void (EclOutputApp::*)(const std::string&, const std::vector<std::string>&)) &EclOutputApp::write)
        .def("writeBool", (void (EclOutputApp::*)(const std::string&, const std::vector<bool>&)) &EclOutputApp::write)
         .def("message", &EclOutputApp::message);   
        
}

//    void message(const std::string& msg);


