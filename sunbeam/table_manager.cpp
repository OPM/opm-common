#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

#include "table_manager.hpp"

namespace py = boost::python;
using namespace Opm;


namespace table_manager {

    double evaluate( const TableManager& tab,
                     std::string tab_name,
                     int tab_idx,
                     std::string col_name, double x ) try {
      return tab[tab_name].getTable(tab_idx).evaluate(col_name, x);
    } catch( std::invalid_argument& e ) {
      throw key_error( e.what() );
    }

    void export_TableManager() {

        py::class_< TableManager >( "Tables", py::no_init )
            .def( "__contains__",   &TableManager::hasTables )
            .def("_evaluate",       &evaluate )
            ;

    }

}
