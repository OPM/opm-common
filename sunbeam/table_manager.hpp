#ifndef SUNBEAM_TABLE_MANAGER_HPP
#define SUNBEAM_TABLE_MANAGER_HPP

#include <boost/python.hpp>
#include "converters.hpp"


namespace Opm {
    class TableManager;
}

namespace table_manager {
    namespace py = boost::python;
    using namespace Opm;


    double evaluate( const TableManager& tab,
                     std::string tab_name,
                     int tab_idx,
                     std::string col_name, double x );

    void export_TableManager();

}

#endif //SUNBEAM_TABLE_MANAGER_HPP
