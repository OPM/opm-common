#include <string>

#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <pybind11/stl.h>

#include "export.hpp"
#include "converters.hpp"

namespace {

    bool contains( const FieldPropsManager& manager, const std::string& kw) {
        if (manager.has<int>(kw))
            return true;
        if (manager.has<double>(kw))
            return true;      

        return false;
    }

    py::array_t<double> get_double_array(const FieldPropsManager& m, const std::string& kw) {
        if (m.has<double>(kw))
            return convert::numpy_array( m.get<double>(kw) );
        else
            throw std::invalid_argument("Keyword '" + kw + "'is not of type double.");
    }

    py::array_t<int> get_int_array(const FieldPropsManager& m, const std::string& kw) {
        if (m.has<int>(kw))
            return convert::numpy_array( m.get<int>(kw) );
        else
            throw std::invalid_argument("Keyword '" + kw + "'is not of type int.");
    }

}


void python::common::export_FieldProperties(py::module& module) {
    
    py::class_< FieldPropsManager >( module, "FieldProperties")
    .def( "__contains__", &contains )
    .def( "get_double_array",  &get_double_array )
    .def( "get_int_array",  &get_int_array )
    ;

}
