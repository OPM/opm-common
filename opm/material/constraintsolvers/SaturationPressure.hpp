// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital

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
/*!
 * \file
 * \copydoc Opm::SaturationPressure
 */
#ifndef OPM_SATURATION_PRESSURE_HPP
#define OPM_SATURATION_PRESSURE_HPP

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace Opm {

/*!
 * \brief Computes the saturation pressure of a mixture at a given temperature
 *        from the cubic equation of state.
 *
 * The bubble-point (dew-point) pressure of a liquid (vapour) with composition
 * \c z is the pressure where the incipient vapour (liquid) phase appears.  The
 * equilibrium ratios K are obtained by successive substitution on the fugacity
 * coefficient ratios at fixed pressure, and the pressure is updated to drive
 * the total amount of the incipient phase, sum_c K_c z_c (or sum_c z_c / K_c),
 * towards one.  The pressure is approached from the single-phase side: when the
 * substitution collapses onto the trivial solution K == 1, the pressure is
 * moved into the two-phase region and the iteration restarted from the Wilson
 * estimate.  Pressures known to be single-phase and known to be two-phase are
 * kept as a bracket around the saturation pressure, and the search bisects
 * within it once both are known, so that a narrow phase envelope is not
 * stepped over.
 *
 * A reservoir gas generally has two dew points at a given temperature.  The
 * upper (retrograde) one is the boundary crossed when the reservoir pressure
 * declines, and it is the one the reference simulator reports as the
 * saturation pressure of a gas.  To match that reference behaviour the
 * dew-point search targets the retrograde branch first and only falls back to
 * the lower branch when no retrograde dew point is found.  This remains to be
 * revisited if the decision changes.
 */
template <class Scalar, class FluidSystem>
class SaturationPressure
{
    static constexpr int numComponents = FluidSystem::numComponents;
    static constexpr int oilPhaseIdx = FluidSystem::oilPhaseIdx;
    static constexpr int gasPhaseIdx = FluidSystem::gasPhaseIdx;

    using EOSType = CompositionalConfig::EOSType;

public:
    using CompVec = std::array<Scalar, numComponents>;

    /// Computes the bubble-point pressure of a liquid with composition \p liquid at
    /// temperature \p temp, along with the equilibrium \p vapor composition.
    /// \return whether the calculation converged
    static bool bubblePressure(const CompVec& liquid,
                               const Scalar temp,
                               const EOSType eosType,
                               Scalar& press,
                               CompVec& vapor)
    { return solve_(liquid, temp, eosType, Mode::Bubble, press, vapor) == Outcome::Converged; }

    /// Computes the dew-point pressure of a vapour with composition \p vapor at
    /// temperature \p temp, along with the equilibrium \p liquid composition.
    /// The upper (retrograde) dew point is preferred; the lower one is only
    /// searched when the upper branch provably has no root, so a plain
    /// convergence failure reports false rather than the wrong branch.
    /// On failure \p press is left untouched while \p liquid holds iteration
    /// scratch and must not be read.
    /// \return whether the calculation converged
    static bool dewPressure(const CompVec& vapor,
                            const Scalar temp,
                            const EOSType eosType,
                            Scalar& press,
                            CompVec& liquid)
    {
        // The lower branch is only a fallback for a mixture that has no upper
        // (retrograde) dew point.  After a plain convergence failure it stays
        // untried: reporting the lower branch then could return the wrong dew
        // point of a mixture that does have both.
        switch (solve_(vapor, temp, eosType, Mode::DewUpper, press, liquid)) {
        case Outcome::Converged:
            return true;
        case Outcome::NoRoot:
            return solve_(vapor, temp, eosType, Mode::DewLower, press, liquid)
                == Outcome::Converged;
        case Outcome::GaveUp:
            return false;
        }
        return false;
    }

private:
    // What a branch search established: a converged saturation point, positive
    // evidence that the branch has no genuine root, or an exhausted iteration
    // from which nothing can be concluded.
    enum class Outcome { Converged, NoRoot, GaveUp };

    // The saturation-pressure branch being searched.  The bubble point and the
    // upper (retrograde) dew point are approached from the high-pressure side,
    // the lower dew point from the low-pressure side.
    enum class Mode { Bubble, DewUpper, DewLower };

    // The Wilson correlation for K_c * press.
    static CompVec wilsonKp_(const Scalar temp)
    {
        CompVec Kp;
        for (int c = 0; c < numComponents; ++c) {
            Kp[c] = FluidSystem::criticalPressure(c) *
                    std::exp(5.373 * (1.0 + FluidSystem::acentricFactor(c)) *
                             (1.0 - FluidSystem::criticalTemperature(c) / temp));
        }
        return Kp;
    }

    static Outcome solve_(const CompVec& z,
                          const Scalar temp,
                          const EOSType eosType,
                          const Mode mode,
                          Scalar& press,
                          CompVec& incipient)
    {
        const CompVec wilsonKp = wilsonKp_(temp);
        const bool bubble = (mode == Mode::Bubble);
        // The bubble point and the retrograde dew point are approached from
        // the high-pressure side, the lower dew point from the low-pressure
        // side.  In every case the single-phase side is the one the scan
        // starts on, and the pressure update moves away from it.
        const bool fromAbove = (mode != Mode::DewLower);

        // The Wilson estimate solves the saturation condition exactly since
        // K ~ 1/p.  The upper dew point is searched from the high-pressure
        // side, so it starts from the same high estimate as the bubble point.
        Scalar p = 0.0;
        if (mode == Mode::DewLower) {
            for (int c = 0; c < numComponents; ++c) {
                p += z[c] / wilsonKp[c];
            }
            p = 1.0 / p;
        }
        else {
            for (int c = 0; c < numComponents; ++c) {
                p += z[c] * wilsonKp[c];
            }
        }

        auto wilsonK = [&wilsonKp](const Scalar pressure) {
            CompVec K;
            std::ranges::transform(wilsonKp, K.begin(),
                                   [pressure](const Scalar Kp) { return Kp / pressure; });
            return K;
        };
        CompVec K = wilsonK(p);

        // The known phase holds z; the incipient phase composition is derived from K.
        const auto knownPhaseIdx = bubble ? oilPhaseIdx : gasPhaseIdx;
        const auto incipientPhaseIdx = bubble ? gasPhaseIdx : oilPhaseIdx;

        CompositionalFluidState<Scalar, FluidSystem> fs;
        fs.setTemperature(temp);
        for (int c = 0; c < numComponents; ++c) {
            fs.setMoleFraction(knownPhaseIdx, c, z[c]);
        }

        // Bracket of the saturation pressure: "single" is a pressure known to
        // lie on the single-phase side, "two" one known to lie inside the
        // two-phase region.  Once both are known the search bisects between
        // them instead of stepping, so the boundary cannot be run past.
        Scalar pSingle{}, pTwo{};
        bool haveSingle = false;
        bool haveTwo = false;
        // Set whenever a scan point ends with the substitution exhausted.  Such
        // a point classifies nothing, so a scan that met one cannot conclude
        // the branch has no root.
        bool anyInconclusive = false;

        // Step used to scan for the two-phase region before the bracket is
        // closed.  It is deliberately fine: a coarse step can cross a narrow
        // phase envelope in one go and leave the mixture looking single-phase
        // on both sides.
        constexpr Scalar scanStep = 0.9;
        constexpr int maxOuter = 200;
        constexpr int maxInner = 100;

        for (int outer = 0; outer < maxOuter; ++outer) {
            fs.setPressure(oilPhaseIdx, p);
            fs.setPressure(gasPhaseIdx, p);

            // Fugacity equality at fixed pressure: K_c = phi_liquid / phi_vapour.
            bool trivial = false;
            bool rootsDistinct = false;
            bool substitutionConverged = false;
            for (int inner = 0; inner < maxInner; ++inner) {
                Scalar sum = 0.0;
                for (int c = 0; c < numComponents; ++c) {
                    incipient[c] = bubble ? K[c] * z[c] : z[c] / K[c];
                    sum += incipient[c];
                }
                for (int c = 0; c < numComponents; ++c) {
                    fs.setMoleFraction(incipientPhaseIdx, c, incipient[c] / sum);
                }

                typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
                paramCache.updatePhase(fs, oilPhaseIdx);
                paramCache.updatePhase(fs, gasPhaseIdx);

                // A pure component or an azeotrope has K = 1 at a genuine
                // saturation point, where the two phases share a composition
                // but occupy different EOS roots.  The molar volumes tell that
                // state apart from the trivial solution, whose phases are one
                // and the same.
                const Scalar vmL = paramCache.molarVolume(oilPhaseIdx);
                const Scalar vmV = paramCache.molarVolume(gasPhaseIdx);
                // The cubic EOS clamps an unphysical root to 1e-7 m^3/mol; a
                // volume at the clamp is no real root, and treating it as a
                // distinct phase would invent a saturation point for a
                // supercritical mixture.
                constexpr Scalar clampedVm = 1.0e-7;
                rootsDistinct = (std::min(vmL, vmV) > 2.0 * clampedVm) &&
                                (std::abs(vmL - vmV) > 1.0e-9 * std::max(vmL, vmV));

                Scalar change = 0.0;
                trivial = true;
                for (int c = 0; c < numComponents; ++c) {
                    const Scalar phiL = FluidSystem::fugacityCoefficient(
                        fs, paramCache, oilPhaseIdx, c);
                    const Scalar phiV = FluidSystem::fugacityCoefficient(
                        fs, paramCache, gasPhaseIdx, c);
                    const Scalar newK = phiL / phiV;
                    // Relative to the magnitude of K: an absolute measure is
                    // unreachable for the large K of a light component.
                    change = std::max(change, std::abs(newK - K[c]) /
                                              std::max(Scalar{1}, std::abs(newK)));
                    trivial = trivial && (std::abs(newK - 1.0) < 1.0e-5);
                    K[c] = newK;
                }
                if (change < 1.0e-12) {
                    substitutionConverged = true;
                    break;
                }
            }

            if (!substitutionConverged) {
                anyInconclusive = true;
            }

            // The trivial test needs a converged K just as the pressure
            // criterion below does: its threshold is 1e-5 while convergence
            // is 1e-12, so an exhausted iteration can read as near-trivial
            // while K is still moving, and would then close the bracket on
            // a pressure it never classified.
            if (substitutionConverged && trivial && !rootsDistinct) {
                // The phases collapsed into one: the pressure lies on the
                // single-phase side of the saturation pressure.  Bisect
                // towards a pressure already known to be two-phase, or scan
                // on if the bracket is not closed yet.
                pSingle = p;
                haveSingle = true;
                p = haveTwo ? std::sqrt(pSingle * pTwo)
                            : p * (fromAbove ? scanStep : Scalar{1} / scanStep);
                K = wilsonK(p);
                continue;
            }

            Scalar sum = 0.0;
            for (int c = 0; c < numComponents; ++c) {
                sum += bubble ? K[c] * z[c] : z[c] / K[c];
            }
            // The pressure criterion is only meaningful once the fixed-pressure
            // substitution has actually reached fugacity equality; exhausting
            // the inner loop is a failure, not a solution.
            if (substitutionConverged && std::abs(sum - 1.0) < 1.0e-10) {
                Scalar distance = 0.0;
                for (int c = 0; c < numComponents; ++c) {
                    incipient[c] = (bubble ? K[c] * z[c] : z[c] / K[c]) / sum;
                    distance += std::abs(incipient[c] - z[c]);
                }
                // A converged K that leaves the incipient phase with both the
                // composition and the EOS root of the known one is the trivial
                // solution wearing a disguise: it satisfies the saturation
                // condition at an arbitrary pressure.  Treat it as the
                // single-phase point it is and keep searching.  Mixtures where
                // the roots have genuinely merged are refused for the same
                // reason; returning nothing beats returning a false pressure.
                if (distance > 1.0e-3 || rootsDistinct) {
                    press = p;
                    return Outcome::Converged;
                }
                pSingle = p;
                haveSingle = true;
                p = haveTwo ? std::sqrt(pSingle * pTwo)
                            : p * (fromAbove ? scanStep : Scalar{1} / scanStep);
                K = wilsonK(p);
                continue;
            }

            // Only a converged substitution certifies the pressure as lying
            // inside the two-phase region; an exhausted one proves nothing
            // and must not pollute the bracket.
            if (substitutionConverged) {
                pTwo = p;
                haveTwo = true;
            }
            // Inside the two-phase region the incipient amount exceeds one and
            // the pressure moves towards the saturation pressure: up on the
            // bubble and retrograde branches, down on the lower dew branch.
            const Scalar factor = (mode == Mode::DewLower) ? 1.0 / sum : sum;
            Scalar pNext = p * std::clamp(factor, Scalar{0.5}, Scalar{2.0});
            // Never step onto or past a pressure already known to be
            // single-phase; bisect towards it instead.
            if (haveSingle && ((fromAbove && pNext >= pSingle) ||
                               (!fromAbove && pNext <= pSingle)))
            {
                pNext = std::sqrt(p * pSingle);
            }
            p = pNext;
        }

        // The scan covered nine decades of pressure without meeting a
        // two-phase state: the branch has no dew or bubble point to find.
        // That conclusion only holds if every point along the way was
        // classified; an exhausted substitution leaves the branch unproven.
        if (!haveTwo) {
            return anyInconclusive ? Outcome::GaveUp : Outcome::NoRoot;
        }
        // A bracket that collapsed without an accepted solution pinned the
        // phase boundary down to a point where only the trivial solution
        // lives; that too is positive evidence the branch has no genuine
        // saturation point.  A bracket still open is merely an unfinished
        // search.
        if (haveSingle &&
            (std::abs(pSingle - pTwo) <= 1.0e-6 * std::max(pSingle, pTwo)))
        {
            return Outcome::NoRoot;
        }
        return Outcome::GaveUp;
    }
};

} // namespace Opm

#endif // OPM_SATURATION_PRESSURE_HPP
