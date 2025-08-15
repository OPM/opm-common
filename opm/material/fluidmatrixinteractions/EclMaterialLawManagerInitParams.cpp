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

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>


namespace Opm {

/* constructors*/

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
InitParams(EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>& parent, const EclipseState& eclState, size_t numCompressedElems) :
    parent_{parent},
    eclState_{eclState},
    numCompressedElems_{numCompressedElems}
{
    // read end point scaling grid properties
    // TODO: these objects might require some memory, can this be simplified?
    if (this->parent_.enableHysteresis()) {
        this->epsImbGridProperties_
            = std::make_unique<EclEpsGridProperties>(this->eclState_, /*useImbibition=*/true);
    }
    this->epsGridProperties_
        = std::make_unique<EclEpsGridProperties>(this->eclState_, /*useImbibition=*/false);
}

/* public methods */

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
run(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>&
    fieldPropIntOnLeafAssigner,
    const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    readUnscaledEpsPointsVectors_();
    readEffectiveParameters_();
    initSatnumRegionArray_(fieldPropIntOnLeafAssigner);
    copySatnumArrays_(fieldPropIntOnLeafAssigner);
    initOilWaterScaledEpsInfo_();
    initMaterialLawParamVectors_();
    Storage<Storage<int>*> satnumArray;
    Storage<Storage<int>*> imbnumArray;
    Storage<Storage<MaterialLawParams>*> mlpArray;
    initArrays_(satnumArray, imbnumArray, mlpArray);
    const auto num_arrays = mlpArray.size();
    for (unsigned i = 0; i < num_arrays; i++) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (unsigned elemIdx = 0; elemIdx < this->numCompressedElems_; ++elemIdx) {
            unsigned satRegionIdx = satRegion_(*satnumArray[i], elemIdx);
            //unsigned satNumCell = this->parent_.satnumRegionArray_[elemIdx];
            HystParams hystParams {*this};
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

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
copySatnumArrays_(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropIntOnLeafAssigner)
{
    copyIntArray_(this->parent_.krnumXArray_, "KRNUMX", fieldPropIntOnLeafAssigner);
    copyIntArray_(this->parent_.krnumYArray_, "KRNUMY", fieldPropIntOnLeafAssigner);
    copyIntArray_(this->parent_.krnumZArray_, "KRNUMZ", fieldPropIntOnLeafAssigner);
    copyIntArray_(this->parent_.imbnumXArray_, "IMBNUMX", fieldPropIntOnLeafAssigner);
    copyIntArray_(this->parent_.imbnumYArray_, "IMBNUMY", fieldPropIntOnLeafAssigner);
    copyIntArray_(this->parent_.imbnumZArray_, "IMBNUMZ", fieldPropIntOnLeafAssigner);
    // create the information for the imbibition region (IMBNUM). By default this is
    // the same as the saturation region (SATNUM)
    this->parent_.imbnumRegionArray_ = this->parent_.satnumRegionArray_;
    copyIntArray_(this->parent_.imbnumRegionArray_, "IMBNUM", fieldPropIntOnLeafAssigner);
    assert(this->numCompressedElems_ == this->parent_.satnumRegionArray_.size());
    assert(!this->parent_.enableHysteresis() || this->numCompressedElems_ == this->parent_.imbnumRegionArray_.size());
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
copyIntArray_(Storage<int>& dest, const std::string& keyword,
              const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropIntOnLeafAssigner)
{
    if (this->eclState_.fieldProps().has_int(keyword)) {
        dest = fieldPropIntOnLeafAssigner(this->eclState_.fieldProps(), keyword, /*needsTranslation*/true);
    }
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
unsigned
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
imbRegion_(Storage<int>& array, unsigned elemIdx)
{
    Storage<int>& default_vec = this->parent_.imbnumRegionArray_;
    return satOrImbRegion_(array, default_vec, elemIdx);
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initArrays_(
        Storage<Storage<int>*>& satnumArray,
        Storage<Storage<int>*>& imbnumArray,
        Storage<Storage<MaterialLawParams>*>& mlpArray)
{
    satnumArray.push_back(&this->parent_.satnumRegionArray_);
    imbnumArray.push_back(&this->parent_.imbnumRegionArray_);
    mlpArray.push_back(&this->parent_.materialLawParams_);
    if (this->parent_.dirMaterialLawParams_) {
        if (this->parent_.hasDirectionalRelperms()) {
            satnumArray.push_back(&this->parent_.krnumXArray_);
            satnumArray.push_back(&this->parent_.krnumYArray_);
            satnumArray.push_back(&this->parent_.krnumZArray_);
        }
        if (this->parent_.hasDirectionalImbnum()) {
            imbnumArray.push_back(&this->parent_.imbnumXArray_);
            imbnumArray.push_back(&this->parent_.imbnumYArray_);
            imbnumArray.push_back(&this->parent_.imbnumZArray_);
        }
        mlpArray.push_back(&(this->parent_.dirMaterialLawParams_->materialLawParamsX_));
        mlpArray.push_back(&(this->parent_.dirMaterialLawParams_->materialLawParamsY_));
        mlpArray.push_back(&(this->parent_.dirMaterialLawParams_->materialLawParamsZ_));
    }
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initMaterialLawParamVectors_()
{
    this->parent_.materialLawParams_.resize(this->numCompressedElems_);
    if (this->parent_.hasDirectionalImbnum() || this->parent_.hasDirectionalRelperms()) {
        this->parent_.dirMaterialLawParams_
            = std::make_unique<DirectionalMaterialLawParams<MaterialLawParams>>(this->numCompressedElems_);
    }
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initOilWaterScaledEpsInfo_()
{
    // This vector will be updated in the hystParams.setDrainageOilWater() in the run() method
    this->parent_.oilWaterScaledEpsInfoDrainage_.resize(this->numCompressedElems_);
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initSatnumRegionArray_(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropIntOnLeafAssigner)
{
    // copy the SATNUM grid property. in some cases this is not necessary, but it
    // should not require much memory anyway...
    auto &satnumArray = this->parent_.satnumRegionArray_;
    satnumArray.resize(this->numCompressedElems_);
    if (this->eclState_.fieldProps().has_int("SATNUM")) {
        satnumArray = fieldPropIntOnLeafAssigner(this->eclState_.fieldProps(), "SATNUM", /*needsTranslation*/true);
    }
    else {
        std::fill(satnumArray.begin(), satnumArray.end(), 0);
    }
}

#if !HAVE_CUDA

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initThreePhaseParams_(HystParams &hystParams,
                      MaterialLawParams& materialParams,
                      unsigned satRegionIdx,
                      unsigned elemIdx)
{
    const auto& epsInfo = this->parent_.oilWaterScaledEpsInfoDrainage_[elemIdx];

    auto oilWaterParams = hystParams.getOilWaterParams();
    auto gasOilParams = hystParams.getGasOilParams();
    auto gasWaterParams = hystParams.getGasWaterParams();
    materialParams.setApproach(this->parent_.threePhaseApproach_);
    switch (materialParams.approach()) {
        case EclMultiplexerApproach::Stone1: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone1>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setSwl(epsInfo.Swl);

            if (!this->parent_.stoneEtas_.empty()) {
                realParams.setEta(this->parent_.stoneEtas_[satRegionIdx]);
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
            realParams.setApproach(this->parent_.twoPhaseApproach_);
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::OnePhase: {
            // Nothing to do, no parameters.
            break;
        }
    } // end switch()
}
#else
template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
initThreePhaseParams_(MaterialLawParams& materialParams,
                      unsigned satRegionIdx,
                      unsigned elemIdx)
{
    const auto& epsInfo = this->parent_.oilWaterScaledEpsInfoDrainage_[elemIdx];

    materialParams.setApproach(this->parent_.threePhaseApproach_);
    switch (materialParams.approach()) {
        case EclMultiplexerApproach::Stone1: {
            auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone1>();
            realParams.setGasOilParams(gasOilParams);
            realParams.setOilWaterParams(oilWaterParams);
            realParams.setSwl(epsInfo.Swl);

            if (!this->parent_.stoneEtas_.empty()) {
                realParams.setEta(this->parent_.stoneEtas_[satRegionIdx]);
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
            realParams.setApproach(this->parent_.twoPhaseApproach_);
            realParams.finalize();
            break;
        }

        case EclMultiplexerApproach::OnePhase: {
            // Nothing to do, no parameters.
            break;
        }
    } // end switch()
}
#endif // !HAVE_CUDA

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
readEffectiveParameters_()
{
    ReadEffectiveParams effectiveReader {*this};
    // populates effective parameter vectors in the parent class (EclMaterialManager)
    effectiveReader.read();
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
readUnscaledEpsPointsVectors_()
{
    if (this->parent_.hasGas && this->parent_.hasOil) {
        readUnscaledEpsPoints_(
            this->parent_.gasOilUnscaledPointsVector_,
            this->parent_.gasOilConfig_,
            EclTwoPhaseSystemType::GasOil
        );
    }
    if (this->parent_.hasOil && this->parent_.hasWater) {
        readUnscaledEpsPoints_(
            this->parent_.oilWaterUnscaledPointsVector_,
            this->parent_.oilWaterConfig_,
            EclTwoPhaseSystemType::OilWater
        );
    }
    if (!this->parent_.hasOil) {
        readUnscaledEpsPoints_(
            this->parent_.gasWaterUnscaledPointsVector_,
            this->parent_.gasWaterConfig_,
            EclTwoPhaseSystemType::GasWater
        );
    }
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
template <class Container>
void
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
readUnscaledEpsPoints_(Container& dest, SharedPtr<EclEpsConfig> config, EclTwoPhaseSystemType system_type)
{
    const size_t numSatRegions = this->eclState_.runspec().tabdims().getNumSatTables();
    dest.resize(numSatRegions);
    for (unsigned satRegionIdx = 0; satRegionIdx < numSatRegions; ++satRegionIdx) {
        dest[satRegionIdx] = SharedPtr<EclEpsScalingPoints<Scalar> >();
        dest[satRegionIdx]->init(this->parent_.unscaledEpsInfo_[satRegionIdx], *config, system_type);
    }
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
unsigned
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
satRegion_(Storage<int>& array, unsigned elemIdx)
{
    Storage<int>& default_vec = this->parent_.satnumRegionArray_;
    return satOrImbRegion_(array, default_vec, elemIdx);
}

template<
    class Traits,
    template<class> class Storage,
    template<typename> typename SharedPtr,
    template<typename, typename...> typename UniquePtr
>
unsigned
EclMaterialLawManager<Traits, Storage, SharedPtr, UniquePtr>::InitParams::
satOrImbRegion_(Storage<int>& array, Storage<int>& default_vec, unsigned elemIdx)
{
    int value;
    if (array.size() > 0) {
        value = array[elemIdx];
    }
    else { // use default value
        value = default_vec[elemIdx];
    }
    return static_cast<unsigned>(value);
}

// Make some actual code, by realizing the previously defined templated class
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,0,1,2>>::InitParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,0,1,2>>::InitParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,2,0,1>>::InitParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,2,0,1>>::InitParams;

} // namespace Opm
