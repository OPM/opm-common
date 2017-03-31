#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultFace.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
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
py::list faultNames( const EclipseState& state ) {
    py::list l;
    const auto& fc = state.getFaults();
    for (size_t i = 0; i < fc.size(); i++) {
        const auto& f = fc.getFault(i);
        l.append(f.getName());
    }
    return l;
}
py::dict jfunc( const EclipseState& s) {
  const auto& tm = s.getTableManager();
  if (!tm.useJFunc())
    return py::dict();
  const auto& j = tm.getJFunc();
  std::string flag = "BOTH";
  std::string dir  = "XY";
  if (j.flag() == JFunc::Flag::WATER)
    flag = "WATER";
  else if (j.flag() == JFunc::Flag::GAS)
    flag = "GAS";

  if (j.direction() == JFunc::Direction::X)
    dir = "X";
  else if (j.direction() == JFunc::Direction::Y)
    dir = "Y";
  else if (j.direction() == JFunc::Direction::Z)
    dir = "Z";

  py::dict ret;
  ret["FLAG"] = flag;
  ret["DIRECTION"] = dir;
  ret["ALPHA_FACTOR"] = j.alphaFactor();
  ret["BETA_FACTOR"] = j.betaFactor();
  if (j.flag() == JFunc::Flag::WATER || j.flag() == JFunc::Flag::BOTH)
    ret["OIL_WATER"] = j.owSurfaceTension();
  if (j.flag() == JFunc::Flag::GAS || j.flag() == JFunc::Flag::BOTH)
    ret["GAS_OIL"] = j.goSurfaceTension();
  return ret;
}


const std::string faceDir( FaceDir::DirEnum dir ) {
  switch (dir) {
  case FaceDir::DirEnum::XPlus:  return "X+";
  case FaceDir::DirEnum::XMinus: return "X-";
  case FaceDir::DirEnum::YPlus:  return "Y+";
  case FaceDir::DirEnum::YMinus: return "Y-";
  case FaceDir::DirEnum::ZPlus:  return "Z+";
  case FaceDir::DirEnum::ZMinus: return "Z-";
  }
  return "Unknown direction";
}

py::list faultFaces( const EclipseState& state, const std::string& name ) {
    py::list l;
    const auto& gr = state.getInputGrid(); // used for global -> IJK
    const auto& fc = state.getFaults();
    const Fault& f = fc.getFault(name);
    for (const auto& ff : f) {
        // for each fault face
        for (size_t g : ff) {
            // for global index g in ff
            const auto ijk = gr.getIJK(g);
            l.append(py::make_tuple(ijk[0], ijk[1], ijk[2], faceDir(ff.getDir())));
        }
    }
    return l;
}
}

namespace cfg {
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
double cellVolume1G( const EclipseGrid& grid, size_t glob_idx) {
  return grid.getCellVolume(glob_idx);
}
double cellVolume3( const EclipseGrid& grid, size_t i_idx, size_t j_idx, size_t k_idx) {
  return grid.getCellVolume(i_idx, j_idx, k_idx);
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
py::list completions( const Well& w) {
    return iterable_to_pylist( w.getCompletions() );
}

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

namespace tables {
double evaluate( const TableManager& tab,
                 std::string tab_name,
                 int tab_idx,
                 std::string col_name, double x ) try {
  return tab[tab_name].getTable(tab_idx).evaluate(col_name, x);
} catch( std::invalid_argument& e ) {
  throw key_error( e.what() );
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


/*
 * state, grid, properties
 */
py::class_< EclipseState >( "EclipseState", py::no_init )
    .add_property( "title", &EclipseState::getTitle )
    .def( "_schedule",      &EclipseState::getSchedule,     ref() )
    .def( "_props",         &EclipseState::get3DProperties, ref() )
    .def( "_grid",          &EclipseState::getInputGrid,    ref() )
    .def( "_cfg",           &EclipseState::cfg,             ref() )
    .def( "_tables",        &EclipseState::getTableManager, ref() )
    .def( "has_input_nnc",  &EclipseState::hasInputNNC )
    .def( "input_nnc",      state::getNNC )
    .def( "faultNames",     state::faultNames )
    .def( "faultFaces",     state::faultFaces )
    .def( "jfunc",          state::jfunc )
    ;

py::class_< EclipseGrid >( "EclipseGrid", py::no_init )
    .def( "_getXYZ",        grid::getXYZ )
    .def( "nactive",        grid::getNumActive )
    .def( "cartesianSize",  grid::getCartesianSize )
    .def( "globalIndex",    grid::getGlobalIndex )
    .def( "getIJK",         grid::getIJK )
    .def( "_cellVolume1G",  grid::cellVolume1G)
    .def( "_cellVolume3",   grid::cellVolume3)
    ;

py::class_< Eclipse3DProperties >( "Eclipse3DProperties", py::no_init )
    .def( "getRegions",   props::regions )
    .def( "__contains__", props::contains )
    .def( "__getitem__",  props::getitem )
    ;

py::class_< TableManager >( "Tables", py::no_init )
    .def( "__contains__",   &TableManager::hasTables )
    .def("_evaluate",       tables::evaluate )
    ;

/*
 * config
 */
py::class_< EclipseConfig >( "EclipseConfig", py::no_init )
    .def( "summary",         &EclipseConfig::summary,    ref())
    .def( "init",            &EclipseConfig::init,       ref())
    .def( "restart",         &EclipseConfig::restart,    ref())
    .def( "simulation",      &EclipseConfig::simulation, ref())
    ;

py::class_< SummaryConfig >( "SummaryConfig", py::no_init )
    .def( "__contains__",    &SummaryConfig::hasKeyword )
    ;

py::class_< InitConfig >( "InitConfig", py::no_init )
    .def( "hasEquil",           &InitConfig::hasEquil )
    .def( "restartRequested",   &InitConfig::restartRequested )
    .def( "getRestartStep"  ,   &InitConfig::getRestartStep )
    ;

py::class_< RestartConfig >( "RestartConfig", py::no_init )
    .def( "getKeyword",          &RestartConfig::getKeyword )
    .def( "getFirstRestartStep", &RestartConfig::getFirstRestartStep )
    .def( "getWriteRestartFile", &RestartConfig::getWriteRestartFile )
    ;

py::class_< SimulationConfig >( "SimulationConfig", py::no_init )
    .def("hasThresholdPressure", &SimulationConfig::hasThresholdPressure )
    .def("useCPR",               &SimulationConfig::useCPR )
    .def("hasDISGAS",            &SimulationConfig::hasDISGAS )
    .def("hasVAPOIL",            &SimulationConfig::hasVAPOIL )
    ;




/*
 * schedule, well, completion, group
 */
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
    .def( "completions", well::completions )
    ;

py::class_< Completion >( "Completion", py::no_init )
    .def( "getI",      &Completion::getI )
    .def( "getJ",      &Completion::getJ )
    .def( "getK",      &Completion::getK )
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



/*
 * misc
 */
py::class_< ParseContext >( "ParseContext" )
    .def( "update", ctx_update )
    ;

py::enum_< InputError::Action >( "action" )
    .value( "throw",  InputError::Action::THROW_EXCEPTION )
    .value( "warn",   InputError::Action::WARN )
    .value( "ignore", InputError::Action::IGNORE )
    ;
}
