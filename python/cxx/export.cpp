#include <pybind11/pybind11.h>
#include "export.hpp"

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

void python::common::export_all(py::module& module) {
    export_ParseContext(module);
    export_Parser(module);
    export_Deck(module);
    export_DeckKeyword(module);
    export_Schedule(module);
    export_Well(module);
    export_Group(module);
    export_Connection(module);
    export_EclipseConfig(module);
    export_FieldProperties(module);
    export_EclipseState(module);
    export_TableManager(module);
    export_EclipseGrid(module);
    export_UnitSystem(module);
    export_Log(module);
    export_IO(module);
    export_EModel(module);
    export_SummaryState(module);
    export_ParserKeywords(module);
}


PYBIND11_MODULE(opmcommon_python, module) {
    python::common::export_all(module);
}

void python::common::export_all_opm_embedded(py::module& module) {
    python::common::export_all(module);
    module.attr("current_ecl_state") = std::make_shared<EclipseState>();
    module.attr("current_summary_state") = std::make_shared<SummaryState>();
    module.attr("current_schedule") = std::make_shared<Schedule>();
    module.attr("current_report_step") = 0;
    module.doc() = R"pbdoc(
        opm_embedded
        -----------------------

        .. currentmodule:: opm_embedded

        .. autosummary::
           :toctree: _generate

           ParseContext
           Parser
           Deck
           DeckKeyword
           Schedule
           Well
           Group
           Connection
           EclipseConfig
           FieldProperties
           EclipseState
           EclipseGrid
           UnitSystem
           EModel
           SummaryState
    )pbdoc";
}

/*
  PYBIND11_MODULE create a Python of all the Python/C++ classes which are
  generated in the python::common::export_all_opm_embedded() function in the wrapping code.
  The same module is created as an embedded python module in PythonInterp.cpp
*/
PYBIND11_MODULE(opm_embedded, module) {
    python::common::export_all_opm_embedded(module);
}