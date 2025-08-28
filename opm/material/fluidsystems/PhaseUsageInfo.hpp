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

#ifndef OPM_PHASEUSAGEINFO_HPP
#define OPM_PHASEUSAGEINFO_HPP

#if HAVE_ECL_INPUT
#include <opm/common/ErrorMacros.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#endif

#include <array>
#include <cassert>

namespace Opm
{

template <typename IndexTraits>
class PhaseUsageInfo {
public:
    static constexpr int numPhases = IndexTraits::numPhases;

    static constexpr int waterPhaseIdx = IndexTraits::waterPhaseIdx;
    static constexpr int oilPhaseIdx = IndexTraits::oilPhaseIdx;
    static constexpr int gasPhaseIdx = IndexTraits::gasPhaseIdx;

    PhaseUsageInfo();

    [[nodiscard]] unsigned numActivePhases() const {
        return numActivePhases_;
    }

    [[nodiscard]] bool phaseIsActive(unsigned phaseIdx) const {
        assert(phaseIdx < numPhases);
        return phaseIsActive_[phaseIdx];
    }

    [[nodiscard]] short canonicalToActivePhaseIdx(unsigned phaseIdx) const {
        assert(phaseIdx<numPhases);
        if (!phaseIsActive(phaseIdx)) return -1;  // old phase_used return -1
        return canonicalToActivePhaseIdx_[phaseIdx];
    }

    [[nodiscard]] short activeToCanonicalPhaseIdx(unsigned activePhaseIdx) const {
        assert(activePhaseIdx< numActivePhases_);
        return activeToCanonicalPhaseIdx_[activePhaseIdx];
    }

    void initFromPhases(const Phases& phases);

#if HAVE_ECL_INPUT
    void initFromState(const EclipseState& eclState);
#endif

    bool hasSolvent() const noexcept {
        return has_solvent;
    }

    bool hasPolymer() const noexcept {
        return has_polymer;
    }

    bool hasEnergy() const noexcept {
        return has_energy;
    }

    bool hasPolymerMW() const noexcept {
        return has_polymermw;
    }

    bool hasFoam() const noexcept {
        return has_foam;
    }

    bool hasBrine() const noexcept {
        return has_brine;
    }

    bool hasZFraction() const noexcept {
       return has_zFraction;
    }

    bool hasMICP() const noexcept {
        return has_micp;
    }

    bool hasCO2orH2Store() const noexcept {
        return has_co2_or_h2store;
    }

private:
    unsigned char numActivePhases_ = 0;
    std::array<bool, numPhases> phaseIsActive_;
    std::array<short, numPhases> activeToCanonicalPhaseIdx_;
    std::array<short, numPhases> canonicalToActivePhaseIdx_;

    bool has_solvent{};
    bool has_polymer{};
    bool has_energy{};
    // polymer molecular weight
    bool has_polymermw{};
    bool has_foam{};
    bool has_brine{};
    bool has_zFraction{};
    bool has_micp{};
    bool has_co2_or_h2store{};

    //  updating the mapping between active and canonical phase indices
    void updateIndexMapping_();

};

}

#endif
