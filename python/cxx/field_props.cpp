#include <string>

#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <pybind11/stl.h>

#include "export.hpp"
#include "converters.hpp"

namespace {

    py::list getitem( const FieldPropsManager& m, const std::string& kw) {
        if (m.has<int>(kw))
            return iterable_to_pylist( m.get<int>(kw) );
        if (m.has<double>(kw))
            return iterable_to_pylist( m.get<double>(kw) );

        throw py::key_error( "no such grid property " + kw );
    }

    bool contains( const FieldPropsManager& manager, const std::string& kw) {
        if (manager.has<int>(kw))
            return true;
        if (manager.has<double>(kw))
            return true;      

        return false;
    }
}


void python::common::export_FieldProperties(py::module& module) {
    
    py::class_< FieldPropsManager >( module, "FieldProperties")
    .def( "__contains__", &contains )
    .def( "__getitem__",  &getitem )
    ;

}
