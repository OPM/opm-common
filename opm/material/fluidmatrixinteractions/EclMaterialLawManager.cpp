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

#include <config.h>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>

#include <opm/common/TimingMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawInitParams.hpp>
#include <opm/material/fluidstates/SimpleModularFluidState.hpp>

#include <algorithm>

namespace Opm::EclMaterialLaw {

template<class TraitsT>
void Manager<TraitsT>::
initFromState(const EclipseState& eclState)
{
    // get the number of saturation regions and the number of cells in the deck
    const auto&  runspec       = eclState.runspec();
    const size_t numSatRegions = runspec.tabdims().getNumSatTables();

    const auto& ph = runspec.phases();
    this->hasGas_ = ph.active(Phase::GAS);
    this->hasOil_ = ph.active(Phase::OIL);
    this->hasWater_ = ph.active(Phase::WATER);

    readGlobalEpsOptions_(eclState);
    readGlobalHysteresisOptions_(eclState);
    readGlobalThreePhaseOptions_(runspec);

    const auto& tables = eclState.getTableManager();

    {
        const auto& stone1exTables = tables.getStone1exTable();

        if (! stone1exTables.empty()) {
            stoneEtas_.clear();
            stoneEtas_.reserve(numSatRegions);

            std::ranges::transform(stone1exTables, std::back_inserter(stoneEtas_),
                                   [](const auto& table)
                                   { return table.eta; });
        }

        const auto& ppcwmaxTables = tables.getPpcwmax();
        this->enablePpcwmax_ = !ppcwmaxTables.empty();

        if (this->enablePpcwmax_) {
            maxAllowPc_.clear();
            modifySwl_.clear();

            maxAllowPc_.reserve(numSatRegions);
            modifySwl_.reserve(numSatRegions);

            for (const auto& table : ppcwmaxTables) {
                maxAllowPc_.push_back(table.max_cap_pres);
                modifySwl_.push_back(table.option);
            }
        }
    }

    this->unscaledEpsInfo_.resize(numSatRegions);

    if (this->hasGas_ + this->hasOil_ + this->hasWater_ == 1) {
        // Single-phase simulation.  Special case.  Nothing to do here.
        return;
    }

    // Multiphase simulation.  Common case.
    const auto tolcrit = runspec.saturationFunctionControls()
        .minimumRelpermMobilityThreshold();

    const auto rtep  = satfunc::getRawTableEndpoints(tables, ph, tolcrit);
    const auto rfunc = satfunc::getRawFunctionValues(tables, ph, rtep);

    for (unsigned satRegionIdx = 0; satRegionIdx < numSatRegions; ++satRegionIdx) {
        this->unscaledEpsInfo_[satRegionIdx]
            .extractUnscaled(rtep, rfunc, satRegionIdx);
    }

    // WAG hysteresis parameters per SATNUM.
    if (eclState.runspec().hysterPar().activeWag()) {
        if (numSatRegions != eclState.getWagHysteresis().size())
            throw std::runtime_error("Inconsistent Wag-hysteresis data");
        wagHystersisConfig_.resize(numSatRegions);
        for (unsigned satRegionIdx = 0; satRegionIdx < numSatRegions; ++satRegionIdx) {
            wagHystersisConfig_[satRegionIdx] = std::make_shared<WagHysteresisConfig::
                WagHysteresisConfigRecord >(eclState.getWagHysteresis()[satRegionIdx]);
        }
    }
}

template<class TraitsT>
void Manager<TraitsT>::
initParamsForElements(const EclipseState& eclState, size_t numCompressedElems,
                      const std::function<std::vector<int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropIntOnLeafAssigner,
                      const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner)
{
    InitParams<Traits> initParams {*this, eclState, numCompressedElems};
    initParams.run(fieldPropIntOnLeafAssigner, lookupIdxOnLevelZeroAssigner);
    params_ = std::move(initParams.params_);
}

// TODO: Better (proper?) handling of mixed wettability systems - see ecl kw OPTIONS switch 74
// Note: Without OPTIONS[74] the negative part of the Pcow curve is not scaled
template<class TraitsT>
std::pair<typename TraitsT::Scalar, bool>
Manager<TraitsT>::
applySwatinit(unsigned elemIdx,
              Scalar pcow,
              Scalar Sw)
{
    // Default is no SWATINIT scaling of the negative part of the Pcow curve, so look up saturation using the input Pcow curve
    if (pcow <= 0.0) {
        return {Sw, /*newSwatInit*/ true};
    }

    auto& elemScaledEpsInfo = params_.oilWaterScaledEpsInfoDrainage[elemIdx];
    if (Sw <= elemScaledEpsInfo.Swl)
        Sw = elemScaledEpsInfo.Swl;

    // specify a fluid state which only stores the saturations
    using FluidState = SimpleModularFluidState<Scalar,
                                                TraitsT::numPhases,
                                                /*numComponents=*/0,
                                                /*FluidSystem=*/void, /* -> don't care */
                                                /*storePressure=*/false,
                                                /*storeTemperature=*/false,
                                                /*storeComposition=*/false,
                                                /*storeFugacity=*/false,
                                                /*storeSaturation=*/true,
                                                /*storeDensity=*/false,
                                                /*storeViscosity=*/false,
                                                /*storeEnthalpy=*/false>;
    FluidState fs;
    fs.setSaturation(TraitsT::wettingPhaseIdx, Sw);
    fs.setSaturation(TraitsT::gasPhaseIdx, 0);
    fs.setSaturation(TraitsT::nonWettingPhaseIdx, 0);
    std::array<Scalar, numPhases> pc = { 0 };
    MaterialLaw::capillaryPressures(pc, materialLawParams(elemIdx), fs);
    Scalar pcowAtSw = pc[oilPhaseIdx] - pc[waterPhaseIdx];
    constexpr const Scalar pcowAtSwThreshold = 1.0e-6; //Pascal

    // avoid division by very small number and avoid negative PCW at connate Sw
    // (look up saturation on input Pcow curve in this case)
    if (pcowAtSw < pcowAtSwThreshold) {
        return {Sw, /*newSwatInit*/ true};
    }

    // Sufficiently positive value, continue with max. capillary pressure (PCW) scaling to honor SWATINIT value
    Scalar newMaxPcow = elemScaledEpsInfo.maxPcow * (pcow/pcowAtSw);

    // Limit max. capillary pressure with PPCWMAX
    bool newSwatInit = false;
    int satRegionIdx = satnumRegionIdx(elemIdx);
    if (enablePpcwmax() && (newMaxPcow > maxAllowPc_[satRegionIdx])) {
        // Two options in PPCWMAX to modify connate Sw or not.  In both cases, init. Sw needs to be
        // re-calculated (done in opm-simulators)
        newSwatInit = true;
        if (modifySwl_[satRegionIdx] == false) {
            // Max. cap. pressure set to PCWO in PPCWMAX
            elemScaledEpsInfo.maxPcow = maxAllowPc_[satRegionIdx];
        }
        else {
            // Max. cap. pressure remains unscaled and connate Sw is set to SWATINIT value
            elemScaledEpsInfo.Swl = Sw;
        }
    }
    // Max. cap. pressure adjusted from SWATINIT data
    else
        elemScaledEpsInfo.maxPcow = newMaxPcow;

    auto& elemEclEpsScalingPoints = oilWaterScaledEpsPointsDrainage(elemIdx);
    elemEclEpsScalingPoints.init(elemScaledEpsInfo,
                                 oilWaterConfig_,
                                 EclTwoPhaseSystemType::OilWater);

    return {Sw, newSwatInit};
}

template<class TraitsT>
void
Manager<TraitsT>::
applyRestartSwatInit(const unsigned elemIdx,
                     const Scalar   maxPcow)
{
    // Maximum capillary pressure adjusted from SWATINIT data.

    auto& elemScaledEpsInfo =
        this->params_.oilWaterScaledEpsInfoDrainage[elemIdx];

    elemScaledEpsInfo.maxPcow = maxPcow;

    this->oilWaterScaledEpsPointsDrainage(elemIdx)
        .init(elemScaledEpsInfo,
              this->oilWaterConfig_,
              EclTwoPhaseSystemType::OilWater);
}

namespace
{
// Helper to get the eps level parameters.
template <class TraitsT, class MaybeHystParams>
auto& getDrainageParams(MaybeHystParams& p)
{
    if constexpr(TraitsT::enableHysteresis) {
        return p.drainageParams();
    } else {
        return p;
    }
}
} // anon namespace

template<class TraitsT>
const typename Manager<TraitsT>::MaterialLawParams&
Manager<TraitsT>::
connectionMaterialLawParams(unsigned satRegionIdx, unsigned elemIdx) const
{
    MaterialLawParams& mlp = const_cast<MaterialLawParams&>(params_.materialLawParams[elemIdx]);

    if (enableHysteresis())
        OpmLog::warning("Warning: Using non-default satnum regions for connection is not tested in combination with hysteresis");
    // Currently we don't support COMPIMP. I.e. use the same table lookup for the hysteresis curves.
    // unsigned impRegionIdx = satRegionIdx;

    // change the sat table it points to.
    switch (mlp.approach()) {
    case EclMultiplexerApproach::Stone1: {
        auto& realParams = mlp.template getRealParams<EclMultiplexerApproach::Stone1>();

        getDrainageParams<Traits>(realParams.oilWaterParams()).setUnscaledPoints(params_.oilWaterUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.oilWaterParams()).setEffectiveLawParams(params_.oilWaterEffectiveParamVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setUnscaledPoints(params_.gasOilUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setEffectiveLawParams(params_.gasOilEffectiveParamVector[satRegionIdx]);
//            if (enableHysteresis()) {
//                getImbParams(realParams.oilWaterParams()).setUnscaledPoints(oilWaterUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.oilWaterParams()).setEffectiveLawParams(oilWaterEffectiveParamVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setUnscaledPoints(gasOilUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setEffectiveLawParams(gasOilEffectiveParamVector_[impRegionIdx]);
//            }
    }
        break;

    case EclMultiplexerApproach::Stone2: {
        auto& realParams = mlp.template getRealParams<EclMultiplexerApproach::Stone2>();
        getDrainageParams<Traits>(realParams.oilWaterParams()).setUnscaledPoints(params_.oilWaterUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.oilWaterParams()).setEffectiveLawParams(params_.oilWaterEffectiveParamVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setUnscaledPoints(params_.gasOilUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setEffectiveLawParams(params_.gasOilEffectiveParamVector[satRegionIdx]);
//            if (enableHysteresis()) {
//                getImbParams(realParams.oilWaterParams()).setUnscaledPoints(oilWaterUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.oilWaterParams()).setEffectiveLawParams(oilWaterEffectiveParamVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setUnscaledPoints(gasOilUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setEffectiveLawParams(gasOilEffectiveParamVector_[impRegionIdx]);
//            }
    }
        break;

    case EclMultiplexerApproach::Default: {
        auto& realParams = mlp.template getRealParams<EclMultiplexerApproach::Default>();
        getDrainageParams<Traits>(realParams.oilWaterParams()).setUnscaledPoints(params_.oilWaterUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.oilWaterParams()).setEffectiveLawParams(params_.oilWaterEffectiveParamVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setUnscaledPoints(params_.gasOilUnscaledPointsVector[satRegionIdx]);
        getDrainageParams<Traits>(realParams.gasOilParams()).setEffectiveLawParams(params_.gasOilEffectiveParamVector[satRegionIdx]);
//            if (enableHysteresis()) {
//                getImbParams(realParams.oilWaterParams()).setUnscaledPoints(oilWaterUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.oilWaterParams()).setEffectiveLawParams(oilWaterEffectiveParamVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setUnscaledPoints(gasOilUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setEffectiveLawParams(gasOilEffectiveParamVector_[impRegionIdx]);
//            }
    }
        break;

    case EclMultiplexerApproach::TwoPhase: {
        auto& realParams = mlp.template getRealParams<EclMultiplexerApproach::TwoPhase>();
        if (realParams.approach() == EclTwoPhaseApproach::GasOil) {
            getDrainageParams<Traits>(realParams.gasOilParams()).setUnscaledPoints(params_.gasOilUnscaledPointsVector[satRegionIdx]);
            getDrainageParams<Traits>(realParams.gasOilParams()).setEffectiveLawParams(params_.gasOilEffectiveParamVector[satRegionIdx]);
        }
        else if (realParams.approach() == EclTwoPhaseApproach::GasWater) {
            getDrainageParams<Traits>(realParams.gasWaterParams()).setUnscaledPoints(params_.gasWaterUnscaledPointsVector[satRegionIdx]);
            getDrainageParams<Traits>(realParams.gasWaterParams()).setEffectiveLawParams(params_.gasWaterEffectiveParamVector[satRegionIdx]);
        }
        else if (realParams.approach() == EclTwoPhaseApproach::OilWater) {
            getDrainageParams<Traits>(realParams.oilWaterParams()).setUnscaledPoints(params_.oilWaterUnscaledPointsVector[satRegionIdx]);
            getDrainageParams<Traits>(realParams.oilWaterParams()).setEffectiveLawParams(params_.oilWaterEffectiveParamVector[satRegionIdx]);
        }
//            if (enableHysteresis()) {
//                getImbParams(realParams.oilWaterParams()).setUnscaledPoints(oilWaterUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.oilWaterParams()).setEffectiveLawParams(oilWaterEffectiveParamVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setUnscaledPoints(gasOilUnscaledPointsVector_[impRegionIdx]);
//                getImbParams(realParams.gasOilParams()).setEffectiveLawParams(gasOilEffectiveParamVector_[impRegionIdx]);
//            }
    }
        break;

    default:
        throw std::logic_error("Enum value for material approach unknown!");
    }
    return mlp;
}

template<class TraitsT>
int
Manager<TraitsT>::
getKrnumSatIdx(unsigned elemIdx, FaceDir::DirEnum facedir) const
{
    using Dir = FaceDir::DirEnum;
    const std::vector<int>* array = nullptr;
    switch(facedir) {
    case Dir::XPlus:
      array = &params_.krnumXArray;
      break;
    case Dir::YPlus:
      array = &params_.krnumYArray;
      break;
    case Dir::ZPlus:
      array = &params_.krnumZArray;
      break;
    default:
      throw std::runtime_error("Unknown face direction");
    }
    if (array->size() > 0) {
      return (*array)[elemIdx];
    }
    else {
      return params_.satnumRegionArray[elemIdx];
    }
}

template<class TraitsT>
void
Manager<TraitsT>::
oilWaterHysteresisParams(Scalar& soMax,
                         Scalar& swMax,
                         Scalar& swMin,
                         unsigned elemIdx) const
{
    if (!enableHysteresis()) {
        throw std::runtime_error("Cannot get hysteresis parameters if hysteresis not enabled.");
    }

    MaterialLaw::oilWaterHysteresisParams(soMax, swMax, swMin, materialLawParams(elemIdx));
}

template<class TraitsT>
void
Manager<TraitsT>::
setOilWaterHysteresisParams(const Scalar& soMax,
                            const Scalar& swMax,
                            const Scalar& swMin,
                            unsigned elemIdx)
{
    if (!enableHysteresis())
        throw std::runtime_error("Cannot set hysteresis parameters if hysteresis not enabled.");

    MaterialLaw::setOilWaterHysteresisParams(soMax, swMax, swMin, materialLawParams(elemIdx));
}

template<class TraitsT>
void
Manager<TraitsT>::
gasOilHysteresisParams(Scalar& sgmax,
                       Scalar& shmax,
                       Scalar& somin,
                       unsigned elemIdx) const
{
    if (!enableHysteresis()) {
        throw std::runtime_error("Cannot get hysteresis parameters if hysteresis not enabled.");
    }

    MaterialLaw::gasOilHysteresisParams(sgmax, shmax, somin, materialLawParams(elemIdx));
}

template<class TraitsT>
void
Manager<TraitsT>::
setGasOilHysteresisParams(const Scalar& sgmax,
                          const Scalar& shmax,
                          const Scalar& somin,
                          unsigned elemIdx)
{
    if (!enableHysteresis())
        throw std::runtime_error("Cannot set hysteresis parameters if hysteresis not enabled.");

    MaterialLaw::setGasOilHysteresisParams(sgmax, shmax, somin, materialLawParams(elemIdx));
}

namespace
{
template<class TraitsT, class MaterialLawParamsT>
EclEpsScalingPoints<typename TraitsT::Scalar>&
owsepdHelper(MaterialLawParamsT& mlp)
{
    if constexpr(TraitsT::enableHysteresis) {
        return mlp.oilWaterParams().drainageParams().scaledPoints();
    } else {
        return mlp.oilWaterParams().scaledPoints();
    }
}
} // anon namespace

template<class TraitsT>
EclEpsScalingPoints<typename TraitsT::Scalar>&
Manager<TraitsT>::
oilWaterScaledEpsPointsDrainage(unsigned elemIdx)
{
    auto& materialParams = params_.materialLawParams[elemIdx];
    switch (materialParams.approach()) {
    case EclMultiplexerApproach::Stone1: {
        auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone1>();
        return owsepdHelper<Traits>(realParams);
    }

    case EclMultiplexerApproach::Stone2: {
        auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Stone2>();
        return owsepdHelper<Traits>(realParams);
    }

    case EclMultiplexerApproach::Default: {
        auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::Default>();
        return owsepdHelper<Traits>(realParams);
    }

    case EclMultiplexerApproach::TwoPhase: {
        auto& realParams = materialParams.template getRealParams<EclMultiplexerApproach::TwoPhase>();
        return owsepdHelper<Traits>(realParams);
    }
    default:
        throw std::logic_error("Enum value for material approach unknown!");
    }
}

template<class TraitsT>
const typename Manager<TraitsT>::MaterialLawParams&
Manager<TraitsT>::
materialLawParamsFunc_(unsigned elemIdx, FaceDir::DirEnum facedir) const
{
    using Dir = FaceDir::DirEnum;
    if (params_.dirMaterialLawParams) {
        switch(facedir) {
            case Dir::XMinus:
            case Dir::XPlus:
                return params_.dirMaterialLawParams->materialLawParamsX_[elemIdx];
            case Dir::YMinus:
            case Dir::YPlus:
                return params_.dirMaterialLawParams->materialLawParamsY_[elemIdx];
            case Dir::ZMinus:
            case Dir::ZPlus:
                return params_.dirMaterialLawParams->materialLawParamsZ_[elemIdx];
            default:
                throw std::runtime_error("Unexpected face direction");
        }
    }
    else {
        return params_.materialLawParams[elemIdx];
    }
}

template<class TraitsT>
void
Manager<TraitsT>::
readGlobalEpsOptions_(const EclipseState& eclState)
{
    enableEndPointScaling_ = eclState.getTableManager().hasTables("ENKRVD");

    // Read the end point scaling configuration (once per run).
    gasOilConfig_.initFromState(eclState, EclTwoPhaseSystemType::GasOil);
    oilWaterConfig_.initFromState(eclState, EclTwoPhaseSystemType::OilWater);
    gasWaterConfig_.initFromState(eclState, EclTwoPhaseSystemType::GasWater);
}

template<class TraitsT>
void
Manager<TraitsT>::
readGlobalHysteresisOptions_(const EclipseState& state)
{
    hysteresisConfig_.initFromState(state.runspec());
}

template<class TraitsT>
void
Manager<TraitsT>::
readGlobalThreePhaseOptions_(const Runspec& runspec)
{
    bool gasEnabled = runspec.phases().active(Phase::GAS);
    bool oilEnabled = runspec.phases().active(Phase::OIL);
    bool waterEnabled = runspec.phases().active(Phase::WATER);

    int numEnabled =
        (gasEnabled?1:0)
        + (oilEnabled?1:0)
        + (waterEnabled?1:0);

    if (numEnabled == 0) {
        throw std::runtime_error("At least one fluid phase must be enabled. (Is: "+std::to_string(numEnabled)+")");
    } else if (numEnabled == 1) {
        threePhaseApproach_ = EclMultiplexerApproach::OnePhase;
    } else if ( numEnabled == 2) {
        threePhaseApproach_ = EclMultiplexerApproach::TwoPhase;
        if (!gasEnabled)
            twoPhaseApproach_ = EclTwoPhaseApproach::OilWater;
        else if (!oilEnabled)
            twoPhaseApproach_ = EclTwoPhaseApproach::GasWater;
        else if (!waterEnabled)
            twoPhaseApproach_ = EclTwoPhaseApproach::GasOil;
    }
    else {
        assert(numEnabled == 3);

        threePhaseApproach_ = EclMultiplexerApproach::Default;
        const auto& satctrls = runspec.saturationFunctionControls();
        if (satctrls.krModel() == SatFuncControls::ThreePhaseOilKrModel::Stone2)
            threePhaseApproach_ = EclMultiplexerApproach::Stone2;
        else if (satctrls.krModel() == SatFuncControls::ThreePhaseOilKrModel::Stone1)
            threePhaseApproach_ = EclMultiplexerApproach::Stone1;
    }
}

template class Manager<ThreePhaseMaterialTraits<double,0,1,2,true,true>>;
template class Manager<ThreePhaseMaterialTraits<float,0,1,2,true,true>>;
template class Manager<ThreePhaseMaterialTraits<double,2,0,1,true,true>>;
template class Manager<ThreePhaseMaterialTraits<float,2,0,1,true,true>>;
template class Manager<ThreePhaseMaterialTraits<double,0,1,2,false,true>>;
template class Manager<ThreePhaseMaterialTraits<float,0,1,2,false,true>>;

} // namespace Opm::EclMaterialLaw
