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

#if HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <opm/material/fluidsystems/PhaseUsageInfo.hpp>
#include <opm/material/fluidsystems/BlackOilDefaultFluidSystemIndices.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#endif

#include <opm/common/ErrorMacros.hpp>
#include <opm/input/eclipse/EclipseState/Phase.hpp>

#include <algorithm>
#include <stdexcept>

#include <fmt/format.h>

namespace Opm {

template <typename IndexTraits>
PhaseUsageInfo<IndexTraits>::PhaseUsageInfo()
{
    reset_();
}

template <typename IndexTraits>
void PhaseUsageInfo<IndexTraits>::updateIndexMapping_() {
    int activePhaseIdx = 0;
    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        if(phaseIsActive_[phaseIdx]){
            this->canonicalToActivePhaseIdx_[phaseIdx] = activePhaseIdx;
            this->activeToCanonicalPhaseIdx_[activePhaseIdx] = phaseIdx;
            activePhaseIdx++;
        }
    }

    int activeCompIdx = 0;
    for (unsigned c = 0; c < numComponents; ++c) {
        // TODO: use phaseIsActive for components checking is a temporary workaround for now
        if (phaseIsActive(IndexTraits::componentToPhaseIdx(c))) {
            activeToCanonicalCompIdx_[activeCompIdx] = c;
            canonicalToActiveCompIdx_[c] = activeCompIdx;
            ++activeCompIdx;
        }
    }
}

template <typename IndexTraits>
void PhaseUsageInfo<IndexTraits>::reset_() {
    numActivePhases_ = 0;
    std::fill_n(&phaseIsActive_[0], numPhases, false);
    std::fill_n(&canonicalToActivePhaseIdx_[0], numPhases, -1);
    std::fill_n(&activeToCanonicalPhaseIdx_[0], numPhases, -1);

    std::fill_n(&activeToCanonicalCompIdx_[0], numComponents, -1);
    std::fill_n(&canonicalToActiveCompIdx_[0], numComponents, -1);
}

#if HAVE_ECL_INPUT
template <typename IndexTraits>
void PhaseUsageInfo<IndexTraits>::initFromState(const EclipseState& eclState)
{
    const auto& phases = eclState.runspec().phases();
    initFromPhases(phases);

    has_biofilm = eclState.runspec().biof();
    has_micp = eclState.runspec().micp();
    has_co2_or_h2store = eclState.runspec().co2Storage() || eclState.runspec().h2Storage();
    enable_dissolved_gas_ = eclState.getSimulationConfig().hasDISGAS();
    enable_vaporized_oil_ = eclState.getSimulationConfig().hasVAPOIL();
    enable_vaporized_water_ = eclState.getSimulationConfig().hasVAPWAT();
    const bool disgasw = eclState.getSimulationConfig().hasDISGASW();
    if (disgasw) {
        if (has_co2_or_h2store) {
            enable_dissolved_gas_in_water_ = true;
        } else if (eclState.runspec().co2Sol() || eclState.runspec().h2Sol()) {
            // For CO2SOL and H2SOL the dissolved gas in water is added in the solvent model
            // The HC gas is not allowed to dissolve into water.
            // For most HC gases this is a reasonable assumption.
            OpmLog::info("CO2SOL/H2SOL is activated together with DISGASW.\n"
                         "Only CO2/H2 is allowed to dissolve into water");
        } else {
            OPM_THROW(std::runtime_error,
                      "DISGASW is only supported in combination with CO2STORE, H2STORE, CO2SOL, or H2SOL.");
        }
    }
}

template <typename IndexTraits>
void PhaseUsageInfo<IndexTraits>::initFromPhases(const Phases& phases) {

    this->reset_();

    if (phases.active(Phase::OIL)) {
        phaseIsActive_[oilPhaseIdx] = true;
        ++numActivePhases_;
    }
    if (phases.active(Phase::GAS)) {
        phaseIsActive_[gasPhaseIdx] = true;
        ++numActivePhases_;
    }
    if (phases.active(Phase::WATER)) {
        phaseIsActive_[waterPhaseIdx] = true;
        ++numActivePhases_;
    }

    // \Note: there is a test using default EclipseState, we allow numActivePhases_ == 0
    // \Note: to let that test pass.
    if (numActivePhases_ > 3) {
         OPM_THROW(std::runtime_error,
                   fmt::format("Fluidsystem and PhaseUsageInfo supports up to 3 phases, but {} is active\n",
                               numActivePhases_));
    }

    has_solvent = phases.active(Phase::SOLVENT);
    has_polymer = phases.active(Phase::POLYMER);
    has_energy = phases.active(Phase::ENERGY);
    has_polymermw = phases.active(Phase::POLYMW);
    has_foam = phases.active(Phase::FOAM);
    has_brine = phases.active(Phase::BRINE);
    has_zFraction = phases.active(Phase::ZFRACTION);

    this->updateIndexMapping_();
    this->updateIndices_();
}
#endif

template <typename IndexTraits>
void PhaseUsageInfo<IndexTraits>::updateIndices_() {
    contiSolventEqIdx_ = has_solvent ? numActivePhases_ : -1000;
}

// Explicit template instantiations for commonly used IndexTraits
template class PhaseUsageInfo<BlackOilDefaultFluidSystemIndices>;

} // namespace Opm
