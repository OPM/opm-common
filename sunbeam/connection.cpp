#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>

#include "sunbeam.hpp"

namespace {

std::string state( const Connection& c ) {
    return WellCompletion::StateEnum2String( c.state );
}

std::string direction( const Connection& c ) {
    return WellCompletion::DirectionEnum2String( c.dir );
}

}


void sunbeam::export_Connection(py::module& module) {

  py::class_< Connection >( module, "Connection")
    .def_property_readonly("direction",            &direction )
    .def_property_readonly("state",                &state )
    .def_property_readonly( "I",                   &Connection::getI )
    .def_property_readonly( "J",                   &Connection::getJ )
    .def_property_readonly( "K",                   &Connection::getK )
    .def_property_readonly( "attached_to_segment", &Connection::attachedToSegment )
    .def_readonly( "center_depth",                 &Connection::center_depth)
    .def_property_readonly( "diameter",            &Connection::getDiameter )
    .def_readonly( "complnum",                     &Connection::complnum)
    .def_readonly("number",                        &Connection::complnum)  // This is deprecated; complnum is the "correct" proeprty name
    .def_readonly( "sat_table_id",                 &Connection::sat_tableId)
    .def_readonly( "segment_number",               &Connection::segment_number)
    .def_property_readonly( "skin_factor",         &Connection::getSkinFactor )
    .def_property_readonly( "transmissibility",    &Connection::getConnectionTransmissibilityFactor )
    .def_readonly( "well_pi",                      &Connection::wellPi );
}
