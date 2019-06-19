
#include <pybind11/pybind11.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>
#include <src/opm/io/eclipse/ERft.cpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      

namespace py = pybind11;


PYBIND11_MODULE(erft_bind, m) {
    
    py::class_<std::vector<std::pair<std::string,std::tuple<int,int,int>>>>(m, "RftReportList")
        .def(py::init<>())
        .def("__len__", [](const std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v) { return v.size(); })
        .def("__getitem__", [](std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v, int item) { return v[item];})
        .def("__iter__", [](std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

    py::class_<Opm::EclIO::ERft>(m, "ERftBind")
        .def(py::init<const std::string &>())
        .def("listOfRftReports", &Opm::EclIO::ERft::listOfRftReports)   
        .def("listOfRftArrays", (std::vector<std::tuple<std::string, Opm::EclIO::eclArrType, int>> (Opm::EclIO::ERft::*)(const std::string&, int, int, int) const) &Opm::EclIO::ERft::listOfRftArrays)   

        .def("hasArray", &Opm::EclIO::ERft::hasArray)   
        .def("getRealRftArray", (const std::vector<float>& (Opm::EclIO::ERft::*)(const std::string&, const std::string&, int, int, int) const) &Opm::EclIO::ERft::getRft<float>)   
        .def("getDoubRftArray", (const std::vector<double>& (Opm::EclIO::ERft::*)(const std::string&, const std::string&, int, int, int) const) &Opm::EclIO::ERft::getRft<double>)   
        .def("getInteRftArray", (const std::vector<int>& (Opm::EclIO::ERft::*)(const std::string&, const std::string&, int, int, int) const) &Opm::EclIO::ERft::getRft<int>)   
        .def("getLogiRftArray", (const std::vector<bool>& (Opm::EclIO::ERft::*)(const std::string&, const std::string&, int, int, int) const) &Opm::EclIO::ERft::getRft<bool>)   
        .def("getCharRftArray", (const std::vector<std::string>& (Opm::EclIO::ERft::*)(const std::string&, const std::string&, int, int, int) const) &Opm::EclIO::ERft::getRft<std::string>);   

}


