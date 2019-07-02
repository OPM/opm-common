/*
  Copyright 2019 SINTEF Digital, Mathematics and Cybernetics.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/parser/eclipse/EclipseState/InitConfig/FoamConfig.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/F.hpp>

namespace Opm {

    // FoamRecord member functions.

    FoamRecord::FoamRecord( const DeckRecord& record )
        : reference_surfactant_concentration_( record.getItem( 0 ).getSIDouble( 0 ) )
        , exponent_( record.getItem( 1 ).getSIDouble( 0 ) )
        , minimum_surfactant_concentration_( record.getItem( 2 ).getSIDouble( 0 ) )
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

    FoamConfig::FoamConfig( const Deck& deck )
    {
        if (deck.hasKeyword<ParserKeywords::FOAMFSC>()) {
            const auto& kw = deck.getKeyword<ParserKeywords::FOAMFSC>();
            this->records = std::vector<FoamRecord>(kw.begin(), kw.end());
        }
    }

    const FoamRecord& FoamConfig::getRecord( std::size_t index ) const {
        return this->records.at( index );
    }

    std::size_t FoamConfig::size() const {
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
