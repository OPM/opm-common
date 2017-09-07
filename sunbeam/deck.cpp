#include <opm/parser/eclipse/Deck/Deck.hpp>

#include "sunbeam.hpp"


namespace {

    size_t size( const Deck& deck ) {
        return deck.size();
    }
    size_t count( const Deck& deck, const std::string& kw ) {
        return deck.count(kw);
    }
    bool hasKeyword( const Deck& deck, const std::string& kw ) {
        return deck.hasKeyword(kw);
    }

    const DeckKeyword& getKeyword0( const Deck& deck, py::tuple i ) {
        const std::string kw = py::extract<const std::string>(py::str(i[0]));
        const size_t index = py::extract<size_t>(i[1]);
        return deck.getKeyword(kw, index);
    }
    const DeckKeyword& getKeyword1( const Deck& deck, const std::string& kw ) {
        return deck.getKeyword(kw);
    }
    const DeckKeyword& getKeyword2( const Deck& deck, size_t index ) {
        return deck.getKeyword(index);
    }

    const DeckView::const_iterator begin( const Deck& deck ) { return deck.begin(); }
    const DeckView::const_iterator   end( const Deck& deck ) { return deck.end();   }

}

void sunbeam::export_Deck() {

    py::class_< Deck >( "Deck", py::no_init )
        .def( "__len__", &size )
        .def( "__contains__", &hasKeyword )
        .def( "__iter__", py::range< ref >( &begin, &end ) )
        .def( "__getitem__", &getKeyword0, ref() )
        .def( "__getitem__", &getKeyword1, ref() )
        .def( "__getitem__", &getKeyword2, ref() )
        .def( "__str__", &str<Deck> )
        .def( "count", &count )
        ;

}
