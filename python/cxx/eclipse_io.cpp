#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclIOdata.hpp>

#include "export.hpp"
#include "converters.hpp"

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
}
