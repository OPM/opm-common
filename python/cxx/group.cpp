#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>
#include <pybind11/stl.h>
#include "common.hpp"


namespace {

  const std::vector<std::string> wellnames( const Group2& g ) {
        return g.wells( );
    }

     int get_vfp_table_nr( const Group2& g ) {
        return g.getGroupNetVFPTable();
    }
}

void python::common::export_Group(py::module& module) {

  py::class_< Group2 >( module, "Group")
    .def_property_readonly( "name", &Group2::name)
    .def( "_vfp_table_nr", &get_vfp_table_nr )
    .def( "_wellnames", &wellnames );

}
