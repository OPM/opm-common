#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclIOdata.hpp>

#include "export.hpp"
#include "converters.hpp"

namespace py = pybind11;


namespace {

using npArray = std::tuple<py::array, Opm::EclIO::eclArrType>;
using EclEntry = std::tuple<std::string, Opm::EclIO::eclArrType, long int>;

npArray get_vector_index(Opm::EclIO::EclFile * file_ptr, std::size_t array_index)
{
    auto array_type = std::get<1>(file_ptr->getList()[array_index]);

    if (array_type == Opm::EclIO::INTE)
        return std::make_tuple (convert::numpy_array( file_ptr->get<int>(array_index)), array_type);

    if (array_type == Opm::EclIO::REAL)
        return std::make_tuple (convert::numpy_array( file_ptr->get<float>(array_index)), array_type);

    if (array_type == Opm::EclIO::DOUB)
        return std::make_tuple (convert::numpy_array( file_ptr->get<double>(array_index)), array_type);

    if (array_type == Opm::EclIO::LOGI)
        return std::make_tuple (convert::numpy_array( file_ptr->get<bool>(array_index)), array_type);

    if (array_type == Opm::EclIO::CHAR)
        return std::make_tuple (convert::numpy_string_array( file_ptr->get<std::string>(array_index)), array_type);

    throw std::logic_error("Data type not supported");
}

size_t get_array_index(const std::vector<EclEntry>& array_list, const std::string& array_name, size_t occurence)
{
    size_t cidx = 0;

    auto it = std::find_if(array_list.begin(), array_list.end(),
                           [&cidx, &array_name, occurence](const EclEntry& entry)
                           {
                              if (std::get<0>(entry) == array_name)
                                  ++cidx;

                              return cidx == occurence + 1;
                           });

    return std::distance(array_list.begin(), it);
}

npArray get_vector_name(Opm::EclIO::EclFile * file_ptr, const std::string& array_name)
{
    if (file_ptr->hasKey(array_name) == false)
        throw std::logic_error("Array " + array_name + " not found in EclFile");

    auto array_list = file_ptr->getList();
    size_t array_index = get_array_index(array_list, array_name, 0);

    return get_vector_index(file_ptr, array_index);
}

npArray get_vector_occurrence(Opm::EclIO::EclFile * file_ptr, const std::string& array_name, size_t occurrence)
{
    if (occurrence >= file_ptr->count(array_name) )
        throw std::logic_error("Occurrence " + std::to_string(occurrence) + " not found in EclFile");

    auto array_list = file_ptr->getList();
    size_t array_index = get_array_index(array_list, array_name, occurrence);

    return get_vector_index(file_ptr, array_index);
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
        .def("__get_list_of_arrays", &Opm::EclIO::EclFile::getList)
        .def("__contains__", &Opm::EclIO::EclFile::hasKey)
        .def("__len__", &Opm::EclIO::EclFile::size)
        .def("count", &Opm::EclIO::EclFile::count)
        .def("__get_data", &get_vector_index)
        .def("__get_data", &get_vector_name)
        .def("__get_data", &get_vector_occurrence);
}
