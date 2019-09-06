#include <string>
#include <exception>

#include <opm/json/JsonObject.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <pybind11/stl.h>

#include "common_state.hpp"
#include "common.hpp"


namespace {

    Deck create_deck( const std::string& deckStr,
                      const ParseContext& pc,
                      const Parser& parser) {

        Opm::ErrorGuard guard;
        auto p = parser.parseFile( deckStr, pc, guard );
        guard.clear();
        return p;
    }


    Deck create_deck_string( const std::string& deckStr,
                             const ParseContext& pc,
                             const Parser& parser) {
        Opm::ErrorGuard guard;
        auto p = parser.parseString( deckStr, pc, guard );
        guard.clear();
        return p;
    }


    SunbeamState * parse_file(const std::string& filename, const ParseContext& context, const Parser& parser) {
        return new SunbeamState(true, filename, context, parser);
    }

    SunbeamState * parse_string(const std::string& filename, const ParseContext& context, const Parser& parser) {
        return new SunbeamState(false, filename, context, parser);
    }

    void add_keyword(Parser* parser, const std::string& json_string) {
        const Json::JsonObject keyword(json_string);
        parser->addParserKeyword(keyword);
    }

    

}

void opmcommon_python::export_Parser(py::module& module) {

    module.def( "parse", parse_file );
    module.def( "parse_string", parse_string);
    module.def( "create_deck", &create_deck );
    module.def( "create_deck_string", &create_deck_string);


    py::class_<ParserKeyword>(module, "ParserKeyword")
        .def_property_readonly("name", &ParserKeyword::getName);


    py::class_<SunbeamState>(module, "SunbeamState")
        .def("_schedule", &SunbeamState::getSchedule, ref_internal)
        .def("_state", &SunbeamState::getEclipseState, ref_internal)
        .def("_deck", &SunbeamState::getDeck, ref_internal)
        .def("_summary_config", &SunbeamState::getSummmaryConfig, ref_internal);


    py::class_<Parser>(module, "Parser")
        .def(py::init<>())
        .def("parse", py::overload_cast<const std::string&>(&Parser::parseFile, py::const_))
        .def("parse"       , py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseFile, py::const_))
        .def("parse_string", py::overload_cast<const std::string&>(&Parser::parseString, py::const_))
        .def("parse_string", py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseString, py::const_))
        .def("add_keyword", add_keyword)
        .def("__getitem__", &Parser::getKeyword, ref_internal);


}
