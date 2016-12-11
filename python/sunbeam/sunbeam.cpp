#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <datetime.h>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

namespace py = boost::python;
using namespace Opm;

namespace {

/* converters */

struct ptime_to_python_datetime {
    static PyObject* convert( const boost::posix_time::ptime& pt ) {
        const auto& date = pt.date();
        return PyDate_FromDate( int( date.year() ),
                                int( date.month() ),
                                int( date.day() ) );

    }
};

template< typename T >
py::list vector_to_pylist( const std::vector< T >& v ) {
    py::list l;
    for( const auto& x : v ) l.append( x );
    return l;
}

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

std::string well_prefphase( const Well* w ) {
    switch( w->getPreferredPhase() ) {
        case Phase::OIL:   return "OIL";
        case Phase::GAS:   return "GAS";
        case Phase::WATER: return "WATER";
        default: throw std::logic_error( "Unhandled enum value" );
    }
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

    return vector_to_pylist( v );
}

}

BOOST_PYTHON_MODULE(libsunbeam) {

PyDateTime_IMPORT;
/* register all converters */
py::to_python_converter< const boost::posix_time::ptime,
                         ptime_to_python_datetime >();

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
    ;

py::class_< std::vector< Well > >( "WellList", py::no_init )
    .def( py::vector_indexing_suite< std::vector< Well > >() )
    ;

py::class_< Schedule >( "Schedule", py::no_init )
    .add_property( "_wells", get_wells )
    .add_property( "start",  get_start_time )
    .add_property( "end",    get_end_time )
    .add_property( "timesteps", get_timesteps )
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
