#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

std::string state( const Connection& c ) {
    return Connection::State2String( c.state() );
}

std::string direction( const Connection& c ) {
    return Connection::Direction2String( c.dir() );
}

std::tuple<int, int, int> get_pos( const Connection& conn ) {
    return std::make_tuple(conn.getI(), conn.getJ(), conn.getK());
}

}


void python::common::export_Connection(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_<Connection>(module, "Connection", Connection_docstring)
        .def_property_readonly("direction", &direction, Connection_direction_docstring)
        .def_property_readonly("state", &state, Connection_state_docstring)
        .def_property_readonly("i", &Connection::getI, Connection_getI_docstring)
        .def_property_readonly("j", &Connection::getJ, Connection_getJ_docstring)
        .def_property_readonly("k", &Connection::getK, Connection_getK_docstring)
        .def_property_readonly("pos", &get_pos, Connection_pos_docstring)
        .def_property_readonly("attached_to_segment", &Connection::attachedToSegment, Connection_attachedToSegment_docstring)
        .def_property_readonly("center_depth", &Connection::depth, Connection_depth_docstring)
        .def_property_readonly("rw", &Connection::rw, Connection_rw_docstring)
        .def_property_readonly("complnum", &Connection::complnum, Connection_complnum_docstring)
        .def_property_readonly("number", &Connection::complnum, Connection_number_docstring)  // DEPRECATED: Use 'complnum' instead.
        .def_property_readonly("sat_table_id", &Connection::satTableId, Connection_satTableId_docstring)
        .def_property_readonly("segment_number", &Connection::segment, Connection_segment_docstring)
        .def_property_readonly("cf", &Connection::CF, Connection_CF_docstring)
        .def_property_readonly("kh", &Connection::Kh, Connection_Kh_docstring);
}
