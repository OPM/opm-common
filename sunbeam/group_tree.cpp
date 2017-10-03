#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include "sunbeam.hpp"

namespace {

    std::string parent( const GroupTree& gt, const std::string& name ) {
        return gt.parent(name);
    }

    py::list children( const GroupTree& gt, const std::string& name ) {
        return iterable_to_pylist(gt.children(name)) ;
    }
}

void sunbeam::export_GroupTree() {

    py::class_< GroupTree >( "GroupTree", py::no_init)

        .def( "_parent", &parent,                       "parent function returning parent of a group")
        .def( "_children", &children,                   "children function returning python"
                                                       " list containing children of a group")
        ;
}
