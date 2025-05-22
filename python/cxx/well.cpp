#include <tuple>

#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <pybind11/stl.h>
#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

    std::vector<Connection> connections( const Well& w ) {
        const auto& well_connections = w.getConnections();
        return std::vector<Connection>(well_connections.begin(), well_connections.end());
    }

    std::string status( const Well& w )  {
        return WellStatus2String( w.getStatus() );
    }

    std::string preferred_phase( const Well& w ) {
        switch( w.getPreferredPhase() ) {
            case Phase::OIL:   return "OIL";
            case Phase::GAS:   return "GAS";
            case Phase::WATER: return "WATER";
            default: throw std::logic_error( "Unhandled enum value" );
        }
    }

    std::tuple<int, int, double> get_pos( const Well& w ) {
        return std::make_tuple(w.getHeadI(), w.getHeadJ(), w.getRefDepth());
    }

}

void python::common::export_Well(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_< Well >( module, "Well", Well_docstring)
        .def_property_readonly( "name", &Well::name, Well_name_docstring)
        .def_property_readonly( "preferred_phase", &preferred_phase, Well_preferred_phase_docstring)
        .def( "pos",             &get_pos, Well_pos_docstring)
        .def( "status",          &status, Well_status_docstring)
        .def( "isdefined",       &Well::hasBeenDefined, py::arg("report_step"), Well_isdefined_docstring)
        .def( "isinjector",      &Well::isInjector, Well_isinjector_docstring)
        .def( "isproducer",      &Well::isProducer, Well_isproducer_docstring)
        .def( "group",           &Well::groupName, Well_group_docstring)
        .def( "guide_rate",      &Well::getGuideRate, Well_guide_rate_docstring)
        .def( "available_gctrl", &Well::isAvailableForGroupControl, Well_available_gctrl_docstring)
        .def( "connections",     &connections, Well_connections_docstring);

}
