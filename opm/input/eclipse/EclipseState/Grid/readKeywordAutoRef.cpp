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

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/EclipseState/Grid/AutoRefManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/readKeywordAutoRef.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp> // AUTOREF

namespace Opm {

void readKeywordAutoRef(const DeckRecord& deckRecord, AutoRefManager& autoRefManager) {
    const auto& NXItem = deckRecord.getItem<ParserKeywords::AUTOREF::NX>();
    const auto& NYItem = deckRecord.getItem<ParserKeywords::AUTOREF::NY>();
    const auto& NZItem = deckRecord.getItem<ParserKeywords::AUTOREF::NZ>();
    const auto& OPTItem = deckRecord.getItem<ParserKeywords::AUTOREF::OPTION_TRANS_MULT>();

    const auto& active_autoRef = autoRefManager.getAutoRef();

    const int nx = NXItem.defaultApplied(0) ? active_autoRef.NX() : NXItem.get<int>(0);
    const int ny = NYItem.defaultApplied(0) ? active_autoRef.NY() : NYItem.get<int>(0);
    const int nz = NZItem.defaultApplied(0) ? active_autoRef.NZ() : NZItem.get<int>(0);
    const double opt = OPTItem.defaultApplied(0) ? active_autoRef.OPTION_TRANS_MULT() : OPTItem.get<double>(0);

    autoRefManager.readKeywordAutoRef(nx, ny, nz, opt);
}

}
