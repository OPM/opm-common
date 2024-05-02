#include <pybind11/pybind11.h>
#include "export.hpp"

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

void python::common::export_all(py::module& module) {

    // Please do not change the order of the export statements below, since
    // the classes exported earlier are used in the classes exported later.
    // If you change the order, compilation works, but the function signature is
    // not correct in the .pyi file created from the Python module opmcommon_python.

    export_Log(module);
    export_IO(module);

    export_ParseContext(module);
    export_ParserKeywords(module);
    export_DeckKeyword(module);
    export_Deck(module);
    export_Parser(module);

    export_UnitSystem(module);
    export_Connection(module);
    export_Well(module);
    export_Group(module);
    export_EclipseConfig(module);
    export_SimulationConfig(module);
    export_FieldProperties(module);
    export_TableManager(module);
    export_EclipseGrid(module);
    export_EModel(module);

    export_ScheduleState(module);
    export_Schedule(module);
    export_EclipseState(module);
    export_SummaryState(module);
}


PYBIND11_MODULE(opmcommon_python, module) {
    python::common::export_all(module);

    pybind11::module submodule = module.def_submodule("embedded");

    // These attributes are placeholders, they get set to the actual EclipseState, Schedule and SummaryState in PyRunModule.cpp
    // If you change anything here, please recreate and update the python stub file for opm_embedded as described in python/README.md
    submodule.attr("current_ecl_state") = std::make_shared<EclipseState>();
    submodule.attr("current_summary_state") = std::make_shared<SummaryState>();
    submodule.attr("current_schedule") = std::make_shared<Schedule>();
    submodule.attr("current_report_step") = 0;
}
