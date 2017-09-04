#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>

#include "completion.hpp"

namespace py = boost::python;
using namespace Opm;


namespace completion {

    void export_Completion() {

        py::class_< Completion >( "Completion", py::no_init )
            .def( "getI",      &Completion::getI )
            .def( "getJ",      &Completion::getJ )
            .def( "getK",      &Completion::getK )
            ;

    }

}
