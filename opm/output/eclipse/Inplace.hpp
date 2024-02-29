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

#ifndef ORIGINAL_OIP
#define ORIGINAL_OIP

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

// The purpose of this class is to transport in-place values from the
// simulator code to the summary output code.  The code is written very much
// to fit in with the current implementation in the simulator.  Functions
// which do not take both region set name and region ID arguments are
// intended for field-level values.
class Inplace
{
public:
    // The Inplace class is implemented in close connection to the black-oil
    // output module in opm-simulators.  There are certain idiosyncracies
    // here which are due to that coupling.  For instance the enumerators
    // PressurePV, HydroCarbonPV, PressureHydroCarbonPV, and
    // DynamicPoreVolume are not included in the return value from phases().
    enum class Phase {
        WATER = 0,              // Omitted from mixingPhases()
        OIL = 1,                // Omitted from mixingPhases()
        GAS = 2,                // Omitted from mixingPhases()
        OilInLiquidPhase = 3,
        OilInGasPhase = 4,
        GasInLiquidPhase = 5,
        GasInGasPhase = 6,
        PoreVolume = 7,
        PressurePV = 8,             // Omitted from both phases() and mixingPhases()
        HydroCarbonPV = 9,          // Omitted from both phases() and mixingPhases()
        PressureHydroCarbonPV = 10, // Omitted from both phases() and mixingPhases()
        DynamicPoreVolume = 11,     // Omitted from both phases() and mixingPhases()
        WaterResVolume = 12,
        OilResVolume = 13,
        GasResVolume = 14,
        SALT = 15,
        CO2InWaterPhase = 16,
        CO2InGasPhaseInMob = 17,
        CO2InGasPhaseMob = 18,
        WaterInGasPhase = 19,
        WaterInWaterPhase = 20,
        CO2Mass = 21,
        CO2MassInWaterPhase = 22,
        CO2MassInGasPhase = 23,
        CO2MassInGasPhaseInMob = 24,
        CO2MassInGasPhaseMob = 25,
    };

    static Inplace serializationTestObject();

    void add(const std::string& region, Phase phase, std::size_t region_number, double value);
    void add(Phase phase, double value);

    double get(const std::string& region, Phase phase, std::size_t region_number) const;
    double get(Phase phase) const;

    bool has(const std::string& region, Phase phase, std::size_t region_number) const;
    bool has(Phase phase) const;

    std::size_t max_region() const;
    std::size_t max_region(const std::string& region_name) const;

    /// Linearised per-region values for a given phase in a specific region
    /// set.
    ///
    /// \param[in] region Region set name, e.g., "FIPNUM" or "FIPABC".
    ///
    /// \param[in] Phase In-place quantity.
    ///
    /// \return Per-region values of requested quantity in the region set
    ///   named by \p region.  The get_vector functions return a vector of
    ///   size max_region() which contains the values added with the add()
    ///   function and is indexed by (region_number - 1).
    std::vector<double>
    get_vector(const std::string& region, Phase phase) const;

    static const std::vector<Phase>& phases();
    static const std::vector<Phase>& mixingPhases();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(phase_values);
    }

    bool operator==(const Inplace& rhs) const;

private:
    using ValueMap = std::unordered_map<std::size_t, double>;
    using PhaseMap = std::unordered_map<Phase, ValueMap>;
    using RegionMap = std::unordered_map<std::string, PhaseMap>;

    RegionMap phase_values{};
};

} // namespace Opm

#endif // ORIGINAL_OIP
