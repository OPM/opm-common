#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>



extern "C" {

    Opm::Eclipse3DProperties * eclipse3d_properties_alloc( Opm::Deck* deck, Opm::TableManager * table_manager , Opm::EclipseGrid * grid) {
        return new Opm::Eclipse3DProperties( *deck, *table_manager, *grid );
    }

    void eclipse3d_properties_free( Opm::Eclipse3DProperties * props ) {
        delete props;
    }

}

