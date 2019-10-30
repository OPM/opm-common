#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>


namespace py = pybind11;

class EclFileTmp : public Opm::EclIO::EclFile {
    
public:
    EclFileTmp(const std::string& filename): Opm::EclIO::EclFile(filename) {};

    py::array_t<float> getFloatNumpy(int arrIndex) {
        std::vector<float> tmp=get<float>(arrIndex);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    };

    py::array_t<double> getDoubleNumpy(int arrIndex) {
        std::vector<double> tmp=get<double>(arrIndex);
        return py::array(py::dtype("d"), {tmp.size()}, {}, &tmp[0]);
    };

    py::array_t<int> getIntegerNumpy(int arrIndex) {
        std::vector<int> tmp=get<int>(arrIndex);
        return py::array(py::dtype("i"), {tmp.size()}, {}, &tmp[0]); 
    };
};


PYBIND11_MODULE(libopmioecl_python, m) {

    py::enum_<Opm::EclIO::eclArrType>(m, "eclArrType", py::arithmetic())
        .value("INTE", Opm::EclIO::INTE)
        .value("REAL", Opm::EclIO::REAL)
        .value("DOUB", Opm::EclIO::DOUB)
        .value("CHAR", Opm::EclIO::CHAR)
        .value("LOGI", Opm::EclIO::LOGI)
        .value("MESS", Opm::EclIO::MESS)
        .export_values();

    py::class_<std::vector<std::tuple<std::string,Opm::EclIO::eclArrType,int>>>(m, "EclEntry")
        .def(py::init<>())
        .def("__len__", [](const std::vector<std::tuple<std::string,Opm::EclIO::eclArrType,int>> &v) { return v.size(); })
        .def("__getitem__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType,int>> &v, int item) { return v[item];})
        .def("__iter__", [](std::vector<std::tuple<std::string,Opm::EclIO::eclArrType,int>> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

        
    py::class_<EclFileTmp>(m, "EclFileBind")
        .def(py::init<const std::string &>())
        .def("getList", &EclFileTmp::getList)   
        .def("hasKey", &EclFileTmp::hasKey)   

        .def("loadAllData", (void (EclFileTmp::*)(void)) &EclFileTmp::loadData)   
        .def("loadDataByIndex", (void (EclFileTmp::*)(int)) &EclFileTmp::loadData)   

        .def("getRealFromIndexNumpy", &EclFileTmp::getFloatNumpy)   
        .def("getDoubFromIndexNumpy", &EclFileTmp::getDoubleNumpy)   
        .def("getInteFromIndexNumpy", &EclFileTmp::getIntegerNumpy)   
        .def("getLogiFromIndex", (const std::vector<bool>& (EclFileTmp::*)(int)) &EclFileTmp::get<bool>)   
        .def("getCharFromIndex", (const std::vector<std::string>& (EclFileTmp::*)(int)) &EclFileTmp::get<std::string>);   
}
