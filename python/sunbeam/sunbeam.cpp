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

py::class_< ParseContext >( "ParseContext" )
    ;
}
