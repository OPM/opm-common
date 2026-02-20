// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/

#include <config.h>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>

#include <opm/common/ErrorMacros.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

#include <string_view>

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template <class Scalar, class IndexTraits, template<typename> typename Storage>
void BlackOilFluidSystem<Scalar,IndexTraits, Storage>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    if (eclState.getSimulationConfig().useEnthalpy()) {
        enthalpy_eq_energy_ = false;
    } else {
        enthalpy_eq_energy_ = true;
    }
    std::size_t num_regions = eclState.runspec().tabdims().getNumPVTTables();
    initBegin(num_regions);

    phaseUsageInfo_.initFromState(eclState);

    // set the surface conditions using the STCOND keyword
    surfaceTemperature = eclState.getTableManager().stCond().temperature;
    surfacePressure = eclState.getTableManager().stCond().pressure;

    // The reservoir temperature does not really belong into the table manager. TODO:
    // change this in opm-parser
    setReservoirTemperature(eclState.getTableManager().rtemp());

    setEnableDissolvedGas(eclState.getSimulationConfig().hasDISGAS());
    setEnableVaporizedOil(eclState.getSimulationConfig().hasVAPOIL());
    setEnableVaporizedWater(eclState.getSimulationConfig().hasVAPWAT());

    if (eclState.getSimulationConfig().hasDISGASW()) {
        if (eclState.runspec().co2Storage() || eclState.runspec().h2Storage())
            setEnableDissolvedGasInWater(eclState.getSimulationConfig().hasDISGASW());
        else if (eclState.runspec().co2Sol() || eclState.runspec().h2Sol()) {
            // For CO2SOL and H2SOL the dissolved gas in water is added in the solvent model
            // The HC gas is not allowed to dissolved into water.
            // For most HC gasses this is a resonable assumption.
            OpmLog::info("CO2SOL/H2SOL is activated together with DISGASW. \n"
                         "Only CO2/H2 is allowed to dissolve into water");
        } else
            OPM_THROW(std::runtime_error,
                      "DISGASW only supported in combination with CO2STORE/H2STORE or CO2SOL/H2SOL");
    }

    if (phaseIsActive(gasPhaseIdx)) {
        gasPvt_.initFromState(eclState, schedule);
    }

    if (phaseIsActive(oilPhaseIdx)) {
        oilPvt_.initFromState(eclState, schedule);
    }

    if (phaseIsActive(waterPhaseIdx)) {
        waterPvt_.initFromState(eclState, schedule);
    }

    // set if we are using constrs tables
   if ((!eclState.getSimulationConfig().hasDISGAS()) && (!phaseIsActive(gasPhaseIdx))) {
      const auto& rsConstTables = eclState.getTableManager().getRsconstTables();
      if (!rsConstTables.empty()) {
          setEnableConstantRs(oilPvt_.approach() == OilPvtApproach::ConstantRsDeadOil);
      }
   }

    // set the reference densities of all PVT regions
    for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
        setReferenceDensities(oilPvt_.approach()   == OilPvtApproach::NoOil     ? 700.0  : oilPvt_.oilReferenceDensity(regionIdx),
                              waterPvt_.approach() == WaterPvtApproach::NoWater ? 1000.0 : waterPvt_.waterReferenceDensity(regionIdx),
                              gasPvt_.approach()   == GasPvtApproach::NoGas     ? 2.0    : gasPvt_.gasReferenceDensity(regionIdx),
                              regionIdx);
    }

    // set default molarMass and mappings
    initEnd();

    // use molarMass of CO2 and Brine as default
    // when we are using the the CO2STORE option
    if (eclState.runspec().co2Storage()) {
        const Scalar salinity = eclState.getCo2StoreConfig().salinity();  // mass fraction
        for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
            if (phaseIsActive(oilPhaseIdx)) // The oil component is used for the brine if OIL is active
                molarMass_[regionIdx][oilCompIdx] = BrineCo2Pvt<Scalar>::Brine::molarMass(salinity);
            if (phaseIsActive(waterPhaseIdx))
                molarMass_[regionIdx][waterCompIdx] = BrineCo2Pvt<Scalar>::Brine::molarMass(salinity);
            if (!phaseIsActive(gasPhaseIdx)) {
                OPM_THROW(std::runtime_error,
                          "CO2STORE requires gas phase\n");
            }
            molarMass_[regionIdx][gasCompIdx] = BrineCo2Pvt<Scalar>::CO2::molarMass();
        }
    }

    // Use molar mass of H2 and Brine as default in H2STORE keyword
    if (eclState.runspec().h2Storage()) {
        // Salinity in mass fraction
        const Scalar molality = eclState.getTableManager().salinity(); // mol/kg
        const Scalar MmNaCl = 58.44e-3; // molar mass of NaCl [kg/mol]
        const Scalar salinity = 1 / ( 1 + 1 / (molality*MmNaCl));
        for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
            if (phaseIsActive(oilPhaseIdx)) // The oil component is used for the brine if OIL is active
                molarMass_[regionIdx][oilCompIdx] = BrineH2Pvt<Scalar>::Brine::molarMass(salinity);
            if (phaseIsActive(waterPhaseIdx))
                molarMass_[regionIdx][waterCompIdx] = BrineH2Pvt<Scalar>::Brine::molarMass(salinity);
            if (!phaseIsActive(gasPhaseIdx)) {
                OPM_THROW(std::runtime_error,
                          "H2STORE requires gas phase\n");
            }
            molarMass_[regionIdx][gasCompIdx] = BrineH2Pvt<Scalar>::H2::molarMass();
        }
    }

    // For co2storage and h2storage we dont have a concept of tables and should not spend time on
    // checking if we are at the saturated front
    setUseSaturatedTables(!(eclState.runspec().h2Storage() || eclState.runspec().co2Storage()));

    setEnableDiffusion(eclState.getSimulationConfig().isDiffusive());
    if (enableDiffusion()) {
        const auto& diffCoeffTables = eclState.getTableManager().getDiffusionCoefficientTable();
        if (!diffCoeffTables.empty()) {
            // if diffusion coefficient table is empty we relay on the PVT model to
            // to give us the coefficients.
            diffusionCoefficients_.resize(num_regions,{0,0,0,0,0,0,0,0,0});
            if (diffCoeffTables.size() != num_regions) {
                OPM_THROW(std::runtime_error,
                          fmt::format("Table sizes mismatch. DiffCoeffs: {}, NumRegions: {}\n",
                                      diffCoeffTables.size(), num_regions));
            }
            for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
                const auto& diffCoeffTable = diffCoeffTables[regionIdx];
                molarMass_[regionIdx][oilCompIdx] = diffCoeffTable.oil_mw;
                molarMass_[regionIdx][gasCompIdx] = diffCoeffTable.gas_mw;
                setDiffusionCoefficient(diffCoeffTable.gas_in_gas, gasCompIdx, gasPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.oil_in_gas, oilCompIdx, gasPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.gas_in_oil, gasCompIdx, oilPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.oil_in_oil, oilCompIdx, oilPhaseIdx, regionIdx);
                if (diffCoeffTable.gas_in_oil_cross_phase > 0 || diffCoeffTable.oil_in_oil_cross_phase > 0) {
                    OPM_THROW(std::runtime_error,
                              "Cross phase diffusion is set in the deck, "
                              "but not implemented in Flow. "
                              "Please default DIFFC item 7 and item 8 "
                              "or set it to zero.");
                }
            }
        } else if ( (eclState.runspec().co2Storage() || eclState.runspec().h2Storage())
                && eclState.runspec().phases().active(Phase::GAS)
                && eclState.runspec().phases().active(Phase::WATER))
        {
            diffusionCoefficients_.resize(num_regions, {0,0,0,0,0,0,0,0,0});
            // diffusion coefficients can be set using DIFFCGAS and DIFFCWAT
            // for CO2STORE and H2STORE cases with gas + water
            const auto& diffCoeffWatTables = eclState.getTableManager().getDiffusionCoefficientWaterTable();
            if (!diffCoeffWatTables.empty()) {
                for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
                    const auto& diffCoeffWatTable = diffCoeffWatTables[regionIdx];
                    setDiffusionCoefficient(diffCoeffWatTable.co2_in_water, gasCompIdx, waterPhaseIdx, regionIdx);
                    setDiffusionCoefficient(diffCoeffWatTable.h2o_in_water, waterCompIdx, waterPhaseIdx, regionIdx);
                }
            }
            const auto& diffCoeffGasTables = eclState.getTableManager().getDiffusionCoefficientGasTable();
            if (!diffCoeffGasTables.empty()) {
                for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
                    const auto& diffCoeffGasTable = diffCoeffGasTables[regionIdx];
                    setDiffusionCoefficient(diffCoeffGasTable.co2_in_gas, gasCompIdx, gasPhaseIdx, regionIdx);
                    setDiffusionCoefficient(diffCoeffGasTable.h2o_in_gas, waterCompIdx, gasPhaseIdx, regionIdx);
                }
            }
        }
    }
}
#endif

