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
#include <opm/material/fluidmatrixinteractions/EclMaterialLawInitParams.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/material/fluidmatrixinteractions/DirectionalMaterialLawParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawHystParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawReadEffectiveParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterialParams.hpp>

#include <cassert>

namespace {

unsigned satOrImbRegion(const std::vector<int>& array,
                        const std::vector<int>& default_vec,
                        unsigned elemIdx)
{
    const int value = array.empty() ? default_vec[elemIdx] : array[elemIdx];
    return static_cast<unsigned>(value);
}

} // anonymous namespace

namespace Opm::EclMaterialLaw {

/* constructors*/

template <class Traits>
InitParams<Traits>::
InitParams(const Manager<Traits>& parent,
           const EclipseState& eclState,
           std::size_t numCompressedElems)
    : parent_{parent}
    , eclState_{eclState}
    , numCompressedElems_{numCompressedElems}
    , epsGridProperties_(this->eclState_, false)
{
    // read end point scaling grid properties
    // TODO: these objects might require some memory, can this be simplified?
    if (this->parent_.enableHysteresis()) {
        this->epsImbGridProperties_
            = std::make_unique<EclEpsGridProperties>(this->eclState_, /*useImbibition=*/true);
    }
}

/* public methods */

template <class Traits>
void
InitParams<Traits>::
run(const IntLookupFunction& fieldPropIntOnLeafAssigner,
    const LookupFunction& lookupIdxOnLevelZeroAssigner)
{
    readUnscaledEpsPointsVectors_();
    readEffectiveParameters_();
    initSatnumRegionArray_(fieldPropIntOnLeafAssigner);
    copySatnumArrays_(fieldPropIntOnLeafAssigner);
    initOilWaterScaledEpsInfo_();
    initMaterialLawParamVectors_();
    std::vector<const std::vector<int>*> satnumArray;
    std::vector<const std::vector<int>*> imbnumArray;
    std::vector<std::vector<MaterialLawParams>*> mlpArray;
    initArrays_(satnumArray, imbnumArray, mlpArray);
    const auto num_arrays = mlpArray.size();
    for (unsigned i = 0; i < num_arrays; i++) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (unsigned elemIdx = 0; elemIdx < this->numCompressedElems_; ++elemIdx) {
            unsigned satRegionIdx = satRegion_(*satnumArray[i], elemIdx);
            //unsigned satNumCell = this->parent_.satnumRegionArray_[elemIdx];
            HystParams<Traits> hystParams{
                params_,
                epsGridProperties_,
                *epsImbGridProperties_,
                this->eclState_,
                this->parent_
            };

            hystParams.setConfig(satRegionIdx);
            hystParams.setDrainageParamsOilGas(elemIdx, satRegionIdx, lookupIdxOnLevelZeroAssigner);
            hystParams.setDrainageParamsOilWater(elemIdx, satRegionIdx, lookupIdxOnLevelZeroAssigner);
            hystParams.setDrainageParamsGasWater(elemIdx, satRegionIdx, lookupIdxOnLevelZeroAssigner);
            if (this->parent_.enableHysteresis()) {
                unsigned imbRegionIdx = imbRegion_(*imbnumArray[i], elemIdx);
                hystParams.setImbibitionParamsOilGas(elemIdx, imbRegionIdx, lookupIdxOnLevelZeroAssigner);
                hystParams.setImbibitionParamsOilWater(elemIdx, imbRegionIdx, lookupIdxOnLevelZeroAssigner);
                hystParams.setImbibitionParamsGasWater(elemIdx, imbRegionIdx, lookupIdxOnLevelZeroAssigner);
            }
            hystParams.finalize();
            initThreePhaseParams_(hystParams, (*mlpArray[i])[elemIdx], satRegionIdx, elemIdx);
        }
    }
}

/* private methods alphabetically sorted*/

