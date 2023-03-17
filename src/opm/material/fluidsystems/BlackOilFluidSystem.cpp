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

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    std::size_t numRegions = eclState.runspec().tabdims().getNumPVTTables();
    initBegin(numRegions);

    numActivePhases_ = 0;
    std::fill_n(&phaseIsActive_[0], numPhases, false);

    if (eclState.runspec().phases().active(Phase::OIL)) {
        phaseIsActive_[oilPhaseIdx] = true;
        ++numActivePhases_;
    }

    if (eclState.runspec().phases().active(Phase::GAS)) {
        phaseIsActive_[gasPhaseIdx] = true;
        ++numActivePhases_;
    }

    if (eclState.runspec().phases().active(Phase::WATER)) {
        phaseIsActive_[waterPhaseIdx] = true;
        ++numActivePhases_;
    }

    // this fluidsystem only supports one, two or three phases
    if (numActivePhases_ < 1 || numActivePhases_ > 3) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Fluidsystem supports 1-3 phases, but {} is active\n",
                              numActivePhases_));
    }

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
        if (eclState.runspec().co2Storage())
            setEnableDissolvedGasInWater(eclState.getSimulationConfig().hasDISGASW());
        else
            OPM_THROW(std::runtime_error,
                      "DISGASW only supported in combination with CO2STORE");
    }

    if (phaseIsActive(gasPhaseIdx)) {
        gasPvt_ = std::make_shared<GasPvt>();
        gasPvt_->initFromState(eclState, schedule);
    }

    if (phaseIsActive(oilPhaseIdx)) {
        oilPvt_ = std::make_shared<OilPvt>();
        oilPvt_->initFromState(eclState, schedule);
    }

    if (phaseIsActive(waterPhaseIdx)) {
        waterPvt_ = std::make_shared<WaterPvt>();
        waterPvt_->initFromState(eclState, schedule);
    }

    // set the reference densities of all PVT regions
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        setReferenceDensities(oilPvt_ ? oilPvt_->oilReferenceDensity(regionIdx) : 700.0,
                              waterPvt_ ? waterPvt_->waterReferenceDensity(regionIdx) : 1000.0,
                              gasPvt_ ? gasPvt_->gasReferenceDensity(regionIdx) : 2.0,
                              regionIdx);
    }

    // set default molarMass and mappings
    initEnd();

    // use molarMass of CO2 and Brine as default
    // when we are using the the CO2STORE option
    if (eclState.runspec().co2Storage()) {
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            if (phaseIsActive(oilPhaseIdx)) // The oil component is used for the brine if OIL is active
                molarMass_[regionIdx][oilCompIdx] = BrineCo2Pvt<Scalar>::Brine::molarMass();
            if (phaseIsActive(waterPhaseIdx))
                molarMass_[regionIdx][waterCompIdx] = BrineCo2Pvt<Scalar>::Brine::molarMass();
            if (!phaseIsActive(gasPhaseIdx)) {
                OPM_THROW(std::runtime_error,
                          "CO2STORE requires gas phase\n");
            }
            molarMass_[regionIdx][gasCompIdx] = BrineCo2Pvt<Scalar>::CO2::molarMass();
        }
    }

    setEnableDiffusion(eclState.getSimulationConfig().isDiffusive());
    if (enableDiffusion()) {
        const auto& diffCoeffTables = eclState.getTableManager().getDiffusionCoefficientTable();
        if (!diffCoeffTables.empty()) {
            // if diffusion coefficient table is empty we relay on the PVT model to
            // to give us the coefficients.
            diffusionCoefficients_.resize(numRegions,{0,0,0,0,0,0,0,0,0});
            if (diffCoeffTables.size() != numRegions) {
                OPM_THROW(std::runtime_error,
                          fmt::format("Table sizes mismatch. DiffCoeffs: {}, NumRegions: {}\n",
                                      diffCoeffTables.size(), numRegions));
            }
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
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
        }
    }
}
#endif

template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::
initBegin(std::size_t numPvtRegions)
{
    isInitialized_ = false;

    enableDissolvedGas_ = true;
    enableDissolvedGasInWater_ = false;
    enableVaporizedOil_ = false;
    enableVaporizedWater_ = false;
    enableDiffusion_ = false;

    oilPvt_ = nullptr;
    gasPvt_ = nullptr;
    waterPvt_ = nullptr;

    surfaceTemperature = 273.15 + 15.56; // [K]
    surfacePressure = 1.01325e5; // [Pa]
    setReservoirTemperature(surfaceTemperature);

    numActivePhases_ = numPhases;
    std::fill_n(&phaseIsActive_[0], numPhases, true);

    resizeArrays_(numPvtRegions);
}

