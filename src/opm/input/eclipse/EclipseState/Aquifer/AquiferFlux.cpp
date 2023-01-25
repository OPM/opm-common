/*
  Copyright (C) 2023 Equinor

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

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>
#include <include/opm/input/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

namespace Opm {
    AquiferFlux::AquiferFlux(const DeckRecord& record)
    : id (record.getItem<ParserKeywords::AQUFLUX::AQUIFER_ID>().get<int>(0))
    , flux(record.getItem<ParserKeywords::AQUFLUX::FLUX>().getSIDouble(0))
    , salt_concentration(record.getItem<ParserKeywords::AQUFLUX::SC_0>().getSIDouble(0))
    {
        if (record.getItem<ParserKeywords::AQUFLUX::TEMP>().hasValue(0)) {
            this->temperature = record.getItem<ParserKeywords::AQUFLUX::TEMP>().getSIDouble(0);
        }

        if (record.getItem<ParserKeywords::AQUFLUX::PRESSURE>().hasValue(0)) {
            this->datum_pressure = record.getItem<ParserKeywords::AQUFLUX::PRESSURE>().getSIDouble(0);
        }
    }

    int AquiferFlux::name() const
    {
        return this->id;
    }

    std::unordered_map<int, AquiferFlux>
            AquiferFlux::aqufluxFromKeywords(const std::vector<const DeckKeyword*>& keywords)
    {
        std::unordered_map<int, AquiferFlux> aquifer_constant_flux;
        for (const auto* keyword : keywords) {
            for (const auto& record : *keyword) {
                AquiferFlux aquifer(record);
                aquifer_constant_flux.insert_or_assign(aquifer.id, aquifer);
            }
        }
        return aquifer_constant_flux;
    }
} // end of namespace Opm