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
/*!
 * \file
 * \copydoc Opm::EclHysteresisTwoPhaseLawParams
 */
#ifndef OPM_ECL_HYSTERESIS_TWO_PHASE_LAW_PARAMS_HPP
#define OPM_ECL_HYSTERESIS_TWO_PHASE_LAW_PARAMS_HPP

#include <opm/input/eclipse/EclipseState/WagHysteresisConfig.hpp>

#include <opm/material/common/EnsureFinalized.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsScalingPoints.hpp>
#include <opm/material/fluidmatrixinteractions/EclHysteresisConfig.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace Opm {

namespace EclHysteresisDetail {

//! True if the effective-law parameters carry end-point scaling information
//! (config() and scaled/unscaled scaling points), as the EclEpsTwoPhaseLaw
//! parameters do. Plain laws such as Brooks-Corey do not.
template <class P, class = void>
struct HasEpsPoints : std::false_type {};

template <class P>
struct HasEpsPoints<P, std::void_t<decltype(std::declval<const P&>().config().enableSatScaling()),
                                   decltype(std::declval<const P&>().scaledPoints().saturationKrwPoints()[2]),
                                   decltype(std::declval<const P&>().unscaledPoints().saturationKrwPoints()[2])>>
    : std::true_type {};

/*!
 * \brief The largest wetting-phase saturation of an end-point scaled two-phase
 *        law, i.e. the last of its wetting-phase relperm saturation scaling points
 *        (the saturation at zero non-wetting saturation for the drainage curve,
 *        and the end of the table for the imbibition curve).
 *
 * Laws without end-point scaling information (e.g. Brooks-Corey) return
 * \c defaultValue.
 */
template <class EffLawParams, class Scalar>
Scalar krwSwMax(const EffLawParams& effParams, const Scalar defaultValue)
{
    if constexpr (HasEpsPoints<EffLawParams>::value) {
        const auto& points = effParams.config().enableSatScaling()
            ? effParams.scaledPoints()
            : effParams.unscaledPoints();
        return points.saturationKrwPoints()[2];
    }
    else {
        return defaultValue;
    }
}

/*!
 * \brief The bounding imbibition curve of the wetting phase, extended beyond the
 *        end of the imbibition table.
 *
 * Beyond the table end (generally at Sg = Sncri) the curve interpolates linearly
 * in saturation to the drainage end point instead of the flat extrapolation.
 * It is used as the bound of the Killough scanning curve and for its anchor at
 * the trapped saturation, independently of EHYSTR item 15 (which only selects
 * the curve used below the trapped saturation): a flat extrapolation of the
 * table is not a meaningful bound.
 */
template <class EffLaw, class Evaluation, class ...Args>
Evaluation krwImbibitionExtended(const typename EffLaw::Params& drainageParams,
                                 const typename EffLaw::Params& imbibitionParams,
                                 const Evaluation& Sw)
{
    using Scalar = typename EffLaw::Scalar;
    Evaluation Krwi = EffLaw::template twoPhaseSatKrw<Evaluation, Args...>(imbibitionParams, Sw);
    const Scalar SwTab = krwSwMax(imbibitionParams, std::numeric_limits<Scalar>::max());
    const Scalar SwEnd = krwSwMax(drainageParams, Scalar(1));
    if (Sw > SwTab && SwEnd > SwTab) {
        const Evaluation KrwdEnd = EffLaw::template twoPhaseSatKrw<Evaluation, Args...>(drainageParams, Evaluation(SwEnd));
        const Evaluation KrwiTab = EffLaw::template twoPhaseSatKrw<Evaluation, Args...>(imbibitionParams, Evaluation(SwTab));
        const Evaluation t = max(Evaluation(0.0), (SwEnd - Sw) / (SwEnd - SwTab));
        Krwi = KrwdEnd + t * (KrwiTab - KrwdEnd);
    }
    return Krwi;
}

} // namespace EclHysteresisDetail

// The independent history needed by ordinary (non-WAG) hysteresis.
template <class Scalar>
struct EclHysteresisCompactState
{
    Scalar pcSwMdc{2.0};
    Scalar pcSwMic{1.0};
    Scalar krnSwMdc{2.0};
    Scalar krwSwMdc{-2.0};
    bool initialImb{false};
    bool isDrain{true};
};

template <class Scalar>
struct EclHysteresisDynamicState
{
    Scalar deltaSwImbKrn{};
    Scalar Sncrt{};
    Scalar Swcrt{};
    Scalar pcSwMdc{2.0};
    Scalar pcSwMic{1.0};
    Scalar krnSwMdc{2.0};
    Scalar krwSwMdc{-2.0};
    Scalar KrndHy{};
    Scalar KrwdHy{};
    Scalar Krwd_sncrt{};
    Scalar SncrtWAG{};
    Scalar SncrtWAGPrev{};
    Scalar SncrtWAGImb{};
    Scalar baseWAG{};
    Scalar aTransf{1.0};
    Scalar cTransf{};
    Scalar swatImbStart{};
    Scalar swatImbStartNxt{};
    Scalar krnSwWAG{2.0};
    Scalar krnSwDrainRevert{2.0};
    Scalar krnSwDrainStart{-2.0};
    Scalar krnSwDrainStartNxt{};
    Scalar krnImbStart{};
    Scalar krnImbStartNxt{};
    Scalar krnDrainStart{};
    Scalar krnDrainStartNxt{};
    Scalar krnSwImbStart{};
    int nState{};
    bool initialImb{false};
    bool isDrain{true};
    bool wasDrain{false};
    bool wagActive{false};

    EclHysteresisCompactState<Scalar> compactState() const
    {
        return {pcSwMdc, pcSwMic, krnSwMdc, krwSwMdc, initialImb, isDrain};
    }

    static EclHysteresisDynamicState fromCompact(const EclHysteresisCompactState<Scalar>& state)
    {
        EclHysteresisDynamicState result;
        result.pcSwMdc = state.pcSwMdc;
        result.pcSwMic = state.pcSwMic;
        result.krnSwMdc = state.krnSwMdc;
        result.krwSwMdc = state.krwSwMdc;
        result.initialImb = state.initialImb;
        result.isDrain = state.isDrain;
        return result;
    }
};

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief A default implementation of the parameters for the material law which
 *        implements the ECL relative permeability and capillary pressure hysteresis
 */
