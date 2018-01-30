#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <pybind11/stl.h>
#include "sunbeam.hpp"


namespace {

    std::vector<Completion> completions( const Well& w, size_t timestep ) {
        const auto& well_completions = w.getCompletions( timestep );
        return std::vector<Completion>(well_completions.begin(), well_completions.end());
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

void sunbeam::export_Well(py::module& module) {

    py::class_< Well >( module, "Well")
        .def_property_readonly( "name", &Well::name )
        .def_property_readonly( "preferred_phase", &preferred_phase )
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
        .def( "__equal__",       &Well::operator== )
        .def( "_completions",    &completions );

}
