#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

    double eval( const TableManager& tab,  std::string tab_name, int tab_idx, std::string col_name, double x ) {
        try {
            return tab[tab_name].getTable(tab_idx).evaluate(col_name, x);
        } catch( std::invalid_argument& e ) {
           throw py::key_error( e.what() );
        }
    }

}

void python::common::export_TableManager(py::module& module) {

    using namespace Opm::Common::DocStrings;

    py::class_< TableManager >( module, "Tables", Tables_docstring)
        .def( "__contains__",   &TableManager::hasTables , Tables_contains_docstring)
        .def("evaluate", &eval, Tables_evaluate_docstring, Tables_evaluate_docstring);

}
