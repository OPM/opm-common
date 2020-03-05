/*
  Copyright 2020 SINTEF Digital, Mathematics and Cybernetics.

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
#include <opm/parser/eclipse/EclipseState/InitConfig/PolymerConfig.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>

namespace Opm
{

PolymerConfig::PolymerConfig(const Deck& deck) :
    has_shrate(deck.hasKeyword<ParserKeywords::SHRATE>())
{
}

PolymerConfig::PolymerConfig(bool shrate)
    : has_shrate(shrate)
{
}

bool
PolymerConfig::shrate() const {
    return has_shrate;
}

bool
PolymerConfig::operator==(const PolymerConfig& data) const
{
    return has_shrate == data.has_shrate;
}

} // namespace Opm
