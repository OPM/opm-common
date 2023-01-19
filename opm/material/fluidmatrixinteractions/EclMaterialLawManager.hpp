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
 * \copydoc Opm::EclMaterialLawManager
 */
#if ! HAVE_ECL_INPUT
#error "Eclipse input support in opm-common is required to use the ECL material manager!"
#endif

#ifndef OPM_ECL_MATERIAL_LAW_MANAGER_HPP
#define OPM_ECL_MATERIAL_LAW_MANAGER_HPP

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>

#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexer.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclHysteresisTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

namespace Opm {

class EclipseState;
class EclEpsConfig;
template<class Scalar> class EclEpsScalingPoints;
template<class Scalar> struct EclEpsScalingPointsInfo;
class EclHysteresisConfig;
enum class EclTwoPhaseSystemType;
class Runspec;
class SgfnTable;
class SgofTable;
class SlgofTable;
class Sof2Table;
class Sof3Table;

/*!
 * \ingroup fluidmatrixinteractions
 *
 * \brief Provides an simple way to create and manage the material law objects
 *        for a complete ECL deck.
 */
template <class TraitsT>
class EclMaterialLawManager
{
private:
    using Traits = TraitsT;
    using Scalar = typename Traits::Scalar;
    enum { waterPhaseIdx = Traits::wettingPhaseIdx };
    enum { oilPhaseIdx = Traits::nonWettingPhaseIdx };
    enum { gasPhaseIdx = Traits::gasPhaseIdx };
    enum { numPhases = Traits::numPhases };

    using GasOilTraits = TwoPhaseMaterialTraits<Scalar, oilPhaseIdx, gasPhaseIdx>;
    using OilWaterTraits = TwoPhaseMaterialTraits<Scalar, waterPhaseIdx, oilPhaseIdx>;
    using GasWaterTraits = TwoPhaseMaterialTraits<Scalar, waterPhaseIdx, gasPhaseIdx>;

    // the two-phase material law which is defined on effective (unscaled) saturations
    using GasOilEffectiveTwoPhaseLaw = SatCurveMultiplexer<GasOilTraits>;
    using OilWaterEffectiveTwoPhaseLaw = SatCurveMultiplexer<OilWaterTraits>;
    using GasWaterEffectiveTwoPhaseLaw = SatCurveMultiplexer<GasWaterTraits>;

    using GasOilEffectiveTwoPhaseParams = typename GasOilEffectiveTwoPhaseLaw::Params;
    using OilWaterEffectiveTwoPhaseParams = typename OilWaterEffectiveTwoPhaseLaw::Params;
    using GasWaterEffectiveTwoPhaseParams = typename GasWaterEffectiveTwoPhaseLaw::Params;

    // the two-phase material law which is defined on absolute (scaled) saturations
    using GasOilEpsTwoPhaseLaw = EclEpsTwoPhaseLaw<GasOilEffectiveTwoPhaseLaw>;
    using OilWaterEpsTwoPhaseLaw = EclEpsTwoPhaseLaw<OilWaterEffectiveTwoPhaseLaw>;
    using GasWaterEpsTwoPhaseLaw = EclEpsTwoPhaseLaw<GasWaterEffectiveTwoPhaseLaw>;
    using GasOilEpsTwoPhaseParams = typename GasOilEpsTwoPhaseLaw::Params;
    using OilWaterEpsTwoPhaseParams = typename OilWaterEpsTwoPhaseLaw::Params;
    using GasWaterEpsTwoPhaseParams = typename GasWaterEpsTwoPhaseLaw::Params;

    // the scaled two-phase material laws with hystersis
    using GasOilTwoPhaseLaw = EclHysteresisTwoPhaseLaw<GasOilEpsTwoPhaseLaw>;
    using OilWaterTwoPhaseLaw = EclHysteresisTwoPhaseLaw<OilWaterEpsTwoPhaseLaw>;
    using GasWaterTwoPhaseLaw = EclHysteresisTwoPhaseLaw<GasWaterEpsTwoPhaseLaw>;
    using GasOilTwoPhaseHystParams = typename GasOilTwoPhaseLaw::Params;
    using OilWaterTwoPhaseHystParams = typename OilWaterTwoPhaseLaw::Params;
    using GasWaterTwoPhaseHystParams = typename GasWaterTwoPhaseLaw::Params;

public:
    // the three-phase material law used by the simulation
    using MaterialLaw = EclMultiplexerMaterial<Traits, GasOilTwoPhaseLaw, OilWaterTwoPhaseLaw, GasWaterTwoPhaseLaw>;
    using MaterialLawParams = typename MaterialLaw::Params;

