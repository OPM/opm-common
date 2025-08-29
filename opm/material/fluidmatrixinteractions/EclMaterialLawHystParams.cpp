// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawHystParams.hpp>

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>

namespace Opm::EclMaterialLaw {

/* constructors*/
template <class Traits>
HystParams<Traits>::
HystParams(typename Manager<Traits>::Params& params,
           const EclEpsGridProperties& epsGridProperties,
           const EclEpsGridProperties& epsImbGridProperties,
           const EclipseState& eclState,
           const Manager<Traits>& parent)
    : params_(params)
    , epsGridProperties_(epsGridProperties)
    , epsImbGridProperties_(epsImbGridProperties)
    , eclState_(eclState)
    , parent_(parent)
{
    gasOilParams_ = std::make_shared<GasOilHystParams>();
    oilWaterParams_ = std::make_shared<OilWaterHystParams>();
    gasWaterParams_ = std::make_shared<GasWaterHystParams>();
}

/* public methods, alphabetically sorted */

template <class Traits>
void
HystParams<Traits>::
finalize()
{
    if (hasGasOil_()) {
        this->gasOilParams_->finalize();
    }
    if (hasOilWater_()) {
        this->oilWaterParams_->finalize();
    }
    if (hasGasWater_()) {
        this->gasWaterParams_->finalize();
    }
}

template <class Traits>
void
HystParams<Traits>::
setConfig(unsigned satRegionIdx)
{
    this->gasOilParams_->setConfig(this->parent_.hysteresisConfig());
    this->oilWaterParams_->setConfig(this->parent_.hysteresisConfig());
    this->gasWaterParams_->setConfig(this->parent_.hysteresisConfig());

    if (this->parent_.hysteresisConfig().enableWagHysteresis()) {
        this->gasOilParams_->setWagConfig(this->parent_.wagHystersisConfig(satRegionIdx));
        this->oilWaterParams_->setWagConfig(this->parent_.wagHystersisConfig(satRegionIdx));
        this->gasWaterParams_->setWagConfig(this->parent_.wagHystersisConfig(satRegionIdx));
    }
}

template <class Traits>
void
HystParams<Traits>::
setDrainageParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                          const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasWater_()) {
        auto [gasWaterScaledInfo, gasWaterScaledPoints]
            = readScaledEpsPointsDrainage_(elemIdx, EclTwoPhaseSystemType::GasWater, lookupIdxOnLevelZeroAssigner);
        typename TwoPhaseTypes<Traits>::GasWaterEpsParams gasWaterDrainParams;
        gasWaterDrainParams.setConfig(this->parent_.gasWaterConfig());
        gasWaterDrainParams.setUnscaledPoints(this->params_.gasWaterUnscaledPointsVector[satRegionIdx]);
        gasWaterDrainParams.setScaledPoints(gasWaterScaledPoints);
        gasWaterDrainParams.setEffectiveLawParams(this->params_.gasWaterEffectiveParamVector[satRegionIdx]);
        gasWaterDrainParams.finalize();
        this->gasWaterParams_->setDrainageParams(gasWaterDrainParams,
                                                 gasWaterScaledInfo,
                                                 EclTwoPhaseSystemType::GasWater);
    }
}

template <class Traits>
void
HystParams<Traits>::
setDrainageParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                        const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasOil_()) {
        auto [gasOilScaledInfo, gasOilScaledPoints]
            = readScaledEpsPointsDrainage_(elemIdx, EclTwoPhaseSystemType::GasOil, lookupIdxOnLevelZeroAssigner);
        typename TwoPhaseTypes<Traits>::GasOilEpsParams gasOilDrainParams;
        gasOilDrainParams.setConfig(this->parent_.gasOilConfig());
        gasOilDrainParams.setUnscaledPoints(this->params_.gasOilUnscaledPointsVector[satRegionIdx]);
        gasOilDrainParams.setScaledPoints(gasOilScaledPoints);
        gasOilDrainParams.setEffectiveLawParams(this->params_.gasOilEffectiveParamVector[satRegionIdx]);
        gasOilDrainParams.finalize();
        this->gasOilParams_->setDrainageParams(gasOilDrainParams,
                                               gasOilScaledInfo,
                                               EclTwoPhaseSystemType::GasOil);
    }
}

