#ifndef OPM_PHASEUSAGEINFO_HPP
#define OPM_PHASEUSAGEINFO_HPP

#if HAVE_ECL_INPUT
#include <opm/common/ErrorMacros.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#endif

#include <array>

#include <fmt/format.h>

namespace Opm
{

template <class IndexTraits>
class PhaseUsageInfo {
public:
    static constexpr int numPhases = 3;

    static constexpr int waterPhaseIdx = IndexTraits::waterPhaseIdx;
    static constexpr int oilPhaseIdx = IndexTraits::oilPhaseIdx;
    static constexpr int gasPhaseIdx = IndexTraits::gasPhaseIdx;

    //! Index of the oil component
    static constexpr int oilCompIdx = IndexTraits::oilCompIdx;
    //! Index of the water component
    static constexpr int waterCompIdx = IndexTraits::waterCompIdx;
    //! Index of the gas component
    static constexpr int gasCompIdx = IndexTraits::gasCompIdx;

    [[nodiscard]] unsigned numActivePhases() const {
        return numActivePhases_;
    }

    [[nodiscard]] bool phaseIsActive(unsigned phaseIdx) const {
        assert(phaseIdx < numPhases);
        return phaseIsActive_[phaseIdx];
    }

    void initBegin() {
        numActivePhases_ = numPhases;
        std::fill_n(&phaseIsActive_[0], numPhases, true);
    }

    void initEnd() {
        int activePhaseIdx = 0;
        for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            if(phaseIsActive_[phaseIdx]){
                this->canonicalToActivePhaseIdx_[phaseIdx] = activePhaseIdx;
                this->activeToCanonicalPhaseIdx_[activePhaseIdx] = phaseIdx;
                activePhaseIdx++;
            }
        }
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

#if HAVE_ECL_INPUT
    void initFromState(const EclipseState& eclState)
    {
        // TODO: check some existing code, whether it is necessary and sensible
 //       initBegin();
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

        // we only support one, two or three phases
        if (numActivePhases_ < 1 || numActivePhases_ > 3) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Fluidsystem and PhaseUsageInfo supports 1-3 phases, but {} is active\n",
                                  numActivePhases_));
        }

        const auto& phases = eclState.runspec().phases();

        has_solvent = phases.active(Phase::SOLVENT);
        has_polymer = phases.active(Phase::POLYMER);
        has_energy = phases.active(Phase::ENERGY);
        has_polymermw = phases.active(Phase::POLYMW);
        has_foam = phases.active(Phase::FOAM);
        has_brine = phases.active(Phase::BRINE);
        has_zFraction = phases.active(Phase::ZFRACTION);

        has_micp = eclState.runspec().micp();
        has_co2_or_h2store = eclState.runspec().co2Storage() || eclState.runspec().h2Storage();
//        initEnd();
    }
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
    std::array<bool, numPhases> phaseIsActive_ {{false, false, false}};
    std::array<short, numPhases> activeToCanonicalPhaseIdx_ {{0, 1, 2}};
    std::array<short, numPhases> canonicalToActivePhaseIdx_ {{0, 1, 2}};

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
};

}

#endif