#include <boost/python.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

namespace py = boost::python;
using namespace Opm;

BOOST_PYTHON_MODULE(libsunbeam) {

EclipseState (*parse_file)( const std::string&, const ParseContext& ) = &Parser::parse;
py::def( "parse", parse_file );

py::class_< EclipseState >( "EclipseState", py::no_init )
    .add_property( "title", &EclipseState::getTitle )
    ;

void (ParseContext::*ctx_update)(const std::string&, InputError::Action) = &ParseContext::update;
py::class_< ParseContext >( "ParseContext" )
    .def( "update", ctx_update )
    ;

py::enum_< InputError::Action >( "action" )
    .value( "throw",  InputError::Action::THROW_EXCEPTION )
    .value( "warn",   InputError::Action::WARN )
    .value( "ignore", InputError::Action::IGNORE )
    ;
}
