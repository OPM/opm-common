#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <pybind11/stl.h>

#include "export.hpp"


namespace {

    void (ParseContext::*ctx_update)(const std::string&, InputError::Action) = &ParseContext::update;

}

void python::common::export_ParseContext(py::module& module) {

    py::class_< ParseContext >(module, "ParseContext" )
        .def(py::init<>())
        .def(py::init<const std::vector<std::pair<std::string, InputError::Action>>>())
        .def( "ignore_keyword", &ParseContext::ignoreKeyword )
        .def( "update", ctx_update );

    py::enum_< InputError::Action >( module, "action" )
      .value( "throw",  InputError::Action::THROW_EXCEPTION )
      .value( "warn",   InputError::Action::WARN )
      .value( "ignore", InputError::Action::IGNORE );

}
