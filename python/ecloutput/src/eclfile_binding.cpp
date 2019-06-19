#include <pybind11/pybind11.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      


//#include <opm/common/ErrorMacros.hpp>
//#include <opm/common/OpmLog/OpmLog.hpp>
//#include <src/opm/common/OpmLog/TimerLog.cpp>
//#include <src/opm/common/OpmLog/CounterLog.cpp>
//#include <src/opm/common/OpmLog/EclipsePRTLog.cpp>  


namespace py = pybind11;


PYBIND11_MODULE(eclfile_bind, m) {

    py::class_<std::vector<int>>(m, "IntVector")
        .def(py::init<>())
        .def(py::init<int, int>())
        .def("clear", &std::vector<int>::clear)
        .def("pop_back", &std::vector<int>::pop_back)
        .def("push_back", [](std::vector<int> &v, int val) { v.push_back(val); })
        .def("__len__", [](const std::vector<int> &v) { return v.size(); })
        .def("__getitem__", [](const std::vector<int> &v, int item) { return v[item]; })
        .def("__iter__", [](std::vector<int> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

    py::class_<std::vector<std::string>>(m, "StringVector")
        .def(py::init<>())
        .def(py::init<int, std::string>())
        .def("clear", &std::vector<std::string>::clear)
        .def("pop_back", &std::vector<std::string>::pop_back)
        .def("push_back", [](std::vector<std::string> &v, std::string val) { v.push_back(val); })
        .def("__len__", [](const std::vector<std::string> &v) { return v.size(); })
        .def("__getitem__", [](const std::vector<std::string> &v, int item) { return v[item]; })
        .def("__iter__", [](std::vector<std::string> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

    py::class_<std::vector<float>>(m, "FloatVector")
        .def(py::init<>())
        .def(py::init<int, float>())
        .def("clear", &std::vector<float>::clear)
        .def("pop_back", &std::vector<float>::pop_back)
        .def("push_back", [](std::vector<float> &v, float val) { v.push_back(val); })
        .def("__len__", [](const std::vector<float> &v) { return v.size(); })
        .def("__getitem__", [](const std::vector<float> &v, int item) { return v[item]; })
        .def("__iter__", [](std::vector<float> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());


    py::class_<std::vector<double>>(m, "DobleVector")
        .def(py::init<>())
        .def(py::init<int, double>())
        .def("clear", &std::vector<double>::clear)
        .def("pop_back", &std::vector<double>::pop_back)
        .def("push_back", [](std::vector<double> &v, double val) { v.push_back(val); })
        .def("__len__", [](const std::vector<double> &v) { return v.size(); })
        .def("__getitem__", [](const std::vector<double> &v, int item) { return v[item]; })
        .def("__iter__", [](std::vector<double> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

    py::class_<std::vector<bool>>(m, "BoolVector")
        .def(py::init<>())
        .def(py::init<int, bool>())
        .def("clear", &std::vector<bool>::clear)
        .def("pop_back", &std::vector<bool>::pop_back)
        .def("push_back", [](std::vector<bool> &v, bool val) { v.push_back(val); })
        .def("__len__", [](const std::vector<bool> &v) { return v.size(); })
        .def("__getitem__", [](const std::vector<bool> &v, int item) { return v[item]; })
        .def("__iter__", [](std::vector<bool> &v) {
            return py::make_iterator(v.begin(), v.end());
    }, py::keep_alive<0, 1>());

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
        
    py::class_<Opm::EclIO::EclFile>(m, "EclFileBind")
        .def(py::init<const std::string &>())
	
        .def("getList", &Opm::EclIO::EclFile::getList)   
        .def("hasKey", &Opm::EclIO::EclFile::hasKey)   

        .def("loadAllData", (void (Opm::EclIO::EclFile::*)(void)) &Opm::EclIO::EclFile::loadData)   
        .def("loadDataByIndex", (void (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::loadData)   
        
        .def("getRealFromIndex", (const std::vector<float>& (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::get<float>)   
        .def("getDoubFromIndex", (const std::vector<double>& (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::get<double>)   
        .def("getInteFromIndex", (const std::vector<int>& (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::get<int>)   
        .def("getLogiFromIndex", (const std::vector<bool>& (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::get<bool>)   
        .def("getCharFromIndex", (const std::vector<std::string>& (Opm::EclIO::EclFile::*)(int)) &Opm::EclIO::EclFile::get<std::string>);   

  
    
}


