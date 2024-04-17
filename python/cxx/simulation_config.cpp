#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>

#include "export.hpp"

/**
 * @brief Function to export the SimulationConfig class and some methods to Python.
 * 
 * In the below class we use std::shared_ptr as the holder type, see https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
 * this makes it possible to share the SimulationConfig (which is created only once per simulation) with e.g. the opm.simulators.BlackOilSimulator Python object.
 * 
 * @param module Reference to the python module.
 */
void python::common::export_SimulationConfig(py::module& module)
{
    py::class_< SimulationConfig , std::shared_ptr<SimulationConfig>>( module, "SimulationConfig")
        .def("hasThresholdPressure", &SimulationConfig::useThresholdPressure )
        .def("useCPR",               &SimulationConfig::useCPR )
        .def("useNONNC",             &SimulationConfig::useNONNC )
        .def("hasDISGAS",            &SimulationConfig::hasDISGAS )
        .def("hasDISGASW",            &SimulationConfig::hasDISGASW )
        .def("hasVAPOIL",            &SimulationConfig::hasVAPOIL )
        .def("hasVAPWAT",            &SimulationConfig::hasVAPWAT );
}
