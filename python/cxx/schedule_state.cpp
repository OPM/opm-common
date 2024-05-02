#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include "export.hpp"

namespace {
        const Group& get_group(const ScheduleState& st, const std::string& group_name) {
        return st.groups.get(group_name);
    }
}

/**
 * @brief Function to export the ScheduleState class and some methods to Python.
 * 
 * In the below class we use std::shared_ptr as the holder type, see https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
 * this makes it possible to share the ScheduleState (which is created only once per simulation) with e.g. the opm.simulators.BlackOilSimulator Python object.
 * 
 * @param module Reference to the python module.
 */
void python::common::export_ScheduleState(py::module& module) {

    py::class_<ScheduleState>(module, "ScheduleState")
        .def_property_readonly("nupcol", py::overload_cast<>(&ScheduleState::nupcol, py::const_))
        .def("group", &get_group, ref_internal, py::arg("group_name"))
        ;
}
