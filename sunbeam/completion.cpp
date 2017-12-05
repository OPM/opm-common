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


void sunbeam::export_Completion() {

    py::class_< Completion >( "Completion", py::no_init )
        .add_property( "direction",           &direction )
        .add_property( "state",               &state )
        .def_readonly( "I",                   &Completion::getI )
        .def_readonly( "J",                   &Completion::getJ )
        .def_readonly( "K",                   &Completion::getK )
        .def_readonly( "attached_to_segment", &Completion::attachedToSegment )
        .def_readonly( "center_depth",        &Completion::getCenterDepth )
        .def_readonly( "diameter",            &Completion::getDiameter )
        .def_readonly( "number",              &Completion::complnum)
        .def_readonly( "sat_table_id",        &Completion::getSatTableId )
        .def_readonly( "segment_number",      &Completion::getSegmentNumber )
        .def_readonly( "skin_factor",         &Completion::getSkinFactor )
        .def_readonly( "transmissibility",    &Completion::getConnectionTransmissibilityFactor )
        .def_readonly( "well_pi",             &Completion::getWellPi )
        ;

}
