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

#include <opm/output/eclipse/Inplace.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

namespace {

static const std::string FIELD_NAME = std::string{"FIELD"};
static const std::size_t FIELD_ID   = 0;

std::size_t region_max(const std::unordered_map<std::size_t, double>& region_map)
{
    return std::accumulate(region_map.begin(), region_map.end(), std::size_t{0},
                           [](const std::size_t max, const auto& elm)
                           {
                               return std::max(max, elm.first);
                           });
}

std::size_t
phase_region_max(const std::unordered_map<Opm::Inplace::Phase, std::unordered_map<std::size_t, double>>& phase_map)
{
    return std::accumulate(phase_map.begin(),
                           phase_map.end(),
                           std::size_t{0},
        [](const std::size_t max, const auto& region_map)
    {
        return std::max(max, region_max(region_map.second));
    });
}

template <typename Vector>
Vector append(Vector first, const Vector& second)
{
    first.insert(first.end(), second.begin(), second.end());
    return first;
}

} // Anonymous namespace

namespace Opm {

Inplace Inplace::serializationTestObject()
{
    Inplace result;
    result.add("test1", Phase::WaterResVolume, 1, 2.0);

    return result;
}

std::string Inplace::EclString(const Phase phase)
{
    using namespace std::string_literals;
    static const auto phaseToEcl = std::unordered_map<Phase, std::string>{
        {Phase::WATER,                               "WIP"s},
        {Phase::OIL,                                 "OIP"s},
        {Phase::GAS,                                 "GIP"s},
        {Phase::OilInLiquidPhase,                    "OIPL"s},
        {Phase::OilInGasPhase,                       "OIPG"s},
        {Phase::GasInLiquidPhase,                    "GIPL"s},
        {Phase::GasInLiquidPhase,                    "GIPL"s},
        {Phase::GasInGasPhase,                       "GIPG"s},
        {Phase::PoreVolume,                          "RPV"s},
        {Phase::WaterResVolume,                      "WIPR"s},
        {Phase::OilResVolume,                        "OIPR"s},
        {Phase::GasResVolume,                        "GIPR"s},
        {Phase::SALT,                                "SIP"s},
        {Phase::CO2InWaterPhase,                     "WCD"s},
        {Phase::CO2InGasPhaseInMob,                  "GCDI"s},
        {Phase::CO2InGasPhaseMob,                    "GCDM"s},
        {Phase::CO2InGasPhaseInMobKrg,               "GKDI"s},
        {Phase::CO2InGasPhaseMobKrg,                 "GKDM"s},
        {Phase::WaterInGasPhase,                     "WIPG"s},
        {Phase::WaterInWaterPhase,                   "WIPL"s},
        {Phase::CO2Mass,                             "GMIP"s},
        {Phase::CO2MassInWaterPhase,                 "GMDS"s},
        {Phase::CO2MassInGasPhase,                   "GMGP"s},
        {Phase::CO2MassInGasPhaseInMob,              "GCDI_KG"s}, // Not used
        {Phase::CO2MassInGasPhaseMob,                "GKDM_KG"s}, // Not used
        {Phase::CO2MassInGasPhaseInMobKrg,           "GKTR"s},
        {Phase::CO2MassInGasPhaseMobKrg,             "GKMO"s},
        {Phase::CO2MassInGasPhaseMaximumTrapped,     "GMTR"s},
        {Phase::CO2MassInGasPhaseMaximumUnTrapped,   "GMMO"s},
        {Phase::CO2MassInGasPhaseEffectiveTrapped,   "GMST"s},
        {Phase::CO2MassInGasPhaseEffectiveUnTrapped, "GMUS"s},
        {Phase::MicrobialMass,                       "MMIP"s},
        {Phase::OxygenMass,                          "MOIP"s},
        {Phase::UreaMass,                            "MUIP"s},
        {Phase::BiofilmMass,                         "MBIP"s},
        {Phase::CalciteMass,                         "MCIP"s},
    };

    const auto it = phaseToEcl.find(phase);

    if (it == phaseToEcl.end()) {
        throw std::logic_error {
            fmt::format("Phase enum with integer value: "
                        "{} not recognized", static_cast<int>(phase))
        };
    }

    return it->second;
}

void Inplace::add(const std::string&   region,
                  const Inplace::Phase phase,
                  const std::size_t    region_id,
                  const double         value)
{
    this->phase_values[region][phase][region_id] = value;
}

void Inplace::add(Inplace::Phase phase, double value)
{
    this->add(FIELD_NAME, phase, FIELD_ID, value);
}

double Inplace::get(const std::string&   region,
                    const Inplace::Phase phase,
                    const std::size_t    region_id) const
{
    auto region_iter = this->phase_values.find(region);
    if (region_iter == this->phase_values.end()) {
        throw std::logic_error {
            fmt::format("No such region: {}", region)
        };
    }

    auto phase_iter = region_iter->second.find(phase);
    if (phase_iter == region_iter->second.end()) {
        throw std::logic_error {
            fmt::format("No such phase: {}:{}",
                        region, static_cast<int>(phase))
        };
    }

    auto value_iter = phase_iter->second.find(region_id);
    if (value_iter == phase_iter->second.end()) {
        throw std::logic_error {
            fmt::format("No such region id: {}:{}:{}",
                        region, static_cast<int>(phase), region_id)
        };
    }

    return value_iter->second;
}

double Inplace::get(Inplace::Phase phase) const
{
    return this->get(FIELD_NAME, phase, FIELD_ID);
}

bool Inplace::has(const std::string& region,
                  const Phase        phase,
                  const std::size_t  region_id) const
{
    auto region_iter = this->phase_values.find(region);
    if (region_iter == this->phase_values.end()) {
        return false;
    }

    auto phase_iter = region_iter->second.find(phase);
    if (phase_iter == region_iter->second.end()) {
        return false;
    }

    return phase_iter->second.find(region_id)
        != phase_iter->second.end();
}

bool Inplace::has(Phase phase) const
{
    return this->has(FIELD_NAME, phase, FIELD_ID);
}

std::size_t Inplace::max_region() const
{
    return std::accumulate(this->phase_values.begin(),
                           this->phase_values.end(),
                           std::size_t{0},
        [](const std::size_t max, const auto& phase_map)
    {
        return std::max(max, phase_region_max(phase_map.second));
    });
}

std::size_t Inplace::max_region(const std::string& region_name) const
{
    auto region_iter = this->phase_values.find(region_name);
    if (region_iter == this->phase_values.end()) {
        throw std::logic_error {
            fmt::format("No such region: {}", region_name)
        };
    }

    return phase_region_max(region_iter->second);
}

std::vector<double>
Inplace::get_vector(const std::string& region,
                    const Phase        phase) const
{
    std::vector<double> v(this->max_region(region), 0.0);

    auto region_iter = this->phase_values.find(region);
    if (region_iter == this->phase_values.end()) {
        throw std::logic_error {
            fmt::format("No such region: {}", region)
        };
    }

    auto phase_iter = region_iter->second.find(phase);
    if (phase_iter == region_iter->second.end()) {
        throw std::logic_error {
            fmt::format("Phase {} does not exist in region {}",
                        static_cast<int>(phase), region)
        };
    }

    for (const auto& [region_id, value] : phase_iter->second) {
        v[region_id - 1] = value;
    }

    return v;
}

const std::vector<Inplace::Phase>& Inplace::phases()
{
    static const auto phases_ = append(std::vector {
        Inplace::Phase::WATER,
        Inplace::Phase::OIL,
        Inplace::Phase::GAS,
        }, mixingPhases());

    return phases_;
}

const std::vector<Inplace::Phase>& Inplace::mixingPhases()
{
    static const auto mixingPhases_ = std::vector {
        Inplace::Phase::OilInLiquidPhase,
        Inplace::Phase::OilInGasPhase,
        Inplace::Phase::GasInLiquidPhase,
        Inplace::Phase::GasInGasPhase,
        Inplace::Phase::PoreVolume,
        Inplace::Phase::WaterResVolume,
        Inplace::Phase::OilResVolume,
        Inplace::Phase::GasResVolume,
        Inplace::Phase::SALT,
        Inplace::Phase::CO2InWaterPhase,
        Inplace::Phase::CO2InGasPhaseInMob,
        Inplace::Phase::CO2InGasPhaseMob,
        Inplace::Phase::CO2InGasPhaseInMobKrg,
        Inplace::Phase::CO2InGasPhaseMobKrg,
        Inplace::Phase::WaterInGasPhase,
        Inplace::Phase::WaterInWaterPhase,
        Inplace::Phase::CO2Mass,
        Inplace::Phase::CO2MassInWaterPhase,
        Inplace::Phase::CO2MassInGasPhase,
        Inplace::Phase::CO2MassInGasPhaseInMob,
        Inplace::Phase::CO2MassInGasPhaseMob,
        Inplace::Phase::CO2MassInGasPhaseInMobKrg,
        Inplace::Phase::CO2MassInGasPhaseMobKrg,
        Inplace::Phase::CO2MassInGasPhaseEffectiveTrapped,
        Inplace::Phase::CO2MassInGasPhaseEffectiveUnTrapped,
        Inplace::Phase::CO2MassInGasPhaseMaximumTrapped,
        Inplace::Phase::CO2MassInGasPhaseMaximumUnTrapped,
        Inplace::Phase::MicrobialMass,
        Inplace::Phase::OxygenMass,
        Inplace::Phase::UreaMass,
        Inplace::Phase::BiofilmMass,
        Inplace::Phase::CalciteMass,
    };

    return mixingPhases_;
}

bool Inplace::operator==(const Inplace& rhs) const
{
    return this->phase_values == rhs.phase_values;
}

} // namespace Opm
