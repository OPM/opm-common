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
#include <opm/input/eclipse/EclipseState/WagHysteresisConfig.hpp>

#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawTwoPhaseTypes.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexer.hpp>
#include <opm/material/fluidmatrixinteractions/EclHysteresisTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>
#include <opm/material/fluidmatrixinteractions/DirectionalMaterialLawParams.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace Opm {

class EclipseState;
class EclEpsGridProperties;
template<class Scalar> class EclEpsScalingPoints;
template<class Scalar> struct EclEpsScalingPointsInfo;
class EclHysteresisConfig;
enum class EclTwoPhaseSystemType;
class FieldPropsManager;
class Runspec;
class SgfnTable;
class SgofTable;
class SlgofTable;
class TableColumn;

}

namespace Opm::EclMaterialLaw {

template<class Traits> class InitParams;

/*!
 * \ingroup fluidmatrixinteractions
 *
 * \brief Provides an simple way to create and manage the material law objects
 *        for a complete ECL deck.
 */
template <class TraitsT>
class Manager
{
    using Traits = TraitsT;
    using Scalar = typename Traits::Scalar;
    static constexpr int gasPhaseIdx = Traits::gasPhaseIdx;
    static constexpr int oilPhaseIdx = Traits::nonWettingPhaseIdx;
    static constexpr int waterPhaseIdx = Traits::wettingPhaseIdx;
    static constexpr int numPhases = Traits::numPhases;
    using GasOilEffectiveParamVector = typename EclMaterialLaw::TwoPhaseTypes<Traits>::GasOilEffectiveParamVector;
    using GasWaterEffectiveParamVector = typename EclMaterialLaw::TwoPhaseTypes<Traits>::GasWaterEffectiveParamVector;
    using OilWaterEffectiveParamVector = typename EclMaterialLaw::TwoPhaseTypes<Traits>::OilWaterEffectiveParamVector;

public:
    // the three-phase material law used by the simulation
    using MaterialLaw = EclMultiplexerMaterial<Traits,
                                               typename EclMaterialLaw::TwoPhaseTypes<Traits>::GasOilLaw,
                                               typename EclMaterialLaw::TwoPhaseTypes<Traits>::OilWaterLaw,
                                               typename EclMaterialLaw::TwoPhaseTypes<Traits>::GasWaterLaw>;
    using MaterialLawParams = typename MaterialLaw::Params;
    using DirectionalMaterialLawParamsPtr = std::unique_ptr<DirectionalMaterialLawParams<MaterialLawParams>>;

private:
    using GasOilScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using GasWaterScalingPointsVector = std::vector<std::shared_ptr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingInfoVector = std::vector<EclEpsScalingPointsInfo<Scalar>>;
    using MaterialLawParamsVector = std::vector<std::shared_ptr<MaterialLawParams>>;

public:
    struct Params
    {
        OilWaterScalingInfoVector oilWaterScaledEpsInfoDrainage{};
        GasOilEffectiveParamVector gasOilEffectiveParamVector{};
        OilWaterEffectiveParamVector oilWaterEffectiveParamVector{};
        GasWaterEffectiveParamVector gasWaterEffectiveParamVector{};
        GasOilScalingPointsVector gasOilUnscaledPointsVector{};
        OilWaterScalingPointsVector oilWaterUnscaledPointsVector{};
        GasWaterScalingPointsVector gasWaterUnscaledPointsVector{};
        std::vector<int> krnumXArray{};
        std::vector<int> krnumYArray{};
        std::vector<int> krnumZArray{};
        std::vector<int> imbnumXArray{};
        std::vector<int> imbnumYArray{};
        std::vector<int> imbnumZArray{};
        std::vector<int> satnumRegionArray{};
        std::vector<int> imbnumRegionArray{};
        std::vector<MaterialLawParams> materialLawParams{};
        DirectionalMaterialLawParamsPtr dirMaterialLawParams{};

        bool hasDirectionalRelperms() const
        {
            return !krnumXArray.empty() ||
                   !krnumYArray.empty() ||
                   !krnumZArray.empty();
        }

        bool hasDirectionalImbnum() const
        {
            return !imbnumXArray.empty() ||
                   !imbnumYArray.empty() ||
                   !imbnumZArray.empty();
        }
    };

