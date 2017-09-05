#include <string>


namespace Opm {
    class DeckKeyword;
}

namespace deck_keyword {
    using namespace Opm;

    std::string write( const DeckKeyword& kw );

    void export_DeckKeyword();

}
