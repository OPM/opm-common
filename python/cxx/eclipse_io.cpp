#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/ERst.cpp>
#include <src/opm/io/eclipse/ESmry.cpp>

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

bool erst_contains(Opm::EclIO::ERst * file_ptr, std::tuple<std::string, int> keyword)
{
    bool hasKeyAtReport = file_ptr->count(std::get<0>(keyword), std::get<1>(keyword)) > 0 ? true : false;
    return hasKeyAtReport;
}

npArray get_erst_by_index(Opm::EclIO::ERst * file_ptr, size_t index, size_t rstep)
{
    auto arrList = file_ptr->listOfRstArrays(rstep);

    if (index >=arrList.size())
        throw std::out_of_range("Array index out of range. ");

    auto array_type = std::get<1>(arrList[index]);

    if (array_type == Opm::EclIO::INTE)
        return std::make_tuple (convert::numpy_array( file_ptr->getRst<int>(index, rstep)), array_type);

    if (array_type == Opm::EclIO::REAL)
        return std::make_tuple (convert::numpy_array( file_ptr->getRst<float>(index, rstep)), array_type);

    if (array_type == Opm::EclIO::DOUB)
        return std::make_tuple (convert::numpy_array( file_ptr->getRst<double>(index, rstep)), array_type);

    if (array_type == Opm::EclIO::LOGI)
        return std::make_tuple (convert::numpy_array( file_ptr->getRst<bool>(index, rstep)), array_type);

    if (array_type == Opm::EclIO::CHAR)
        return std::make_tuple (convert::numpy_string_array( file_ptr->getRst<std::string>(index, rstep)), array_type);

    throw std::logic_error("Data type not supported");
}


npArray get_erst_vector(Opm::EclIO::ERst * file_ptr, const std::string& key, size_t rstep, size_t occurrence)
{
    if (occurrence >= static_cast<size_t>(file_ptr->count(key, rstep)))
        throw std::out_of_range("file have less than " + std::to_string(occurrence + 1) + " arrays in selected report step");

    auto array_list = file_ptr->listOfRstArrays(rstep);

    size_t array_index = get_array_index(array_list, key, 0);

    return get_erst_by_index(file_ptr, array_index, rstep);
}


py::array get_smry_vector(Opm::EclIO::ESmry * file_ptr, const std::string& key)
{
    return convert::numpy_array( file_ptr->get(key) );
}


py::array get_smry_vector_at_rsteps(Opm::EclIO::ESmry * file_ptr, const std::string& key)
{
    return convert::numpy_array( file_ptr->get_at_rstep(key) );
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

    py::class_<Opm::EclIO::ERst>(m, "ERst")
        .def(py::init<const std::string &>())
        .def("__has_report_step", &Opm::EclIO::ERst::hasReportStepNumber)
        .def("load_report_step", &Opm::EclIO::ERst::loadReportStepNumber)
        .def_property_readonly("report_steps", &Opm::EclIO::ERst::listOfReportStepNumbers)
        .def("__len__", &Opm::EclIO::ERst::numberOfReportSteps)
        .def("count", &Opm::EclIO::ERst::count)
        .def("__contains", &erst_contains)
        .def("__get_list_of_arrays", &Opm::EclIO::ERst::listOfRstArrays)
        .def("__get_data", &get_erst_by_index)
        .def("__get_data", &get_erst_vector);

   py::class_<Opm::EclIO::ESmry>(m, "ESmry")
        .def(py::init<const std::string &, const bool>(), py::arg("filename"), py::arg("load_base_run") = false)
        .def("__contains__", &Opm::EclIO::ESmry::hasKey)
        .def("__len__", &Opm::EclIO::ESmry::numberOfTimeSteps)
        .def("__get_all", &get_smry_vector)
        .def("__get_at_rstep", &get_smry_vector_at_rsteps)
        .def("__get_startdat", &Opm::EclIO::ESmry::get_startdat)
        .def_property_readonly("list_of_keys", &Opm::EclIO::ESmry::keywordList);
}
