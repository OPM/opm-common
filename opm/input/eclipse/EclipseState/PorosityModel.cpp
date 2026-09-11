/*
  Copyright 2026 TNO

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

#include <opm/input/eclipse/EclipseState/PorosityModel.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>

namespace Opm {

PorosityModel::PorosityModel(const Deck& deck)
{
    if (deck.hasKeyword<ParserKeywords::DUALPERM>()) {
        this->m_type = Type::DualPermeability;
    } else if (deck.hasKeyword<ParserKeywords::DUALPORO>()) {
        this->m_type = Type::DualPorosity;
    }

    if (deck.hasKeyword<ParserKeywords::NODPPM>()) {
        this->m_scale_fracture_perm = false;
    }
}

PorosityModel::Type PorosityModel::type() const noexcept
{
    return this->m_type;
}

bool PorosityModel::dualContinuum() const noexcept
{
    return this->m_type != Type::SinglePorosity;
}

bool PorosityModel::dualPermeability() const noexcept
{
    return this->m_type == Type::DualPermeability;
}

bool PorosityModel::fracturePermeabilityScalingActive() const noexcept
{
    return this->dualContinuum() && this->m_scale_fracture_perm;
}

PorosityModel PorosityModel::serializationTestObject()
{
    PorosityModel result;
    result.m_type = Type::DualPermeability;
    result.m_scale_fracture_perm = false;

    return result;
}

bool PorosityModel::operator==(const PorosityModel& other) const
{
    return (this->m_type == other.m_type)
        && (this->m_scale_fracture_perm == other.m_scale_fracture_perm);
}

} // namespace Opm
