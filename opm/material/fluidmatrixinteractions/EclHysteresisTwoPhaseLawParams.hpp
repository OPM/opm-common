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

#include <opm/material/common/EnsureFinalized.hpp>
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
        // krwSwMdc_ = 2.0;

        pcSwMic_ = -1.0;
        initialImb_ = false;
        oilWaterSystem_ = false;
        pcmaxd_ = 0.0;
        pcmaxi_ = 0.0;

        deltaSwImbKrn_ = 0.0;
        // deltaSwImbKrw_ = 0.0;
    }

    static EclHysteresisTwoPhaseLawParams serializationTestObject()
    {
        EclHysteresisTwoPhaseLawParams<EffLawT> result;
        result.deltaSwImbKrn_ = 1.0;
        result.Sncrt_ = 2.0;
        result.initialImb_ = true;
        result.pcSwMic_ = 3.0;
        result.krnSwMdc_ = 4.0;
        result.KrndHy_ = 5.0;

        return result;
    }

    /*!
     * \brief Calculate all dependent quantities once the independent
     *        quantities of the parameter object have been set.
     */
    void finalize()
    {
        if (config().enableHysteresis()) {
            if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().pcHysteresisModel() == 0) {
                C_ = 1.0/(Sncri_ - Sncrd_ + 1.0e-12) - 1.0/(Snmaxd_ - Sncrd_);
                curvatureCapPrs_ =  config().curvatureCapPrs();
            }

            updateDynamicParams_();
        }

        EnsureFinalized :: finalize();
    }

    /*!
     * \brief Set the endpoint scaling configuration object.
     */
    void setConfig(std::shared_ptr<EclHysteresisConfig> value)
    { config_ = *value; }

    /*!
     * \brief Returns the endpoint scaling configuration object.
     */
    const EclHysteresisConfig& config() const
    { return config_; }

    /*!
     * \brief Sets the parameters used for the drainage curve
     */
    void setDrainageParams(const EffLawParams& value,
                           const EclEpsScalingPointsInfo<Scalar>& info,
                           EclTwoPhaseSystemType twoPhaseSystem)
    {
        drainageParams_ = value;

        oilWaterSystem_ = (twoPhaseSystem == EclTwoPhaseSystemType::OilWater);

        if (!config().enableHysteresis())
            return;

        if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Sncrd_ = info.Sgcr+info.Swl;
                Snmaxd_ = info.Sgu+info.Swl;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
            else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Sncrd_ = info.Sgcr;
                Snmaxd_ = info.Sgu;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Sncrd_ = info.Sowcr;
                Snmaxd_ = 1.0 - info.Swl - info.Sgl;
                KrndMax_ = EffLawT::twoPhaseSatKrn(drainageParams(), 1.0-Snmaxd_);
            }
        }

        // Additional Killough hysteresis model for pc
        if (config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Swcrd_ = info.Sogcr;
                pcmaxd_ = info.maxPcgo;
            } else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Swcrd_ = info.Swcr;
                pcmaxd_ = info.maxPcgo + info.maxPcow;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Swcrd_ = info.Swcr;
                pcmaxd_ = -17.0; // At this point 'info.maxPcow' holds pre-swatinit value ...;
            }
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

        // Killough hysteresis model for nonw kr
        if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Sncri_ = info.Sgcr+info.Swl;
            }
            else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Sncri_ = info.Sgcr;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Sncri_ = info.Sowcr;
            }
        }

        // Killough hysteresis model for pc
        if (config().pcHysteresisModel() == 0) {
            if (twoPhaseSystem == EclTwoPhaseSystemType::GasOil) {
                Swcri_ = info.Sogcr;
                Swmaxi_ = 1.0 - info.Sgl - info.Swl;
                pcmaxi_ = info.maxPcgo;
            } else if (twoPhaseSystem == EclTwoPhaseSystemType::GasWater) {
                Swcri_ = info.Swcr;
                Swmaxi_ = 1.0 - info.Sgl;
                pcmaxi_ = info.maxPcgo + info.maxPcow;
            }
            else {
                assert(twoPhaseSystem == EclTwoPhaseSystemType::OilWater);
                Swcri_ = info.Swcr;
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
    void setKrwSwMdc(Scalar /* value */)
    {}
    //    { krwSwMdc_ = value; };

    /*!
     * \brief Get the saturation of the wetting phase where the last switch from the main
     *        drainage curve to imbibition happend on the relperm curve for the
     *        wetting phase.
     */
    Scalar krwSwMdc() const
    { return 2.0; }
    //    { return krwSwMdc_; };

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
    void setDeltaSwImbKrw(Scalar /* value */)
    {}
    //    { deltaSwImbKrw_ = value; }

    /*!
     * \brief Returns the saturation value which must be added if krw is calculated using
     *        the imbibition curve.
     *
     * This means that krw(Sw) = krw_drainage(Sw) if Sw < SwMdc and
     * krw(Sw) = krw_imbibition(Sw + Sw_shift,krw) else
     */
    Scalar deltaSwImbKrw() const
    { return 0.0; }
//    { return deltaSwImbKrw_; }

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

    Scalar Snmaxd() const
    { return Snmaxd_; }

    Scalar Snhy() const
    { return 1.0 - krnSwMdc_; }

    Scalar krnWght() const
    { return KrndHy_/KrndMax_; }

    Scalar pcWght() const // Aligning pci and pcd at Swir
    {
        if (pcmaxd_ < 0.0)
            return EffLawT::twoPhaseSatPcnw(drainageParams(), 0.0)/(pcmaxi_+1e-6);
        else
            return pcmaxd_/(pcmaxi_+1e-6);
    }

    Scalar curvatureCapPrs() const
    { return curvatureCapPrs_;}


    /*!
     * \brief Notify the hysteresis law that a given wetting-phase saturation has been seen
     *
     * This updates the scanning curves and the imbibition<->drainage reversal points as
     * appropriate.
     */
    void update(Scalar pcSw, Scalar /* krwSw */, Scalar krnSw)
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

/*
        // This is quite hacky: Eclipse says that it only uses relperm hysteresis for the
        // wetting phase (indicated by '0' for the second item of the EHYSTER keyword),
        // even though this makes about zero sense: one would expect that hysteresis can
        // be limited to the oil phase, but the oil phase is the wetting phase of the
        // gas-oil twophase system whilst it is non-wetting for water-oil.
        if (krwSw < krwSwMdc_)
        {
            krwSwMdc_ = krwSw;
            updateParams = true;
        }
*/

        if (krnSw < krnSwMdc_) {
            krnSwMdc_ = krnSw;
            KrndHy_ = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_);
            updateParams = true;
        }

        if (updateParams)
            updateDynamicParams_();
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        // only serializes dynamic state - see update() and updateDynamic_()
        serializer(deltaSwImbKrn_);
        serializer(Sncrt_);
        serializer(initialImb_);
        serializer(pcSwMic_);
        serializer(krnSwMdc_);
        serializer(KrndHy_);
    }

    bool operator==(const EclHysteresisTwoPhaseLawParams& rhs) const
    {
        return this->deltaSwImbKrn_ == rhs.deltaSwImbKrn_ &&
               this->Sncrt_ == rhs.Sncrt_ &&
               this->initialImb_ == rhs.initialImb_ &&
               this->pcSwMic_ == rhs.pcSwMic_ &&
               this->krnSwMdc_ == rhs.krnSwMdc_ &&
               this->KrndHy_ == rhs.KrndHy_;
    }

