#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/ERst.cpp>

#include "export.hpp"
#include "converters.hpp"

#include <sstream>   
#include <iomanip>

namespace py = pybind11;


namespace {

py::array get_vector(Opm::EclIO::EclFile * file_ptr, std::size_t array_index) {
    auto array_type = std::get<1>(file_ptr->getList()[array_index]);
    if (array_type == Opm::EclIO::INTE)
        return convert::numpy_array( file_ptr->get<int>(array_index) );

    if (array_type == Opm::EclIO::REAL)
        return convert::numpy_array( file_ptr->get<float>(array_index) );

    if (array_type == Opm::EclIO::DOUB)
        return convert::numpy_array( file_ptr->get<double>(array_index) );

    if (array_type == Opm::EclIO::LOGI)
        return convert::numpy_array( file_ptr->get<bool>(array_index) );

    if (array_type == Opm::EclIO::CHAR)
        return convert::numpy_string_array( file_ptr->get<std::string>(array_index) );

    throw std::logic_error("Data type not supported");
}

bool erst_contains(Opm::EclIO::ERst * file_ptr, std::tuple<std::string, int> keyword) {

    bool hasKeyAtReport = file_ptr->count(std::get<0>(keyword), std::get<1>(keyword)) > 0 ? true : false;
    
    return hasKeyAtReport;
}

py::array get_erst_vector(Opm::EclIO::ERst * file_ptr, std::tuple<std::string, int, int> arg) {

    std::string key = std::get<0>(arg);
    int reportStepNumber = std::get<1>(arg);
    int occurrence = std::get<2>(arg);
    
    if (occurrence >= file_ptr->count(key, reportStepNumber))
        throw std::out_of_range("file have less than " + std::to_string(occurrence + 1) + " arrays in selected reportstep");
    
    size_t ind = 0;
    int nFound = -1;

    auto arrList = file_ptr->listOfRstArrays(reportStepNumber);

    while (nFound < occurrence){
         
        if (std::get<0>(arrList[ind]) == key)
            nFound++;

        ind++;
    }
    
    ind--;
    
    auto array_type = std::get<1>(arrList[ind]);
    
    if (array_type == Opm::EclIO::INTE)
        return convert::numpy_array( file_ptr->getRst<int>(key, reportStepNumber, occurrence) );

    if (array_type == Opm::EclIO::REAL)
        return convert::numpy_array( file_ptr->getRst<float>(key, reportStepNumber, occurrence) );

    if (array_type == Opm::EclIO::DOUB)
        return convert::numpy_array( file_ptr->getRst<double>(key, reportStepNumber, occurrence) );

    if (array_type == Opm::EclIO::LOGI)
        return convert::numpy_array( file_ptr->getRst<bool>(key, reportStepNumber, occurrence) );

    if (array_type == Opm::EclIO::CHAR)
        return convert::numpy_string_array( file_ptr->getRst<std::string>(key, reportStepNumber, occurrence) );

    throw std::logic_error("Data type not supported");
}

std::string info_erst(Opm::EclIO::ERst * file_ptr) {

    std::vector<int> seqnum_list = file_ptr->listOfReportStepNumbers();
    std::string info_string = "List of report steps\n\n";
    
    std::vector<std::string> monthStr = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    for (auto report : seqnum_list){
        std::stringstream seqnumBuffer;
        std::stringstream dateBuffer;

        auto inteh = file_ptr->getRst<int>("INTEHEAD", report, 0);
        
        seqnumBuffer << "seqnum = " << std::setw(4) << std::setfill(' ') << report << " ";
        dateBuffer << "Date: " << std::setw(2) << std::setfill('0') << inteh[64] << "-" << monthStr[inteh[65]-1] << "-" << std::to_string(inteh[66]); 
        
        info_string = info_string + seqnumBuffer.str() + dateBuffer.str() + "\n";
    }
    
    return info_string;
}

py::array get_erst_by_index(Opm::EclIO::ERst * file_ptr, size_t index, size_t reportStepNumber) {

    auto arrList = file_ptr->listOfRstArrays(reportStepNumber);

    if ((index >=arrList.size()) || (index < 0))
        throw std::out_of_range("Array index out of range. ");
        
    auto array_type = std::get<1>(arrList[index]);
    
    if (array_type == Opm::EclIO::INTE)
        return convert::numpy_array( file_ptr->getRst<int>(index, reportStepNumber) );

    if (array_type == Opm::EclIO::REAL)
        return convert::numpy_array( file_ptr->getRst<float>(index, reportStepNumber) );

    if (array_type == Opm::EclIO::DOUB)
        return convert::numpy_array( file_ptr->getRst<double>(index, reportStepNumber) );

    if (array_type == Opm::EclIO::LOGI)
        return convert::numpy_array( file_ptr->getRst<bool>(index, reportStepNumber) );

    if (array_type == Opm::EclIO::CHAR)
        return convert::numpy_string_array( file_ptr->getRst<std::string>(index, reportStepNumber) );

    throw std::logic_error("Data type not supported");
}
}



void python::common::export_IO(py::module& m) {

    py::enum_<Opm::EclIO::eclArrType>(m, "eclArrType", py::arithmetic())
        .value("INTE", Opm::EclIO::INTE)
        .value("REAL", Opm::EclIO::REAL)
        .value("DOUB", Opm::EclIO::DOUB)
        .value("CHAR", Opm::EclIO::CHAR)
        .value("LOGI", Opm::EclIO::LOGI)
        .value("MESS", Opm::EclIO::MESS)
        .export_values();

    py::class_<Opm::EclIO::EclFile>(m, "EclFile")
        .def(py::init<const std::string &, bool>(), py::arg("filename"), py::arg("preload") = false)
        .def_property_readonly("arrays", &Opm::EclIO::EclFile::getList)
        .def("getListOfArrays", &Opm::EclIO::EclFile::getList)
        .def("__contains__", &Opm::EclIO::EclFile::hasKey)
        .def("__len__", &Opm::EclIO::EclFile::size)
        .def("__getitem__", &get_vector)
        .def("get", &get_vector);

    py::class_<Opm::EclIO::ERst>(m, "ERst")
        .def(py::init<const std::string &>())
        .def("get_report_steps", &Opm::EclIO::ERst::listOfReportStepNumbers)   
        .def("count", &Opm::EclIO::ERst::count)   
        .def("has_report_step_number", &Opm::EclIO::ERst::hasReportStepNumber)   
        .def("get_list_of_arrays", &Opm::EclIO::ERst::listOfRstArrays)
        .def("load_report_step", &Opm::EclIO::ERst::loadReportStepNumber)
        .def("__len__", &Opm::EclIO::ERst::size)
        .def("__contains__", &erst_contains)   
        .def("__getitem__", &get_erst_vector)
        .def("__str__", &info_erst)
        .def("__repr__", &info_erst)
        .def("get", &get_erst_by_index);
}
