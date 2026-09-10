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

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace Opm {

/*!
 * \brief Computes the saturation pressure of a mixture at a given temperature
 *        from the cubic equation of state.
 *
 * The bubble-point (dew-point) pressure of a liquid (vapour) with composition
 * \f$z\f$ is the pressure where an infinitesimal vapour (liquid) phase is in
 * equilibrium with it. At fixed temperature and pressure, phase equilibrium
 * requires equal component fugacities:
 * \f[
 *     x_i\,\phi_i^L(T,p,x) = y_i\,\phi_i^V(T,p,y),
 *     \qquad
 *     K_i = \frac{y_i}{x_i} = \frac{\phi_i^L}{\phi_i^V}.
 * \f]
 * Here \f$x\f$ and \f$y\f$ are liquid and vapour mole fractions and
 * \f$\phi_i^L\f$ and \f$\phi_i^V\f$ are the EOS fugacity coefficients.
 * For a known liquid, \f$x=z\f$ and the unnormalised incipient vapour is
 * \f$Y_i=K_i z_i\f$. For a known vapour, \f$y=z\f$ and the unnormalised
 * incipient liquid is \f$X_i=z_i/K_i\f$. Consequently the saturation equations
 * are
 * \f[
 *     S_b(p) = \sum_i K_i(p)z_i = 1
 *     \quad\hbox{and}\quad
 *     S_d(p) = \sum_i \frac{z_i}{K_i(p)} = 1.
 * \f]
 * The implementation solves fugacity equality by successive substitution at
 * fixed pressure, with dominant-eigenvalue extrapolation when it contracts
 * slowly. It then solves \f$F(p)=\ln S(p)=0\f$ in \f$\ln p\f$ with fixed-point
 * or secant steps, switching to Illinois regula falsi after bracketing the
 * boundary. Trivial \f$K_i=1\f$ trials restart from the Wilson estimate at the
 * next scan pressure.
 *
 * Fugacity equality and \f$S=1\f$ also admit stationary points inside a
 * two-phase region. Candidate roots are therefore screened by a Michelsen
 * tangent-plane stability trial. With unnormalised trial amounts \f$Y_i\f$,
 * \f$S_Y=\sum_iY_i\f$, and trial composition \f$w_i=Y_i/S_Y\f$, its stationary
 * equations are
 * \f[
 *     Y_i = z_i\frac{\phi_i(T,p,z)}{\phi_i(T,p,w)}.
 * \f]
 * At a stationary point,
 * \f[
 *     \operatorname{TPD}(w)
 *       = \sum_iw_i\ln\!\frac{w_i\phi_i(T,p,w)}{z_i\phi_i(T,p,z)}
 *       = -\ln S_Y.
 * \f]
 * Therefore \f$S_Y>1\f$ proves instability; otherwise this trial does not
 * reject the candidate. The trial solver works in \f$u_i=\ln Y_i\f$ and
 * combines substitution with damped Newton steps near critical conditions.
 * Both boundaries of one envelope satisfy fugacity equality and \f$S=1\f$,
 * so an accepted candidate is also classified by its enrichment along the
 * Wilson volatility direction and must belong to the branch searched.
 *
 * A mixture may have lower and upper dew points at a given temperature.  The
 * upper (retrograde) one is the boundary crossed as pressure declines from
 * the single-phase gas region, so the dew-point search tries it first.
 * If the scan fails, a bubble-point search or continuation from a lower dew
 * point can locate the other boundary of the connected vapour/liquid envelope.
 * Continuation advances in \f$\ln p\f$, carries stationary trial amounts from
 * one pressure to the next, and brackets the sign change of \f$\ln S_Y\f$.
 * A failed scan does not establish that no root exists: near a critical point,
 * the two-phase interval can be narrower than the pressure sampling step.
 * A damped Newton stationary solve completes fixed-pressure substitutions that
 * stall near criticality. During continuation, a failed trial point does not
 * discard an existing bracket: alternate interior points are tried, and a
 * bounded forward search can cross an interval of numerical nonconvergence.
 * If a nontrivial stationary branch terminates at a critical endpoint before
 * producing a negative residual, its last point is subjected to the same EOS,
 * fugacity and stability certification as a sign-bracketed boundary.
 * Branch resolution depends on the precision of \c Scalar. The
 * double-precision validation sweep found no isolated unresolved states. In
 * single precision, the convergence thresholds share the same epsilon floor,
 * and a near-critical dew search can resolve the opposite boundary of the same
 * envelope.
 * Input compositions must contain finite, non-negative mole fractions with a
 * positive total. They are rescaled to sum to one, so they need not do so on
 * entry.
 */
template <class Scalar, class FluidSystem>
class SaturationPressure
{
    static constexpr int numComponents = FluidSystem::numComponents;
    static constexpr int oilPhaseIdx = FluidSystem::oilPhaseIdx;
    static constexpr int gasPhaseIdx = FluidSystem::gasPhaseIdx;

    using EOSType = CompositionalConfig::EOSType;
    using ParameterCache = typename FluidSystem::template ParameterCache<Scalar>;

public:
    using CompVec = std::array<Scalar, numComponents>;

    /// Computes the bubble-point pressure of a liquid with composition \p liquid at
    /// temperature \p temp, along with the equilibrium \p vapor composition.
    /// On failure, \p press and \p vapor are both unchanged.
    /// \return whether the calculation converged
    [[nodiscard]] static bool bubblePressure(const CompVec& liquid,
                                             const Scalar temp,
                                             const EOSType eosType,
                                             Scalar& press,
                                             CompVec& vapor)
    {
        // The search uses the incipient composition as scratch, so give it one
        // of its own and publish only once it has converged.
        Scalar p{};
        CompVec incipient{};
        if (solve_(normalized_(liquid), temp, eosType, Mode::Bubble, p, incipient)
            != Outcome::Converged) {
            return false;
        }

        press = p;
        vapor = incipient;
        return true;
    }

    /// Computes the dew-point pressure of a vapour with composition \p vapor at
    /// temperature \p temp, along with the equilibrium \p liquid composition.
    /// The upper (retrograde) dew point is preferred. If its initial search
    /// fails, trace the envelope from a lower dew point or a bubble point.
    /// Return the lower point only for a pure fluid or when a bubble point
    /// establishes the other boundary of the connected vapour/liquid envelope.
    /// On failure, \p press and \p liquid are both unchanged.
    /// \return whether the calculation converged
    [[nodiscard]] static bool dewPressure(const CompVec& vapor,
                                          const Scalar temp,
                                          const EOSType eosType,
                                          Scalar& press,
                                          CompVec& liquid)
    {
        // Every trial below writes through to its output, and a later one may
        // still fail, so stage both and publish only on success.
        Scalar p{};
        CompVec incipient{};
        if (!dewPressure_(normalized_(vapor), temp, eosType, p, incipient)) {
            return false;
        }

        press = p;
        liquid = incipient;
        return true;
    }

private:
    /// \copydoc dewPressure
    /// Takes an already normalised \p z and may write to its outputs on failure.
    static bool dewPressure_(const CompVec& z,
                             const Scalar temp,
                             const EOSType eosType,
                             Scalar& press,
                             CompVec& liquid)
    {
        if (solve_(z, temp, eosType, Mode::DewUpper, press, liquid)
            == Outcome::Converged) {
            return true;
        }

        Scalar pLower{};
        CompVec lowerLiquid{};
        if (solve_(z, temp, eosType, Mode::DewLower, pLower, lowerLiquid)
            != Outcome::Converged) {
            // Both dew scans can miss a narrow envelope. Try locating its
            // bubble boundary, then trace down to the dew point.
            Scalar pBubble{};
            CompVec bubbleVapor{};
            if (solve_(z, temp, eosType, Mode::Bubble, pBubble, bubbleVapor)
                != Outcome::Converged) {
                return false;
            }
            return traceEnvelope_(z, temp, eosType, pBubble, bubbleVapor,
                                  /*fromBubble=*/true, press, liquid);
        }

        if (std::ranges::count_if(z, [](const Scalar zi) { return zi > 0.0; }) == 1) {
            // A pure fluid has the same bubble and dew pressure.
            press = pLower;
            liquid = lowerLiquid;
            return true;
        }

        // An independently located bubble boundary identifies an ordinary
        // envelope without needing to trace it from the lower dew point.
        Scalar pBubble{};
        CompVec bubbleVapor{};
        if (solve_(z, temp, eosType, Mode::Bubble, pBubble, bubbleVapor)
                == Outcome::Converged && pBubble >= pLower) {
            press = pLower;
            liquid = lowerLiquid;
            return true;
        }
        // A lower root alone is a seed, not a fallback answer. Start just
        // inside and follow the envelope to its upper dew or bubble boundary.
        return traceEnvelope_(z, temp, eosType, pLower, lowerLiquid,
                              /*fromBubble=*/false, press, liquid);
    }

    // A finite scan cannot establish that an unsampled interval has no root.
    enum class Outcome { Converged, GaveUp };

    // The saturation-pressure branch being searched.  The bubble point and the
    // upper (retrograde) dew point are approached from the high-pressure side,
    // the lower dew point from the low-pressure side.
    enum class Mode { Bubble, DewUpper, DewLower };

    enum class Stability { Stable, Unstable, Indeterminate };

    // Keep the established double-precision thresholds while making each
    // convergence test meaningful at Scalar's precision. Every threshold below
    // reaches this floor in single precision, so criteria that are orders apart
    // in double collapse onto one value there and limit float branch resolution.
    static constexpr Scalar precisionTolerance_(const Scalar minimum)
    {
        return std::max(minimum,
                        Scalar{100} * std::numeric_limits<Scalar>::epsilon());
    }

    static constexpr Scalar nonlinearTolerance_ = precisionTolerance_(Scalar{1.0e-12});
    static constexpr Scalar stabilityTolerance_ = precisionTolerance_(Scalar{1.0e-8});
    static constexpr Scalar boundaryResidualTolerance_ = precisionTolerance_(Scalar{1.0e-10});
    static constexpr Scalar boundaryPressureTolerance_ = precisionTolerance_(Scalar{1.0e-8});
    static constexpr Scalar rootVolumeTolerance_ = precisionTolerance_(Scalar{1.0e-9});
    static constexpr Scalar directionTolerance_ = precisionTolerance_(Scalar{1.0e-6});

    // Rescale a composition to sum to one. The boundary residual is measured
    // against one, so a composition that sums to 1 + d cannot meet that test
    // once |d| exceeds the tolerance. Callers assemble z in floating point,
    // from a flash or from a restart file's single-precision mole fractions,
    // so it need not sum exactly.
    static CompVec normalized_(const CompVec& z)
    {
        const Scalar sum = std::accumulate(z.begin(), z.end(), Scalar{0});
        if (!positiveFinite_(sum)) {
            return z;
        }

        CompVec out{};
        std::ranges::transform(z, out.begin(),
                               [sum](const Scalar zi) { return zi / sum; });
        return out;
    }

    static bool positiveFinite_(const Scalar value)
    {
        return std::isfinite(value) && value > 0.0;
    }

    /*!
     * \brief Decide whether the cubic equation of state has two distinct
     *        physical roots at one composition.
     *
     * Pure fluids and azeotropes have equal phase compositions at saturation,
     * so distinct liquid and vapour roots are their only nontriviality
     * certificate. Both roots are taken at the same composition: comparing
     * the roots at the known and incipient compositions instead lets a small
     * composition difference pass as a second root at float precision.
     *
     * \param[in] fs Fluid state supplying the temperature and pressure.
     * \param[in] composition Composition at which both roots are evaluated.
     * \param[in] eosType Cubic EOS type.
     * \return whether both roots are physical and their molar volumes differ
     *         by more than \c rootVolumeTolerance_.
     */
    static bool rootsDistinctAtComposition_(
        const CompositionalFluidState<Scalar, FluidSystem>& fs,
        const CompVec& composition,
        const EOSType eosType)
    {
        auto rootState = fs;
        for (int c = 0; c < numComponents; ++c) {
            rootState.setMoleFraction(oilPhaseIdx, c, composition[c]);
            rootState.setMoleFraction(gasPhaseIdx, c, composition[c]);
        }

        ParameterCache rootCache(eosType);
        rootCache.updatePhase(rootState, oilPhaseIdx);
        rootCache.updatePhase(rootState, gasPhaseIdx);
        const Scalar vmL = rootCache.molarVolume(oilPhaseIdx);
        const Scalar vmV = rootCache.molarVolume(gasPhaseIdx);
        constexpr Scalar clampedVm = 1.0e-7;
        return positiveFinite_(vmL) && positiveFinite_(vmV)
            && std::min(vmL, vmV) > 2.0 * clampedVm
            && std::abs(vmL - vmV)
                > rootVolumeTolerance_ * std::max(vmL, vmV);
    }

    /*!
     * \brief Apply bounded dominant-eigenvalue extrapolation to a fixed-point
     *        iterate.
     *
     * Let \f$d^n=\ln v^{n+1}-\ln v^n\f$ be the current logarithmic update and
     * \f$d^{n-1}\f$ the previous one. The dominant contraction factor and the
     * remaining geometric-series correction are estimated as
     * \f[
     *     r = \frac{d^n\mathbin{\cdot}d^{n-1}}
     *              {d^{n-1}\mathbin{\cdot}d^{n-1}},
     *     \qquad
     *     \Delta\ln v_i = \frac{r}{1-r}d_i^n.
     * \f]
     * Only aligned updates (\f$r>0\f$) are accelerated. The estimate is capped
     * at 0.98 and each component correction at \f$|\Delta\ln v_i|\leq\ln 2\f$,
     * so a single extrapolation changes a positive amount by at most a factor
     * of two and cannot deliberately underflow it to zero.
     *
     * \param[in,out] values Positive fixed-point variables \f$v\f$.
     * \param[in] d Current logarithmic update \f$d^n\f$.
     * \param[in] dPrev Previous logarithmic update \f$d^{n-1}\f$.
     */
    static void accelerate_(CompVec& values, const CompVec& d, const CompVec& dPrev)
    {
        Scalar num = 0.0;
        Scalar den = 0.0;
        for (int c = 0; c < numComponents; ++c) {
            num += d[c] * dPrev[c];
            den += dPrev[c] * dPrev[c];
        }
        if (!positiveFinite_(den) || !positiveFinite_(num)) {
            return;
        }
        const Scalar ratio = std::min(num / den, Scalar{0.98});
        const Scalar remaining = ratio / (1.0 - ratio);
        const Scalar maxStep = std::log(Scalar{2});
        CompVec next = values;
        for (int c = 0; c < numComponents; ++c) {
            if (values[c] == 0.0) { // Absent component in a stability trial.
                continue;
            }
            const Scalar step = remaining * d[c];
            if (!std::isfinite(step)) {
                return;
            }
            next[c] *= std::exp(std::clamp(step, -maxStep, maxStep));
            if (!positiveFinite_(next[c])) {
                return;
            }
        }
        values = next;
    }

    /*!
     * \brief Evaluate the pressure-independent numerator of the Wilson
     *        equilibrium-ratio estimate.
     *
     * The Wilson correlation is
     * \f[
     *     K_i^W(T,p) = \frac{p_{c,i}}{p}
     *       \exp\!\left[5.373(1+\omega_i)
     *       \left(1-\frac{T_{c,i}}{T}\right)\right]
     *       = \frac{Kp_i(T)}{p}.
     * \f]
     * This function returns \f$Kp_i(T)\f$; callers divide by pressure. Wilson
     * values initialise phase-composition iterations and the pressure scan.
     *
     * \param[in] temp Absolute temperature \f$T\f$.
     */
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

    /*!
     * \brief Test a candidate known phase in the omitted tangent-plane trial
     *        direction.
     *
     * The method starts an additional Wilson-initialised trial on the same EOS
     * root as the known phase, covering the direction omitted by the saturation
     * iteration. For \f$w_i=Y_i/\sum_jY_j\f$, successive substitution applies
     * \f[
     *     Y_i^{n+1} = z_i
     *       \frac{\phi_i(T,p,z)}{\phi_i(T,p,w^n)}.
     * \f]
     * At a converged stationary point, \f$\sum_iY_i>1\f$ is equivalent to a
     * negative tangent-plane distance and therefore proves that \f$z\f$ is
     * unstable. A sum at or below one passes this trial within tolerance.
     * Slowly contracting trials are completed by stationaryTrial_(); numerical
     * failure produces Stability::Indeterminate rather than a stability claim.
     *
     * \param[in] fs Fluid state containing \f$T\f$, \f$p\f$, and \f$z\f$.
     * \param[in] z Composition of the phase being tested.
     * \param[in] knownPhaseIdx EOS phase/root used by the known phase and trial.
     * \param[in] wilsonK Wilson equilibrium-ratio estimate at the trial pressure.
     * \param[in] knownIsLiquid Selects the heavy or light Wilson trial direction.
     * \param[in] eosType Cubic EOS type.
     */
    static Stability knownPhaseStability_(const CompositionalFluidState<Scalar, FluidSystem>& fs,
                                          const CompVec& z,
                                          const unsigned knownPhaseIdx,
                                          const CompVec& wilsonK,
                                          const bool knownIsLiquid,
                                          const EOSType eosType)
    {
        // The known phase keeps its own state; the trial takes its root type
        // through the same phase index in a second state.
        ParameterCache knownCache(eosType);
        knownCache.updatePhase(fs, knownPhaseIdx);
        CompVec lnPhiZ;
        for (int c = 0; c < numComponents; ++c) {
            lnPhiZ[c] = std::log(
                FluidSystem::fugacityCoefficient(fs, knownCache, knownPhaseIdx, c));
            if (!std::isfinite(lnPhiZ[c])) {
                return Stability::Indeterminate;
            }
        }

        // A liquid is tested against a heavier liquid, a vapour against a
        // lighter vapour: the direction the incipient-phase search leaves out.
        CompVec Y;
        for (int c = 0; c < numComponents; ++c) {
            Y[c] = knownIsLiquid ? z[c] / wilsonK[c] : z[c] * wilsonK[c];
        }

        auto trial = fs;
        ParameterCache trialCache(eosType);
        // Dominant-eigenvalue extrapolation, as in the saturation search.
        CompVec dPrev{};
        Scalar sumPrev = std::numeric_limits<Scalar>::quiet_NaN();
        int settled = 0;
        constexpr int maxStabilityIterations = 500;
        for (int iter = 0; iter < maxStabilityIterations; ++iter) {
            Scalar sumY = 0.0;
            for (const Scalar amount : Y) {
                sumY += amount;
            }
            if (!positiveFinite_(sumY)) {
                return Stability::Indeterminate;
            }
            for (int c = 0; c < numComponents; ++c) {
                trial.setMoleFraction(knownPhaseIdx, c, Y[c] / sumY);
            }
            const int unchanged = iter == 0
                ? ParameterCache::None : ParameterCache::Temperature | ParameterCache::Pressure;
            trialCache.updatePhase(trial, knownPhaseIdx, unchanged);

            Scalar change = 0.0;
            Scalar sumNew = 0.0;
            CompVec d{};
            for (int c = 0; c < numComponents; ++c) {
                if (z[c] <= 0.0) {
                    Y[c] = 0.0;
                    continue;
                }
                const Scalar lnPhiY =
                    std::log(FluidSystem::fugacityCoefficient(trial, trialCache, knownPhaseIdx, c));
                const Scalar Ynew = std::exp(std::log(z[c]) + lnPhiZ[c] - lnPhiY);
                if (!positiveFinite_(Ynew) || !positiveFinite_(Y[c])) {
                    return Stability::Indeterminate;
                }
                change = std::max(change, std::abs(Ynew - Y[c]) / std::max(Scalar{1}, Ynew));
                d[c] = std::log(Ynew) - std::log(Y[c]);
                Y[c] = Ynew;
                sumNew += Ynew;
            }
            // At a stationary point the total decides stability. A flat total
            // during iteration does not establish stationarity: near critical
            // states use Newton to finish the slowly contracting trial.
            const auto verdict = [](const Scalar total) {
                return total > 1.0 + stabilityTolerance_
                    ? Stability::Unstable : Stability::Stable;
            };
            if (change < boundaryResidualTolerance_) {
                return verdict(sumNew);
            }
            if (std::abs(sumNew - sumPrev)
                <= nonlinearTolerance_ * std::max(Scalar{1}, sumNew)) {
                if (++settled == 8) {
                    CompVec candidate = Y;
                    Scalar total{};
                    if (stationaryTrial_(fs, z, knownPhaseIdx, knownPhaseIdx,
                                         eosType, candidate, total)) {
                        return verdict(total);
                    }
                }
            }
            else {
                settled = 0;
            }
            sumPrev = sumNew;
            if (iter % 4 == 3) {
                accelerate_(Y, d, dPrev);
            }
            dPrev = d;
        }
        Scalar total{};
        if (stationaryTrial_(fs, z, knownPhaseIdx, knownPhaseIdx, eosType, Y, total)) {
            return total > 1.0 + stabilityTolerance_
                ? Stability::Unstable : Stability::Stable;
        }
        return Stability::Indeterminate;
    }

    /*!
     * \brief Converge one tangent-plane stationary trial at fixed temperature
     *        and pressure.
     *
     * The unknowns are logarithmic trial amounts \f$u_i=\ln Y_i\f$, which keep
     * all present components positive. With \f$S_Y=\sum_j\exp(u_j)\f$ and
     * \f$w_i=\exp(u_i)/S_Y\f$, the nonlinear residual is
     * \f[
     *     R_i(u) = u_i + \ln\phi_i(T,p,w)
     *              - \ln z_i - \ln\phi_i(T,p,z).
     * \f]
     * Thus \f$R_i=0\f$ is the stationary equation
     * \f$Y_i=z_i\phi_i(z)/\phi_i(w)\f$. The first iterations use the equivalent
     * fixed-point update \f$u_i\leftarrow u_i-R_i\f$, limited to \f$\ln 2\f$
     * per component. The method then forms a finite-difference Jacobian in
     * \f$u\f$ and accepts damped Newton steps only when they reduce
     * \f$\lVert R\rVert_\infty\f$.
     *
     * \param[in] fs Fluid state containing the fixed \f$T\f$ and \f$p\f$.
     * \param[in] z Composition and reference fugacity of the known phase.
     * \param[in] knownPhase EOS root used to evaluate \f$\phi_i(T,p,z)\f$.
     * \param[in] trialPhase EOS root used to evaluate \f$\phi_i(T,p,w)\f$.
     * \param[in] eosType Cubic EOS type.
     * \param[in,out] Y Initial and converged unnormalised trial amounts.
     * \param[out] sum Converged value of \f$S_Y=\sum_iY_i\f$.
     * \return true only when the stationary residual converges.
     */
    static bool stationaryTrial_(const CompositionalFluidState<Scalar, FluidSystem>& fs,
                                 const CompVec& z,
                                 const unsigned knownPhase,
                                 const unsigned trialPhase,
                                 const EOSType eosType,
                                 CompVec& Y,
                                 Scalar& sum)
    {
        using Vector = Dune::FieldVector<Scalar, numComponents>;
        using Matrix = Dune::FieldMatrix<Scalar, numComponents, numComponents>;
        ParameterCache cache(eosType);
        cache.updatePhase(fs, knownPhase);
        CompVec target{};
        Vector u(0.0);
        for (int c = 0; c < numComponents; ++c) {
            if (z[c] > 0.0) {
                if (!positiveFinite_(Y[c])) {
                    return false;
                }
                target[c] = std::log(z[c])
                    + std::log(FluidSystem::fugacityCoefficient(fs, cache, knownPhase, c));
                u[c] = std::log(Y[c]);
            }
        }
        auto trial = fs;
        bool trialCacheInitialized = false;
        const auto residual = [&](const Vector& v, Vector& r) {
            CompVec amounts{};
            Scalar total = 0.0;
            for (int c = 0; c < numComponents; ++c) {
                amounts[c] = z[c] > 0.0 ? std::exp(v[c]) : Scalar{0};
                if (z[c] > 0.0 && !positiveFinite_(amounts[c])) {
                    return false;
                }
                total += amounts[c];
            }
            if (!positiveFinite_(total)) {
                return false;
            }
            for (int c = 0; c < numComponents; ++c) {
                trial.setMoleFraction(trialPhase, c, amounts[c] / total);
            }
            const int unchanged = trialCacheInitialized
                ? ParameterCache::Temperature | ParameterCache::Pressure : ParameterCache::None;
            cache.updatePhase(trial, trialPhase, unchanged);
            trialCacheInitialized = true;
            for (int c = 0; c < numComponents; ++c) {
                r[c] = z[c] > 0.0
                    ? v[c] + std::log(FluidSystem::fugacityCoefficient(trial, cache, trialPhase, c))
                        - target[c]
                    : v[c];
                if (!std::isfinite(r[c])) {
                    return false;
                }
            }
            return true;
        };

        // Allow substitution to converge before forming the Newton Jacobian.
        constexpr int maxTrialIterations = 100;
        constexpr int substitutionPasses = 8;
        constexpr int maxBacktracks = 10;
        for (int iter = 0; iter < maxTrialIterations; ++iter) {
            Vector r;
            if (!residual(u, r)) {
                return false;
            }
            const Scalar norm = r.infinity_norm();
            if (norm < nonlinearTolerance_) {
                sum = 0.0;
                for (int c = 0; c < numComponents; ++c) {
                    Y[c] = z[c] > 0.0 ? std::exp(u[c]) : Scalar{0};
                    sum += Y[c];
                }
                return positiveFinite_(sum);
            }

            bool accepted = false;
            if (iter >= substitutionPasses) {
                Matrix jac(0.0);
                // A fixed step in ln Y gives a relative perturbation of Y.
                constexpr Scalar h = 1.0e-5;
                for (int c = 0; c < numComponents; ++c) {
                    Vector v = u;
                    v[c] += h;
                    Vector perturbed;
                    if (!residual(v, perturbed)) {
                        return false;
                    }
                    for (int row = 0; row < numComponents; ++row) {
                        jac[row][c] = (perturbed[row] - r[row]) / h;
                    }
                }
                Vector step;
                try {
                    jac.solve(step, r);
                    // Cap the first trial step at two in the log variables.
                    const Scalar stepNorm = step.infinity_norm();
                    Scalar damping = (stepNorm > 0.0)
                        ? std::min(Scalar{1}, Scalar{2} / stepNorm)
                        : Scalar{1};
                    for (int backtrack = 0; backtrack < maxBacktracks; ++backtrack) {
                        Vector v = u;
                        v.axpy(-damping, step);
                        Vector next;
                        if (residual(v, next) && next.infinity_norm() < norm) {
                            u = v;
                            accepted = true;
                            break;
                        }
                        damping *= 0.5;
                    }
                }
                catch (const Dune::FMatrixError&) {
                    // A singular Newton system does not invalidate the
                    // substitution step or establish phase stability.
                }
            }
            if (!accepted) {
                for (int c = 0; c < numComponents; ++c) {
                    u[c] -= std::clamp(r[c], -std::log(Scalar{2}), std::log(Scalar{2}));
                }
            }
        }
        return false;
    }

    /*!
     * \brief Follow a connected two-phase interval from one known boundary to
     *        the other.
     *
     * The continuation variable is \f$q=\ln p\f$. At every pressure, both
     * liquid-like and vapour-like stationary trials are solved from the
     * continued amounts and from fresh Wilson seeds. Among nontrivial trials,
     * the residual is the largest converged value
     * \f[
     *     F(q)=\max_k\ln\!\left(\sum_iY_i^{(k)}(q)\right).
     * \f]
     * A positive value proves that the current pressure lies inside the
     * two-phase interval. Starting a small distance inside the seed boundary,
     * the method walks in \f$q\f$, carries \f$Y\f$ between pressures, and grows
     * the step until \f$F\leq0\f$ brackets the other boundary. A safeguarded
     * secant method then solves \f$F=0\f$; trial amounts are interpolated in
     * logarithmic space to preserve positivity and branch continuity. Failed
     * trial evaluations are retried at dyadic interior points. If local
     * continuation stalls, bounded forward probes look for the next evaluable
     * point while retaining the last certified inside point.
     *
     * A nontrivial branch need not change sign before it ends. Where the
     * interval closes at a critical point the incipient phase merges with the
     * known one and the trial ceases to exist, so no step ever lands outside.
     * The walk then contracts onto the boundary instead of bracketing it, and
     * a residual already within tolerance is certified where it stands.
     *
     * The converged boundary is classified from enrichment relative to Wilson
     * volatility,
     * \f[
     *     D=\sum_i(w_i-z_i)\ln K_i^W.
     * \f]
     * \f$D>0\f$ denotes a vapour-like incipient phase (bubble point), while
     * \f$D<0\f$ denotes a liquid-like incipient phase (dew point). Fugacity
     * equality, physical EOS roots, and known-phase stability are checked
     * before returning the boundary. The roots are not required to be
     * distinct: they coincide at a critical endpoint, which is one of the
     * boundaries this continuation exists to reach.
     *
     * From a lower dew seed the method walks upward: an upper dew point is
     * returned, whereas a bubble boundary confirms that the seed was the only
     * dew point. From a bubble seed it walks downward and accepts only a dew
     * boundary.
     *
     * \param[in] z Composition whose saturation pressure is sought.
     * \param[in] temp Absolute temperature.
     * \param[in] eosType Cubic EOS type.
     * \param[in] pSeed Pressure of the known boundary.
     * \param[in] seedTrial Equilibrium composition at the known boundary.
     * \param[in] fromBubble Whether the seed is a bubble rather than dew point.
     * \param[out] press Selected dew-point pressure.
     * \param[out] liquid Incipient liquid composition at \p press.
     * \return whether continuation found and certified the requested result.
     */
    static bool traceEnvelope_(const CompVec& z,
                               const Scalar temp,
                               const EOSType eosType,
                               const Scalar pSeed,
                               const CompVec& seedTrial,
                               const bool fromBubble,
                               Scalar& press,
                               CompVec& liquid)
    {
        const Scalar dir = fromBubble ? Scalar{-1} : Scalar{1};
        CompositionalFluidState<Scalar, FluidSystem> fs;
        fs.setTemperature(temp);
        for (int c = 0; c < numComponents; ++c) {
            fs.setMoleFraction(gasPhaseIdx, c, z[c]);
            fs.setMoleFraction(oilPhaseIdx, c, z[c]);
        }
        const auto Kp = wilsonKp_(temp);
        const auto evaluate = [&](const Scalar lnp, CompVec& Y, Scalar& f) {
            const Scalar p = std::exp(lnp);
            if (!positiveFinite_(p)) {
                return false;
            }
            fs.setPressure(oilPhaseIdx, p);
            fs.setPressure(gasPhaseIdx, p);
            ParameterCache cache(eosType);
            cache.updatePhase(fs, oilPhaseIdx);
            cache.updatePhase(fs, gasPhaseIdx);
            Scalar gibbsDifference = 0.0;
            for (int c = 0; c < numComponents; ++c) {
                if (z[c] > 0.0) {
                    gibbsDifference += z[c] * std::log(
                        FluidSystem::fugacityCoefficient(fs, cache, oilPhaseIdx, c)
                        / FluidSystem::fugacityCoefficient(fs, cache, gasPhaseIdx, c));
                }
            }
            if (!std::isfinite(gibbsDifference)) {
                return false;
            }
            const unsigned knownPhase = gibbsDifference < 0.0 ? oilPhaseIdx : gasPhaseIdx;
            Scalar best = -std::numeric_limits<Scalar>::infinity();
            CompVec bestY{};
            bool complete = true;
            for (const unsigned trialPhase : {oilPhaseIdx, gasPhaseIdx}) {
                bool converged = false;
                for (int seed = 0; seed < 2; ++seed) {
                    CompVec candidate = Y;
                    if (seed == 1) {
                        for (int c = 0; c < numComponents; ++c) {
                            const Scalar k = Kp[c] / p;
                            candidate[c] = trialPhase == oilPhaseIdx ? z[c] / k : z[c] * k;
                        }
                    }
                    Scalar sum{};
                    if (!stationaryTrial_(fs, z, knownPhase, trialPhase,
                                          eosType, candidate, sum)) {
                        continue;
                    }
                    converged = true;
                    Scalar distance = 0.0;
                    for (int c = 0; c < numComponents; ++c) {
                        distance += std::abs(candidate[c] / sum - z[c]);
                    }
                    // The trivial zero is not a sign for root bracketing.
                    if (distance > 1.0e-3 && std::log(sum) > best) {
                        best = std::log(sum);
                        bestY = candidate;
                    }
                }
                complete = complete && converged;
            }
            if (!std::isfinite(best) || (best <= 0.0 && !complete)) {
                return false;
            }
            f = best;
            Y = bestY;
            return true;
        };

        const auto certifyBoundary = [&](const Scalar lnp, CompVec Y, const Scalar f) {
            const Scalar p = std::exp(lnp);
            CompVec K = Kp;
            Scalar direction = 0.0;
            for (int c = 0; c < numComponents; ++c) {
                K[c] /= p;
                Y[c] /= std::exp(f);
                direction += (Y[c] - z[c]) * std::log(K[c]);
            }
            // Identify the continued trial by its enrichment in the
            // Wilson volatility direction, then verify fugacity equality
            // with the corresponding liquid/vapour EOS roots.
            if (std::abs(direction) < directionTolerance_) {
                return false;
            }
            const bool bubble = direction > 0.0;
            const unsigned knownPhase = bubble ? oilPhaseIdx : gasPhaseIdx;
            const unsigned trialPhase = bubble ? gasPhaseIdx : oilPhaseIdx;
            fs.setPressure(oilPhaseIdx, p);
            fs.setPressure(gasPhaseIdx, p);
            for (int c = 0; c < numComponents; ++c) {
                fs.setMoleFraction(knownPhase, c, z[c]);
                fs.setMoleFraction(trialPhase, c, Y[c]);
            }
            ParameterCache cache(eosType);
            cache.updatePhase(fs, oilPhaseIdx);
            cache.updatePhase(fs, gasPhaseIdx);
            const Scalar vmL = cache.molarVolume(oilPhaseIdx);
            const Scalar vmV = cache.molarVolume(gasPhaseIdx);
            if (!positiveFinite_(vmL) || !positiveFinite_(vmV)
                || std::min(vmL, vmV) <= 2.0e-7) {
                return false;
            }
            for (int c = 0; c < numComponents; ++c) {
                if (z[c] > 0.0) {
                    const Scalar a = z[c] * FluidSystem::fugacityCoefficient(
                        fs, cache, knownPhase, c);
                    const Scalar b = Y[c] * FluidSystem::fugacityCoefficient(
                        fs, cache, trialPhase, c);
                    if (!positiveFinite_(a) || !positiveFinite_(b)
                        || std::abs(std::log(a / b)) > stabilityTolerance_) {
                        return false;
                    }
                }
            }
            if (knownPhaseStability_(fs, z, knownPhase, K, bubble, eosType)
                != Stability::Stable) {
                return false;
            }
            if (fromBubble) {
                // Down from a bubble point the envelope can only close
                // at a dew point; anything else is not this envelope.
                if (bubble) {
                    return false;
                }
                press = p;
                liquid = Y;
                return true;
            }
            press = bubble ? pSeed : p;
            liquid = bubble ? seedTrial : Y;
            return true;
        };

        const Scalar seed = std::log(pSeed);
        Scalar lo{}, fLo{};
        CompVec yLo{};
        Scalar step = 0.01;
        bool inside = false;
        // Halve the seed offset until a trial lands inside the envelope.
        constexpr int maxSeedRefinements = 20;
        // With the 0.1 maximum step below, 400 iterations can span 40 units
        // in ln(p), including a high-pressure seed and a sub-bar dew point.
        constexpr int maxWalkSteps = 400;
        constexpr int maxBoundarySteps = 80;
        for (int refine = 0; refine < maxSeedRefinements; ++refine) {
            lo = seed + dir * step;
            yLo = seedTrial;
            if (evaluate(lo, yLo, fLo) && fLo > boundaryResidualTolerance_) {
                inside = true;
                break;
            }
            step *= 0.5;
        }
        if (!inside) {
            return false;
        }

        Scalar hi{}, fHi{};
        CompVec yHi{};
        bool bracketed = false;
        for (int iter = 0; iter < maxWalkSteps; ++iter) {
            hi = lo + dir * step;
            yHi = yLo;
            if (!evaluate(hi, yHi, fHi)) {
                step *= 0.5;
                if (step < boundaryPressureTolerance_) {
                    if (std::abs(fLo) < boundaryResidualTolerance_
                        && certifyBoundary(lo, yLo, fLo)) {
                        return true;
                    }
                    bool recovered = false;
                    // A fixed-pressure trial can fail over a finite interval
                    // even though the stationary branch converges on both
                    // sides. Use only a converged residual to update or close
                    // the bracket.
                    // Twenty 0.1-spaced probes bound the skipped interval to
                    // two units in ln(p), or a pressure ratio of exp(2).
                    constexpr int maxGapProbes = 20;
                    for (int probe = 1; probe <= maxGapProbes; ++probe) {
                        hi = lo + dir * Scalar{0.1} * probe;
                        yHi = yLo;
                        if (!evaluate(hi, yHi, fHi)) {
                            continue;
                        }
                        recovered = true;
                        if (fHi <= 0.0) {
                            bracketed = true;
                            break;
                        }
                        lo = hi;
                        fLo = fHi;
                        yLo = yHi;
                        step = 0.01;
                        break;
                    }
                    if (bracketed) {
                        break;
                    }
                    if (recovered) {
                        continue;
                    }
                    return false;
                }
                continue;
            }
            if (fHi <= 0.0) {
                bracketed = true;
                break;
            }
            lo = hi;
            fLo = fHi;
            yLo = yHi;
            step = std::min(Scalar{0.1}, Scalar{2} * step);
        }
        if (!bracketed) {
            return false;
        }

        // A gap-recovered bracket can span pressures where no continuation
        // composition was found. The interpolated Y below is only an initial
        // guess; each result must converge and pass final certification.
        for (int iter = 0; iter < maxBoundarySteps; ++iter) {
            // Safeguard the secant so that even a flat residual contracts
            // the bracket; interpolate the trial amounts in log space.
            Scalar fraction = std::clamp(fLo / (fLo - fHi), Scalar{0.1}, Scalar{0.9});
            Scalar mid{};
            CompVec Y{};
            Scalar f{};
            const auto attempt = [&](const Scalar candidateFraction) {
                fraction = candidateFraction;
                mid = lo + fraction * (hi - lo);
                for (int c = 0; c < numComponents; ++c) {
                    if (z[c] > 0.0) {
                        Y[c] = std::exp((1.0 - fraction) * std::log(yLo[c])
                                       + fraction * std::log(yHi[c]));
                    }
                }
                return evaluate(mid, Y, f);
            };
            bool evaluated = attempt(fraction);
            // One failed fixed-pressure solve does not invalidate the two
            // converged bracket ends. Try progressively finer dyadic points.
            // Four levels try 15 interior points, down to 1/16 of the bracket,
            // while keeping the retry work bounded.
            constexpr int maxAlternateLevels = 4;
            for (int level = 1; !evaluated && level <= maxAlternateLevels; ++level) {
                const int denominator = 1 << level;
                for (int numerator = 1; numerator < denominator; numerator += 2) {
                    if (attempt(Scalar(numerator) / Scalar(denominator))) {
                        evaluated = true;
                        break;
                    }
                }
            }
            if (!evaluated) {
                return false;
            }
            if (std::abs(hi - lo) < boundaryPressureTolerance_
                && std::abs(f) < boundaryResidualTolerance_) {
                return certifyBoundary(mid, Y, f);
            }
            if (f > 0.0) {
                lo = mid;
                fLo = f;
                yLo = Y;
            }
            else {
                hi = mid;
                fHi = f;
                yHi = Y;
            }
        }
        return false;
    }

    /*!
     * \brief Search one bubble- or dew-pressure branch.
     *
     * Wilson's \f$K_i^W=Kp_i/p\f$ supplies a pressure at which its saturation
     * sum is one:
     * \f[
     *     p_0^b=\sum_i z_iKp_i,
     *     \qquad
     *     p_0^d=\left(\sum_i\frac{z_i}{Kp_i}\right)^{-1}.
     * \f]
     * Bubble and upper-dew searches move down from the high-pressure estimate;
     * the lower-dew search moves upward from \f$p_0^d\f$.
     *
     * At each pressure, successive substitution solves
     * \f[
     *     K_i^{n+1}=\frac{\phi_i^L(z)}{\phi_i^V(y^n)}
     *         \quad\hbox{(bubble)},
     *     \qquad
     *     K_i^{n+1}=\frac{\phi_i^L(x^n)}{\phi_i^V(z)}
     *         \quad\hbox{(dew)},
     * \f]
     * with \f$y_i^n=K_i^nz_i/S_b\f$ or
     * \f$x_i^n=z_i/(K_i^nS_d)\f$. A slowly contracting substitution is
     * completed by the damped Newton stationary solver at the same pressure.
     * Once the fixed-pressure iteration converges,
     * \f$F=\ln S\f$ classifies and drives the pressure search: \f$F>0\f$ is
     * inside the two-phase region and \f$F<0\f$ is outside. Before bracketing,
     * the method uses a direction-checked secant step or the fixed-point step
     * \f$\ln p_{n+1}=\ln p_n\mathbin{\pm}F_n\f$. Within a bracket it uses the
     * Illinois regula-falsi formula
     * \f[
     *     q_{n+1}=q_{out}-F_{out}
     *       \frac{q_{in}-q_{out}}{F_{in}-F_{out}},
     *     \qquad q=\ln p,
     * \f]
     * with bisection safeguards. Pressure changes are capped at a factor of
     * two. A converged \f$S=1\f$ state is returned only after rejecting the
     * trivial \f$K=1\f$ solution, confirming from
     * \f$D=\sum_i(w_i-z_i)\ln K_i^W\f$ that the incipient phase belongs to
     * the branch being searched rather than to the opposite boundary of the
     * same envelope, and certifying known-phase stability.
     *
     * A finite scan can miss a narrow phase interval, so exhausting the search
     * returns Outcome::GaveUp and never claims that the branch has no root.
     *
     * \param[in] z Known liquid composition for bubble mode, or known vapour
     *              composition for either dew mode.
     * \param[in] temp Absolute temperature.
     * \param[in] eosType Cubic EOS type.
     * \param[in] mode Branch and scan direction.
     * \param[out] press Saturation pressure; unchanged unless converged.
     * \param[out] incipient Equilibrium composition of the incipient phase.
     * \return Outcome::Converged for a certified boundary, otherwise
     *         Outcome::GaveUp.
     */
    static Outcome solve_(const CompVec& z,
                          const Scalar temp,
                          const EOSType eosType,
                          const Mode mode,
                          Scalar& press,
                          CompVec& incipient)
    {
        if (!positiveFinite_(temp)) {
            return Outcome::GaveUp;
        }
        const CompVec wilsonKp = wilsonKp_(temp);
        const bool bubble = (mode == Mode::Bubble);
        // The bubble point and the retrograde dew point are approached from
        // the high-pressure side, the lower dew point from the low-pressure side.
        const bool fromAbove = (mode != Mode::DewLower);

        // Wilson K scales as 1/p, so its saturation sum supplies the initial
        // pressure. The upper dew search starts from the same high estimate as
        // the bubble search.
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

        const auto wilsonK = [&wilsonKp](const Scalar pressure) {
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

        // pOut is a tentative endpoint from a trivial trial or a converged
        // incipient amount below one. pIn has a converged amount above one,
        // establishing instability. Distinct EOS roots alone do not determine
        // which side of the boundary a trial occupies.
        Scalar pOut{}, pIn{};
        bool haveOut = false;
        bool haveIn = false;
        // ln(sum) at the bracket ends. A trivial trial supplies no residual,
        // so fOut is only known after a nontrivial converged one.
        Scalar fOut{}, fIn{};
        bool haveFOut = false;
        // The previous converged non-trivial trial, for the secant step, and
        // which bracket end the previous step replaced (Illinois damping).
        Scalar lnpPrev{}, fPrev{};
        bool havePrev = false;
        int lastSide = 0;
        // This scan step can skip a narrow envelope; dewPressure() tries
        // continuation from another boundary if the scan fails.
        constexpr Scalar scanStep = 0.9;
        constexpr int maxPressureIterations = 200;
        constexpr int maxSubstitutionIterations = 500;

        const auto safeguardPressure = [&](Scalar pNext) {
            if (haveIn && haveOut) {
                const Scalar lo = std::min(pIn, pOut);
                const Scalar hi = std::max(pIn, pOut);
                if (!(pNext > lo && pNext < hi)) {
                    pNext = std::sqrt(lo * hi);
                }
            }
            else if (haveOut && ((fromAbove && pNext >= pOut) ||
                                 (!fromAbove && pNext <= pOut)))
            {
                pNext = std::sqrt(p * pOut);
            }
            return pNext;
        };

        for (int outer = 0; outer < maxPressureIterations; ++outer) {
            if (!positiveFinite_(p)) {
                return Outcome::GaveUp;
            }
            fs.setPressure(oilPhaseIdx, p);
            fs.setPressure(gasPhaseIdx, p);

            ParameterCache paramCache(eosType);
            CompVec phiKnown;

            // Fugacity equality at fixed pressure: K_c = phi_liquid / phi_vapour.
            bool trivial = false;
            bool substitutionConverged = false;
            // The previous change of ln K, for the dominant-eigenvalue
            // extrapolation below.
            CompVec dPrev{};
            for (int inner = 0; inner < maxSubstitutionIterations; ++inner) {
                Scalar sum = 0.0;
                for (int c = 0; c < numComponents; ++c) {
                    if (!positiveFinite_(K[c])) {
                        return Outcome::GaveUp;
                    }
                    incipient[c] = bubble ? K[c] * z[c] : z[c] / K[c];
                    sum += incipient[c];
                }
                if (!positiveFinite_(sum)) {
                    return Outcome::GaveUp;
                }
                for (int c = 0; c < numComponents; ++c) {
                    fs.setMoleFraction(incipientPhaseIdx, c, incipient[c] / sum);
                }

                // The known phase is fixed throughout the substitution at this pressure.
                if (inner == 0) {
                    paramCache.updatePhase(fs, knownPhaseIdx);
                    for (int c = 0; c < numComponents; ++c) {
                        phiKnown[c] = FluidSystem::fugacityCoefficient(
                            fs, paramCache, knownPhaseIdx, c);
                    }
                }
                const int unchanged = inner == 0
                    ? ParameterCache::None : ParameterCache::Temperature | ParameterCache::Pressure;
                paramCache.updatePhase(fs, incipientPhaseIdx, unchanged);

                Scalar change = 0.0;
                trivial = true;
                CompVec d;
                for (int c = 0; c < numComponents; ++c) {
                    const Scalar phiIncipient = FluidSystem::fugacityCoefficient(
                        fs, paramCache, incipientPhaseIdx, c);
                    const Scalar newK = bubble ? phiKnown[c] / phiIncipient
                                              : phiIncipient / phiKnown[c];
                    if (!positiveFinite_(newK)) {
                        return Outcome::GaveUp;
                    }
                    // Relative to the magnitude of K: an absolute measure is
                    // unreachable for the large K of a light component.
                    change = std::max(change, std::abs(newK - K[c]) /
                                              std::max(Scalar{1}, std::abs(newK)));
                    trivial = trivial && (std::abs(newK - 1.0) < 1.0e-5);
                    d[c] = std::log(newK) - std::log(K[c]);
                    K[c] = newK;
                }
                if (change < nonlinearTolerance_) {
                    substitutionConverged = true;
                    break;
                }

                // Every fourth pass, extrapolate the remaining geometric
                // series in ln K using the last two substitution changes.
                if (inner % 4 == 3) {
                    accelerate_(K, d, dPrev);
                }
                dPrev = d;
            }

            // Finish slowly contracting near-critical substitutions with the
            // damped Newton stationary solver used by envelope continuation.
            if (!substitutionConverged) {
                CompVec candidate{};
                for (int c = 0; c < numComponents; ++c) {
                    if (z[c] > 0.0) {
                        candidate[c] = bubble ? K[c] * z[c] : z[c] / K[c];
                    }
                }
                Scalar candidateSum{};
                if (stationaryTrial_(fs, z, knownPhaseIdx, incipientPhaseIdx,
                                     eosType, candidate, candidateSum)) {
                    for (int c = 0; c < numComponents; ++c) {
                        fs.setMoleFraction(incipientPhaseIdx, c,
                                           candidate[c] / candidateSum);
                    }
                    paramCache.updatePhase(fs, incipientPhaseIdx);
                    trivial = true;
                    for (int c = 0; c < numComponents; ++c) {
                        const Scalar phiIncipient = FluidSystem::fugacityCoefficient(
                            fs, paramCache, incipientPhaseIdx, c);
                        const Scalar newK = bubble ? phiKnown[c] / phiIncipient
                                                  : phiIncipient / phiKnown[c];
                        if (!positiveFinite_(newK)) {
                            return Outcome::GaveUp;
                        }
                        trivial = trivial && (std::abs(newK - 1.0) < 1.0e-5);
                        K[c] = newK;
                    }
                    substitutionConverged = true;
                }
            }

            const bool rootsDistinct = substitutionConverged
                && rootsDistinctAtComposition_(fs, z, eosType);

            const auto markOutside = [&](const bool withValue, const Scalar value) {
                pOut = p;
                haveOut = true;
                haveFOut = withValue;
                fOut = value;
            };
            // Step from a trial the substitution could not classify: bisect
            // the bracket if there is one, otherwise continue the scan.
            const auto stepUnclassified = [&]() {
                p = haveIn ? std::sqrt(pOut * pIn)
                           : p * (fromAbove ? scanStep : Scalar{1} / scanStep);
                K = wilsonK(p);
            };

            // The looser triviality tolerance can be met before K converges.
            // Only a converged trial may update the scan endpoint.
            if (substitutionConverged && trivial && !rootsDistinct) {
                // No nontrivial trial was found. Advance the scan endpoint;
                // collapse onto one root does not establish phase stability.
                markOutside(false, Scalar{0});
                lastSide = 0;
                stepUnclassified();
                continue;
            }

            Scalar sum = 0.0;
            for (int c = 0; c < numComponents; ++c) {
                sum += bubble ? K[c] * z[c] : z[c] / K[c];
            }
            if (!positiveFinite_(sum)) {
                return Outcome::GaveUp;
            }
            // The pressure criterion is only meaningful once the fixed-pressure
            // substitution has actually reached fugacity equality; exhausting
            // the inner loop is a failure, not a solution.
            if (substitutionConverged
                && std::abs(sum - 1.0) < boundaryResidualTolerance_) {
                Scalar distance = 0.0;
                Scalar direction = 0.0;
                const CompVec candidateWilsonK = wilsonK(p);
                for (int c = 0; c < numComponents; ++c) {
                    incipient[c] = (bubble ? K[c] * z[c] : z[c] / K[c]) / sum;
                    distance += std::abs(incipient[c] - z[c]);
                    direction += (incipient[c] - z[c]) * std::log(candidateWilsonK[c]);
                }
                // Require a distinct phase, then check known-phase stability:
                // fugacity equality also admits stationary points inside the
                // two-phase region, including nearly identical compositions
                // occupying different EOS roots.
                // A direct dew iteration can also converge to the bubble
                // boundary of z (and vice versa).  Classify the incipient
                // phase by enrichment along the Wilson volatility direction,
                // as envelope continuation does.  A zero direction is valid
                // only for a pure-fluid or azeotropic boundary with distinct
                // roots.
                const bool directionMatches = std::abs(direction) < directionTolerance_
                    ? rootsDistinct : (bubble == (direction > 0.0));
                if ((distance > 1.0e-3 || rootsDistinct) && directionMatches) {
                    const auto stability =
                        knownPhaseStability_(fs, z, knownPhaseIdx,
                                             candidateWilsonK, bubble, eosType);
                    if (stability == Stability::Indeterminate) {
                        return Outcome::GaveUp;
                    }
                    if (stability == Stability::Stable) {
                        press = p;
                        return Outcome::Converged;
                    }
                }
                // Reject this stationary point and continue searching.
                markOutside(false, Scalar{0});
                lastSide = 0;
                stepUnclassified();
                continue;
            }

            // The incipient amount is the residual of the search: ln(sum) is
            // positive inside the two-phase region and negative outside it,
            // and the root is where it vanishes.
            const Scalar f = std::log(sum);
            const Scalar lnp = std::log(p);

            if (!substitutionConverged) {
                // An exhausted substitution classifies nothing.  Take the
                // fixed-point step K ~ 1/p implies, p * sum towards the root,
                // and let the carried-over K keep converging within the bracket.
                const Scalar lnpFixed = lnp + (fromAbove ? f : -f);
                p = safeguardPressure(std::exp(lnp + std::clamp(lnpFixed - lnp,
                                                               std::log(Scalar{0.5}),
                                                               std::log(Scalar{2}))));
                continue;
            }

            if (f > 0) {
                // Inside the two-phase region.  Illinois: replacing the same
                // end twice in a row halves the value kept at the other end,
                // so regula falsi cannot stall against it.
                if (haveIn && lastSide > 0 && haveFOut) {
                    fOut *= 0.5;
                }
                pIn = p;
                fIn = f;
                haveIn = true;
                lastSide = 1;
            }
            else {
                if (haveOut && haveFOut && lastSide < 0 && haveIn) {
                    fIn *= 0.5;
                }
                markOutside(true, f);
                lastSide = -1;
            }

            // Propose the next pressure in ln p.  Regula falsi within a bracket
            // whose ends both carry a value; a secant through the previous
            // trial before the bracket closes, provided the slope has the sign
            // the branch implies (sum grows towards the root: falling pressure
            // from above, rising pressure from below); and the fixed-point
            // step otherwise.
            Scalar lnpNext{};
            bool proposed = false;
            if (haveIn && haveOut && haveFOut) {
                const Scalar lnpIn = std::log(pIn);
                const Scalar lnpOut = std::log(pOut);
                if (fIn != fOut) {
                    lnpNext = lnpOut - fOut * (lnpIn - lnpOut) / (fIn - fOut);
                    proposed = true;
                }
            }
            if (!proposed && havePrev && lnp != lnpPrev) {
                const Scalar slope = (f - fPrev) / (lnp - lnpPrev);
                const bool slopeOk = fromAbove ? (slope < -nonlinearTolerance_)
                                               : (slope > nonlinearTolerance_);
                if (slopeOk) {
                    lnpNext = lnp - f / slope;
                    proposed = true;
                }
            }
            if (!proposed) {
                lnpNext = lnp + (fromAbove ? f : -f);
            }
            lnpPrev = lnp;
            fPrev = f;
            havePrev = true;

            // Limit extrapolation to a factor of two and stay inside a known
            // bracket. This step limit alone cannot prevent skipping an
            // unbracketed narrow envelope.
            lnpNext = lnp + std::clamp(lnpNext - lnp, std::log(Scalar{0.5}), std::log(Scalar{2}));
            p = safeguardPressure(std::exp(lnpNext));
        }

        // Neither a finite scan nor collapse onto a trivial stationary point
        // proves that an upper root is absent.
        return Outcome::GaveUp;
    }
};

} // namespace Opm

#endif // OPM_SATURATION_PRESSURE_HPP
