#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>

extern "C" {

    int deck_item_get_size( const Opm::DeckItem * item ) {
        return static_cast<int>(item->size());
    }

    /*
      These types must be *manually* syncronized with the values in
      the Python module: opm/deck/item_type_enum.py
    */

    int deck_item_get_type( const Opm::DeckItem * item ) {
        if (dynamic_cast<const Opm::DeckIntItem* >(item))
            return 1;

        if (dynamic_cast<const Opm::DeckStringItem *>(item))
            return 2;

        if (dynamic_cast<const Opm::DeckDoubleItem *>(item))
            return 3;

        return 0;
    }

    int deck_item_iget_int( const Opm::DeckItem * item , int index) {
        return item->getInt(static_cast<size_t>(index));
    }

    double deck_item_iget_double( const Opm::DeckItem * item , int index) {
        return item->getRawDouble(static_cast<size_t>(index));
    }

    const char * deck_item_iget_string( const Opm::DeckItem * item , int index) {
        const std::string& string = item->getString(static_cast<size_t>(index));
        return string.c_str();
    }
}


