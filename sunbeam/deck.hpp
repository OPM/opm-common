#include <boost/python.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>


namespace Opm {
    class Deck;
    class DeckKeyword;
}

namespace deck {
    using namespace Opm;

    size_t size( const Deck& deck );
    size_t count( const Deck& deck, const std::string& kw );
    bool hasKeyword( const Deck& deck, const std::string& kw );
    const DeckKeyword& getKeyword0( const Deck& deck, boost::python::tuple i );
    const DeckKeyword& getKeyword1( const Deck& deck, const std::string& kw );
    const DeckKeyword& getKeyword2( const Deck& deck, size_t index );
    const DeckView::const_iterator begin( const Deck& deck );
    const DeckView::const_iterator   end( const Deck& deck );

    void export_Deck();

}