private:
    void updateDynamicParams_()
    {
        // HACK: Eclipse seems to disable the wetting-phase relperm even though this is
        // quite pointless from the physical POV. (see comment above)
/*
        // calculate the saturation deltas for the relative permeabilities
        Scalar krwMdcDrainage = EffLawT::twoPhaseSatKrw(drainageParams(), krwSwMdc_);
        Scalar SwKrwMdcImbibition = EffLawT::twoPhaseSatKrwInv(imbibitionParams(), krwMdcDrainage);
        deltaSwImbKrw_ = SwKrwMdcImbibition - krwSwMdc_;
*/
        if (config().krHysteresisModel() == 0 || config().krHysteresisModel() == 1) {
            Scalar krnMdcDrainage = EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_);
            Scalar SwKrnMdcImbibition = EffLawT::twoPhaseSatKrnInv(imbibitionParams(), krnMdcDrainage);
            deltaSwImbKrn_ = SwKrnMdcImbibition - krnSwMdc_;
            assert(std::abs(EffLawT::twoPhaseSatKrn(imbibitionParams(), krnSwMdc_ + deltaSwImbKrn_)
                            - EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_)) < 1e-8);
        }

        // Scalar pcMdcDrainage = EffLawT::twoPhaseSatPcnw(drainageParams(), pcSwMdc_);
        // Scalar SwPcMdcImbibition = EffLawT::twoPhaseSatPcnwInv(imbibitionParams(), pcMdcDrainage);
        // deltaSwImbPc_ = SwPcMdcImbibition - pcSwMdc_;

