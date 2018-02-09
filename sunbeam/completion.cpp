#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>

#include "sunbeam.hpp"

namespace {

std::string state( const Completion& c ) {
    return WellCompletion::StateEnum2String( c.getState() );
}

std::string direction( const Completion& c ) {
    return WellCompletion::DirectionEnum2String( c.getDirection() );
}

}


void sunbeam::export_Completion(py::module& module) {

  py::class_< Completion >( module, "Completion")
    .def_property_readonly("direction",            &direction )
    .def_property_readonly("state",                &state )
    .def_property_readonly( "I",                   &Completion::getI )
    .def_property_readonly( "J",                   &Completion::getJ )
    .def_property_readonly( "K",                   &Completion::getK )
    .def_property_readonly( "attached_to_segment", &Completion::attachedToSegment )
    .def_property_readonly( "center_depth",        &Completion::getCenterDepth )
    .def_property_readonly( "diameter",            &Completion::getDiameter )
    .def_property_readonly( "number",              &Completion::complnum)
    .def_property_readonly( "sat_table_id",        &Completion::getSatTableId )
    .def_property_readonly( "segment_number",      &Completion::getSegmentNumber )
    .def_property_readonly( "skin_factor",         &Completion::getSkinFactor )
    .def_property_readonly( "transmissibility",    &Completion::getConnectionTransmissibilityFactor )
    .def_property_readonly( "well_pi",             &Completion::getWellPi );
}
