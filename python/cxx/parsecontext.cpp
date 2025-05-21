#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <pybind11/stl.h>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

    void (ParseContext::*ctx_update)(const std::string&, InputErrorAction) = &ParseContext::update;

}

void python::common::export_ParseContext(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_< ParseContext >(module, "ParseContext", ParseContext_docstring)
        .def(py::init<>(), ParseContext_init_docstring)
        .def(py::init<const std::vector<std::pair<std::string, InputErrorAction>>>(), py::arg("action_list"), ParseContext_init_with_actions_docstring)
        .def( "ignore_keyword", &ParseContext::ignoreKeyword, py::arg("keyword"), ParseContext_ignore_keyword_docstring)
        .def( "update", ctx_update, py::arg("keyword"), py::arg("action"), ParseContext_update_docstring);

    py::enum_< InputErrorAction >( module, "action", action_docstring)
      .value( "throw",  InputErrorAction::THROW_EXCEPTION, action_throw_docstring)
      .value( "warn",   InputErrorAction::WARN, action_warn_docstring)
      .value( "ignore", InputErrorAction::IGNORE, action_ignore_docstring);

}
