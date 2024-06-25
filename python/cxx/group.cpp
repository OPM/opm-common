#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <pybind11/stl.h>
#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

  const std::vector<std::string> wellnames( const Group& g ) {
        return g.wells( );
    }
}

void python::common::export_Group(py::module& module) {

  using namespace Opm::Common::DocStrings;

  py::class_< Group >( module, "Group", GroupClass_docstring)
    .def_property_readonly( "name", &Group::name, Group_name_docstring)
    .def_property_readonly( "num_wells", &Group::numWells, Group_num_wells_docstring)
    .def_property_readonly( "well_names", &wellnames, Group_well_names_docstring);

}
