#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/FoamConfig.hpp>

namespace Opm {

    FoamRecord::FoamRecord( const DeckRecord& record ) :
    {}


    /* */

    Foam::Foam( const DeckKeyword& keyword ) :
        records( keyword.begin(), keyword.end() )
    {}

    const FoamRecord& Foam::getRecord( size_t id ) const {
        return this->records.at( id );
    }

    size_t Foam::size() const {
        return this->records.size();
    }

    bool Foam::empty() const {
        return this->records.empty();
    }

    Foam::const_iterator Foam::begin() const {
        return this->records.begin();
    }

    Foam::const_iterator Foam::end() const {
        return this->records.end();
    }
}
