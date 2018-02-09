#include <opm/json/JsonObject.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <pybind11/stl.h>

#include "sunbeam.hpp"


namespace {

    Deck parseDeck( const std::string& deckStr,
                    const std::vector<std::string>& keywords,
                    bool isFile,
                    const ParseContext& pc ) {
        Parser p;
        for (const auto& keyword : keywords) {
            const Json::JsonObject jkw(keyword);
            p.addParserKeyword(jkw);
        }
        return isFile ? p.parseFile(deckStr, pc) : p.parseString(deckStr, pc);
    }

    EclipseState * parse(const std::string& filename, const ParseContext& context) {
        Parser p;
        const auto deck = p.parseFile(filename, context);
        return new EclipseState(deck,context);
    }

    EclipseState * parseData(const std::string& deckStr, const ParseContext& context) {
        Parser p;
        const auto deck = p.parseString(deckStr, context);
        return new EclipseState(deck,context);
    }

    void (ParseContext::*ctx_update)(const std::string&, InputError::Action) = &ParseContext::update;
}

void sunbeam::export_Parser(py::module& module) {

    module.def( "parse", parse );
    module.def( "parse_data", parseData );
    module.def( "parse_deck", &parseDeck );

    py::class_< ParseContext >(module, "ParseContext" )
        .def(py::init<>())
        .def( "update", ctx_update );

    py::enum_< InputError::Action >( module, "action" )
      .value( "throw",  InputError::Action::THROW_EXCEPTION )
      .value( "warn",   InputError::Action::WARN )
      .value( "ignore", InputError::Action::IGNORE );
}
