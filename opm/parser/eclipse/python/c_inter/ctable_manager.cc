#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

extern "C" {

    Opm::TableManager * table_manager_alloc( const Opm::Deck * deck ) {
        return new Opm::TableManager( *deck );
    }


    void table_manager_free( Opm::TableManager * table_manager ) {
        delete table_manager;
    }
}
