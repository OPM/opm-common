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

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {

template <class Scalar, class IndexTraits>
void BlackOilFluidSystem<Scalar,IndexTraits>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    size_t numRegions = eclState.runspec().tabdims().getNumPVTTables();
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

    // set the surface conditions using the STCOND keyword
    surfaceTemperature = eclState.getTableManager().stCond().temperature;
    surfacePressure = eclState.getTableManager().stCond().pressure;

    // The reservoir temperature does not really belong into the table manager. TODO:
    // change this in opm-parser
    setReservoirTemperature(eclState.getTableManager().rtemp());

    // this fluidsystem only supports two or three phases
    assert(numActivePhases_ >= 1 && numActivePhases_ <= 3);

    setEnableDissolvedGas(eclState.getSimulationConfig().hasDISGAS());
    setEnableVaporizedOil(eclState.getSimulationConfig().hasVAPOIL());
    setEnableVaporizedWater(eclState.getSimulationConfig().hasVAPWAT());

    if (eclState.getSimulationConfig().hasDISGASW()) {
        if (eclState.runspec().co2Storage())
            setEnableDissolvedGasInWater(eclState.getSimulationConfig().hasDISGASW());
        else
            throw std::runtime_error("DISGASW only supported in combination with CO2STORE");
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
                molarMass_[regionIdx][oilCompIdx] = BrineCo2Pvt<Scalar>::Brine::molarMass();
            assert(phaseIsActive(gasPhaseIdx));
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
            assert(diffCoeffTables.size() == numRegions);
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& diffCoeffTable = diffCoeffTables[regionIdx];
                molarMass_[regionIdx][oilCompIdx] = diffCoeffTable.oil_mw;
                molarMass_[regionIdx][gasCompIdx] = diffCoeffTable.gas_mw;
                setDiffusionCoefficient(diffCoeffTable.gas_in_gas, gasCompIdx, gasPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.oil_in_gas, oilCompIdx, gasPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.gas_in_oil, gasCompIdx, oilPhaseIdx, regionIdx);
                setDiffusionCoefficient(diffCoeffTable.oil_in_oil, oilCompIdx, oilPhaseIdx, regionIdx);
                if (diffCoeffTable.gas_in_oil_cross_phase > 0 || diffCoeffTable.oil_in_oil_cross_phase > 0) {
                    throw std::runtime_error("Cross phase diffusion is set in the deck, but not implemented in Flow. "
                                             "Please default DIFFC item 7 and item 8 or set it to zero.");
                }
            }
        }
    }
}

template class BlackOilFluidSystem<double,BlackOilDefaultIndexTraits>;
template class BlackOilFluidSystem<float,BlackOilDefaultIndexTraits>;

} // namespace Opm
