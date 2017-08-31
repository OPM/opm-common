#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>

#include "eclipse_3d_properties.hpp"

namespace py = boost::python;
using namespace Opm;

using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;


namespace eclipse_3d_properties {

    py::list getitem( const Eclipse3DProperties& p, const std::string& kw) {
        const auto& ip = p.getIntProperties();
        if (ip.supportsKeyword(kw) && ip.hasKeyword(kw))
            return iterable_to_pylist(p.getIntGridProperty(kw).getData());

        const auto& dp = p.getDoubleProperties();
        if (dp.supportsKeyword(kw) && dp.hasKeyword(kw))
            return iterable_to_pylist(p.getDoubleGridProperty(kw).getData());
        throw key_error( "no such grid property " + kw );
    }

    bool contains( const Eclipse3DProperties& p, const std::string& kw) {
        return
            (p.getIntProperties().supportsKeyword(kw) &&
             p.getIntProperties().hasKeyword(kw))
            ||
            (p.getDoubleProperties().supportsKeyword(kw) &&
             p.getDoubleProperties().hasKeyword(kw))
            ;
    }
    py::list regions( const Eclipse3DProperties& p, const std::string& kw) {
        return iterable_to_pylist( p.getRegions(kw) );
    }


    void export_Eclipse3DProperties() {


        py::class_< Eclipse3DProperties >( "Eclipse3DProperties", py::no_init )
            .def( "getRegions",   &regions )
            .def( "__contains__", &contains )
            .def( "__getitem__",  &getitem )
            ;

    }

}