#define INSTANTIATE_TYPE(T) \
    template<> PhaseUsageInfo<BlackOilDefaultFluidSystemIndices> \
               BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::phaseUsageInfo_ = {};   \
    template<> T BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::surfaceTemperature = 0.0; \
    template<> T BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::surfacePressure = 0.0; \
    template<> T BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::reservoirTemperature_ = 0.0; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableDissolvedGas_ = true; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableDissolvedGasInWater_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableConstantRs_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableVaporizedOil_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableVaporizedWater_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enableDiffusion_ = false; \
    template<> OilPvtMultiplexer<T> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::oilPvt_ = {}; \
    template<> GasPvtMultiplexer<T> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::gasPvt_ = {}; \
    template<> WaterPvtMultiplexer<T> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::waterPvt_ = {}; \
    template<> std::vector<std::array<T, 3>> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::referenceDensity_ = {}; \
    template<> std::vector<std::array<T, 3>> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::molarMass_ = {}; \
    template<> std::vector<std::array<T, 9>> \
        BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::diffusionCoefficients_ = {}; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::isInitialized_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::useSaturatedTables_ = false; \
    template<> bool BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>::enthalpy_eq_energy_ = false; \
    template class BlackOilFluidSystem<T, BlackOilDefaultFluidSystemIndices, VectorWithDefaultAllocator>;
    // IMPORTANT: The class must be instantiated after the template template specializations
    //    or else the static variable above will appear as undefined in the generated object file.

INSTANTIATE_TYPE(double)
INSTANTIATE_TYPE(float)

} // namespace Opm
