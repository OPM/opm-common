#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

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

class ESmryTmp : public Opm::EclIO::ESmry {
    
public:
    ESmryTmp(const std::string& filename, bool loadBaseRunData): Opm::EclIO::ESmry(filename, loadBaseRunData) {};

    py::array_t<float> get_numpy(const std::string& name) {
        std::vector<float> tmp=get(name);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    };

    py::array_t<float> get_at_rstep_numpy(const std::string& name) {
        std::vector<float> tmp=get_at_rstep(name);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    };
    
};


PYBIND11_MODULE(esmry_bind, m) {

    py::class_<ESmryTmp>(m, "ESmryBind")
        .def(py::init<const std::string &, bool>())
        .def("hasKey", &ESmryTmp::hasKey)   
        .def("keywordList", &ESmryTmp::keywordList)   
        .def("get", &ESmryTmp::get)   
        .def("getNumpy", &ESmryTmp::get_numpy)   
        .def("getAtRstep", &ESmryTmp::get_at_rstep)   
        .def("getAtRstepNumpy", &ESmryTmp::get_at_rstep_numpy)   
        .def("numberOfVectors", &ESmryTmp::numberOfVectors);   
    
}


