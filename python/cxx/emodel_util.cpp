#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <opm/utility/EModel.hpp>

#include "export.hpp"
#include "converters.hpp"

#include <opm/common/utility/numeric/calculateCellVol.hpp>

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace py = pybind11;

namespace {


using EclEntry = std::tuple<std::string, Opm::EclIO::eclArrType, int>;


Opm::EclIO::eclArrType getArrayType(EModel * file_ptr, std::string key){

    std::vector<std::string> inteVect = {"I", "J", "K", "ROW", "COLUMN", "LAYER"};

    if(std::find(inteVect.begin(), inteVect.end(), key) != inteVect.end()) {
        return Opm::EclIO::INTE;
    }

    auto arrayList = file_ptr->getListOfParameters();

    size_t n = 0;

    while (n < arrayList.size() ){

        if (std::get<0>(arrayList[n]) == key){
            return std::get<1>(arrayList[n]);
        }

        n++;
    }

    throw std::logic_error("Array not found in EModel");
}


py::array get_param(EModel * file_ptr, std::string key)
{
    Opm::EclIO::eclArrType arrType = getArrayType(file_ptr, key);

    if (arrType == Opm::EclIO::REAL){
        std::vector<float> vect = file_ptr->getParam<float>(key);
        return py::array(py::dtype("f"), {vect.size()}, {}, &vect[0]);
    } else if (arrType == Opm::EclIO::INTE){
        std::vector<int> vect = file_ptr->getParam<int>(key);
        return py::array(py::dtype("i"), {vect.size()}, {}, &vect[0]);
    } else
        throw std::logic_error("Data type not supported");
}


void add_int_filter_1value(EModel * file_ptr, std::string key, std::string opr, int value)
{
    file_ptr->addFilter<int>(key, opr, value);
}

void add_float_filter_1value(EModel * file_ptr, std::string key, std::string opr, float value)
{
    file_ptr->addFilter<float>(key, opr, value);
}

void add_int_filter_2values(EModel * file_ptr, std::string key, std::string opr, int value1, int value2)
{
    file_ptr->addFilter<int>(key, opr, value1, value2);
}

void add_float_filter_2values(EModel * file_ptr, std::string key, std::string opr, float value1, float value2)
{
    file_ptr->addFilter<float>(key, opr, value1, value2);
}

} // name space


void python::common::export_EModel(py::module& m) {

    using namespace Opm::Common::DocStrings;

    m.def("calc_cell_vol",calculateCellVol);

    py::class_<EModel>(m, "EModel", EModel_docstring)
        .def(py::init<const std::string &>(), py::arg("filename"), EModel_init_docstring)
        .def("__contains__", &EModel::hasParameter, py::arg("parameter"), EModel_contains_docstring)
        .def("grid_dims", &EModel::gridDims, EModel_grid_dims_docstring)
        .def("active_cells", &EModel::getNumberOfActiveCells, EModel_active_cells_docstring)
        .def("set_depth_fwl", &EModel::setDepthfwl, py::arg("fwl"), EModel_set_depth_fwl_docstring)
        .def("add_hc_filter", &EModel::addHCvolFilter, EModel_add_hc_filter_docstring)
        .def("get_list_of_arrays", &EModel::getListOfParameters, EModel_get_list_of_arrays_docstring)
        .def("active_report_step", &EModel::getActiveReportStep, EModel_active_report_step_docstring)
        .def("get_report_steps", &EModel::getListOfReportSteps, EModel_get_report_steps_docstring)
        .def("has_report_step", &EModel::hasReportStep, py::arg("rstep"), EModel_has_report_step_docstring)
        .def("set_report_step", &EModel::setReportStep, py::arg("rstep"), EModel_set_report_step_docstring)
        .def("reset_filter", &EModel::resetFilter, EModel_reset_filter_docstring)
        .def("get", &get_param, py::arg("key"), EModel_get_docstring)
        .def("__add_filter", &add_int_filter_1value, py::arg("key"), py::arg("operator"), py::arg("value"), EModel_add_filter_int1_docstring)
        .def("__add_filter", &add_float_filter_1value, py::arg("key"), py::arg("operator"), py::arg("value"), EModel_add_filter_float1_docstring)
        .def("__add_filter", &add_int_filter_2values, py::arg("key"), py::arg("operator"), py::arg("value1"), py::arg("value2"), EModel_add_filter_int2_docstring)
        .def("__add_filter", &add_float_filter_2values, py::arg("key"), py::arg("operator"), py::arg("value1"), py::arg("value2"), EModel_add_filter_float2_docstring);

    m.attr("EModel_add_filter_docstring") = EModel_add_filter_docstring;

}
