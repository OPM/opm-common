#include <vector>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

void python::common::export_EclipseConfig(py::module& module)
{
    using namespace Opm::Common::DocStrings;

    py::class_< EclipseConfig >( module, "EclipseConfig" , EclipseConfig_docstring)
        .def( "init",            py::overload_cast<>(&EclipseConfig::init, py::const_), EclipseConfig_init_docstring);

    // Note: In the below class we std::shared_ptr as the holder type, see:
    //
    //  https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
    //
    // this makes it possible to share the returned object with e.g. and
    //   opm.simulators.BlackOilSimulator Python object
    //
    py::class_< SummaryConfig, std::shared_ptr<SummaryConfig> >( module, "SummaryConfig", SummaryConfig_docstring )
        .def(py::init([](const Deck& deck, const EclipseState& state, const Schedule& schedule) {
            return SummaryConfig( deck, schedule, state.fieldProps(), state.aquifer() );
        }), SummaryConfig_init_docstring )
        .def( "__contains__", &SummaryConfig::hasKeyword, SummaryConfig_contains_docstring );

    py::class_< InitConfig >( module, "InitConfig", InitConfig_docstring )
        .def( "hasEquil", &InitConfig::hasEquil, InitConfig_hasEquil_docstring )
        .def( "restartRequested", &InitConfig::restartRequested, InitConfig_restartRequested_docstring )
        .def( "getRestartStep", &InitConfig::getRestartStep, InitConfig_getRestartStep_docstring );

    py::class_< IOConfig >( module, "IOConfig", IOConfig_docstring );

}