template <class Traits>
void
InitParams<Traits>::
copySatnumArrays_(const IntLookupFunction& fieldPropIntOnLeafAssigner)
{
    copyIntArray_(params_.krnumXArray, "KRNUMX", fieldPropIntOnLeafAssigner);
    copyIntArray_(params_.krnumYArray, "KRNUMY", fieldPropIntOnLeafAssigner);
    copyIntArray_(params_.krnumZArray, "KRNUMZ", fieldPropIntOnLeafAssigner);
    copyIntArray_(params_.imbnumXArray, "IMBNUMX", fieldPropIntOnLeafAssigner);
    copyIntArray_(params_.imbnumYArray, "IMBNUMY", fieldPropIntOnLeafAssigner);
    copyIntArray_(params_.imbnumZArray, "IMBNUMZ", fieldPropIntOnLeafAssigner);
    // create the information for the imbibition region (IMBNUM). By default this is
    // the same as the saturation region (SATNUM)
    params_.imbnumRegionArray = params_.satnumRegionArray;
    copyIntArray_(params_.imbnumRegionArray, "IMBNUM", fieldPropIntOnLeafAssigner);
    assert(this->numCompressedElems_ == params_.satnumRegionArray.size());
    assert(!this->parent_.enableHysteresis() || this->numCompressedElems_ == params_.imbnumRegionArray.size());
}

template <class Traits>
void
InitParams<Traits>::
copyIntArray_(std::vector<int>& dest,
              const std::string& keyword,
              const IntLookupFunction& fieldPropIntOnLeafAssigner) const
{
    if (this->eclState_.fieldProps().has_int(keyword)) {
        dest = fieldPropIntOnLeafAssigner(this->eclState_.fieldProps(), keyword, /*needsTranslation*/true);
    }
}

template <class Traits>
unsigned
InitParams<Traits>::
imbRegion_(const std::vector<int>& array, unsigned elemIdx) const
{
    const std::vector<int>& default_vec = params_.imbnumRegionArray;
    return satOrImbRegion(array, default_vec, elemIdx);
}

template <class Traits>
void
InitParams<Traits>::
initArrays_(std::vector<const std::vector<int>*>& satnumArray,
            std::vector<const std::vector<int>*>& imbnumArray,
            std::vector<std::vector<MaterialLawParams>*>& mlpArray)
{
    satnumArray.push_back(&params_.satnumRegionArray);
    imbnumArray.push_back(&params_.imbnumRegionArray);
    mlpArray.push_back(&params_.materialLawParams);
    if (params_.dirMaterialLawParams) {
        if (this->params_.hasDirectionalRelperms()) {
            satnumArray.push_back(&params_.krnumXArray);
            satnumArray.push_back(&params_.krnumYArray);
            satnumArray.push_back(&params_.krnumZArray);
        }
        if (this->params_.hasDirectionalImbnum()) {
            imbnumArray.push_back(&params_.imbnumXArray);
            imbnumArray.push_back(&params_.imbnumYArray);
            imbnumArray.push_back(&params_.imbnumZArray);
        }
        mlpArray.push_back(&params_.dirMaterialLawParams->materialLawParamsX_);
        mlpArray.push_back(&params_.dirMaterialLawParams->materialLawParamsY_);
        mlpArray.push_back(&params_.dirMaterialLawParams->materialLawParamsZ_);
    }
}

template <class Traits>
void
InitParams<Traits>::
initMaterialLawParamVectors_()
{
    params_.materialLawParams.resize(this->numCompressedElems_);
    if (this->params_.hasDirectionalImbnum() || this->params_.hasDirectionalRelperms()) {
        params_.dirMaterialLawParams
            = std::make_unique<DirectionalMaterialLawParams<MaterialLawParams>>(this->numCompressedElems_);
    }
}

template <class Traits>
void
InitParams<Traits>::
initOilWaterScaledEpsInfo_()
{
    // This vector will be updated in the hystParams.setDrainageOilWater() in the run() method
    params_.oilWaterScaledEpsInfoDrainage.resize(this->numCompressedElems_);
}

template <class Traits>
void
InitParams<Traits>::
initSatnumRegionArray_(const IntLookupFunction& fieldPropIntOnLeafAssigner)
{
    // copy the SATNUM grid property. in some cases this is not necessary, but it
    // should not require much memory anyway...
    params_.satnumRegionArray.resize(this->numCompressedElems_);
    if (this->eclState_.fieldProps().has_int("SATNUM")) {
        params_.satnumRegionArray = fieldPropIntOnLeafAssigner(this->eclState_.fieldProps(),
                                                               "SATNUM", /*needsTranslation*/true);
    }
    else {
        std::fill(params_.satnumRegionArray.begin(), params_.satnumRegionArray.end(), 0);
    }
}

