#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>

#include "export.hpp"

namespace {

std::string state( const Connection& c ) {
    return Connection::State2String( c.state() );
}

std::string direction( const Connection& c ) {
    return Connection::Direction2String( c.dir() );
}

}


void python::common::export_Connection(py::module& module) {

  py::class_< Connection >( module, "Connection")
    .def_property_readonly("direction",            &direction )
    .def_property_readonly("state",                &state )
    .def_property_readonly( "I",                   &Connection::getI )
    .def_property_readonly( "J",                   &Connection::getJ )
    .def_property_readonly( "K",                   &Connection::getK )
    .def_property_readonly( "attached_to_segment", &Connection::attachedToSegment )
    .def_property_readonly( "center_depth",        &Connection::depth)
    .def_property_readonly( "rw",                  &Connection::rw)
    .def_property_readonly( "complnum",            &Connection::complnum)
    .def_property_readonly( "number",              &Connection::complnum)  // This is deprecated; complnum is the "correct" proeprty name
    .def_property_readonly( "sat_table_id",        &Connection::satTableId)
    .def_property_readonly( "segment_number",      &Connection::segment)
    .def_property_readonly( "CF",                  &Connection::CF)
    .def_property_readonly( "Kh",                  &Connection::Kh)
    .def_property_readonly( "well_pi",             &Connection::wellPi );
}
