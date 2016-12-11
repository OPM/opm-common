#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

namespace py = boost::python;
using namespace Opm;

namespace {

std::vector< Well > get_wells( const Schedule& sch ) {
    std::vector< Well > wells;
    for( const auto& w : sch.getWells() )
        wells.push_back( *w );

    return wells;
}

/* alias some of boost's long names and operations */
using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;

template< typename F >
auto mkref( F f ) -> decltype( py::make_function( f, ref() ) ) {
    return py::make_function( f, ref() );
}

template< typename F >
auto mkcopy( F f ) -> decltype( py::make_function( f, copy() ) ) {
    return py::make_function( f, copy() );
}

std::string well_status( const Well* w, size_t timestep ) {
    return WellCommon::Status2String( w->getStatus( timestep ) );
}

}

BOOST_PYTHON_MODULE(libsunbeam) {

EclipseState (*parse_file)( const std::string&, const ParseContext& ) = &Parser::parse;
py::def( "parse", parse_file );

py::class_< EclipseState >( "EclipseState", py::no_init )
    .add_property( "title", &EclipseState::getTitle )
    .def( "_schedule", &EclipseState::getSchedule, ref() )
    ;

int    (Well::*headI)() const = &Well::getHeadI;
int    (Well::*headJ)() const = &Well::getHeadI;
double (Well::*refD)()  const = &Well::getRefDepth;

int    (Well::*headIts)(size_t) const = &Well::getHeadI;
int    (Well::*headJts)(size_t) const = &Well::getHeadI;
double (Well::*refDts)(size_t)  const = &Well::getRefDepth;

py::class_< Well >( "Well", py::no_init )
    .add_property( "name", mkcopy( &Well::name ) )
    .def( "I",   headI )
    .def( "I",   headIts )
    .def( "J",   headJ )
    .def( "J",   headJts )
    .def( "ref", refD )
    .def( "ref", refDts )
    .def( "status", &well_status )
    .def( "isdefined",  &Well::hasBeenDefined )
    .def( "isinjector", &Well::isInjector )
    .def( "isproducer", &Well::isProducer )
    ;

py::class_< std::vector< Well > >( "WellList", py::no_init )
    .def( py::vector_indexing_suite< std::vector< Well > >() )
    ;

py::class_< Schedule >( "Schedule", py::no_init )
    .add_property( "_wells", get_wells )
    .def( "__contains__", &Schedule::hasWell )
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
