#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include "sunbeam.hpp"
#include "converters.hpp"

void sunbeam::export_DeckKeyword() {
    py::class_< DeckKeyword >( "DeckKeyword", py::no_init )
        .def( "__repr__", &DeckKeyword::name, copy() )
        .def( "__str__", &str<DeckKeyword> )
        .def( "__len__", &DeckKeyword::size )
        ;
}
