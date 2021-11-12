#include <fmt/format.h>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include "export.hpp"


void python::common::export_UnitSystem(py::module& module)
{
    py::class_<UnitSystem>(module, "UnitSystem")
        .def_property_readonly( "name", &UnitSystem::getName );


    py::class_<Dimension>(module, "Dimension")
        .def_property_readonly("scaling", &Dimension::getSIScaling)
        .def_property_readonly("offset", &Dimension::getSIOffset)
        .def("__repr__", [](const Dimension& dim) {
            auto scaling = dim.getSIScaling();
            auto offset = dim.getSIOffset();
            if (dim.getSIOffset() != 0)
                return fmt::format("Dimension(scaling={}, offset={})", scaling, offset);
            else
                return fmt::format("Dimension(scaling={})", scaling);
        });
}
