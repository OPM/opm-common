/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <chrono>

#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {


std::vector<std::string> groups(const SummaryState * st) {
    return st->groups();
}

std::vector<std::string> wells(const SummaryState * st) {
    return st->wells();
}


}


void python::common::export_SummaryState(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_<SummaryState, std::shared_ptr<SummaryState>>(module, "SummaryState", SummaryStateClass_docstring)
        .def(py::init<std::time_t>())
        .def("update", &SummaryState::update)
        .def("update_well_var", &SummaryState::update_well_var, py::arg("well_name"), py::arg("variable_name"), py::arg("new_value"), SummaryState_update_well_var_docstring)
        .def("update_group_var", &SummaryState::update_group_var, py::arg("group_name"), py::arg("variable_name"), py::arg("new_value"), SummaryState_update_group_var_docstring)
        .def("well_var", py::overload_cast<const std::string&, const std::string&>(&SummaryState::get_well_var, py::const_), py::arg("well_name"), py::arg("variable_name"), SummaryState_well_var_docstring)
        .def("group_var", py::overload_cast<const std::string&, const std::string&>(&SummaryState::get_group_var, py::const_), py::arg("group_name"), py::arg("variable_name"), SummaryState_group_var_docstring)
        .def("elapsed", &SummaryState::get_elapsed, SummaryState_elapsed_docstring)
        .def_property_readonly("groups", groups, SummaryState_groups_docstring)
        .def_property_readonly("wells", wells, SummaryState_wells_docstring)
        .def("__contains__", &SummaryState::has)
        .def("has_well_var", py::overload_cast<const std::string&, const std::string&>(&SummaryState::has_well_var, py::const_), py::arg("well_name"), py::arg("variable_name"), SummaryState_has_well_var_docstring)
        .def("has_group_var", py::overload_cast<const std::string&, const std::string&>(&SummaryState::has_group_var, py::const_), py::arg("group_name"), py::arg("variable_name"), SummaryState_has_group_var_docstring)
        .def("__setitem__", &SummaryState::set)
        .def("__getitem__", py::overload_cast<const std::string&>(&SummaryState::get, py::const_))
        ;
}
