#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <pybind11/stl.h>

#include "common.hpp"
#include "converters.hpp"

namespace {

    py::list getitem( const Eclipse3DProperties& p, const std::string& kw) {
        const auto& ip = p.getIntProperties();
        if (ip.supportsKeyword(kw) && ip.hasKeyword(kw))
            return iterable_to_pylist(p.getIntGridProperty(kw).getData());


        const auto& dp = p.getDoubleProperties();
        if (dp.supportsKeyword(kw) && dp.hasKeyword(kw))
            return iterable_to_pylist(p.getDoubleGridProperty(kw).getData());

        throw py::key_error( "no such grid property " + kw );
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

    std::vector<int> regions( const Eclipse3DProperties& p, const std::string& kw) {
        return p.getRegions(kw);
    }

}

void sunbeam::export_Eclipse3DProperties(py::module& module) {

  py::class_< Eclipse3DProperties >( module, "Eclipse3DProperties") 
    .def( "getRegions",   &regions )
    .def( "__contains__", &contains )
    .def( "__getitem__",  &getitem )
    ;

}
