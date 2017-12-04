#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <pybind11/stl.h>
#include "sunbeam.hpp"


namespace {

  const std::set<std::string> wellnames( const Group& g, size_t timestep ) {
        return g.getWells( timestep );
    }

     int get_vfp_table_nr( const Group& g, size_t timestep ) {
        return g.getGroupNetVFPTable(timestep);
    }
}

void sunbeam::export_Group(py::module& module) {

  py::class_< Group >( module, "Group")
    .def_property_readonly( "name", &Group::name)
    .def( "_vfp_table_nr", &get_vfp_table_nr )
    .def( "_wellnames", &wellnames );

}
