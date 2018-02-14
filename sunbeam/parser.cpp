#include <opm/json/JsonObject.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <pybind11/stl.h>

#include "sunbeam_state.hpp"
#include "sunbeam.hpp"


namespace {

    Deck create_deck( const std::string& deckStr,
                      const ParseContext& pc,
                      const Parser& parser) {
        return parser.parseFile( deckStr, pc);
    }


    Deck create_deck_string( const std::string& deckStr,
                             const ParseContext& pc,
                             const Parser& parser) {
        return parser.parseString( deckStr, pc);
    }


    SunbeamState * parse_file(const std::string& filename, const ParseContext& context, const Parser& parser) {
        return new SunbeamState(true, filename, context, parser);
    }

    SunbeamState * parse_string(const std::string& filename, const ParseContext& context, const Parser& parser) {
        return new SunbeamState(false, filename, context, parser);
    }

    void (ParseContext::*ctx_update)(const std::string&, InputError::Action) = &ParseContext::update;

    void add_keyword(Parser* parser, const std::string& json_string) {
        const Json::JsonObject keyword(json_string);
        parser->addParserKeyword(keyword);
    }
}

void sunbeam::export_Parser(py::module& module) {

    module.def( "parse", parse_file );
    module.def( "parse_string", parse_string);
    module.def( "create_deck", &create_deck );
    module.def( "create_deck_string", &create_deck_string);

    py::class_<SunbeamState>(module, "SunbeamState")
        .def("_schedule", &SunbeamState::getSchedule, ref_internal)
        .def("_state", &SunbeamState::getEclipseState, ref_internal)
        .def("_deck", &SunbeamState::getDeck, ref_internal)
        .def("_summary_config", &SunbeamState::getSummmaryConfig, ref_internal);

    py::class_< ParseContext >(module, "ParseContext" )
        .def(py::init<>())
        .def(py::init<const std::vector<std::pair<std::string, InputError::Action>>>())
        .def( "update", ctx_update );

    py::enum_< InputError::Action >( module, "action" )
      .value( "throw",  InputError::Action::THROW_EXCEPTION )
      .value( "warn",   InputError::Action::WARN )
      .value( "ignore", InputError::Action::IGNORE );


    py::class_<Parser>(module, "Parser")
        .def(py::init<>())
        .def("add_keyword", add_keyword);
}
