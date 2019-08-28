/*
  Copyright 2019 by Norce.

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

#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellSaltwaterProperties.hpp>

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/UDAValue.hpp>

void Opm::WellSaltwaterProperties::handleWSALT(const DeckRecord& rec)
{
    this->m_saltwaterConcentration = rec.getItem("CONCENTRATION").get<UDAValue>(0).get<double>();
}

bool Opm::WellSaltwaterProperties::operator!=(const WellSaltwaterProperties& other) const
{
    return this->m_saltwaterConcentration != other.m_saltwaterConcentration;
}
