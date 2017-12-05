#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include "sunbeam.hpp"


namespace {

    py::list completions( const Well& w, size_t timestep ) {
        return iterable_to_pylist( w.getCompletions( timestep ) );
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

void sunbeam::export_Well() {

    py::class_< Well >( "Well", py::no_init )
        .add_property( "name", mkcopy( &Well::name ) )
        .add_property( "preferred_phase", &preferred_phase )
        .def( "I",               headI )
        .def( "I",               headI_at )
        .def( "J",               headJ )
        .def( "J",               headJ_at )
        .def( "ref",             refD )
        .def( "ref",             refD_at )
        .def( "status",          &status )
        .def( "isdefined",       &Well::hasBeenDefined )
        .def( "isinjector",      &Well::isInjector )
        .def( "isproducer",      &Well::isProducer )
        .def( "group",           &Well::getGroupName )
        .def( "guide_rate",      &Well::getGuideRate )
        .def( "available_gctrl", &Well::isAvailableForGroupControl )
        .def( "__eq__",          &Well::operator== )
        .def( "_completions",    &completions )
        ;

    py::class_< std::vector< Well > >( "WellList", py::no_init )
        .def( py::vector_indexing_suite< std::vector< Well > >() )
        ;

}