    EclMaterialLawManager();
    ~EclMaterialLawManager();

private:
    // internal typedefs
    using GasOilEffectiveParamVector = std::vector<std::shared_ptr<GasOilEffectiveTwoPhaseParams>>;
    using OilWaterEffectiveParamVector = std::vector<std::shared_ptr<OilWaterEffectiveTwoPhaseParams>>;
    using GasWaterEffectiveParamVector = std::vector<std::shared_ptr<GasWaterEffectiveTwoPhaseParams>>;

    using GasOilScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using GasWaterScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingInfoVector = std::vector<EclEpsScalingPointsInfo<Scalar>>;
    using GasOilParamVector = std::vector<std::shared_ptr<GasOilTwoPhaseHystParams>>;
    using OilWaterParamVector = std::vector<std::shared_ptr<OilWaterTwoPhaseHystParams>>;
    using GasWaterParamVector = std::vector<std::shared_ptr<GasWaterTwoPhaseHystParams>>;
    using MaterialLawParamsVector = std::vector<std::shared_ptr<MaterialLawParams>>;

public:
    void initFromState(const EclipseState& eclState);

    void initParamsForElements(const EclipseState& eclState, size_t numCompressedElems);

    /*!
     * \brief Modify the initial condition according to the SWATINIT keyword.
     *
     * The method returns the water saturation which yields a givenn capillary
     * pressure. The reason this method is not folded directly into initFromState() is
     * that the capillary pressure given depends on the particuars of how the simulator
     * calculates its initial condition.
     */
    Scalar applySwatinit(unsigned elemIdx,
                         Scalar pcow,
                         Scalar Sw);

    bool enableEndPointScaling() const
    { return enableEndPointScaling_; }

    bool enableHysteresis() const
    { return hysteresisConfig_->enableHysteresis(); }

    MaterialLawParams& materialLawParams(unsigned elemIdx)
    {
        assert(elemIdx <  materialLawParams_.size());
        return materialLawParams_[elemIdx];
    }

    const MaterialLawParams& materialLawParams(unsigned elemIdx) const
    {
        assert(elemIdx <  materialLawParams_.size());
        return materialLawParams_[elemIdx];
    }

    /*!
     * \brief Returns a material parameter object for a given element and saturation region.
     *
     * This method changes the saturation table idx in the original material law parameter object.
     * In the context of ECL reservoir simulators, this is required to properly handle
     * wells with its own saturation table idx. In order to reset the saturation table idx
     * in the materialLawparams_ call the method with the cells satRegionIdx
     */
    const MaterialLawParams& connectionMaterialLawParams(unsigned satRegionIdx, unsigned elemIdx) const;

    int satnumRegionIdx(unsigned elemIdx) const
    { return satnumRegionArray_[elemIdx]; }

    int getKrnumSatIdx(unsigned elemIdx, FaceDir::DirEnum facedir) const;

    bool hasDirectionalRelperms() const
    {
        return !krnumXArray_.empty() || !krnumYArray_.empty() || !krnumZArray_.empty();
    }

    int imbnumRegionIdx(unsigned elemIdx) const
    { return imbnumRegionArray_[elemIdx]; }

    template <class FluidState>
    void updateHysteresis(const FluidState& fluidState, unsigned elemIdx)
    {
        if (!enableHysteresis())
            return;

        MaterialLaw::updateHysteresis(materialLawParams_[elemIdx], fluidState);
    }

    void oilWaterHysteresisParams(Scalar& pcSwMdc,
                                  Scalar& krnSwMdc,
                                  unsigned elemIdx) const;

    void setOilWaterHysteresisParams(const Scalar& pcSwMdc,
                                     const Scalar& krnSwMdc,
                                     unsigned elemIdx);

    void gasOilHysteresisParams(Scalar& pcSwMdc,
                                Scalar& krnSwMdc,
                                unsigned elemIdx) const;

    void setGasOilHysteresisParams(const Scalar& pcSwMdc,
                                   const Scalar& krnSwMdc,
                                   unsigned elemIdx);

    EclEpsScalingPoints<Scalar>& oilWaterScaledEpsPointsDrainage(unsigned elemIdx);

    const EclEpsScalingPointsInfo<Scalar>& oilWaterScaledEpsInfoDrainage(size_t elemIdx) const
    { return oilWaterScaledEpsInfoDrainage_[elemIdx]; }

private:
    void readGlobalEpsOptions_(const EclipseState& eclState);

    void readGlobalHysteresisOptions_(const EclipseState& state);

    void readGlobalThreePhaseOptions_(const Runspec& runspec);

    template <class Container>
    void readGasOilEffectiveParameters_(Container& dest,
                                        const EclipseState& eclState,
                                        unsigned satRegionIdx);

