#include <string>
#include <exception>

#include <opm/json/JsonObject.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeyword.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <pybind11/stl.h>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>

#include "export.hpp"


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

    void add_keyword(Parser* parser, const std::string& json_string) {
        const Json::JsonObject keyword(json_string);
        parser->addParserKeyword(keyword);
    }

}

void python::common::export_Parser(py::module& module) {

    module.def( "create_deck", &create_deck );
    module.def( "create_deck_string", &create_deck_string);


    py::class_<ParserKeyword>(module, "ParserKeyword")
        .def_property_readonly("name", &ParserKeyword::getName);

    py::enum_<Opm::Ecl::SectionType>(module, "eclSectionType", py::arithmetic())
        .value("GRID", Opm::Ecl::GRID)
        .value("PROPS", Opm::Ecl::PROPS)
        .value("REGIONS", Opm::Ecl::REGIONS)
        .value("SOLUTION", Opm::Ecl::SOLUTION)
        .value("SUMMARY", Opm::Ecl::SUMMARY)
        .value("SCHEDULE", Opm::Ecl::SCHEDULE)
        .export_values();

    py::class_<Parser>(module, "Parser")
        .def(py::init<bool>(), py::arg("add_default") = true)
        .def("parse", py::overload_cast<const std::string&>(&Parser::parseFile, py::const_))
        .def("parse"       , py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseFile, py::const_))
        .def("parse"       , py::overload_cast<const std::string&, const ParseContext&, const std::vector<Opm::Ecl::SectionType>&>(&Parser::parseFile, py::const_))
        .def("parse_string", py::overload_cast<const std::string&>(&Parser::parseString, py::const_))
        .def("parse_string", py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseString, py::const_))
        .def("add_keyword",  py::overload_cast<ParserKeyword>(&Parser::addParserKeyword))
        .def("add_keyword", add_keyword)
        .def("__getitem__", &Parser::getKeyword, ref_internal);


}
