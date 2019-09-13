#include <vector>  // workaround for missing import in opm-parser/Equil.hpp
#include <opm/parser/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>

#include "common.hpp"


void python::common::export_EclipseConfig(py::module& module)
{
    py::class_< EclipseConfig >( module, "EclipseConfig" )
      .def( "init",            &EclipseConfig::init, ref_internal)
      .def( "restart",         &EclipseConfig::restart, ref_internal);

    py::class_< SummaryConfig >( module, "SummaryConfig")
        .def( "__contains__",    &SummaryConfig::hasKeyword );

    py::class_< InitConfig >( module, "InitConfig")
        .def( "hasEquil",           &InitConfig::hasEquil )
        .def( "restartRequested",   &InitConfig::restartRequested )
        .def( "getRestartStep"  ,   &InitConfig::getRestartStep );

    py::class_< RestartConfig >( module, "RestartConfig")
        .def( "getKeyword",          &RestartConfig::getKeyword )
        .def( "getFirstRestartStep", &RestartConfig::getFirstRestartStep )
        .def( "getWriteRestartFile", &RestartConfig::getWriteRestartFile, py::arg("reportStep"), py::arg("log") = true);

    py::class_< SimulationConfig >( module, "SimulationConfig")
        .def("hasThresholdPressure", &SimulationConfig::useThresholdPressure )
        .def("useCPR",               &SimulationConfig::useCPR )
        .def("hasDISGAS",            &SimulationConfig::hasDISGAS )
        .def("hasVAPOIL",            &SimulationConfig::hasVAPOIL );
}
