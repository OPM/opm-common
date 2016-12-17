#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include "converters.hpp"

namespace py = boost::python;
using namespace Opm;

namespace {

py::list group_wellnames( const Group& g, size_t timestep ) {
    return iterable_to_pylist( g.getWells( timestep )  );
}

std::string well_status( const Well* w, size_t timestep ) {
    return WellCommon::Status2String( w->getStatus( timestep ) );
}

std::string well_prefphase( const Well* w ) {
    switch( w->getPreferredPhase() ) {
        case Phase::OIL:   return "OIL";
        case Phase::GAS:   return "GAS";
        case Phase::WATER: return "WATER";
        default: throw std::logic_error( "Unhandled enum value" );
    }
}

std::vector< Well > get_wells( const Schedule& sch ) {
    std::vector< Well > wells;
    for( const auto& w : sch.getWells() )
        wells.push_back( *w );

    return wells;
}

const Well& get_well( const Schedule& sch, const std::string& name ) try {
    return *sch.getWell( name );
} catch( const std::invalid_argument& e ) {
    throw key_error( name );
}


boost::posix_time::ptime get_start_time( const Schedule* s ) {
    return boost::posix_time::from_time_t( s->posixStartTime() );
}

boost::posix_time::ptime get_end_time( const Schedule* s ) {
    return boost::posix_time::from_time_t( s->posixEndTime() );
}

py::list get_timesteps( const Schedule* s ) {
    const auto& tm = s->getTimeMap();
    std::vector< boost::posix_time::ptime > v;
    v.reserve( tm.size() );

    for( size_t i = 0; i < tm.size(); ++i ) v.push_back( tm[ i ] );

    return iterable_to_pylist( v );
}

}

BOOST_PYTHON_MODULE(libsunbeam) {

PyDateTime_IMPORT;
/* register all converters */
py::to_python_converter< const boost::posix_time::ptime,
                         ptime_to_python_datetime >();

py::register_exception_translator< key_error >( &key_error::translate );

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
    .add_property( "preferred_phase", &well_prefphase )
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
    .def( "group", &Well::getGroupName )
    .def( "guide_rate", &Well::getGuideRate )
    .def( "available_gctrl", &Well::isAvailableForGroupControl )
    .def( "__eq__", &Well::operator== )
    ;

py::class_< std::vector< Well > >( "WellList", py::no_init )
    .def( py::vector_indexing_suite< std::vector< Well > >() )
    ;

py::class_< Group >( "Group", py::no_init )
    .def( "name",       mkcopy( &Group::name ) )
    .def( "_wellnames", group_wellnames )
    ;

py::class_< Schedule >( "Schedule", py::no_init )
    .add_property( "_wells", get_wells )
    .add_property( "start",  get_start_time )
    .add_property( "end",    get_end_time )
    .add_property( "timesteps", get_timesteps )
    .def( "__contains__", &Schedule::hasWell )
    .def( "__getitem__", get_well, ref() )
    .def( "_group", &Schedule::getGroup, ref() )
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
