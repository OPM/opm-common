#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include "sunbeam.hpp"
#include "converters.hpp"

namespace {

    std::string parent( const GroupTree& gt, const std::string& name ) {
        return gt.parent(name);
    }

    py::list children( const GroupTree& gt, const std::string& name ) {
        return iterable_to_pylist(gt.children(name)) ;
    }
}

void sunbeam::export_GroupTree(py::module& module) {

  py::class_< GroupTree >(module, "GroupTree")

        .def( "_parent", &parent,                       "parent function returning parent of a group")
        .def( "_children", &children,                   "children function returning python"
                                                       " list containing children of a group")
        ;
}
