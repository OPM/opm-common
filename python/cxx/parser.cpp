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

#include <python/cxx/OpmCommonPythonDoc.hpp>

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
        ParserKeyword kw(keyword);
        parser->addParserKeyword(kw);
    }

}

void python::common::export_Parser(py::module& module) {

    using namespace Opm::Common::DocStrings;

    module.def( "create_deck", &create_deck );
    module.def( "create_deck_string", &create_deck_string);


    py::class_<ParserKeyword>(module, "ParserKeyword", ParserKeyword_docstring)
        .def_property_readonly("name", &ParserKeyword::getName, ParserKeyword_name_docstring);

    py::enum_<Opm::Ecl::SectionType>(module, "eclSectionType", py::arithmetic(), eclSectionType_docstring)
        .value("RUNSPEC",   Opm::Ecl::RUNSPEC,   eclSectionType_RUNSPEC_docstring)
        .value("GRID",      Opm::Ecl::GRID,      eclSectionType_GRID_docstring)
        .value("EDIT",      Opm::Ecl::EDIT,      eclSectionType_EDIT_docstring)
        .value("PROPS",     Opm::Ecl::PROPS,     eclSectionType_PROPS_docstring)
        .value("REGIONS",   Opm::Ecl::REGIONS,   eclSectionType_REGIONS_docstring)
        .value("SOLUTION",  Opm::Ecl::SOLUTION,  eclSectionType_SOLUTION_docstring)
        .value("SUMMARY",   Opm::Ecl::SUMMARY,   eclSectionType_SUMMARY_docstring)
        .value("SCHEDULE",  Opm::Ecl::SCHEDULE,  eclSectionType_SCHEDULE_docstring)
        .export_values();

    py::class_<Parser>(module, "Parser", Parser_docstring)
        .def(py::init<bool>(), py::arg("add_default") = true, Parser_init_docstring)
        .def("parse"       , py::overload_cast<const std::string&>(&Parser::parseFile, py::const_), py::arg("filename"), Parser_parse_file_docstring)
        .def("parse"       , py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseFile, py::const_),
            py::arg("filename"), py::arg("context"), Parser_parse_file_context_docstring)
        .def("parse"       , py::overload_cast<const std::string&, const ParseContext&, const std::vector<Opm::Ecl::SectionType>&>(&Parser::parseFile, py::const_),
            py::arg("filename"), py::arg("context"), py::arg("sections"), Parser_parse_file_context_sections_docstring)
        .def("parse_string", py::overload_cast<const std::string&>(&Parser::parseString, py::const_), py::arg("data"), Parser_parse_string_docstring)
        .def("parse_string", py::overload_cast<const std::string&, const ParseContext&>(&Parser::parseString, py::const_),
            py::arg("data"), py::arg("context"), Parser_parse_string_context_docstring)
        .def("add_keyword",  py::overload_cast<ParserKeyword>(&Parser::addParserKeyword), py::arg("keyword"), Parser_add_keyword_docstring)
        .def("add_keyword", add_keyword, py::arg("keyword_json"), Parser_add_keyword_json_docstring)
        .def("__getitem__", &Parser::getKeyword, ref_internal, py::arg("keyword"), Parser_getitem_docstring);


}
