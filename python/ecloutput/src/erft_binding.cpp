
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

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

class ERftTmp : public Opm::EclIO::ERft {
    
public:
    ERftTmp(const std::string& filename): Opm::EclIO::ERft(filename) {};

    py::array_t<int> getRftIntegerNumpy(const std::string& name, const std::string& wellName, int year, int month, int day) {
        std::vector<int> tmp=getRft<int>(name, wellName, year, month, day);
        return py::array(py::dtype("i"), {tmp.size()}, {}, &tmp[0]);  
    };

    py::array_t<float> getRftFloatNumpy(const std::string& name, const std::string& wellName, int year, int month, int day) {
        std::vector<float> tmp=getRft<float>(name, wellName, year, month, day);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);  
    };

    py::array_t<double> getRftDoubleNumpy(const std::string& name, const std::string& wellName, int year, int month, int day) {
        std::vector<double> tmp=getRft<double>(name, wellName, year, month, day);
        return py::array(py::dtype("d"), {tmp.size()}, {}, &tmp[0]);  
    };
    
};



PYBIND11_MODULE(erft_bind, m) {
    
    py::class_<std::vector<std::pair<std::string,std::tuple<int,int,int>>>>(m, "RftReportList")
        .def(py::init<>())
        .def("__len__", [](const std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v) { return v.size(); })
        .def("__getitem__", [](std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v, int item) { return v[item];})
        .def("__iter__", [](std::vector<std::pair<std::string,std::tuple<int,int,int>>> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

    py::class_<ERftTmp>(m, "ERftBind")
        .def(py::init<const std::string &>())
        .def("listOfRftReports", &ERftTmp::listOfRftReports)   
        .def("listOfRftArrays", (std::vector<std::tuple<std::string, Opm::EclIO::eclArrType, int>> (ERftTmp::*)(const std::string&, int, int, int) const) &ERftTmp::listOfRftArrays)   

        .def("hasArray", &ERftTmp::hasArray)   

        .def("getInteRftArrayNumpy", &ERftTmp::getRftIntegerNumpy)   
        .def("getRealRftArrayNumpy", &ERftTmp::getRftFloatNumpy)   
        .def("getDoubRftArrayNumpy", &ERftTmp::getRftDoubleNumpy)   

        .def("getRealRftArray", (const std::vector<float>& (ERftTmp::*)(const std::string&, const std::string&, int, int, int) const) &ERftTmp::getRft<float>)   
        .def("getDoubRftArray", (const std::vector<double>& (ERftTmp::*)(const std::string&, const std::string&, int, int, int) const) &ERftTmp::getRft<double>)   
        .def("getInteRftArray", (const std::vector<int>& (ERftTmp::*)(const std::string&, const std::string&, int, int, int) const) &ERftTmp::getRft<int>)   
        .def("getLogiRftArray", (const std::vector<bool>& (ERftTmp::*)(const std::string&, const std::string&, int, int, int) const) &ERftTmp::getRft<bool>)   
        .def("getCharRftArray", (const std::vector<std::string>& (ERftTmp::*)(const std::string&, const std::string&, int, int, int) const) &ERftTmp::getRft<std::string>);   

}


