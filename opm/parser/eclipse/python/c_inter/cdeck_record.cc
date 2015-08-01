#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

extern "C" {

    int deck_record_get_size( const Opm::DeckRecord * record ) {
        return static_cast<int>(record->size());
    }

    bool deck_record_has_item(const Opm::DeckRecord * record , const char * item) {
        return record->hasItem( item );
    }

    Opm::DeckItem * deck_record_iget_item( const Opm::DeckRecord * record , int index) {
        auto shared = record->getItem( static_cast<size_t>(index));
        return shared.get();
    }

    Opm::DeckItem * deck_record_get_item( const Opm::DeckRecord * record , const char * name) {
        auto shared = record->getItem( name );
        return shared.get();
    }

}


