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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/FoamConfig.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/F.hpp>

namespace Opm
{

// FoamData member functions.

FoamData::FoamData(const DeckRecord& FOAMFSC_record, const DeckRecord& FOAMROCK_record)
    : reference_surfactant_concentration_(FOAMFSC_record.getItem(0).getSIDouble(0))
    , exponent_(FOAMFSC_record.getItem(1).getSIDouble(0))
    , minimum_surfactant_concentration_(FOAMFSC_record.getItem(2).getSIDouble(0))
    , allow_desorption_(static_cast<FoamAllowDesorption>(FOAMROCK_record.getItem(0).get<int>(0)))
    , rock_density_(FOAMROCK_record.getItem(1).getSIDouble(0))
{
    // Check validity of adsorption index.
    const int ads_ind = FOAMROCK_record.getItem(0).get<int>(0);
    if (ads_ind < 1 || ads_ind > 2) {
        throw std::runtime_error("Illegal adsorption index in FOAMROCK, must be 1 or 2.");
    }
}

double
FoamData::referenceSurfactantConcentration() const
{
    return this->reference_surfactant_concentration_;
}

double
FoamData::exponent() const
{
    return this->exponent_;
}

double
FoamData::minimumSurfactantConcentration() const
{
    return this->minimum_surfactant_concentration_;
}

FoamData::FoamAllowDesorption
FoamData::allowDesorption() const
{
    return this->allow_desorption_;
}

double
FoamData::rockDensity() const
{
    return this->rock_density_;
}

// FoamConfig member functions.

FoamConfig::FoamConfig(const Deck& deck)
{
    if (deck.hasKeyword<ParserKeywords::FOAMFSC>()) {
        const auto& kw_foamfsc = deck.getKeyword<ParserKeywords::FOAMFSC>();
        if (!deck.hasKeyword<ParserKeywords::FOAMROCK>()) {
            throw std::runtime_error("FOAMFSC present but no FOAMROCK keyword found.");
        }
        const auto& kw_foamrock = deck.getKeyword<ParserKeywords::FOAMROCK>();
        if (kw_foamfsc.size() != kw_foamrock.size()) {
            throw std::runtime_error("FOAMFSC and FOAMROCK keywords have different number of records.");
        }
        const int num_records = kw_foamfsc.size();
        this->data_.reserve(num_records);
        for (int record = 0; record < num_records; ++record) {
            this->data_.emplace_back(kw_foamfsc.getRecord(record), kw_foamrock.getRecord(record));
        }
    }
}

const FoamData&
FoamConfig::getRecord(std::size_t index) const
{
    return this->data_.at(index);
}

std::size_t
FoamConfig::size() const
{
    return this->data_.size();
}

bool
FoamConfig::empty() const
{
    return this->data_.empty();
}

FoamConfig::const_iterator
FoamConfig::begin() const
{
    return this->data_.begin();
}

FoamConfig::const_iterator
FoamConfig::end() const
{
    return this->data_.end();
}
} // namespace Opm
