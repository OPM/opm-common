#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>

#include "sunbeam.hpp"

void sunbeam::export_Completion() {

    py::class_< Completion >( "Completion", py::no_init )
        .def( "getI",      &Completion::getI )
        .def( "getJ",      &Completion::getJ )
        .def( "getK",      &Completion::getK )
        ;

}
