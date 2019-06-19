#include <pybind11/pybind11.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <src/opm/io/eclipse/ESmry.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      

namespace py = pybind11;


PYBIND11_MODULE(esmry_bind, m) {

    py::class_<Opm::EclIO::ESmry>(m, "ESmryBind")
        .def(py::init<const std::string &, bool>())
        .def("hasKey", &Opm::EclIO::ESmry::hasKey)   
        .def("keywordList", &Opm::EclIO::ESmry::keywordList)   
        .def("get", &Opm::EclIO::ESmry::get)   
      //  .def("getStartdat", &Opm::EclIO::ESmry::getStartdat)   
        .def("numberOfVectors", &Opm::EclIO::ESmry::numberOfVectors);   

}


