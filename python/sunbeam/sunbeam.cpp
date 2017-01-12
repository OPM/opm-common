#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include "converters.hpp"

namespace py = boost::python;
using namespace Opm;

namespace {

namespace state {
py::list getNNC( const EclipseState& state ) {
    py::list l;
    for( const auto& x : state.getInputNNC().nncdata() )
        l.append( py::make_tuple( x.cell1, x.cell2, x.trans )  );
    return l;
}
}

namespace grid {
py::tuple getXYZ( const EclipseGrid& grid ) {
    return py::make_tuple( grid.getNX(),
                           grid.getNY(),
                           grid.getNZ());
}
int getNumActive( const EclipseGrid& grid ) {
    return grid.getNumActive();
}
int getCartesianSize( const EclipseGrid& grid ) {
    return grid.getCartesianSize();
}
int getGlobalIndex( const EclipseGrid& grid, int i, int j, int k ) {
    return grid.getGlobalIndex(i, j, k);
}
py::tuple getIJK( const EclipseGrid& grid, int g ) {
    const auto& ijk = grid.getIJK(g);
    return py::make_tuple(ijk[0], ijk[1], ijk[2]);
}
}

namespace props {
py::list getitem( const Eclipse3DProperties& p, const std::string& kw) {
    const auto& ip = p.getIntProperties();
    if (ip.supportsKeyword(kw) && ip.hasKeyword(kw))
        return iterable_to_pylist(p.getIntGridProperty(kw).getData());

    const auto& dp = p.getDoubleProperties();
    if (dp.supportsKeyword(kw) && dp.hasKeyword(kw))
        return iterable_to_pylist(p.getDoubleGridProperty(kw).getData());
    throw key_error( "no such grid property " + kw );
}

bool contains( const Eclipse3DProperties& p, const std::string& kw) {
    return
        (p.getIntProperties().supportsKeyword(kw) &&
         p.getIntProperties().hasKeyword(kw))
        ||
        (p.getDoubleProperties().supportsKeyword(kw) &&
         p.getDoubleProperties().hasKeyword(kw))
        ;
}
py::list regions( const Eclipse3DProperties& p, const std::string& kw) {
    return iterable_to_pylist( p.getRegions(kw) );
}
}

namespace group {
py::list wellnames( const Group& g, size_t timestep ) {
    return iterable_to_pylist( g.getWells( timestep )  );
}

}

namespace well {
std::string status( const Well& w, size_t timestep )  {
    return WellCommon::Status2String( w.getStatus( timestep ) );
}

std::string preferred_phase( const Well& w ) {
    switch( w.getPreferredPhase() ) {
        case Phase::OIL:   return "OIL";
        case Phase::GAS:   return "GAS";
        case Phase::WATER: return "WATER";
        default: throw std::logic_error( "Unhandled enum value" );
    }
}

int    (Well::*headI)() const = &Well::getHeadI;
int    (Well::*headJ)() const = &Well::getHeadI;
double (Well::*refD)()  const = &Well::getRefDepth;

int    (Well::*headI_at)(size_t) const = &Well::getHeadI;
int    (Well::*headJ_at)(size_t) const = &Well::getHeadI;
double (Well::*refD_at)(size_t)  const = &Well::getRefDepth;

}

namespace schedule {

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


boost::posix_time::ptime get_start_time( const Schedule& s ) {
    return boost::posix_time::from_time_t( s.posixStartTime() );
}

boost::posix_time::ptime get_end_time( const Schedule& s ) {
    return boost::posix_time::from_time_t( s.posixEndTime() );
}

py::list get_timesteps( const Schedule& s ) {
    const auto& tm = s.getTimeMap();
    std::vector< boost::posix_time::ptime > v;
    v.reserve( tm.size() );

    for( size_t i = 0; i < tm.size(); ++i ) v.push_back( tm[ i ] );

    return iterable_to_pylist( v );
}

py::list get_groups( const Schedule& sch ) {
    std::vector< Group > groups;
    for( const auto& g : sch.getGroups() )
        groups.push_back( *g );

    return iterable_to_pylist( groups );
}

}

EclipseState (*parse)( const std::string&, const ParseContext& ) = &Parser::parse;
EclipseState (*parseData) (const std::string &data, const ParseContext& context) = &Parser::parseData;
void (ParseContext::*ctx_update)(const std::string&, InputError::Action) = &ParseContext::update;

}

BOOST_PYTHON_MODULE(libsunbeam) {
/*
 * Python C-API requires this macro to be invoked before anything from
 * datetime.h is used.
 */
PyDateTime_IMPORT;

/* register all converters */
py::to_python_converter< const boost::posix_time::ptime,
                         ptime_to_python_datetime >();

py::register_exception_translator< key_error >( &key_error::translate );

py::def( "parse", parse );
py::def( "parseData", parseData );

py::class_< EclipseState >( "EclipseState", py::no_init )
    .add_property( "title", &EclipseState::getTitle )
    .def( "_schedule",      &EclipseState::getSchedule,     ref() )
    .def( "_props",         &EclipseState::get3DProperties, ref() )
    .def( "_grid",          &EclipseState::getInputGrid,    ref() )
    .def( "has_input_nnc",  &EclipseState::hasInputNNC )
    .def( "input_nnc",      state::getNNC )
    ;

py::class_< EclipseGrid >( "EclipseGrid", py::no_init )
    .def( "_getXYZ",        grid::getXYZ )
    .def( "nactive",        grid::getNumActive )
    .def( "cartesianSize",  grid::getCartesianSize )
    .def( "globalIndex",    grid::getGlobalIndex )
    .def( "getIJK",         grid::getIJK )
    ;

py::class_< Eclipse3DProperties >( "Eclipse3DProperties", py::no_init )
    .def( "getRegions",   props::regions )
    .def( "__contains__", props::contains )
    .def( "__getitem__",  props::getitem )
    ;

py::class_< Well >( "Well", py::no_init )
    .add_property( "name", mkcopy( &Well::name ) )
    .add_property( "preferred_phase", &well::preferred_phase )
    .def( "I",   well::headI )
    .def( "I",   well::headI_at )
    .def( "J",   well::headJ )
    .def( "J",   well::headJ_at )
    .def( "ref", well::refD )
    .def( "ref", well::refD_at )
    .def( "status",     &well::status )
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
    .add_property( "name", mkcopy( &Group::name ) )
    .def( "_wellnames", group::wellnames )
    ;

py::class_< Schedule >( "Schedule", py::no_init )
    .add_property( "_wells", schedule::get_wells )
    .add_property( "_groups", schedule::get_groups )
    .add_property( "start",  schedule::get_start_time )
    .add_property( "end",    schedule::get_end_time )
    .add_property( "timesteps", schedule::get_timesteps )
    .def( "__contains__", &Schedule::hasWell )
    .def( "__getitem__", schedule::get_well, ref() )
    .def( "_group", &Schedule::getGroup, ref() )
    ;

py::class_< ParseContext >( "ParseContext" )
    .def( "update", ctx_update )
    ;

py::enum_< InputError::Action >( "action" )
    .value( "throw",  InputError::Action::THROW_EXCEPTION )
    .value( "warn",   InputError::Action::WARN )
    .value( "ignore", InputError::Action::IGNORE )
    ;
}