    void readGasOilEffectiveParametersSgof_(GasOilEffectiveTwoPhaseParams& effParams,
                                            const Scalar Swco,
                                            const double tolcrit,
                                            const SgofTable& sgofTable);

    void readGasOilEffectiveParametersSlgof_(GasOilEffectiveTwoPhaseParams& effParams,
                                             const Scalar Swco,
                                             const double tolcrit,
                                             const SlgofTable& slgofTable);

    void readGasOilEffectiveParametersFamily2_(GasOilEffectiveTwoPhaseParams& effParams,
                                               const Scalar Swco,
                                               const double tolcrit,
                                               const Sof3Table& sof3Table,
                                               const SgfnTable& sgfnTable);

    void readGasOilEffectiveParametersFamily2_(GasOilEffectiveTwoPhaseParams& effParams,
                                               const Scalar Swco,
                                               const double tolcrit,
                                               const Sof2Table& sof2Table,
                                               const SgfnTable& sgfnTable);

    template <class Container>
    void readOilWaterEffectiveParameters_(Container& dest,
                                          const EclipseState& eclState,
                                          unsigned satRegionIdx);

    template <class Container>
    void readGasWaterEffectiveParameters_(Container& dest,
                                        const EclipseState& eclState,
                                        unsigned satRegionIdx);

    template <class Container>
    void readGasOilUnscaledPoints_(Container& dest,
                                   std::shared_ptr<EclEpsConfig> config,
                                   const EclipseState& /* eclState */,
                                   unsigned satRegionIdx);

    template <class Container>
    void readOilWaterUnscaledPoints_(Container& dest,
                                     std::shared_ptr<EclEpsConfig> config,
                                     const EclipseState& /* eclState */,
                                     unsigned satRegionIdx);

    template <class Container>
    void readGasWaterUnscaledPoints_(Container& dest,
                                     std::shared_ptr<EclEpsConfig> config,
                                     const EclipseState& /* eclState */,
                                     unsigned satRegionIdx);

    std::tuple<EclEpsScalingPointsInfo<Scalar>,
               EclEpsScalingPoints<Scalar>>
    readScaledPoints_(const EclEpsConfig& config,
                      const EclipseState& eclState,
                      const EclEpsGridProperties& epsGridProperties,
                      unsigned elemIdx,
                      EclTwoPhaseSystemType type);

    void initThreePhaseParams_(const EclipseState& /* eclState */,
                               MaterialLawParams& materialParams,
                               unsigned satRegionIdx,
                               const EclEpsScalingPointsInfo<Scalar>& epsInfo,
                               std::shared_ptr<OilWaterTwoPhaseHystParams> oilWaterParams,
                               std::shared_ptr<GasOilTwoPhaseHystParams> gasOilParams,
                               std::shared_ptr<GasWaterTwoPhaseHystParams> gasWaterParams);

    bool enableEndPointScaling_;
    std::shared_ptr<EclHysteresisConfig> hysteresisConfig_;

    std::shared_ptr<EclEpsConfig> oilWaterEclEpsConfig_;
    std::vector<EclEpsScalingPointsInfo<Scalar>> unscaledEpsInfo_;
    OilWaterScalingInfoVector oilWaterScaledEpsInfoDrainage_;

    std::shared_ptr<EclEpsConfig> gasWaterEclEpsConfig_;

    GasOilScalingPointsVector gasOilUnscaledPointsVector_;
    OilWaterScalingPointsVector oilWaterUnscaledPointsVector_;
    GasWaterScalingPointsVector gasWaterUnscaledPointsVector_;

    GasOilEffectiveParamVector gasOilEffectiveParamVector_;
    OilWaterEffectiveParamVector oilWaterEffectiveParamVector_;
    GasWaterEffectiveParamVector gasWaterEffectiveParamVector_;

    EclMultiplexerApproach threePhaseApproach_ = EclMultiplexerApproach::Default;
    // this attribute only makes sense for twophase simulations!
    enum EclTwoPhaseApproach twoPhaseApproach_ = EclTwoPhaseApproach::GasOil;

    std::vector<MaterialLawParams> materialLawParams_;

    std::vector<int> satnumRegionArray_;
    std::vector<int> krnumXArray_;
    std::vector<int> krnumYArray_;
    std::vector<int> krnumZArray_;
    std::vector<int> imbnumRegionArray_;
    std::vector<Scalar> stoneEtas;

    bool hasGas;
    bool hasOil;
    bool hasWater;

    std::shared_ptr<EclEpsConfig> gasOilConfig;
    std::shared_ptr<EclEpsConfig> oilWaterConfig;
    std::shared_ptr<EclEpsConfig> gasWaterConfig;
};
} // namespace Opm

#endif
