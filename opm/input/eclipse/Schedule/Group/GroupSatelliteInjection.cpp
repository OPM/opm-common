/*
  Copyright 2025 Equinor ASA.

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

#include <config.h>

#include <opm/input/eclipse/Schedule/Group/GroupSatelliteInjection.hpp>

#include <opm/input/eclipse/EclipseState/Phase.hpp>

#include <map>
#include <optional>
#include <vector>

Opm::GroupSatelliteInjection::Rate
Opm::GroupSatelliteInjection::Rate::serializationTestObject()
{
    return Rate{}.surface(123.45).reservoir(6789.1011).calorific(1.21314);
}

Opm::GroupSatelliteInjection::Rate&
Opm::GroupSatelliteInjection::Rate::surface(const double q)
{
    this->surface_.emplace(q);
    return *this;
}

Opm::GroupSatelliteInjection::Rate&
Opm::GroupSatelliteInjection::Rate::reservoir(const double q)
{
    this->resv_.emplace(q);
    return *this;
}

Opm::GroupSatelliteInjection::Rate&
Opm::GroupSatelliteInjection::Rate::calorific(const double c)
{
    this->calorific_.emplace(c);
    return *this;
}

// ---------------------------------------------------------------------------

Opm::GroupSatelliteInjection::GroupSatelliteInjection(const std::string& group)
    : group_ { group }
{}

Opm::GroupSatelliteInjection
Opm::GroupSatelliteInjection::serializationTestObject()
{
    auto i = GroupSatelliteInjection { "G" };

    i.rate(Phase::GAS) = Rate::serializationTestObject();
    i.rate(Phase::WATER) = Rate::serializationTestObject();

    return i;
}

Opm::GroupSatelliteInjection::Rate&
Opm::GroupSatelliteInjection::rate(const Phase phase)
{
    const auto& [ratePos, inserted] =
        this->i_.try_emplace(phase, this->rates_.size());

    if (inserted) {
        this->rates_.emplace_back();
    }

    return this->rates_[ratePos->second];
}

std::optional<Opm::GroupSatelliteInjection::RateIx>
Opm::GroupSatelliteInjection::rateIndex(const Phase phase) const
{
    const auto ratePos = this->i_.find(phase);
    if (ratePos == this->i_.end()) {
        // This group has no satellite injection rate defined for 'phase'.
        return {};
    }

    return { ratePos->second };
}

bool Opm::GroupSatelliteInjection::operator==(const GroupSatelliteInjection& that) const
{
    return (this->group_ == that.group_)
        && (this->i_ == that.i_)
        && (this->rates_ == that.rates_)
        ;
}
