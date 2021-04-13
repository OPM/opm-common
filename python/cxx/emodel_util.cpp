#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <opm/utility/EModel.hpp>

#include "export.hpp"
#include "converters.hpp"

#include <opm/common/utility/numeric/calculateCellVol.hpp>

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

    m.def("calc_cell_vol",calculateCellVol);

    py::class_<EModel>(m, "EModel")
        .def(py::init<const std::string &>())
        .def("__contains__", &EModel::hasParameter)
        .def("grid_dims", &EModel::gridDims)
        .def("active_cells", &EModel::getNumberOfActiveCells)
        .def("set_depth_fwl", &EModel::setDepthfwl)
        .def("add_hc_filter", &EModel::addHCvolFilter)
        .def("get_list_of_arrays", &EModel::getListOfParameters)
        .def("active_report_step", &EModel::getActiveReportStep)
        .def("get_report_steps", &EModel::getListOfReportSteps)
        .def("has_report_step", &EModel::hasReportStep)
        .def("set_report_step", &EModel::setReportStep)
        .def("reset_filter", &EModel::resetFilter)
        .def("get", &get_param)
        .def("__add_filter", &add_int_filter_1value)
        .def("__add_filter", &add_float_filter_1value)
        .def("__add_filter", &add_int_filter_2values)
        .def("__add_filter", &add_float_filter_2values);

}
