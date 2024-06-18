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

#include <cassert>
#include <cmath>
#include <memory>
namespace Opm {
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

    EclHysteresisTwoPhaseLawParams()
    {
        // These are initialized to two (even though they represent saturations)
        // to signify that the values are larger than physically possible, and force
        // using the drainage curve before the first saturation update.
        pcSwMdc_ = 2.0;
        krnSwMdc_ = 2.0;
        krwSwMdc_ = -2.0;
        krnSwDrainRevert_ = 2.0;
        krnSwDrainStart_ = -2.0;
        krnSwWAG_ = 2.0;

        pcSwMic_ = -1.0;
        initialImb_ = false;
        oilWaterSystem_ = false;
        gasOilSystem_ = false;
        pcmaxd_ = 0.0;
        pcmaxi_ = 0.0;

        deltaSwImbKrn_ = 0.0;
        //deltaSwImbKrw_ = 0.0;

        Swco_ = 0.0;
        swatImbStart_ = 0.0;
        isDrain_ = true;
        cTransf_ = 0.0;
        tolWAG_ = 0.001;
        nState_ = 0;
    }

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
                C_ = 1.0/(Sncri_ - Sncrd_ + 1.0e-12) - 1.0/(Snmaxd_ - Sncrd_);
                curvatureCapPrs_ =  config().curvatureCapPrs();
            }
            if (config().krHysteresisModel() == 4) {
                Cw_ = 1.0/(Swcri_ - Swcrd_ + 1.0e-12) - 1.0/(Swmaxd_ - Swcrd_);
            }
            updateDynamicParams_();
        }

        EnsureFinalized :: finalize();
    }

    /*!
     * \brief Set the hysteresis configuration object.
     */
    void setConfig(std::shared_ptr<EclHysteresisConfig> value)
    { config_ = *value; }

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
                Swmaxi_ = 1.0 - info.Sgl;
                pcmaxi_ = info.maxPcgo + info.maxPcow;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Swmaxi_ = info.Swu;
                pcmaxi_ = info.maxPcow;
            }
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

    Scalar krwWght() const
    { 
        return KrwdHy_/KrwdMax_; }

    Scalar krwdMax() const
    { 
        return KrwdMax_; }

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
        Evaluation SgT = 1.0 - Sw - SncrtWAG(); // Sg-Sg_crit_trapped
        Scalar SgCut = wagConfig().wagImbCurveLinearFraction()*(Snhy()- SncrtWAG());
        Evaluation Swf = 1.0;
        //Scalar C = wagConfig().wagLandsParam();
        Scalar C = cTransf_;

        if (SgT > SgCut) {
            Swf -= (Sncrd() + 0.5*( SgT + Opm::sqrt( SgT*SgT + 4.0/C*SgT))); // 1-Sgf
        }
        else {
            SgCut = std::max(Scalar(0.000001), SgCut);
            Scalar SgCutValue = 0.5*( SgCut + Opm::sqrt( SgCut*SgCut + 4.0/C*SgCut));
            Scalar SgCutSlope = SgCutValue/SgCut;
            SgT *= SgCutSlope;
            Swf -= (Sncrd() + SgT);
        }

        return Swf;
    }

    template <class Evaluation>
    Evaluation computeKrImbWAG(const Evaluation& Sw)  const
    {
        Evaluation Swf = Sw;
        if (nState_ <= 2)  // Skipping for "higher order" curves seems consistent with benchmark, further investigations needed ...
            Swf = computeSwf(Sw);
        if (Swf <= krnSwDrainStart_) { // Use secondary drainage curve
            Evaluation Krg = EffLawT::twoPhaseSatKrn(drainageParams_, Swf);
            Evaluation KrgImb2 = (Krg-krnDrainStart_)*reductionDrain() + krnImbStart_;
            return KrgImb2;
        }
        else { // Fallback to primary drainage curve
            Evaluation Sn = Sncrd_;
            if (Swf < 1.0-SncrtWAG_) {
                // Notation: Sn.. = Sg.. + Swco
                Evaluation dd = (1.0-krnSwImbStart_ - Sncrd_) / (1.0-krnSwDrainStart_ - SncrtWAG_);
                Sn += (1.0-Swf-SncrtWAG_)*dd;
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
            if (pcSwMdc_ == 2.0 && pcSw+1.0e-6 < Swcrd_ && oilWaterSystem_) {
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
            updateParams = true;
        }

        if (krwSw > krwSwMdc_) {
            krwSwMdc_ = krwSw;
            KrwdHy_ = EffLawT::twoPhaseSatKrw(drainageParams(), krwSwMdc_);
            updateParams = true;
        }

        // for non WAG hysteresis we still keep track of the process
        // for output purpose.
        if (!gasOilHysteresisWAG()) {
            this->isDrain_ = (krnSw <= this->krnSwMdc_);
        } else {
            wasDrain_ = isDrain_;

            if (swatImbStartNxt_ < 0.0) { // Initial check ...
                swatImbStartNxt_ = std::max(Swco_, Swco_ + krnSw - krwSw);
                // check if we are in threephase state sw > swco + tolWag and so > tolWag
                // (sw = swco + krnSw - krwSw and so = krwSw for oil/gas params)
                if ( (swatImbStartNxt_ > Swco_ + tolWAG_) && krwSw > tolWAG_) {
                    swatImbStart_ = swatImbStartNxt_;
                    krnSwWAG_ = krnSw;
                    krnSwDrainStartNxt_ = krnSwWAG_;
                    krnSwDrainStart_ = krnSwDrainStartNxt_;
                    wasDrain_ = false; // Signal start from threephase state ...
                }
            }

            if (isDrain_) {
                if (krnSw <= krnSwWAG_+tolWAG_) { // continue along drainage curve
                    krnSwWAG_ = std::min(krnSw, krnSwWAG_);
                    krnSwDrainRevert_ = krnSwWAG_;
                    updateParams = true;
                }
                else { // start new imbibition curve
                    isDrain_ = false;
                    krnSwWAG_ = krnSw;
                    updateParams = true;
                }
            }
            else {
                if (krnSw >= krnSwWAG_-tolWAG_) { // continue along imbibition curve
                    krnSwWAG_ = std::max(krnSw, krnSwWAG_);
                    krnSwDrainStartNxt_ = krnSwWAG_;
                    swatImbStartNxt_ = std::max(swatImbStartNxt_, Swco_ + krnSw - krwSw);
                    updateParams = true;
                }
                else { // start new drainage curve
                    isDrain_ = true;
                    krnSwDrainStart_ = krnSwDrainStartNxt_;
                    swatImbStart_ = swatImbStartNxt_;
                    krnSwWAG_ = krnSw;
                    updateParams = true;
                }
            }

        }

        if (updateParams)
            updateDynamicParams_();

        return updateParams;
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

        if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().krHysteresisModel() == 4 || config().pcHysteresisModel() == 0) {
            Scalar Snhy = 1.0 - krnSwMdc_;
            if (Snhy > Sncrd_) {
                Sncrt_ = Sncrd_ + (Snhy - Sncrd_)/((1.0+config().modParamTrapped()*(Snmaxd_-Snhy)) + C_*(Snhy - Sncrd_));
            }
            else
            {
                Sncrt_ = Sncrd_;
            }
        }

        if (config().krHysteresisModel() == 4) {
            Scalar Swhy = krwSwMdc_;
            if (Swhy >= Swcrd_) {
                Swcrt_ = Swcrd_ + (Swhy - Swcrd_)/((1.0+config().modParamTrapped()*(Swmaxd_-Swhy)) + Cw_*(Swhy - Swcrd_));
            } else
            {
                Swcrt_ = Swcrd_;
            }
        }


        if (gasOilHysteresisWAG()) {
            if (isDrain_ && krnSwMdc_ == krnSwWAG_) {
                Scalar Snhy = 1.0 - krnSwMdc_;
                SncrtWAG_ = Sncrd_;
                if (Snhy > Sncrd_) {
                    SncrtWAG_ += (Snhy - Sncrd_)/(1.0+config().modParamTrapped()*(Snmaxd_-Snhy) + wagConfig().wagLandsParam()*(Snhy - Sncrd_));
                }
            }

            if (isDrain_ && (1.0-krnSwDrainRevert_) > SncrtWAG_) { //Reversal from drain to imb
                cTransf_ = 1.0/(SncrtWAG_-Sncrd_ + 1.0e-12) - 1.0/(1.0-krnSwDrainRevert_-Sncrd_);
            }

            if (!wasDrain_ && isDrain_) { // Start of new drainage cycle
                if (threePhaseState() || nState_>1) { // Never return to primary (two-phase) state after leaving
                    nState_ += 1;
                    krnDrainStart_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwDrainStart_);
                    krnImbStart_ = krnImbStartNxt_;
                    // Scanning shift for primary drainage
                    krnSwImbStart_ = EffLawT::twoPhaseSatKrnInv(drainageParams(), krnImbStart_);
                }
            }

            if (!wasDrain_ && !isDrain_) { //Moving along current imb curve
                krnDrainStartNxt_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwWAG_);
                if (threePhaseState()) {
                    krnImbStartNxt_ = computeKrImbWAG(krnSwWAG_);
                }
                else {
                    Scalar swf = computeSwf(krnSwWAG_);
                    krnImbStartNxt_ = EffLawT::twoPhaseSatKrn(drainageParams(), swf);
                }
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
    Scalar krwSwMdc_;
    Scalar krnSwMdc_;
    Scalar pcSwMdc_;

    // largest wettinging phase saturation along main imbibition curve
    Scalar pcSwMic_;
    // Initial process is imbibition (for initial saturations at or below critical drainage saturation)
    bool initialImb_;

    bool oilWaterSystem_;
    bool gasOilSystem_;


    // offsets added to wetting phase saturation uf using the imbibition curves need to
    // be used to calculate the wetting phase relperm, the non-wetting phase relperm and
    // the capillary pressure
    //Scalar deltaSwImbKrw_;
    Scalar deltaSwImbKrn_;
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
    Scalar Cw_{};

    Scalar KrndMax_{}; // Krn_drain(Snmaxd_)
    Scalar KrwdMax_{}; // Krw_drain(Swmaxd_)
    Scalar KrndHy_{};  // Krn_drain(1-krnSwMdc_)
    Scalar KrwdHy_{};  // Krw_drain(1-krwSwMdc_)

    Scalar pcmaxd_;  // max pc for drain
    Scalar pcmaxi_;  // max pc for imb

    Scalar curvatureCapPrs_{}; // curvature parameter used for capillary pressure hysteresis

    Scalar Sncrt_{}; // trapped non-wetting phase saturation
    Scalar Swcrt_{}; // trapped wetting phase saturation

    // Used for WAG hysteresis
    Scalar Swco_;               // Connate water.
    Scalar swatImbStart_;     // Water saturation at start of current drainage curve (end of previous imb curve).
    Scalar swatImbStartNxt_{};    // Water saturation at start of next drainage curve (end of current imb curve).
    Scalar krnSwWAG_;           // Saturation value after latest completed timestep.
    Scalar krnSwDrainRevert_;   // Saturation value at end of current drainage curve.
    Scalar cTransf_;            // Modified Lands constant used for free gas calculations to obtain consistent scanning curve
                                //  when reversion to imb occurs above historical maximum gas saturation (i.e. Sw > krwSwMdc_).
    Scalar krnSwDrainStart_;    // Saturation value at start of current drainage curve (end of previous imb curve).
    Scalar krnSwDrainStartNxt_{}; // Saturation value at start of current drainage curve (end of previous imb curve).
    Scalar krnImbStart_{};      // Relperm at start of current drainage curve (end of previous imb curve).
    Scalar krnImbStartNxt_{};   // Relperm at start of next drainage curve (end of current imb curve).
    Scalar krnDrainStart_{};    // Primary (input) relperm evaluated at start of current drainage curve.
    Scalar krnDrainStartNxt_{}; // Primary (input) relperm evaluated at start of next drainage curve.
    bool isDrain_;              // Status is either drainage or imbibition
    bool wasDrain_{};           // Previous status.
    Scalar krnSwImbStart_{};    // Saturation value where primary drainage relperm equals krnImbStart_

    int nState_;                // Number of cycles. Primary cycle is nState_=1.

    Scalar SncrtWAG_{};
    Scalar tolWAG_;
};

} // namespace Opm

#endif
