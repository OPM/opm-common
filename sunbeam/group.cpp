#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>

#include "sunbeam.hpp"


namespace {

    py::list wellnames( const Group& g, size_t timestep ) {
        return iterable_to_pylist( g.getWells( timestep )  );
    }
}

void sunbeam::export_Group() {

    py::class_< Group >( "Group", py::no_init )
        .add_property( "name", mkcopy( &Group::name ) )
        .def( "_wellnames", &wellnames )
        ;

}
