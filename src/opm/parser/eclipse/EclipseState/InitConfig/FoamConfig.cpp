#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/FoamConfig.hpp>

namespace Opm {

    // FoamRecord member functions.

    FoamRecord::FoamRecord( const DeckRecord& record ) :
        reference_surfactant_concentration_( record.getItem( 0 ).getSIDouble( 0 ) ),
        exponent_( record.getItem( 1 ).getSIDouble( 0 ) ),
        minimum_surfactant_concentration_( record.getItem( 2 ).getSIDouble( 0 ) )
    {}

    double FoamRecord::referenceSurfactantConcentration() const {
        return this->reference_surfactant_concentration_;
    }

    double FoamRecord::exponent() const {
        return this->exponent_;
    }

    double FoamRecord::minimumSurfactantConcentration() const {
        return this->minimum_surfactant_concentration_;
    }

    // FoamConfig member functions.

    FoamConfig::FoamConfig( const DeckKeyword& keyword ) :
        records( keyword.begin(), keyword.end() )
    {}

    const FoamRecord& FoamConfig::getRecord( size_t id ) const {
        return this->records.at( id );
    }

    size_t FoamConfig::size() const {
        return this->records.size();
    }

    bool FoamConfig::empty() const {
        return this->records.empty();
    }

    FoamConfig::const_iterator FoamConfig::begin() const {
        return this->records.begin();
    }

    FoamConfig::const_iterator FoamConfig::end() const {
        return this->records.end();
    }
} // namespace Opm