    void initFromState(const EclipseState& eclState);

    // \brief Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
    //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    //        Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
    //        leaf gridview cell with index 'elemIdx', its 'lookupIdx' (index of the parent/equivalent cell on level zero).
    void initParamsForElements(const EclipseState& eclState, size_t numCompressedElems,
                               const std::function<std::vector<int>(const FieldPropsManager&, const std::string&, bool)>&
                               fieldPropIntOnLeafAssigner,
                               const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);

    /*!
     * \brief Modify the initial condition according to the SWATINIT keyword.
     *
     * The method returns the water saturation which yields a givenn capillary
     * pressure. The reason this method is not folded directly into initFromState() is
     * that the capillary pressure given depends on the particuars of how the simulator
     * calculates its initial condition.
     */
    std::pair<Scalar, bool>
    applySwatinit(unsigned elemIdx,
                  Scalar pcow,
                  Scalar Sw);

    /// Apply SWATINIT-like scaling of oil/water capillary pressure curve at
    /// simulation restart.
    ///
    /// \param[in] elemIdx Active cell index
    ///
    /// \param[in] maxPcow Scaled maximum oil/water capillary pressure.
    ///   Typically the PPCW restart file array's entry for the
    ///   corresponding cell.
    void applyRestartSwatInit(const unsigned elemIdx, const Scalar maxPcow);

    bool enableEndPointScaling() const
    { return enableEndPointScaling_; }

    bool enablePpcwmax() const
    { return enablePpcwmax_; }

    const EclHysteresisConfig& hysteresisConfig() const
    { return hysteresisConfig_; }

    bool enableHysteresis() const
    { return hysteresisConfig_.enableHysteresis(); }

    bool enablePCHysteresis() const
    { return hysteresisConfig_.enablePCHysteresis(); }

    bool enableWettingHysteresis() const
    { return hysteresisConfig_.enableWettingHysteresis(); }

    bool enableNonWettingHysteresis() const
    { return hysteresisConfig_.enableNonWettingHysteresis(); }

    bool hasGas() const
    { return hasGas_; }

    bool hasOil() const
    { return hasOil_; }

    bool hasWater() const
    { return hasWater_; }

    const EclEpsScalingPointsInfo<Scalar>& unscaledEpsInfo(unsigned satRegionIdx) const
    { return unscaledEpsInfo_[satRegionIdx]; }

    std::shared_ptr<WagHysteresisConfig::WagHysteresisConfigRecord>
    wagHystersisConfig(unsigned satRegionIdx) const
    { return wagHystersisConfig_[satRegionIdx]; }

    const EclEpsConfig& gasOilConfig() const
    { return gasOilConfig_; }

    const EclEpsConfig& gasWaterConfig() const
    { return gasWaterConfig_; }

    const EclEpsConfig& oilWaterConfig() const
    { return oilWaterConfig_; }

    MaterialLawParams& materialLawParams(unsigned elemIdx)
    {
        assert(elemIdx <  params_.materialLawParams.size());
        return params_.materialLawParams[elemIdx];
    }

    const MaterialLawParams& materialLawParams(unsigned elemIdx) const
    {
        assert(elemIdx <  params_.materialLawParams.size());
        return params_.materialLawParams[elemIdx];
    }

    const MaterialLawParams& materialLawParams(unsigned elemIdx, FaceDir::DirEnum facedir) const
    { return materialLawParamsFunc_(elemIdx, facedir); }

    MaterialLawParams& materialLawParams(unsigned elemIdx, FaceDir::DirEnum facedir)
    { return const_cast<MaterialLawParams&>(materialLawParamsFunc_(elemIdx, facedir)); }

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
    { return params_.satnumRegionArray[elemIdx]; }

    int getKrnumSatIdx(unsigned elemIdx, FaceDir::DirEnum facedir) const;

    bool hasDirectionalRelperms() const
    { return params_.hasDirectionalRelperms(); }

    bool hasDirectionalImbnum() const
    { return params_.hasDirectionalImbnum(); }

    int imbnumRegionIdx(unsigned elemIdx) const
    { return params_.imbnumRegionArray[elemIdx]; }

