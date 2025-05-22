#include <fmt/format.h>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

void python::common::export_UnitSystem(py::module& module)
{

    using namespace Opm::Common::DocStrings;

    py::class_<UnitSystem>(module, "UnitSystem", UnitSystem_docstring)
        .def_property_readonly( "name", &UnitSystem::getName, UnitSystem_name_docstring);


    py::class_<Dimension>(module, "Dimension", Dimension_docstring)
        .def_property_readonly("scaling", &Dimension::getSIScaling, Dimension_scaling_docstring)
        .def_property_readonly("offset", &Dimension::getSIOffset, Dimension_offset_docstring)
        .def("__repr__", [](const Dimension& dim) {
            auto scaling = dim.getSIScaling();
            auto offset = dim.getSIOffset();
            if (offset != 0)
                return fmt::format("Dimension(scaling={}, offset={})", scaling, offset);
            else
                return fmt::format("Dimension(scaling={})", scaling);
        }, Dimension_repr_docstring);
}