template <class Traits>
void
HystParams<Traits>::
setDrainageParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                          const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    // We need to compute the oil-water scaled info even if we are running a two-phase case without
    // water (e.g. gas-oil). The reason is that the oil-water scaled info is used when computing
    // the initial condition see e.g. equilibrationhelpers.cc and initstateequil.cc
    // Therefore, the below 7 lines should not be put inside the if(hasOilWater_){} below.
    auto [oilWaterScaledInfo, oilWaterScaledPoints]
        = readScaledEpsPointsDrainage_(elemIdx, EclTwoPhaseSystemType::OilWater, lookupIdxOnLevelZeroAssigner);
    // TODO: This will reassign the same EclEpsScalingPointsInfo for each facedir
    //  since we currently does not support facedir for the scaling points info
    //  When such support is added, we need to extend the below vector which has info for each cell
    //   to include three more vectors, one with info for each facedir of a cell
    params_.oilWaterScaledEpsInfoDrainage[elemIdx] = oilWaterScaledInfo;
    if (hasOilWater_()) {
        typename TwoPhaseTypes<Traits>::OilWaterEpsParams oilWaterDrainParams;
        oilWaterDrainParams.setConfig(this->parent_.oilWaterConfig());
        oilWaterDrainParams.setUnscaledPoints(this->params_.oilWaterUnscaledPointsVector[satRegionIdx]);
        oilWaterDrainParams.setScaledPoints(oilWaterScaledPoints);
        oilWaterDrainParams.setEffectiveLawParams(this->params_.oilWaterEffectiveParamVector[satRegionIdx]);
        oilWaterDrainParams.finalize();
        oilWaterParams_->setDrainageParams(oilWaterDrainParams, oilWaterScaledInfo, EclTwoPhaseSystemType::OilWater);
    }
}

template <class Traits>
void
HystParams<Traits>::
setImbibitionParamsGasWater(unsigned elemIdx, unsigned imbRegionIdx,
                            const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasWater_()) {
        auto [gasWaterScaledInfo, gasWaterScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::GasWater, lookupIdxOnLevelZeroAssigner);
        typename EclMaterialLaw::TwoPhaseTypes<Traits>::GasWaterEpsParams gasWaterImbParamsHyst;
        gasWaterImbParamsHyst.setConfig(this->parent_.gasWaterConfig());
        gasWaterImbParamsHyst.setUnscaledPoints(this->params_.gasWaterUnscaledPointsVector[imbRegionIdx]);
        gasWaterImbParamsHyst.setScaledPoints(gasWaterScaledPoints);
        gasWaterImbParamsHyst.setEffectiveLawParams(this->params_.gasWaterEffectiveParamVector[imbRegionIdx]);
        gasWaterImbParamsHyst.finalize();
        this->gasWaterParams_->setImbibitionParams(gasWaterImbParamsHyst,
                                                   gasWaterScaledInfo,
                                                   EclTwoPhaseSystemType::GasWater);
    }
}

template <class Traits>
void
HystParams<Traits>::
setImbibitionParamsOilGas(unsigned elemIdx, unsigned imbRegionIdx,
                          const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasOil_()) {
        auto [gasOilScaledInfo, gasOilScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::GasOil, lookupIdxOnLevelZeroAssigner);

        typename TwoPhaseTypes<Traits>::GasOilEpsParams gasOilImbParamsHyst;
        gasOilImbParamsHyst.setConfig(this->parent_.gasOilConfig());
        gasOilImbParamsHyst.setUnscaledPoints(this->params_.gasOilUnscaledPointsVector[imbRegionIdx]);
        gasOilImbParamsHyst.setScaledPoints(gasOilScaledPoints);
        gasOilImbParamsHyst.setEffectiveLawParams(this->params_.gasOilEffectiveParamVector[imbRegionIdx]);
        gasOilImbParamsHyst.finalize();
        this->gasOilParams_->setImbibitionParams(gasOilImbParamsHyst,
                                                 gasOilScaledInfo,
                                                 EclTwoPhaseSystemType::GasOil);
    }
}

