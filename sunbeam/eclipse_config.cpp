#include <vector>  // workaround for missing import in opm-parser/Equil.hpp
#include <opm/parser/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>

#include "sunbeam.hpp"


void sunbeam::export_EclipseConfig()
{

    py::class_< EclipseConfig >( "EclipseConfig", py::no_init )
        .def( "init",            &EclipseConfig::init,       ref())
        .def( "restart",         &EclipseConfig::restart,    ref())
        ;

    py::class_< SummaryConfig >( "SummaryConfig", py::no_init )
        .def( "__contains__",    &SummaryConfig::hasKeyword )
        ;

    py::class_< InitConfig >( "InitConfig", py::no_init )
        .def( "hasEquil",           &InitConfig::hasEquil )
        .def( "restartRequested",   &InitConfig::restartRequested )
        .def( "getRestartStep"  ,   &InitConfig::getRestartStep )
        ;

    py::class_< RestartConfig >( "RestartConfig", py::no_init )
        .def( "getKeyword",          &RestartConfig::getKeyword )
        .def( "getFirstRestartStep", &RestartConfig::getFirstRestartStep )
        .def( "getWriteRestartFile", &RestartConfig::getWriteRestartFile )
        ;

    py::class_< SimulationConfig >( "SimulationConfig", py::no_init )
        .def("hasThresholdPressure", &SimulationConfig::hasThresholdPressure )
        .def("useCPR",               &SimulationConfig::useCPR )
        .def("hasDISGAS",            &SimulationConfig::hasDISGAS )
        .def("hasVAPOIL",            &SimulationConfig::hasVAPOIL )
        ;
}