template <class EffLawT>
class EclHysteresisTwoPhaseLawParams : public EnsureFinalized
{
    using EffLawParams = typename EffLawT::Params;
    using Scalar = typename EffLawParams::Traits::Scalar;

public:
    using Traits = typename EffLawParams::Traits;
    using HysteresisDynamicState = EclHysteresisDynamicState<Scalar>;

    static EclHysteresisTwoPhaseLawParams serializationTestObject()
    {
        EclHysteresisTwoPhaseLawParams<EffLawT> result;
        result.deltaSwImbKrn_ = 1.0;
        //result.deltaSwImbKrw_ = 1.0;
        result.Sncrt_ = 2.0;
        result.Swcrt_ = 2.5;
        result.initialImb_ = true;
        result.pcSwMic_ = 3.0;
        result.krnSwMdc_ = 4.0;
        result.krwSwMdc_ = 4.5;
        result.KrndHy_ = 5.0;
        result.KrwdHy_ = 6.0;

        return result;
    }

    /*!
     * \brief Calculate all dependent quantities once the independent
     *        quantities of the parameter object have been set.
     */
    void finalize()
    {
        if (config().enableHysteresis()) {
            if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().krHysteresisModel() == 4 || config().pcHysteresisModel() == 0) {
                C_ = 1.0/(Sncri_ - Sncrd_ + saturationEpsilon_) - 1.0/(Snmaxd_ - Sncrd_);
                curvatureCapPrs_ =  config().curvatureCapPrs();
            }
            if (config().krHysteresisModel() == 4) {
                Cw_ = 1.0/(Swcri_ - Swcrd_ + saturationEpsilon_) - 1.0/(Swmaxd_ - Swcrd_);
                Krwd_sncri_ = EffLawT::twoPhaseSatKrw(drainageParams(), 1 - Sncri());
            }
            updateDynamicParams_();
        }
        EnsureFinalized :: finalize();
    }

    /*!
     * \brief Set the hysteresis configuration object.
     */
    void setConfig(const EclHysteresisConfig& value)
    { config_ = value; }

    /*!
     * \brief Returns the hysteresis configuration object.
     */
    const EclHysteresisConfig& config() const
    { return config_; }

    /*!
     * \brief Set the WAG-hysteresis configuration object.
     */
    void setWagConfig(std::shared_ptr<WagHysteresisConfig::WagHysteresisConfigRecord> value)
    {
        wagConfig_ = value;
        cTransf_ = wagConfig().wagLandsParam();
    }

    /*!
     * \brief Returns the WAG-hysteresis configuration object.
     */
    const WagHysteresisConfig::WagHysteresisConfigRecord& wagConfig() const
    { return *wagConfig_; }

    /*!
     * \brief Sets the parameters used for the drainage curve
     */
    void setDrainageParams(const EffLawParams& value,
                           const EclEpsScalingPointsInfo<Scalar>& info,
                           EclTwoPhaseSystemType twoPhaseSystem)
    {
        drainageParams_ = value;

        oilWaterSystem_ = (twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
        gasWaterSystem_ = (twoPhaseSystem == EclTwoPhaseSystemType::GasWater);
        gasOilSystem_ = (twoPhaseSystem == EclTwoPhaseSystemType::GasOil);

        if (!config().enableHysteresis())
            return;
        if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().krHysteresisModel() == 4 || config().pcHysteresisModel() == 0 || gasOilHysteresisWAG()) {
            Swco_ = info.Swl;
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Sncrd_ = info.Sgcr + info.Swl;
                Swcrd_ = info.Sogcr;
                Snmaxd_ = info.Sgu + info.Swl;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
            else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Sncrd_ = info.Sgcr;
                Swcrd_ = info.Swcr;
                Snmaxd_ = info.Sgu;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Sncrd_ = info.Sowcr;
                Swcrd_ = info.Swcr;
                Snmaxd_ = 1.0 - info.Swl - info.Sgl;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
        }

        if (config().krHysteresisModel() == 4) {
            //Swco_ = info.Swl;
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Swmaxd_ = 1.0 - info.Sgl - info.Swl;
                KrwdMax_ = EffLawT::twoPhaseSatKrw(drainageParams(), Swmaxd_);
            }
            else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Swmaxd_ = info.Swu;
                KrwdMax_ = EffLawT::twoPhaseSatKrw(drainageParams(), Swmaxd_);
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Swmaxd_ = info.Swu;
                KrwdMax_ = EffLawT::twoPhaseSatKrw(drainageParams(), Swmaxd_);
            }
        }

        // Additional Killough hysteresis model for pc
        if (config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                pcmaxd_ = info.maxPcgo;
            } else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                pcmaxd_ = info.maxPcgo + info.maxPcow;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                pcmaxd_ = -17.0; // At this point 'info.maxPcow' holds pre-swatinit value ...;
            }
        }

        // For WAG hysteresis, assume initial state along primary drainage curve.
        if (gasOilHysteresisWAG()) {
            swatImbStart_ = Swco_;
            swatImbStartNxt_ = -1.0; // Trigger check for saturation gt Swco at first update ...
            cTransf_ = wagConfig().wagLandsParam();
            krnSwDrainStart_ = Sncrd_;
            krnSwDrainStartNxt_ = Sncrd_;
            krnImbStart_ = 0.0;
            krnImbStartNxt_ = 0.0;
            krnDrainStart_ = 0.0;
            krnDrainStartNxt_ = 0.0;
            isDrain_ = true;
            wasDrain_ = true;
            krnSwImbStart_ = Sncrd_;
            SncrtWAG_ = Sncrd_;
            SncrtWAGPrev_ = Sncrd_;
            SncrtWAGImb_ = Sncrd_;
            baseWAG_ = Sncrd_;
            aTransf_ = 1.0;
            nState_ = 1;
        }
    }

    /*!
     * \brief Returns the parameters used for the drainage curve
     */
    const EffLawParams& drainageParams() const
    { return drainageParams_; }

    EffLawParams& drainageParams()
    { return drainageParams_; }

    /*!
     * \brief Sets the parameters used for the imbibition curve
     */
    void setImbibitionParams(const EffLawParams& value,
                             const EclEpsScalingPointsInfo<Scalar>& info,
                             EclTwoPhaseSystemType twoPhaseSystem)
    {
        imbibitionParams_ = value;

        if (!config().enableHysteresis())
            return;

        // Store critical nonwetting saturation
        if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
            Sncri_ = info.Sgcr + info.Swl;
            Swcri_ = info.Sogcr;
        }
        else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
            Sncri_ = info.Sgcr;
            Swcri_ = info.Swcr;
        }
        else {
            assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
            Sncri_ = info.Sowcr;
            Swcri_ = info.Swcr;
        }

        // Killough hysteresis model for pc
        if (config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Swmaxi_ = 1.0 - info.Sgl - info.Swl;
                pcmaxi_ = info.maxPcgo;
            } else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Swmaxi_ = info.Swu;
                pcmaxi_ = info.maxPcgo + info.maxPcow;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Swmaxi_ = info.Swu;
                pcmaxi_ = info.maxPcow;
            }
        }

        if (config().krHysteresisModel() == 4) {
            Krwi_snmax_ = EffLawT::twoPhaseSatKrw(imbibitionParams(), 1 - Snmaxd());
            Krwi_snrmax_ = EffLawT::twoPhaseSatKrw(imbibitionParams(), 1 - Sncri());
        }
    }

    /*!
     * \brief Returns the parameters used for the imbibition curve
     */
    const EffLawParams& imbibitionParams() const
    { return imbibitionParams_; }

    EffLawParams& imbibitionParams()
    { return imbibitionParams_; }

    /*!
     * \brief Get the saturation of the wetting phase where the last switch from the main
     *        drainage curve to imbibition happend on the capillary pressure curve.
     */
    Scalar pcSwMdc() const
    { return pcSwMdc_; }

    Scalar pcSwMic() const
    { return pcSwMic_; }

    /*!
     * \brief Status of initial process.
     */
    bool initialImb() const
    { return initialImb_; }

    /*!
     * \brief Set the saturation of the wetting phase where the last switch from the main
     *        drainage curve (MDC) to imbibition happend on the relperm curve for the
     *        wetting phase.
     */
    void setKrwSwMdc(Scalar value)
    { krwSwMdc_ = value; };

    /*!
     * \brief Get the saturation of the wetting phase where the last switch from the main
     *        drainage curve to imbibition happend on the relperm curve for the
     *        wetting phase.
     */
    Scalar krwSwMdc() const
    { return krwSwMdc_; };

    /*!
     * \brief Set the saturation of the wetting phase where the last switch from the main
     *        drainage curve (MDC) to imbibition happend on the relperm curve for the
     *        non-wetting phase.
     */
    void setKrnSwMdc(Scalar value)
    { krnSwMdc_ = value; }

    /*!
     * \brief Get the saturation of the wetting phase where the last switch from the main
     *        drainage curve to imbibition happend on the relperm curve for the
     *        non-wetting phase.
     */
    Scalar krnSwMdc() const
    { return krnSwMdc_; }

    /*!
     * \brief Sets the saturation value which must be added if krw is calculated using
     *        the imbibition curve.
     *
     * This means that krw(Sw) = krw_drainage(Sw) if Sw < SwMdc and
     * krw(Sw) = krw_imbibition(Sw + Sw_shift,krw) else
     */
    //void setDeltaSwImbKrw(Scalar value)
    //{ deltaSwImbKrw_ = value; }

    /*!
     * \brief Returns the saturation value which must be added if krw is calculated using
     *        the imbibition curve.
     *
     * This means that krw(Sw) = krw_drainage(Sw) if Sw < SwMdc and
     * krw(Sw) = krw_imbibition(Sw + Sw_shift,krw) else
     */
    //Scalar deltaSwImbKrw() const
    //{ return deltaSwImbKrw_; }

    /*!
     * \brief Sets the saturation value which must be added if krn is calculated using
     *        the imbibition curve.
     *
     * This means that krn(Sw) = krn_drainage(Sw) if Sw < SwMdc and
     * krn(Sw) = krn_imbibition(Sw + Sw_shift,krn) else
     */
    void setDeltaSwImbKrn(Scalar value)
    { deltaSwImbKrn_ = value; }

    /*!
     * \brief Returns the saturation value which must be added if krn is calculated using
     *        the imbibition curve.
     *
     * This means that krn(Sw) = krn_drainage(Sw) if Sw < SwMdc and
     * krn(Sw) = krn_imbibition(Sw + Sw_shift,krn) else
     */
    Scalar deltaSwImbKrn() const
    { return deltaSwImbKrn_; }


    Scalar Swcri() const
    { return Swcri_; }

    Scalar Swcrd() const
    { return Swcrd_; }

    Scalar Swmaxi() const
    { return Swmaxi_; }

    Scalar Sncri() const
    { return Sncri_; }

    Scalar Sncrd() const
    { return Sncrd_; }

    Scalar Sncrt() const
    { return Sncrt_; }

    Scalar Swcrt() const
    { return Swcrt_; }

    Scalar SnTrapped(bool maximumTrapping) const
    {
        if(!maximumTrapping && isDrain_)
            return 0.0;

        // For Killough the trapped saturation is already computed
        if( config().krHysteresisModel() > 1 )
            return Sncrt_;
        else // For Carlson we use the shift to compute it from the critial saturation
            return Sncri_ + deltaSwImbKrn_;
    }

    Scalar SnStranded(Scalar sg, Scalar krg) const {
        const Scalar sn = EffLawT::twoPhaseSatKrnInv(drainageParams_, krg);
        return sg - (1.0 - sn) + Sncrd_;
    }

    Scalar SwTrapped() const
    {
        if( config().krHysteresisModel() == 0 || config().krHysteresisModel() == 2)
            return Swcrd_;

        if( config().krHysteresisModel() == 1 || config().krHysteresisModel() == 3)
            return Swcri_;

        // For Killough the trapped saturation is already computed
        if( config().krHysteresisModel() == 4 )
            return Swcrt_;

        return 0.0;
        //else // For Carlson we use the shift to compute it from the critial saturation
        //    return Swcri_ + deltaSwImbKrw_;
    }

    Scalar SncrtWAG() const
    { return SncrtWAG_; }

    /*!
     * \brief Non-wetting saturation below which the non-wetting phase is immobile
     *        on the current WAG imbibition curve.
     *
     * The relative permeability is zero (with a zero derivative) below this
     * saturation, and it increases steeply above it if the trapped part of the
     * historical maximum saturation is large. Returns a negative value if WAG
     * hysteresis is not active, or if nothing is trapped on the current curve.
     */
    Scalar SnTrappedWAG() const
    {
        if (!gasOilHysteresisWAG() || !(SncrtWAG_ > baseWAG_ + saturationEpsilon_)) {
            return -1.0;
        }
        return SncrtWAG_;
    }

    Scalar Snmaxd() const
    { return Snmaxd_; }

    Scalar Swmaxd() const
    { return Swmaxd_; }

    Scalar Snhy() const
    { return 1.0 - krnSwMdc_; }

    Scalar Swhy() const
    { return krwSwMdc_; }

    Scalar Swco() const
    { return Swco_; }

    Scalar krnWght() const
    { return KrndHy_/KrndMax_; }

    /*!
     * \brief The Killough wetting phase relative permeability at the trapped
     *        non-wetting saturation, independent of the current drainage value.
     *
     * This is the end value of the Killough scanning curve at Sw = 1 - Sncrt.
     */
    Scalar krwiAtTrapped() const
    {
        // a = 1 (deltaKrw)^a Formulation according to KILLOUGH 1976
        Scalar deltaKrw = Krwi_snrmax() - Krwd_sncri();
        // Measure the saturations from the drainage critical saturation so that
        // the correction vanishes when Sncrt -> Sncrd (reversal just above the
        // critical saturation); identical to Sncrt/Sncri for Sncrd = 0.
        const Scalar den = Sncri() - Sncrd();
        const Scalar ratio = (den > saturationEpsilon_)
            ? (Sncrt() - Sncrd()) / den
            : Sncrt() / max(saturationEpsilon_, Sncri());
        return Krwd_sncrt() + deltaKrw * ratio;
    }

    Scalar krwdMax() const
    {
        return KrwdMax_; }

    Scalar KrwdHy() const
    {
        return KrwdHy_;
    }


    Scalar Krwd_sncri() const
    {
        return Krwd_sncri_;
    }

    Scalar Krwi_snmax() const
    {
        return Krwi_snmax_;
    }

    Scalar Krwi_snrmax() const
    {
        return Krwi_snrmax_;
    }

    Scalar Krwd_sncrt() const
    {
        return Krwd_sncrt_;
    }

    /*!
     * \brief The imbibition wetting-phase relperm at the trapped non-wetting
     *        saturation, Sw = 1 - Sncrt (Killough).
     */
    Scalar Krwi_sncrt() const
    {
        return Krwi_sncrt_;
    }

    /*!
     * \brief The Killough wetting-phase relperm at the trapped saturation,
     *        Sw = 1 - Sncrt.
     *
     * This is krwiAtTrapped() limited to lie between the drainage and the
     * (extended) imbibition curves, as the scanning curve is. Using the same
     * value as anchor for the continuations below the trapped saturation keeps
     * Krw continuous there also when the tables are not proportional. It only
     * depends on the hysteresis state, so it is updated together with Sncrt.
     */
    Scalar krwTrappedAnchor() const
    {
        return KrwTrappedAnchor_;
    }

    Scalar pcWght() const // Aligning pci and pcd at Swir
    {
        if (pcmaxd_ < 0.0)
            return EffLawT::twoPhaseSatPcnw(drainageParams(), 0.0)/(pcmaxi_+1e-6);
        else
            return pcmaxd_/(pcmaxi_+1e-6);
    }

    Scalar curvatureCapPrs() const
    { return curvatureCapPrs_;}

    bool gasOilHysteresisWAG() const
    { return (config().enableWagHysteresis() && gasOilSystem_ && wagConfig().wagGasFlag()) ; }

    Scalar reductionDrain() const
    { return std::pow(Swco_/(swatImbStart_+tolWAG_*wagConfig().wagWaterThresholdSaturation()), wagConfig().wagSecondaryDrainageReduction());}

    Scalar reductionDrainNxt() const
    { return std::pow(Swco_/(swatImbStartNxt_+tolWAG_*wagConfig().wagWaterThresholdSaturation()), wagConfig().wagSecondaryDrainageReduction());}

    bool threePhaseState() const
    { return (swatImbStart_ > (Swco_ + wagConfig().wagWaterThresholdSaturation()) ); }

    Scalar nState() const
    { return nState_;}

    Scalar krnSwDrainRevert() const
    { return krnSwDrainRevert_;}

    Scalar krnDrainStart() const
    { return krnDrainStart_;}

    Scalar krnDrainStartNxt() const
    { return krnDrainStartNxt_;}

    Scalar krnImbStart() const
    { return krnImbStart_;}

    Scalar krnImbStartNxt() const
    { return krnImbStartNxt_;}

    Scalar krnSwWAG() const
    { return krnSwWAG_;}

    Scalar krnSwDrainStart() const
    { return krnSwDrainStart_;}

    Scalar krnSwDrainStartNxt() const
    { return krnSwDrainStartNxt_;}

    Scalar krnSwImbStart() const
    { return krnSwImbStart_;}

    Scalar tolWAG() const
    { return tolWAG_;}

    template <class Evaluation>
    Evaluation computeSwf(const Evaluation& Sw)  const
    {
        // Free gas saturation (in "Sn = Sg + Swco" notation) on the current
        // imbibition curve. The dynamically trapped part follows
        //   Sn - SncrtWAG = Snf - Snf/(A + C*Snf),   Snf = free part above baseWAG,
        // i.e. Land's formula (A=1) or Killough's modification frozen at the
        // reversal point (A>1).
        Evaluation SgT = 1.0 - Sw - SncrtWAG(); // Sg-Sg_crit_trapped
        Scalar SgCut = wagConfig().wagImbCurveLinearFraction()*(Snhy()- SncrtWAG());
        Evaluation Swf = 1.0;
        const Scalar C = cTransf_;
        const Scalar A = aTransf_;
        const auto freeSat = [C, A](const auto& d) {
            const auto q = C*d - (A - 1.0);
            return (q + Opm::sqrt(q*q + 4.0*C*A*d)) / (2.0*C);
        };

        if (SgT > SgCut) {
            Swf -= (baseWAG_ + freeSat(SgT)); // 1-Sgf
        }
        else {
            SgCut = std::max(Scalar(0.000001), SgCut);
            const Scalar SgCutValue = freeSat(SgCut);
            const Scalar SgCutSlope = SgCutValue/SgCut;
            SgT *= SgCutSlope;
            Swf -= (baseWAG_ + SgT);
        }

        return Swf;
    }

    template <class Evaluation>
    Evaluation computeKrImbWAG(const Evaluation& Sw)  const
    {
        const Evaluation Swf = computeSwf(Sw);
        if (Swf <= krnSwDrainStart_) { // Use secondary drainage curve
            Evaluation Krg = EffLawT::twoPhaseSatKrn(drainageParams_, Swf);
            Evaluation KrgImb2 = (Krg-krnDrainStart_)*reductionDrain() + krnImbStart_;
            return KrgImb2;
        }
        else { // Fallback to primary drainage curve
            Evaluation Sn = Sncrd_;
            if (Swf < 1.0-baseWAG_) {
                // Notation: Sn.. = Sg.. + Swco
                const Scalar den = std::max(Scalar(1.0-krnSwDrainStart_ - baseWAG_), saturationEpsilon_);
                const Scalar dd = (1.0-krnSwImbStart_ - Sncrd_) / den;
                Sn += (1.0-Swf-baseWAG_)*dd;
            }
            Evaluation KrgDrn1 = EffLawT::twoPhaseSatKrn(drainageParams_, 1.0 - Sn);
            return KrgDrn1;
        }
    }

    /*!
     * \brief Notify the hysteresis law that a given wetting-phase saturation has been seen
     *
     * This updates the scanning curves and the imbibition<->drainage reversal points as
     * appropriate.
     */
    bool update(Scalar pcSw, Scalar krwSw, Scalar krnSw)
    {
        bool updateParams = false;

        if (config().pcHysteresisModel() == 0 && pcSw < pcSwMdc_) {
            if (pcSwMdc_ == 2.0 && pcSw+1.0e-6 < Swcrd_ && (oilWaterSystem_ || gasWaterSystem_)) {
               initialImb_ = true;
            }
            pcSwMdc_ = pcSw;
            updateParams = true;
        }

        if (initialImb_ && pcSw > pcSwMic_) {
            pcSwMic_ = pcSw;
            updateParams = true;
        }

        if (krnSw < krnSwMdc_) {
            krnSwMdc_ = krnSw;
            KrndHy_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_);
            if (config().krHysteresisModel() == 4) {
                KrwdHy_ = EffLawT::twoPhaseSatKrw(drainageParams(), krnSwMdc_);
            }
            updateParams = true;
        }
        if (krwSw > krwSwMdc_) {
            krwSwMdc_ = krwSw; // Only used for output at the moment
        }

        // for non WAG hysteresis we still keep track of the process
        // for output purpose.
        if (!gasOilHysteresisWAG()) {
            this->isDrain_ = (krnSw <= this->krnSwMdc_);
        } else {
            wasDrain_ = isDrain_;

            const Scalar swCur = Swco_ + krnSw - krwSw; // sw = swco + krnSw - krwSw for oil/gas params
            if (swatImbStartNxt_ < 0.0) { // Initial check ...
                swatImbStartNxt_ = std::max(Swco_, swCur);
            }

            if (isDrain_) {
                if (krnSw <= krnSwWAG_+tolWAG_) { // continue along drainage curve
                    krnSwWAG_ = std::min(krnSw, krnSwWAG_);
                    krnSwDrainRevert_ = krnSwWAG_;
                    updateParams = true;
                }
                else { // start new imbibition curve
                    isDrain_ = false;
                    SncrtWAGImb_ = SncrtWAG_;
                    krnSwWAG_ = krnSw;
                    // A next drainage curve may start right away, also for a gas
                    // saturation increase below the tolerance.
                    krnSwDrainStartNxt_ = krnSw;
                    swatImbStartNxt_ = std::max(Swco_, swCur);
                    updateParams = true;
                }
            }
            else {
                if (krnSw >= krnSwWAG_-tolWAG_) { // continue along imbibition curve
                    krnSwWAG_ = std::max(krnSw, krnSwWAG_);
                    krnSwDrainStartNxt_ = krnSwWAG_;
                    // Water saturation at the start of the next drainage curve
                    swatImbStartNxt_ = std::max(Swco_, swCur);
                    updateParams = true;
                }
                else { // start new drainage curve
                    // Gas immobile at the reversal point (e.g. waiting for gas to arrive):
                    // the drainage starts when the gas arrives, at the current water saturation.
                    const bool gasArrival = (nState_ == 1) &&
                        (1.0 - krnSwDrainStartNxt_ <= Sncrd_ + tolWAG_);
                    isDrain_ = true;
                    // If all gas was trapped at the reversal point, the new drainage curve
                    // starts at the trapped saturation.
                    krnSwDrainStart_ = std::min(krnSwDrainStartNxt_, Scalar(1.0 - SncrtWAG_));
                    swatImbStart_ = gasArrival ? std::max(Swco_, swCur) : swatImbStartNxt_;
                    krnSwWAG_ = krnSw;
                    updateParams = true;
                }
            }

            // Two-phase drainage (primary or retracing the primary imbibition curve) turns
            // into three-phase drainage as soon as the water saturation exceeds the
            // threshold. This also covers an initial three-phase state. A drainage started
            // in this update keeps the origin set by the reversal above.
            if (isDrain_ && wasDrain_ && nState_ == 1 && krwSw > tolWAG_ &&
                swCur > Swco_ + wagConfig().wagWaterThresholdSaturation())
            {
                if (1.0 - krnSw <= Sncrd_ + tolWAG_) {
                    // No mobile gas yet: wait for the gas to arrive (in an imbibition
                    // state) such that the drainage starts at the actual water saturation.
                    isDrain_ = false;
                    krnSwWAG_ = krnSw;
                    krnSwDrainStartNxt_ = krnSw;
                    swatImbStartNxt_ = swCur;
                }
                else {
                    // Start secondary drainage from the current point on the two-phase curve.
                    // Same curve selection as in EclHysteresisTwoPhaseLaw::twoPhaseSatKrn().
                    krnImbStartNxt_ = (krnSw <= krnSwMdc_)
                        ? EffLawT::twoPhaseSatKrn(drainageParams(), krnSw)
                        : EffLawT::twoPhaseSatKrn(drainageParams(), computeSwf(krnSw));
                    krnSwDrainStart_ = std::min(krnSw, Scalar(1.0 - SncrtWAGImb_));
                    krnSwDrainStartNxt_ = krnSw;
                    swatImbStart_ = swCur;
                    swatImbStartNxt_ = swCur;
                    krnSwWAG_ = krnSw;
                    krnSwDrainRevert_ = krnSw;
                    wasDrain_ = false; // Signal start of new drainage cycle
                }
                updateParams = true;
            }

        }

        if (updateParams)
            updateDynamicParams_();

        return updateParams;
    }

    HysteresisDynamicState captureState() const
    {
        return {
            .deltaSwImbKrn = deltaSwImbKrn_,
            .Sncrt = Sncrt_,
            .Swcrt = Swcrt_,
            .pcSwMdc = pcSwMdc_,
            .pcSwMic = pcSwMic_,
            .krnSwMdc = krnSwMdc_,
            .krwSwMdc = krwSwMdc_,
            .KrndHy = KrndHy_,
            .KrwdHy = KrwdHy_,
            .Krwd_sncrt = Krwd_sncrt_,
            .SncrtWAG = SncrtWAG_,
            .SncrtWAGPrev = SncrtWAGPrev_,
            .SncrtWAGImb = SncrtWAGImb_,
            .baseWAG = baseWAG_,
            .aTransf = aTransf_,
            .cTransf = cTransf_,
            .swatImbStart = swatImbStart_,
            .swatImbStartNxt = swatImbStartNxt_,
            .krnSwWAG = krnSwWAG_,
            .krnSwDrainRevert = krnSwDrainRevert_,
            .krnSwDrainStart = krnSwDrainStart_,
            .krnSwDrainStartNxt = krnSwDrainStartNxt_,
            .krnImbStart = krnImbStart_,
            .krnImbStartNxt = krnImbStartNxt_,
            .krnDrainStart = krnDrainStart_,
            .krnDrainStartNxt = krnDrainStartNxt_,
            .krnSwImbStart = krnSwImbStart_,
            .nState = nState_,
            .initialImb = initialImb_,
            .isDrain = isDrain_,
            .wasDrain = wasDrain_,
            .wagActive = gasOilHysteresisWAG()
        };
    }

    void restoreState(const HysteresisDynamicState& state)
    {
        pcSwMdc_ = state.pcSwMdc;
        pcSwMic_ = state.pcSwMic;
        krnSwMdc_ = state.krnSwMdc;
        krwSwMdc_ = state.krwSwMdc;
        initialImb_ = state.initialImb;
        isDrain_ = state.isDrain;

        if (gasOilHysteresisWAG()) {
            // WAG has additional cycle history that cannot be reconstructed
            // from the conventional extrema.
            deltaSwImbKrn_ = state.deltaSwImbKrn;
            Sncrt_ = state.Sncrt;
            Swcrt_ = state.Swcrt;
            KrndHy_ = state.KrndHy;
            KrwdHy_ = state.KrwdHy;
            Krwd_sncrt_ = state.Krwd_sncrt;
            if (config().krHysteresisModel() == 4) {
                updateKilloughWettingAnchor_();
            }
            SncrtWAG_ = state.SncrtWAG;
            SncrtWAGPrev_ = state.SncrtWAGPrev;
            SncrtWAGImb_ = state.SncrtWAGImb;
            baseWAG_ = state.baseWAG;
            aTransf_ = state.aTransf;
            cTransf_ = state.cTransf;
            swatImbStart_ = state.swatImbStart;
            swatImbStartNxt_ = state.swatImbStartNxt;
            krnSwWAG_ = state.krnSwWAG;
            krnSwDrainRevert_ = state.krnSwDrainRevert;
            krnSwDrainStart_ = state.krnSwDrainStart;
            krnSwDrainStartNxt_ = state.krnSwDrainStartNxt;
            krnImbStart_ = state.krnImbStart;
            krnImbStartNxt_ = state.krnImbStartNxt;
            krnDrainStart_ = state.krnDrainStart;
            krnDrainStartNxt_ = state.krnDrainStartNxt;
            krnSwImbStart_ = state.krnSwImbStart;
            nState_ = state.nState;
            wasDrain_ = state.wasDrain;
            return;
        }

        // The compact snapshot does not store values derived from the
        // extrema. Rebuild them without entering the stateful WAG update.
        if (krnSwMdc_ < 2.0) {
            KrndHy_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_);
            if (config().krHysteresisModel() == 4)
                KrwdHy_ = EffLawT::twoPhaseSatKrw(drainageParams(), krnSwMdc_);
        }
        updateConventionalDerivedParams_();
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        // only serializes dynamic state - see update() and updateDynamic_()
        serializer(deltaSwImbKrn_);
        //serializer(deltaSwImbKrw_);
        serializer(Sncrt_);
        serializer(Swcrt_);
        serializer(initialImb_);
        serializer(pcSwMic_);
        serializer(krnSwMdc_);
        serializer(krwSwMdc_);
        serializer(KrndHy_);
        serializer(KrwdHy_);
    }

    bool operator==(const EclHysteresisTwoPhaseLawParams& rhs) const
    {
        return this->deltaSwImbKrn_ == rhs.deltaSwImbKrn_ &&
               //this->deltaSwImbKrw_ == rhs.deltaSwImbKrw_ &&
               this->Sncrt_ == rhs.Sncrt_ &&
               this->Swcrt_ == rhs.Swcrt_ &&
               this->initialImb_ == rhs.initialImb_ &&
               this->pcSwMic_ == rhs.pcSwMic_ &&
               this->krnSwMdc_ == rhs.krnSwMdc_ &&
               this->krwSwMdc_ == rhs.krwSwMdc_ &&
               this->KrndHy_ == rhs.KrndHy_ &&
               this->KrwdHy_ == rhs.KrwdHy_;
    }