template <class Traits>
void
HystParams<Traits>::
setImbibitionParamsOilWater(unsigned elemIdx, unsigned imbRegionIdx,
                            const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    if (hasOilWater_()) {
        auto [oilWaterScaledInfo, oilWaterScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::OilWater, lookupIdxOnLevelZeroAssigner);
        typename TwoPhaseTypes<Traits>::OilWaterEpsParams oilWaterImbParamsHyst;
        oilWaterImbParamsHyst.setConfig(this->parent_.oilWaterConfig());
        oilWaterImbParamsHyst.setUnscaledPoints(this->params_.oilWaterUnscaledPointsVector[imbRegionIdx]);
        oilWaterImbParamsHyst.setScaledPoints(oilWaterScaledPoints);
        oilWaterImbParamsHyst.setEffectiveLawParams(this->params_.oilWaterEffectiveParamVector[imbRegionIdx]);
        oilWaterImbParamsHyst.finalize();
        this->oilWaterParams_->setImbibitionParams(oilWaterImbParamsHyst,
                                                   oilWaterScaledInfo,
                                                   EclTwoPhaseSystemType::OilWater);
    }
}

/* private methods, alphabetically sorted */

template <class Traits>
bool
HystParams<Traits>::
hasGasOil_()
{
    return this->parent_.hasGas() && this->parent_.hasOil();
}

template <class Traits>
bool
HystParams<Traits>::
hasGasWater_()
{
    return this->parent_.hasGas() && this->parent_.hasWater() && !this->parent_.hasOil();
}

template <class Traits>
bool
HystParams<Traits>::
hasOilWater_()
{
    return this->parent_.hasOil() && this->parent_.hasWater();
}

template <class Traits>
std::tuple<EclEpsScalingPointsInfo<typename Traits::Scalar>,
           EclEpsScalingPoints<typename Traits::Scalar>>
HystParams<Traits>::
readScaledEpsPoints_(const EclEpsGridProperties& epsGridProperties,
                     unsigned elemIdx,
                     EclTwoPhaseSystemType type,
                     const LookupFunction& fieldPropIdxOnLevelZero)
{
    const EclEpsConfig& config =
        (type == EclTwoPhaseSystemType::OilWater) ?
            this->parent_.oilWaterConfig()        :
            this->parent_.gasOilConfig();
    // For CpGrids with LGRs, field prop is inherited from parent/equivalent cell from level 0.
    // 'lookupIdx' is the index on level zero of the parent cell or the equivalent cell of the
    // leaf grid view cell with index 'elemIdx'.
    const auto lookupIdx = fieldPropIdxOnLevelZero(elemIdx);
    unsigned satRegionIdx = epsGridProperties.satRegion( lookupIdx /* coincides with elemIdx when no LGRs */ );
    // Copy-construct a new instance of EclEpsScalingPointsInfo
    EclEpsScalingPointsInfo<Scalar> destInfo(this->parent_.unscaledEpsInfo(satRegionIdx));
    // TODO: currently epsGridProperties does not implement a face direction, e.g. SWLX, SWLY,...
    //  when these keywords get implemented, we need to use include facedir in the lookup
    destInfo.extractScaled(this->eclState_, epsGridProperties, lookupIdx /* coincides with elemIdx when no LGRs */);

    EclEpsScalingPoints<Scalar> destPoint;
    destPoint.init(destInfo, config, type);
    return {destInfo, destPoint};
}

template <class Traits>
std::tuple<EclEpsScalingPointsInfo<typename Traits::Scalar>,
           EclEpsScalingPoints<typename Traits::Scalar>>
HystParams<Traits>::
readScaledEpsPointsDrainage_(unsigned elemIdx, EclTwoPhaseSystemType type,
                             const LookupFunction& fieldPropIdxOnLevelZero)
{
    return readScaledEpsPoints_(epsGridProperties_, elemIdx, type, fieldPropIdxOnLevelZero);
}

template <class Traits>
std::tuple<EclEpsScalingPointsInfo<typename Traits::Scalar>,
           EclEpsScalingPoints<typename Traits::Scalar>>
HystParams<Traits>::
readScaledEpsPointsImbibition_(unsigned elemIdx, EclTwoPhaseSystemType type,
                               const LookupFunction& fieldPropIdxOnLevelZero)
{
    return readScaledEpsPoints_(epsImbGridProperties_, elemIdx, type, fieldPropIdxOnLevelZero);
}

// Make some actual code, by realizing the previously defined templated class
template class HystParams<ThreePhaseMaterialTraits<double,0,1,2>>;
template class HystParams<ThreePhaseMaterialTraits<float,0,1,2>>;
template class HystParams<ThreePhaseMaterialTraits<double,2,0,1>>;
template class HystParams<ThreePhaseMaterialTraits<float,2,0,1>>;

} // namespace Opm::EclMaterialLaw
