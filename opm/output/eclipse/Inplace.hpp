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
        CO2InGasPhaseInMobKrg = 19,
        CO2InGasPhaseMobKrg = 20,
        WaterInGasPhase = 21,
        WaterInWaterPhase = 22,
        CO2Mass = 23,
        CO2MassInWaterPhase = 24,
        CO2MassInGasPhase = 25,
        CO2MassInGasPhaseInMob = 26,
        CO2MassInGasPhaseMob = 27,
        CO2MassInGasPhaseInMobKrg = 28,
        CO2MassInGasPhaseMobKrg = 29,
        CO2MassInGasPhaseEffectiveTrapped = 30,
        CO2MassInGasPhaseEffectiveUnTrapped = 31,
        CO2MassInGasPhaseMaximumTrapped = 32,
        CO2MassInGasPhaseMaximumUnTrapped = 33,
        MicrobialMass = 34,
        OxygenMass = 35,
        UreaMass = 36,
        BiofilmMass = 37,
        CalciteMass = 38,
    };

    /// Create non-defaulted object suitable for testing the serialisation
    /// operation.
    static Inplace serializationTestObject();

    /// Converts phase enum to ECL textual representation.
    static std::string EclString(const Phase phase);

    /// Assign value of particular quantity in specific region of named
    /// region set.
    ///
    /// \param[in] region Region set name such as FIPNUM or FIPABC.
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \param[in] region_number Region ID for which to assign a new
    ///   in-place quantity value.
    ///
    /// \param[in] value Numerical value of \p phase quantity in \p
    ///   region_number region of \p region region set.
    void add(const std::string& region,
             Phase              phase,
             std::size_t        region_number,
             double             value);

    /// Assign field-level value of particular quantity.
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \param[in] value Numerical value of field-level \p phase quantity.
    void add(Phase phase, double value);

    /// Retrieve numerical value of particular quantity in specific region
    /// of named region set.
    ///
    /// This function will throw an exception if the requested value has not
    /// been assigned in a previous call to add().
    ///
    /// \param[in] region Region set name such as FIPNUM or FIPABC.
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \param[in] region_number Region ID for which to retrieve a the
    ///   in-place quantity value.
    ///
    /// \return Numerical value of \p phase quantity in \p region_number
    ///   region of \p region region set.
    double get(const std::string& region,
               Phase              phase,
               std::size_t        region_number) const;

    /// Retrieve field-level value of particular quantity.
    ///
    /// This function will throw an exception if the requested value has not
    /// been assigned in a previous call to add().
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \return Numerical value of field-level \p phase quantity.
    double get(Phase phase) const;

    /// Check existence of particular quantity in specific region of named
    /// region set.
    ///
    /// \param[in] region Region set name such as FIPNUM or FIPABC.
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \param[in] region_number Region ID for which to check existence of
    ///   the in-place quantity value.
    ///
    /// \return Whether or not a value of the specific quantity exists in
    ///   the specific region of the named region set.
    bool has(const std::string& region,
             Phase              phase,
             std::size_t        region_number) const;

    /// Check existence of specific field-level quantity.
    ///
    /// \param[in] phase In-place quantity.
    ///
    /// \return Whether or not a value of the specific quantity exists at
    ///   field level.
    bool has(Phase phase) const;

    /// Retrieve the maximum region ID registered across all quantities in
    /// all registered region sets.
    std::size_t max_region() const;

    /// Retrieve the maximum region ID across all quantities registered for
    /// a specific region set.
    ///
    /// \param[in] region_name Region set name such as FIPNUM or FIPABC.
    ///
    /// \return Maximum region ID across all quantities registered for \p
    ///   region_name.  Zero if \p region_name does not have any registered
    ///   quantities in this in-place quantity collection.
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

    /// Get iterable list of all quantities which can be handled/updated in
    /// a generic way.
    static const std::vector<Phase>& phases();

    /// Get iterable list of all quantities, other than "pure" phases, which
    /// can be handled/updated in a generic way.
    static const std::vector<Phase>& mixingPhases();

    /// Serialisation interface.
    ///
    /// \tparam Serializer Object serialisation protocol implementation
    ///
    /// \param[in,out] serializer Serialisation object
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(phase_values);
    }

    /// Equality predicate.
    ///
    /// \param[in] rhs Object against which \c *this will be compared for
    /// equality.
    ///
    /// \return Whether or not \c *this equals \p rhs.
    bool operator==(const Inplace& rhs) const;

private:
    using ValueMap = std::unordered_map<std::size_t, double>;
    using PhaseMap = std::unordered_map<Phase, ValueMap>;
    using RegionMap = std::unordered_map<std::string, PhaseMap>;

    /// Numerical values of all registered quantities in all registered
    /// region sets.
    RegionMap phase_values{};
};

} // namespace Opm

#endif // ORIGINAL_OIP