private:
    void updateDynamicParams_()
    {
        updateConventionalDerivedParams_();

        if (gasOilHysteresisWAG()) {
            updateWagDerivedParams_();
        }
    }

    void updateConventionalDerivedParams_()
    {
        // calculate the saturation deltas for the relative permeabilities
        //if (false) { // we dont support Carlson for wetting phase hysteresis
            //Scalar krwMdcDrainage = EffLawT::twoPhaseSatKrw(drainageParams(), krwSwMdc_);
            //Scalar SwKrwMdcImbibition = EffLawT::twoPhaseSatKrwInv(imbibitionParams(), krwMdcDrainage);
            //deltaSwImbKrw_ = SwKrwMdcImbibition - krwSwMdc_;
        //}

        if (config().krHysteresisModel() == 0 || config().krHysteresisModel() == 1) {
            Scalar krnMdcDrainage = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_);
            Scalar SwKrnMdcImbibition = EffLawT::twoPhaseSatKrnInv(imbibitionParams(), krnMdcDrainage);
            deltaSwImbKrn_ = SwKrnMdcImbibition - krnSwMdc_;
        }

        // Scalar pcMdcDrainage = EffLawT::twoPhaseSatPcnw(drainageParams(), pcSwMdc_);
        // Scalar SwPcMdcImbibition = EffLawT::twoPhaseSatPcnwInv(imbibitionParams(), pcMdcDrainage);
        // deltaSwImbPc_ = SwPcMdcImbibition - pcSwMdc_;

        if (config().krHysteresisModel() == 2 ||
            config().krHysteresisModel() == 3 ||
            config().krHysteresisModel() == 4 ||
            config().pcHysteresisModel() == 0)
        {
            const Scalar snhy = 1.0 - krnSwMdc_;
            if (snhy > Sncrd_) {
                Sncrt_ = Sncrd_ + (snhy - Sncrd_) /
                        ((1.0 + config().modParamTrapped()*(Snmaxd_ - snhy)) + C_ * (snhy - Sncrd_));
            }
            else {
                Sncrt_ = Sncrd_;
            }
        }

        if (config().krHysteresisModel() == 4) {
            Scalar swhy = krnSwMdc_;
            if (swhy >= Swcrd_) {
                Swcrt_ = Swcrd_ + (swhy - Swcrd_) /
                        ((1.0 + config().modParamTrapped() * (Swmaxd_ - swhy)) + Cw_ * (swhy - Swcrd_));
            } else {
                Swcrt_ = Swcrd_;
            }
            Krwd_sncrt_ = EffLawT::twoPhaseSatKrw(drainageParams(), 1 - Sncrt());
            updateKilloughWettingAnchor_();
        }
    }

    void updateKilloughWettingAnchor_()
    {
        const Scalar SwT = 1.0 - Sncrt();
        Krwi_sncrt_ = EffLawT::twoPhaseSatKrw(imbibitionParams(), SwT);
        const Scalar Krwd = Krwd_sncrt_;
        const Scalar Krwi = EclHysteresisDetail::krwImbibitionExtended<EffLawT, Scalar>(drainageParams(),
                                                                                        imbibitionParams(),
                                                                                        SwT);
        KrwTrappedAnchor_ = std::min(std::max(krwiAtTrapped(), std::min(Krwd, Krwi)), std::max(Krwd, Krwi));
    }

    void updateWagDerivedParams_()
    {
        if (isDrain_ && krnSwMdc_ == krnSwWAG_) {
            Scalar snhy = 1.0 - krnSwMdc_;
            SncrtWAG_ = Sncrd_;
            if (snhy > Sncrd_) {
                SncrtWAG_ += (snhy - Sncrd_) /
                    (1.0 + config().modParamTrapped() * (Snmaxd_ - snhy) +
                     wagConfig().wagLandsParam() * (snhy - Sncrd_));
            }
        }

        if (!wasDrain_ && isDrain_) { // Start of new drainage cycle
            if (threePhaseState() || nState_ > 1) { // Never return to primary (two-phase) state after leaving
                nState_ += 1;
                SncrtWAGPrev_ = SncrtWAGImb_;
                krnDrainStart_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwDrainStart_);
                krnImbStart_ = krnImbStartNxt_;
                // Scanning shift for primary drainage
                krnSwImbStart_ = EffLawT::twoPhaseSatKrnInv(drainageParams(), krnImbStart_);
            }
        }

        if (isDrain_) { // Parameters for a reversal from drain to imb
            // Primary (two-phase) imbibition curves are based on the critical saturation
            // and the historical maximum. Subsequent curves are based on the trapped
            // saturation from the previous cycle and the current reversal point.
            baseWAG_ = (nState_ == 1) ? Sncrd_ : SncrtWAGPrev_;
            const Scalar snRev = (nState_ == 1) ? Snhy() : 1.0 - krnSwDrainRevert_;
            aTransf_ = 1.0;
            if (baseWAG_ <= Sncrd_) {
                aTransf_ += config().modParamTrapped() * (Snmaxd_ - Snhy());
            }
            if (snRev > SncrtWAG_) {
                const Scalar C = 1.0 / (SncrtWAG_ - baseWAG_ + saturationEpsilon_) - aTransf_ / (snRev - baseWAG_);
                // For a vanishing free gas saturation at the reversal point, the two
                // terms cancel and round-off may give a non-positive constant, for
                // which the free gas saturation in computeSwf() is undefined (NaN).
                // The shape of the curve is immaterial then, so use Land's parameter.
                cTransf_ = (C > 0.0 && std::isfinite(C)) ? C : Scalar(wagConfig().wagLandsParam());
            }
        }

        if (!isDrain_) { // Starting or moving along current imb curve
            krnDrainStartNxt_ = EffLawT::twoPhaseSatKrn(drainageParams(), std::min(krnSwWAG_, Scalar(1.0 - SncrtWAG_)));
            if (nState_ > 1) {
                krnImbStartNxt_ = computeKrImbWAG(krnSwWAG_);
            }
            else {
                Scalar swf = computeSwf(krnSwWAG_);
                krnImbStartNxt_ = EffLawT::twoPhaseSatKrn(drainageParams(), swf);
            }
        }
    }

    EclHysteresisConfig config_{};
    std::shared_ptr<WagHysteresisConfig::WagHysteresisConfigRecord> wagConfig_{};
    EffLawParams imbibitionParams_{};
    EffLawParams drainageParams_{};

    // largest wettinging phase saturation which is on the main-drainage curve. These are
    // three different values because the sourounding code can choose to use different
    // definitions for the saturations for different quantities
    Scalar krwSwMdc_{-2.0};
    Scalar krnSwMdc_{2.0};
    Scalar pcSwMdc_{2.0};

    // largest wettinging phase saturation along main imbibition curve
    Scalar pcSwMic_{1.0};
    // Initial process is imbibition (for initial saturations at or below critical drainage saturation)
    bool initialImb_{false};

    bool oilWaterSystem_{false};
    bool gasOilSystem_{false};
    bool gasWaterSystem_{false};


    // offsets added to wetting phase saturation uf using the imbibition curves need to
    // be used to calculate the wetting phase relperm, the non-wetting phase relperm and
    // the capillary pressure
    //Scalar deltaSwImbKrw_{};
    Scalar deltaSwImbKrn_{};
    //Scalar deltaSwImbPc_;

    // the following uses the conventions of the Eclipse technical description:
    //
    // Sncrd_: critical non-wetting phase saturation for the drainage curve
    // Sncri_: critical non-wetting phase saturation for the imbibition curve
    // Swcri_: critical wetting phase saturation for the imbibition curve
    // Swcrd_: critical wetting phase saturation for the drainage curve
    // Swmaxi_; maximum wetting phase saturation for the imbibition curve
    // Snmaxd_: non-wetting phase saturation where the non-wetting relperm reaches its
    //          maximum on the drainage curve
    // C_: factor required to calculate the trapped non-wetting phase saturation using
    //     the Killough approach
    // Cw_: factor required to calculate the trapped wetting phase saturation using
    //     the Killough approach
    // Swcod_: connate water saturation value used for wag hysteresis (2. drainage)
    Scalar Sncrd_{};
    Scalar Sncri_{};
    Scalar Swcri_{};
    Scalar Swcrd_{};
    Scalar Swmaxi_{};
    Scalar Snmaxd_{};
    Scalar Swmaxd_{};
    Scalar C_{};

    Scalar KrndMax_{}; // Krn_drain(Snmaxd_)
    Scalar KrwdMax_{}; // Krw_drain(Swmaxd_)
    Scalar KrndHy_{};  // Krn_drain(1-krnSwMdc_)


    // For wetting hysterese Killough
    Scalar Cw_{};
    Scalar KrwdHy_{};
    Scalar Krwd_sncri_{};
    Scalar Krwi_snmax_{};
    Scalar Krwi_snrmax_{};
    Scalar Krwd_sncrt_{};
    Scalar Krwi_sncrt_{};
    Scalar KrwTrappedAnchor_{};
    Scalar Swcrt_{}; // trapped wetting phase saturation

    Scalar pcmaxd_{};  // max pc for drain
    Scalar pcmaxi_{};  // max pc for imb

    Scalar curvatureCapPrs_{}; // curvature parameter used for capillary pressure hysteresis

    Scalar Sncrt_{}; // trapped non-wetting phase saturation

    // Used for WAG hysteresis
    Scalar Swco_{};                // Connate water.
    Scalar swatImbStart_{};        // Water saturation at start of current drainage curve (end of previous imb curve).
    Scalar swatImbStartNxt_{};     // Water saturation at start of next drainage curve (end of current imb curve).
    Scalar krnSwWAG_{2.0};         // Saturation value after latest completed timestep.
    Scalar krnSwDrainRevert_{2.0}; // Saturation value at end of current drainage curve.
    Scalar cTransf_{};             // Modified Lands constant used for free gas calculations to obtain consistent scanning curve
                                   //  when reversion to imb occurs above historical maximum gas saturation (i.e. Sw > krwSwMdc_).
    Scalar krnSwDrainStart_{-2.0}; // Saturation value at start of current drainage curve (end of previous imb curve).
    Scalar krnSwDrainStartNxt_{};  // Saturation value at start of current drainage curve (end of previous imb curve).
    Scalar krnImbStart_{};         // Relperm at start of current drainage curve (end of previous imb curve).
    Scalar krnImbStartNxt_{};      // Relperm at start of next drainage curve (end of current imb curve).
    Scalar krnDrainStart_{};       // Primary (input) relperm evaluated at start of current drainage curve.
    Scalar krnDrainStartNxt_{};    // Primary (input) relperm evaluated at start of next drainage curve.
    bool isDrain_{true};           // Status is either drainage or imbibition
    bool wasDrain_{};              // Previous status.
    Scalar krnSwImbStart_{};       // Saturation value where primary drainage relperm equals krnImbStart_

    int nState_{};                 // Number of cycles. Primary cycle is nState_=1.

    Scalar SncrtWAG_{};
    Scalar SncrtWAGPrev_{};        // Trapped saturation at the end of the previous cycle.
    Scalar SncrtWAGImb_{};         // Trapped saturation at the latest drain->imb reversal.
    Scalar baseWAG_{};             // Lower end of the free saturation range for the current imbibition curve.
    Scalar aTransf_{1.0};          // Killough modification factor used for free gas calculations.
    Scalar tolWAG_{0.001};

    // Regularisation of saturation differences used as denominators.
    static constexpr Scalar saturationEpsilon_ = 1.0e-12;
};

} // namespace Opm

#endif
