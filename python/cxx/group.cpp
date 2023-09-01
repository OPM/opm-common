#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <pybind11/stl.h>
#include "export.hpp"


namespace {

  const std::vector<std::string> wellnames( const Group& g ) {
        return g.wells( );
    }
}

void python::common::export_Group(py::module& module) {

  py::class_< Group >( module, "Group")
    .def_property_readonly( "name", &Group::name)
    .def_property_readonly( "num_wells", &Group::numWells)
    .def_property_readonly( "well_names", &wellnames );

}
