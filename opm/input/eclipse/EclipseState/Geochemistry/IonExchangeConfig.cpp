/*
  Copyright (C) 2025 Equinor

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
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Geochemistry/IonExchangeConfig.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/I.hpp>

namespace Opm {

IonExchangeConfig::IonExchangeConfig(const Deck& deck) {
    using IONEX = ParserKeywords::IONEX;
    if (deck.hasKeyword<IONEX>()) {
        const auto& keyword = deck.get<IONEX>().back();
        const auto& item = keyword.getRecord(0).getItem<IONEX::data>();

        OpmLog::info( keyword.location().format("\nInitializing ion exchange from {keyword} in {file} line {line}") );
        this->initializeSpeciesType(item, deck, SpeciesType::IONEX);
    }
}

}  // namespace Opm