template <class Traits>
void
InitParams<Traits>::
initThreePhaseParams_(HystParams<Traits>& hystParams,
                      MaterialLawParams& materialParams,
                      unsigned satRegionIdx,
                      unsigned elemIdx)
{
    const auto& epsInfo = this->params_.oilWaterScaledEpsInfoDrainage[elemIdx];

    auto oilWaterParams = hystParams.getOilWaterParams();
    auto gasOilParams = hystParams.getGasOilParams();
    auto gasWaterParams = hystParams.getGasWaterParams();
    materialParams.setApproach(this->parent_.threePhaseApproach());
    switch (materialParams.approach()) {
        case EclMultiplexerApproach::Stone1: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone1>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setSwl(epsInfo.Swl);

            if (!this->parent_.stoneEtas().empty()) {
                realParams.setEta(this->parent_.stoneEtas()[satRegionIdx]);
            }
            else
                realParams.setEta(1.0);
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::Stone2: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone2>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setSwl(epsInfo.Swl);
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::Default: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Default>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setSwl(epsInfo.Swl);
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::TwoPhase: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::TwoPhase>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setGasWaterParams(gasWaterParams);
            realParams.setApproach(this->parent_.twoPhaseApproach());
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::OnePhase: {
            // Nothing to do, no parameters.
            break;
        }
    } // end switch()
}

template <class Traits>
void
InitParams<Traits>::
readEffectiveParameters_()
{
    ReadEffectiveParams<Traits> effectiveReader{
        params_,
        this->eclState_,
        this->parent_
    };

    // populates effective parameter vectors in the parent class (EclMaterialManager)
    effectiveReader.read();
}

template <class Traits>
void
InitParams<Traits>::
readUnscaledEpsPointsVectors_()
{
    if (this->parent_.hasGas() && this->parent_.hasOil()) {
        readUnscaledEpsPoints_(
            params_.gasOilUnscaledPointsVector,
            this->parent_.gasOilConfig(),
            EclTwoPhaseSystemType::GasOil
        );
    }
    if (this->parent_.hasOil() && this->parent_.hasWater()) {
        readUnscaledEpsPoints_(
            params_.oilWaterUnscaledPointsVector,
            this->parent_.oilWaterConfig(),
            EclTwoPhaseSystemType::OilWater
        );
    }
    if (!this->parent_.hasOil()) {
        readUnscaledEpsPoints_(
            params_.gasWaterUnscaledPointsVector,
            this->parent_.gasWaterConfig(),
            EclTwoPhaseSystemType::GasWater
        );
    }
}

template <class Traits>
template <class Container>
void
InitParams<Traits>::
readUnscaledEpsPoints_(Container& dest,
                       const EclEpsConfig& config,
                       EclTwoPhaseSystemType system_type)
{
    const std::size_t numSatRegions = this->eclState_.runspec().tabdims().getNumSatTables();
    dest.resize(numSatRegions);
    for (unsigned satRegionIdx = 0; satRegionIdx < numSatRegions; ++satRegionIdx) {
        dest[satRegionIdx] = std::make_shared<EclEpsScalingPoints<Scalar> >();
        dest[satRegionIdx]->init(this->parent_.unscaledEpsInfo(satRegionIdx), config, system_type);
    }
}

template <class Traits>
unsigned
InitParams<Traits>::
satRegion_(const std::vector<int>& array, unsigned elemIdx) const
{
    const std::vector<int>& default_vec = params_.satnumRegionArray;
    return satOrImbRegion(array, default_vec, elemIdx);
}

// Make some actual code, by realizing the previously defined templated class
template class InitParams<ThreePhaseMaterialTraits<double,0,1,2>>;
template class InitParams<ThreePhaseMaterialTraits<float,0,1,2>>;
template class InitParams<ThreePhaseMaterialTraits<double,2,0,1>>;
template class InitParams<ThreePhaseMaterialTraits<float,2,0,1>>;

} // namespace Opm::EclMaterialLaw