template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::
setReferenceDensities(Scalar rhoOil,
                      Scalar rhoWater,
                      Scalar rhoGas,
                      unsigned regionIdx)
{
    referenceDensity_[regionIdx][oilPhaseIdx] = rhoOil;
    referenceDensity_[regionIdx][waterPhaseIdx] = rhoWater;
    referenceDensity_[regionIdx][gasPhaseIdx] = rhoGas;
}

template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::initEnd()
{
    // calculate the final 2D functions which are used for interpolation.
    std::size_t numRegions = molarMass_.size();
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
        // calculate molar masses

        // water is simple: 18 g/mol
        molarMass_[regionIdx][waterCompIdx] = 18e-3;

        if (phaseIsActive(gasPhaseIdx)) {
            // for gas, we take the density at standard conditions and assume it to be ideal
            Scalar p = surfacePressure;
            Scalar T = surfaceTemperature;
            Scalar rho_g = referenceDensity_[/*regionIdx=*/0][gasPhaseIdx];
            molarMass_[regionIdx][gasCompIdx] = Constants<Scalar>::R*T*rho_g / p;
        }
        else
            // hydrogen gas. we just set this do avoid NaNs later
            molarMass_[regionIdx][gasCompIdx] = 2e-3;

        // finally, for oil phase, we take the molar mass from the spe9 paper
        molarMass_[regionIdx][oilCompIdx] = 175e-3; // kg/mol
    }


    int activePhaseIdx = 0;
    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        if(phaseIsActive(phaseIdx)){
            canonicalToActivePhaseIdx_[phaseIdx] = activePhaseIdx;
            activeToCanonicalPhaseIdx_[activePhaseIdx] = phaseIdx;
            activePhaseIdx++;
        }
    }
    isInitialized_ = true;
}

template <class Scalar, class IndexTraits>
const char* BlackOilFluidSystem<Scalar,IndexTraits>::
phaseName(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        return "water";
    case oilPhaseIdx:
        return "oil";
    case gasPhaseIdx:
        return "gas";

    default:
        throw std::logic_error(fmt::format("Phase index {} is unknown", phaseIdx));
    }
}

template <class Scalar, class IndexTraits>
unsigned BlackOilFluidSystem<Scalar,IndexTraits>::
solventComponentIndex(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        return waterCompIdx;
    case oilPhaseIdx:
        return oilCompIdx;
    case gasPhaseIdx:
        return gasCompIdx;

    default:
        throw std::logic_error(fmt::format("Phase index {} is unknown", phaseIdx));
    }
}

template <class Scalar, class IndexTraits>
unsigned BlackOilFluidSystem<Scalar,IndexTraits>::
soluteComponentIndex(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        if (enableDissolvedGasInWater())
            return gasCompIdx;
        throw std::logic_error("The water phase does not have any solutes in the black oil model!");
    case oilPhaseIdx:
        return gasCompIdx;
    case gasPhaseIdx:
        return oilCompIdx;

    default:
        throw std::logic_error(fmt::format("Phase index {} is unknown", phaseIdx));
    }
}

template <class Scalar, class IndexTraits>
const char* BlackOilFluidSystem<Scalar,IndexTraits>::
componentName(unsigned compIdx)
{
    switch (compIdx) {
    case waterCompIdx:
        return "Water";
    case oilCompIdx:
        return "Oil";
    case gasCompIdx:
        return "Gas";

    default:
        throw std::logic_error(fmt::format("Component index {} is unknown", compIdx));
    }
}

template <class Scalar, class IndexTraits>
short BlackOilFluidSystem<Scalar,IndexTraits>::
activeToCanonicalPhaseIdx(unsigned activePhaseIdx)
{
    assert(activePhaseIdx<numActivePhases());
    return activeToCanonicalPhaseIdx_[activePhaseIdx];
}

template <class Scalar, class IndexTraits>
short BlackOilFluidSystem<Scalar,IndexTraits>::
canonicalToActivePhaseIdx(unsigned phaseIdx)
{
    assert(phaseIdx<numPhases);
    assert(phaseIsActive(phaseIdx));
    return canonicalToActivePhaseIdx_[phaseIdx];
}

template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::
resizeArrays_(std::size_t numRegions)
{
    molarMass_.resize(numRegions);
    referenceDensity_.resize(numRegions);
}

template class BlackOilFluidSystem<double,BlackOilDefaultIndexTraits>;
template class BlackOilFluidSystem<float,BlackOilDefaultIndexTraits>;

} // namespace Opm