//        assert(std::abs(EffLawT::twoPhaseSatPcnw(imbibitionParams(), pcSwMdc_ + deltaSwImbPc_)
//                        - EffLawT::twoPhaseSatPcnw(drainageParams(), pcSwMdc_)) < 1e-8);
//        assert(std::abs(EffLawT::twoPhaseSatKrn(imbibitionParams(), krnSwMdc_ + deltaSwImbKrn_)
//                        - EffLawT::twoPhaseSatKrn(drainageParams(), krnSwMdc_)) < 1e-8);
//        assert(std::abs(EffLawT::twoPhaseSatKrw(imbibitionParams(), krwSwMdc_ + deltaSwImbKrw_)
//                        - EffLawT::twoPhaseSatKrw(drainageParams(), krwSwMdc_)) < 1e-8);

        if (config().krHysteresisModel() == 2 || config().krHysteresisModel() == 3 || config().pcHysteresisModel() == 0) {
            Scalar Snhy = 1.0 - krnSwMdc_;
            if (Snhy > Sncrd_)
                Sncrt_ = Sncrd_ + (Snhy - Sncrd_)/((1.0+config().modParamTrapped()*(Snmaxd_-Snhy)) + C_*(Snhy - Sncrd_));
            else
            {
                Sncrt_ = Sncrd_;
            }
        }
    }

    EclHysteresisConfig config_;
    EffLawParams imbibitionParams_;
    EffLawParams drainageParams_;

    // largest wettinging phase saturation which is on the main-drainage curve. These are
    // three different values because the sourounding code can choose to use different
    // definitions for the saturations for different quantities
    //Scalar krwSwMdc_;
    Scalar krnSwMdc_;
    Scalar pcSwMdc_;

    // largest wettinging phase saturation along main imbibition curve
    Scalar pcSwMic_;
    // Initial process is imbibition (for initial saturations at or below critical drainage saturation)
    bool initialImb_;

    bool oilWaterSystem_;

    // offsets added to wetting phase saturation uf using the imbibition curves need to
    // be used to calculate the wetting phase relperm, the non-wetting phase relperm and
    // the capillary pressure
    //Scalar deltaSwImbKrw_;
    Scalar deltaSwImbKrn_;
    //Scalar deltaSwImbPc_;

    // trapped non-wetting phase saturation
    Scalar Sncrt_;

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
    Scalar Sncrd_;
    Scalar Sncri_;
    Scalar Swcri_;
    Scalar Swcrd_;
    Scalar Swmaxi_;
    Scalar Snmaxd_;
    Scalar C_;

    Scalar KrndMax_; // Krn_drain(Snmaxd_)
    Scalar KrndHy_;  // Krn_drain(1-krnSwMdc_)

    Scalar pcmaxd_;  // max pc for drain
    Scalar pcmaxi_;  // max pc for imb

    Scalar curvatureCapPrs_; // curvature parameter used for capillary pressure hysteresis
};

} // namespace Opm

#endif
