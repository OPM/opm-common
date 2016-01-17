#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SimpleTable.hpp>

extern "C" {

    bool table_has_column( const Opm::SimpleTable * table , const char * column ) {
        return table->hasColumn( column );
    }


    int table_get_num_rows( const Opm::SimpleTable * table ) {
        return table->numRows();
    }


    double table_get_value( const Opm::SimpleTable * table , const char * column , int row_index) {
        return table->get( column , row_index );
    }


    double table_evaluate( const Opm::SimpleTable * table , const char * column , int row_index) {
        return table->evaluate( column , row_index );
    }
}
