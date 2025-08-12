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
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>

namespace Opm {

/* constructors*/
template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
HystParams(EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams& init_params) :
    init_params_{init_params}, parent_{init_params_.parent_},
    eclState_{init_params_.eclState_}
{
    gasOilParams_ = std::make_shared<GasOilTwoPhaseHystParams>();
    oilWaterParams_ = std::make_shared<OilWaterTwoPhaseHystParams>();
    gasWaterParams_ = std::make_shared<GasWaterTwoPhaseHystParams>();
}

/* public methods, alphabetically sorted */

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
finalize()
{
    if (hasGasOil_())
        this->gasOilParams_->finalize();
    if (hasOilWater_())
        this->oilWaterParams_->finalize();
    if (hasGasWater_())
        this->gasWaterParams_->finalize();
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::shared_ptr<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::GasOilTwoPhaseHystParams>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
getGasOilParams()
{
    return gasOilParams_;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::shared_ptr<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::OilWaterTwoPhaseHystParams>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
getOilWaterParams()
{
    return oilWaterParams_;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::shared_ptr<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::GasWaterTwoPhaseHystParams>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
getGasWaterParams()
{
    return gasWaterParams_;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setConfig(unsigned satRegionIdx)
{
    this->gasOilParams_->setConfig(this->parent_.hysteresisConfig_);
    this->oilWaterParams_->setConfig(this->parent_.hysteresisConfig_);
    this->gasWaterParams_->setConfig(this->parent_.hysteresisConfig_);

    if (this->parent_.hysteresisConfig_->enableWagHysteresis()) {
        this->gasOilParams_->setWagConfig(this->parent_.wagHystersisConfig_[satRegionIdx]);
        this->oilWaterParams_->setWagConfig(this->parent_.wagHystersisConfig_[satRegionIdx]);
        this->gasWaterParams_->setWagConfig(this->parent_.wagHystersisConfig_[satRegionIdx]);
    }

} // namespace Opm

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setDrainageParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                          const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasWater_()) {
        auto [gasWaterScaledInfo, gasWaterScaledPoints]
            = readScaledEpsPointsDrainage_(elemIdx, EclTwoPhaseSystemType::GasWater, lookupIdxOnLevelZeroAssigner);
        GasWaterEpsTwoPhaseParams gasWaterDrainParams;
        gasWaterDrainParams.setConfig(this->parent_.gasWaterConfig_);
        gasWaterDrainParams.setUnscaledPoints(this->parent_.gasWaterUnscaledPointsVector_[satRegionIdx]);
        gasWaterDrainParams.setScaledPoints(gasWaterScaledPoints);
        gasWaterDrainParams.setEffectiveLawParams(this->parent_.gasWaterEffectiveParamVector_[satRegionIdx]);
        gasWaterDrainParams.finalize();
        this->gasWaterParams_->setDrainageParams(gasWaterDrainParams, gasWaterScaledInfo, EclTwoPhaseSystemType::GasWater);
    }
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setDrainageParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                        const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasOil_()) {
        auto [gasOilScaledInfo, gasOilScaledPoints]
            = readScaledEpsPointsDrainage_(elemIdx, EclTwoPhaseSystemType::GasOil, lookupIdxOnLevelZeroAssigner);
        GasOilEpsTwoPhaseParams gasOilDrainParams;
        gasOilDrainParams.setConfig(this->parent_.gasOilConfig_);
        gasOilDrainParams.setUnscaledPoints(this->parent_.gasOilUnscaledPointsVector_[satRegionIdx]);
        gasOilDrainParams.setScaledPoints(gasOilScaledPoints);
        gasOilDrainParams.setEffectiveLawParams(this->parent_.gasOilEffectiveParamVector_[satRegionIdx]);
        gasOilDrainParams.finalize();
        this->gasOilParams_->setDrainageParams(gasOilDrainParams, gasOilScaledInfo, EclTwoPhaseSystemType::GasOil);
    }
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setDrainageParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                          const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
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
    this->parent_.oilWaterScaledEpsInfoDrainage_[elemIdx] = oilWaterScaledInfo;
    if (hasOilWater_()) {
        OilWaterEpsTwoPhaseParams oilWaterDrainParams;
        oilWaterDrainParams.setConfig(this->parent_.oilWaterConfig_);
        oilWaterDrainParams.setUnscaledPoints(this->parent_.oilWaterUnscaledPointsVector_[satRegionIdx]);
        oilWaterDrainParams.setScaledPoints(oilWaterScaledPoints);
        oilWaterDrainParams.setEffectiveLawParams(this->parent_.oilWaterEffectiveParamVector_[satRegionIdx]);
        oilWaterDrainParams.finalize();
        oilWaterParams_->setDrainageParams(oilWaterDrainParams, oilWaterScaledInfo, EclTwoPhaseSystemType::OilWater);
    }
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setImbibitionParamsGasWater(unsigned elemIdx, unsigned imbRegionIdx,
                            const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasWater_()) {
        auto [gasWaterScaledInfo, gasWaterScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::GasWater, lookupIdxOnLevelZeroAssigner);
        GasWaterEpsTwoPhaseParams gasWaterImbParamsHyst;
        gasWaterImbParamsHyst.setConfig(this->parent_.gasWaterConfig_);
        gasWaterImbParamsHyst.setUnscaledPoints(this->parent_.gasWaterUnscaledPointsVector_[imbRegionIdx]);
        gasWaterImbParamsHyst.setScaledPoints(gasWaterScaledPoints);
        gasWaterImbParamsHyst.setEffectiveLawParams(this->parent_.gasWaterEffectiveParamVector_[imbRegionIdx]);
        gasWaterImbParamsHyst.finalize();
        this->gasWaterParams_->setImbibitionParams(gasWaterImbParamsHyst,
                                                   gasWaterScaledInfo,
                                                   EclTwoPhaseSystemType::GasWater);

    }
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setImbibitionParamsOilGas(unsigned elemIdx, unsigned imbRegionIdx,
                          const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    if (hasGasOil_()) {
        auto [gasOilScaledInfo, gasOilScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::GasOil, lookupIdxOnLevelZeroAssigner);

        GasOilEpsTwoPhaseParams gasOilImbParamsHyst;
        gasOilImbParamsHyst.setConfig(this->parent_.gasOilConfig_);
        gasOilImbParamsHyst.setUnscaledPoints(this->parent_.gasOilUnscaledPointsVector_[imbRegionIdx]);
        gasOilImbParamsHyst.setScaledPoints(gasOilScaledPoints);
        gasOilImbParamsHyst.setEffectiveLawParams(this->parent_.gasOilEffectiveParamVector_[imbRegionIdx]);
        gasOilImbParamsHyst.finalize();
        this->gasOilParams_->setImbibitionParams(gasOilImbParamsHyst,
                                                 gasOilScaledInfo,
                                                 EclTwoPhaseSystemType::GasOil);
    }
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
setImbibitionParamsOilWater(unsigned elemIdx, unsigned imbRegionIdx,
                            const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    if (hasOilWater_()) {
        auto [oilWaterScaledInfo, oilWaterScaledPoints]
            = readScaledEpsPointsImbibition_(elemIdx, EclTwoPhaseSystemType::OilWater, lookupIdxOnLevelZeroAssigner);
        OilWaterEpsTwoPhaseParams oilWaterImbParamsHyst;
        oilWaterImbParamsHyst.setConfig(this->parent_.oilWaterConfig_);
        oilWaterImbParamsHyst.setUnscaledPoints(this->parent_.oilWaterUnscaledPointsVector_[imbRegionIdx]);
        oilWaterImbParamsHyst.setScaledPoints(oilWaterScaledPoints);
        oilWaterImbParamsHyst.setEffectiveLawParams(this->parent_.oilWaterEffectiveParamVector_[imbRegionIdx]);
        oilWaterImbParamsHyst.finalize();
        this->oilWaterParams_->setImbibitionParams(oilWaterImbParamsHyst,
                                                   oilWaterScaledInfo,
                                                   EclTwoPhaseSystemType::OilWater);

    }
}

/* private methods, alphabetically sorted */

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
bool
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
hasGasOil_()
{
    return this->parent_.hasGas && this->parent_.hasOil;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
bool
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
hasGasWater_()
{
    return this->parent_.hasGas && this->parent_.hasWater && !this->parent_.hasOil;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
bool
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
hasOilWater_()
{
    return this->parent_.hasOil && this->parent_.hasWater;
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::tuple<
  EclEpsScalingPointsInfo<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>,
  EclEpsScalingPoints<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>
>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
readScaledEpsPoints_(const EclEpsGridProperties& epsGridProperties, unsigned elemIdx, EclTwoPhaseSystemType type,
                     const std::function<unsigned(unsigned)>& fieldPropIdxOnLevelZero)
{
    const EclEpsConfig& config = (type == EclTwoPhaseSystemType::OilWater)?  *(this->parent_.oilWaterConfig_): *(this->parent_.gasOilConfig_);
    // For CpGrids with LGRs, field prop is inherited from parent/equivalent cell from level 0.
    // 'lookupIdx' is the index on level zero of the parent cell or the equivalent cell of the
    // leaf grid view cell with index 'elemIdx'.
    const auto lookupIdx = fieldPropIdxOnLevelZero(elemIdx);
    unsigned satRegionIdx = epsGridProperties.satRegion( lookupIdx /* coincides with elemIdx when no LGRs */ );
    // Copy-construct a new instance of EclEpsScalingPointsInfo
    EclEpsScalingPointsInfo<Scalar> destInfo(this->parent_.unscaledEpsInfo_[satRegionIdx]);
    // TODO: currently epsGridProperties does not implement a face direction, e.g. SWLX, SWLY,...
    //  when these keywords get implemented, we need to use include facedir in the lookup
    destInfo.extractScaled(this->eclState_, epsGridProperties, lookupIdx /* coincides with elemIdx when no LGRs */);

    EclEpsScalingPoints<Scalar> destPoint;
    destPoint.init(destInfo, config, type);
    return {destInfo, destPoint};
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::tuple<
  EclEpsScalingPointsInfo<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>,
  EclEpsScalingPoints<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>
>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
readScaledEpsPointsDrainage_(unsigned elemIdx, EclTwoPhaseSystemType type,
                             const std::function<unsigned(unsigned)>& fieldPropIdxOnLevelZero)
{
    const auto& epsGridProperties = this->init_params_.epsGridProperties_;
    return readScaledEpsPoints_(*epsGridProperties, elemIdx, type, fieldPropIdxOnLevelZero);
}

template<
    class Traits,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
std::tuple<
  EclEpsScalingPointsInfo<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>,
  EclEpsScalingPoints<typename EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::Scalar>
>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::HystParams::
readScaledEpsPointsImbibition_(unsigned elemIdx, EclTwoPhaseSystemType type,
                               const std::function<unsigned(unsigned)>& fieldPropIdxOnLevelZero)
{
    const auto& epsGridProperties = this->init_params_.epsImbGridProperties_;
    return readScaledEpsPoints_(*epsGridProperties, elemIdx, type, fieldPropIdxOnLevelZero);
}

// Make some actual code, by realizing the previously defined templated class
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,0,1,2>>::InitParams::HystParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,0,1,2>>::InitParams::HystParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,2,0,1>>::InitParams::HystParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,2,0,1>>::InitParams::HystParams;


} // namespace Opm