    EclMultiplexerApproach threePhaseApproach() const
    { return threePhaseApproach_; }

    EclTwoPhaseApproach twoPhaseApproach() const
    { return twoPhaseApproach_; }

    const std::vector<Scalar>& stoneEtas() const
    { return stoneEtas_; }

    template <class FluidState>
    bool updateHysteresis(const FluidState& fluidState, unsigned elemIdx)
    {
        OPM_TIMEFUNCTION_LOCAL();
        if (!enableHysteresis())
            return false;
        bool changed = MaterialLaw::updateHysteresis(materialLawParams(elemIdx), fluidState);
        if (hasDirectionalRelperms() || hasDirectionalImbnum()) {
            using Dir = FaceDir::DirEnum;
            constexpr int ndim = 3;
            const Dir facedirs[] = {Dir::XPlus, Dir::YPlus, Dir::ZPlus};
            for (int i = 0; i<ndim; i++) {
                const bool ischanged =
                    MaterialLaw::updateHysteresis(materialLawParams(elemIdx, facedirs[i]), fluidState);
                changed = changed || ischanged;
            }
        }
        return changed;
    }

    void oilWaterHysteresisParams(Scalar& soMax,
                                  Scalar& swMax,
                                  Scalar& swMin,
                                  unsigned elemIdx) const;

    void setOilWaterHysteresisParams(const Scalar& soMax,
                                     const Scalar& swMax,
                                     const Scalar& swMin,
                                     unsigned elemIdx);

    void gasOilHysteresisParams(Scalar& sgmax,
                                Scalar& shmax,
                                Scalar& somin,
                                unsigned elemIdx) const;

    void setGasOilHysteresisParams(const Scalar& sgmax,
                                   const Scalar& shmax,
                                   const Scalar& somin,
                                   unsigned elemIdx);

    EclEpsScalingPoints<Scalar>& oilWaterScaledEpsPointsDrainage(unsigned elemIdx);

    const EclEpsScalingPointsInfo<Scalar>& oilWaterScaledEpsInfoDrainage(size_t elemIdx) const
    { return params_.oilWaterScaledEpsInfoDrainage[elemIdx]; }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        // This is for restart serialization.
        // Only dynamic state in the parameters need to be stored.
        // For that reason we do not serialize the vector
        // as that would recreate the objects inside.
        for (auto& mat : params_.materialLawParams) {
            serializer(mat);
        }
    }

    EclMultiplexerApproach threePhaseApproach() const
    {
        return this->threePhaseApproach_;
    }

    bool satCurveIsAllPiecewiseLinear() const
    {
        return this->onlyPiecewiseLinear_;
    }

private:
    const MaterialLawParams& materialLawParamsFunc_(unsigned elemIdx, FaceDir::DirEnum facedir) const;

    void readGlobalEpsOptions_(const EclipseState& eclState);

    void readGlobalHysteresisOptions_(const EclipseState& state);

    void readGlobalThreePhaseOptions_(const Runspec& runspec);

    bool enableEndPointScaling_{false};
    EclHysteresisConfig hysteresisConfig_;
    std::vector<std::shared_ptr<WagHysteresisConfig::WagHysteresisConfigRecord>> wagHystersisConfig_;

    EclEpsConfig oilWaterEclEpsConfig_;
    std::vector<EclEpsScalingPointsInfo<Scalar>> unscaledEpsInfo_;

    Params params_;

    EclMultiplexerApproach threePhaseApproach_ = EclMultiplexerApproach::Default;
    // this attribute only makes sense for twophase simulations!
    EclTwoPhaseApproach twoPhaseApproach_ = EclTwoPhaseApproach::GasOil;
    bool onlyPiecewiseLinear_ = true;

    std::vector<Scalar> stoneEtas_;

    bool enablePpcwmax_{false};
    std::vector<Scalar> maxAllowPc_;
    std::vector<bool> modifySwl_;

    bool hasGas_{true};
    bool hasOil_{true};
    bool hasWater_{true};

    EclEpsConfig gasOilConfig_;
    EclEpsConfig oilWaterConfig_;
    EclEpsConfig gasWaterConfig_;
};

} // namespace Opm::EclMaterialLaw

#endif
