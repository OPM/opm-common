#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

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
    using namespace Opm::Common::DocStrings;

    py::class_< SimulationConfig , std::shared_ptr<SimulationConfig>>( module, "SimulationConfig", SimulationConfig_docstring)
        .def("hasThresholdPressure", &SimulationConfig::useThresholdPressure, SimulationConfig_hasThresholdPressure_docstring )
        .def("useCPR",               &SimulationConfig::useCPR, SimulationConfig_useCPR_docstring )
        .def("useNONNC",             &SimulationConfig::useNONNC, SimulationConfig_useNONNC_docstring )
        .def("hasDISGAS",            &SimulationConfig::hasDISGAS, SimulationConfig_hasDISGAS_docstring )
        .def("hasDISGASW",           &SimulationConfig::hasDISGASW, SimulationConfig_hasDISGASW_docstring )
        .def("hasVAPOIL",            &SimulationConfig::hasVAPOIL, SimulationConfig_hasVAPOIL_docstring )
        .def("hasVAPWAT",            &SimulationConfig::hasVAPWAT, SimulationConfig_hasVAPWAT_docstring );
}
