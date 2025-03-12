/*
  Copyright 2020 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>

#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
    std::optional<double> liftOptMaxRateIfDefined(const double x)
    {
        return (x > 0.0) ? std::optional { x } : std::nullopt;
    }
} // Anonymous namespace

namespace Opm {

bool GasLiftGroup::active(const RestartIO::RstGroup& rst_group)
{
    return (rst_group.glift_max_rate +
            rst_group.glift_max_supply) != 0.0;
}

GasLiftGroup::GasLiftGroup(const RestartIO::RstGroup& rst_group)
    : m_name          { rst_group.name }
    , m_max_lift_gas  { liftOptMaxRateIfDefined(rst_group.glift_max_supply) }
    , m_max_total_gas { liftOptMaxRateIfDefined(rst_group.glift_max_rate) }
{}

GasLiftGroup GasLiftGroup::serializationTestObject()
{
    GasLiftGroup group{};

    group.m_name = "GR";
    group.m_max_lift_gas  = 100;
    group.m_max_total_gas = 200;

    return group;
}

bool GasLiftGroup::operator==(const GasLiftGroup& other) const
{
    return (this->m_name == other.m_name)
        && (this->m_max_lift_gas == other.m_max_lift_gas)
        && (this->m_max_total_gas == other.m_max_total_gas)
        ;
}

// ---------------------------------------------------------------------------

GasLiftWell::GasLiftWell(const RestartIO::RstWell& rst_well)
    : m_name            { rst_well.name }
    , m_max_rate        { liftOptMaxRateIfDefined(rst_well.glift_max_rate) }
    , m_min_rate        { rst_well.glift_min_rate }
    , m_use_glo         { rst_well.glift_active }
    , m_weight          { rst_well.glift_weight_factor }
    , m_inc_weight      { rst_well.glift_inc_weight_factor }
    , m_alloc_extra_gas { rst_well.glift_alloc_extra_gas }
{}

// Unfortunately it seems just using the rst_well.glift_active flag is not
// sufficient to determine whether or not the well should be included in gas
// lift optimization.  The current implementation based on numerical values
// found in the restart file is pure guesswork.
bool GasLiftWell::active(const RestartIO::RstWell& rst_well)
{
    return (rst_well.glift_max_rate +
            rst_well.glift_min_rate +
            rst_well.glift_weight_factor) != 0.0;
}

GasLiftWell GasLiftWell::serializationTestObject()
{
    GasLiftWell well{};

    well.m_name = "WELL";
    well.m_max_rate = 2000;
    well.m_min_rate = 56;
    well.m_use_glo = true;
    well.m_weight = 1.25;
    well.m_inc_weight = 0.25;
    well.m_alloc_extra_gas = false;

    return well;
}

bool GasLiftWell::operator==(const GasLiftWell& other) const
{
    return (this->m_name == other.m_name)
        && (this->m_max_rate == other.m_max_rate)
        && (this->m_min_rate == other.m_min_rate)
        && (this->m_use_glo  == other.m_use_glo)
        && (this->m_weight   == other.m_weight)
        && (this->m_inc_weight == other.m_inc_weight)
        && (this->m_alloc_extra_gas == other.m_alloc_extra_gas)
        ;
}

// ---------------------------------------------------------------------------

bool GasLiftOpt::active() const
{
    return this->m_increment > 0.0;
}

void GasLiftOpt::gaslift_increment(const double gaslift_increment)
{
    this->m_increment = gaslift_increment;
}

double GasLiftOpt::gaslift_increment() const
{
    return this->m_increment;
}

void GasLiftOpt::min_eco_gradient(const double min_eco_gradient)
{
    this->m_min_eco_gradient = min_eco_gradient;
}

double GasLiftOpt::min_eco_gradient() const
{
    return this->m_min_eco_gradient;
}

void GasLiftOpt::min_wait(const double min_wait)
{
    this->m_min_wait = min_wait;
}

double GasLiftOpt::min_wait() const
{
    return this->m_min_wait;
}

void GasLiftOpt::all_newton(const bool all_newton)
{
    this->m_all_newton = all_newton;
}

bool GasLiftOpt::all_newton() const
{
    return this->m_all_newton;
}

const GasLiftGroup& GasLiftOpt::group(const std::string& gname) const
{
    const auto iter = this->m_groups.find(gname);
    if (iter == this->m_groups.end()) {
        throw std::out_of_range {
            "Group " + gname + " is not configured for gas lift optimization"
        };
    }

    return iter->second;
}

bool GasLiftOpt::has_well(const std::string& wname) const
{
    return this->m_wells.find(wname) != this->m_wells.end();
}

bool GasLiftOpt::has_group(const std::string& gname) const
{
    return this->m_groups.find(gname) != this->m_groups.end();
}

void GasLiftOpt::add_group(const GasLiftGroup& group)
{
    this->m_groups.insert_or_assign(group.name(), group);
}

void GasLiftOpt::add_well(const GasLiftWell& well)
{
    this->m_wells.insert_or_assign(well.name(), well);
}

const GasLiftWell& GasLiftOpt::well(const std::string& wname) const
{
    const auto iter = this->m_wells.find(wname);
    if (iter == this->m_wells.end()) {
        throw std::out_of_range {
            "Well " + wname + " is not configured for gas lift optimization"
        };
    }

    return iter->second;
}

GasLiftOpt GasLiftOpt::serializationTestObject()
{
    GasLiftOpt glo{};

    glo.m_increment = 0.123;
    glo.m_min_eco_gradient = 0.456;
    glo.m_min_wait = 89.1;
    glo.m_all_newton = false;

    glo.add_well(GasLiftWell::serializationTestObject());
    glo.m_wells.insert_or_assign("W2", GasLiftWell::serializationTestObject());

    glo.add_group(GasLiftGroup::serializationTestObject());
    glo.m_groups.insert_or_assign("G2", GasLiftGroup::serializationTestObject());
    glo.m_groups.insert_or_assign("G3", GasLiftGroup::serializationTestObject());

    return glo;
}

bool GasLiftOpt::operator==(const GasLiftOpt& other) const
{
    return (this->m_increment == other.m_increment)
        && (this->m_min_eco_gradient == other.m_min_eco_gradient)
        && (this->m_min_wait == other.m_min_wait)
        && (this->m_all_newton == other.m_all_newton)
        && (this->m_groups == other.m_groups)
        && (this->m_wells == other.m_wells)
        ;
}

} // namespace Opm
