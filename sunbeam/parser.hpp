#include <boost/python.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>

namespace Opm {
    class Deck;
    class ParseContext;
}

namespace parser {
    using namespace Opm;

    Deck parseDeck( const std::string& deckStr,
                    const boost::python::list& keywords,
                    bool isFile,
                    const ParseContext& = ParseContext() );

    void export_Parser();
}
