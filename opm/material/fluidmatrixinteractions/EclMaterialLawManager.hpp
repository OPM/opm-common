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


#include "EclTwoPhaseMaterial.hpp" // For GPU version we will directly access this material layer

#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/WagHysteresisConfig.hpp>

#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexer.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclHysteresisTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>
#include <opm/material/fluidmatrixinteractions/DirectionalMaterialLawParams.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

namespace Opm {

class EclipseState;
class EclEpsConfig;
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

/*!
 * \ingroup fluidmatrixinteractions
 *
 * \brief Provides an simple way to create and manage the material law objects
 *        for a complete ECL deck.
 */
template<
    class TraitsT,
    template<class> class Storage = VectorWithDefaultAllocator,
    template<typename> typename SharedPtr = std::shared_ptr,
    template<typename, typename...> typename UniquePtr = std::unique_ptr
>
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

#if !HAVE_CUDA

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

#else // HAVE_CUDA

    using GasOilEffectiveTwoPhaseLaw = PiecewiseLinearTwoPhaseMaterial<GasOilTraits, PiecewiseLinearTwoPhaseMaterialParams<GasOilTraits, Storage<Scalar>>>;
    using OilWaterEffectiveTwoPhaseLaw = PiecewiseLinearTwoPhaseMaterial<OilWaterTraits, PiecewiseLinearTwoPhaseMaterialParams<OilWaterTraits, Storage<Scalar>>>;
    using GasWaterEffectiveTwoPhaseLaw = PiecewiseLinearTwoPhaseMaterial<GasWaterTraits, PiecewiseLinearTwoPhaseMaterialParams<GasWaterTraits, Storage<Scalar>>>;

    using GasOilEffectiveTwoPhaseParams = typename GasOilEffectiveTwoPhaseLaw::Params;
    using OilWaterEffectiveTwoPhaseParams = typename OilWaterEffectiveTwoPhaseLaw::Params;
    using GasWaterEffectiveTwoPhaseParams = typename GasWaterEffectiveTwoPhaseLaw::Params;

#endif // !HAVE_CUDA

public:
    // the three-phase material law used by the simulation
    #if !HAVE_CUDA
    using MaterialLaw = EclMultiplexerMaterial<Traits, GasOilTwoPhaseLaw, OilWaterTwoPhaseLaw, GasWaterTwoPhaseLaw>;
    #else // HAVE_CUDA
    using MaterialLaw = EclTwoPhaseMaterial<TraitsT, GasOilEffectiveTwoPhaseLaw, OilWaterEffectiveTwoPhaseLaw, GasWaterEffectiveTwoPhaseLaw>;
    #endif
    using MaterialLawParams = typename MaterialLaw::Params;
    using DirectionalMaterialLawParamsPtr = UniquePtr<DirectionalMaterialLawParams<MaterialLawParams>>;

    EclMaterialLawManager();
    ~EclMaterialLawManager();

private:
    // internal typedefs
    using GasOilEffectiveParamVector = Storage<SharedPtr<GasOilEffectiveTwoPhaseParams>>;
    using OilWaterEffectiveParamVector = Storage<SharedPtr<OilWaterEffectiveTwoPhaseParams>>;
    using GasWaterEffectiveParamVector = Storage<SharedPtr<GasWaterEffectiveTwoPhaseParams>>;

    #if !HAVE_CUDA
    using GasOilScalingPointsVector = Storage<SharedPtr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingPointsVector = Storage<SharedPtr<EclEpsScalingPoints<Scalar>>>;
    using GasWaterScalingPointsVector = Storage<SharedPtr<EclEpsScalingPoints<Scalar>>>;
    using OilWaterScalingInfoVector = Storage<EclEpsScalingPointsInfo<Scalar>>;
    using GasOilParamVector = Storage<SharedPtr<GasOilTwoPhaseHystParams>>;
    using OilWaterParamVector = Storage<SharedPtr<OilWaterTwoPhaseHystParams>>;
    using GasWaterParamVector = Storage<SharedPtr<GasWaterTwoPhaseHystParams>>;
    using MaterialLawParamsVector = Storage<SharedPtr<MaterialLawParams>>;
    #endif // !HAVE_CUDA

    // helper classes

    // This class' implementation is defined in "EclMaterialLawManagerInitParams.cpp"
    class InitParams {
    public:
        InitParams(EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>& parent, const EclipseState& eclState, size_t numCompressedElems);
        // \brief Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
        //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
        //        Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
        //        leaf gridview cell with index 'elemIdx', its 'lookupIdx' (index of the parent/equivalent cell on level zero).
        void run(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropIntOnLeafAssigner,
                 const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
    private:
        
        #if !HAVE_CUDA
        class HystParams;
        #endif // !HAVE_CUDA
        // \brief Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
        //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
        void copySatnumArrays_(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>&
                               fieldPropIntOnLeafAssigner);
        // \brief Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
        //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
        void copyIntArray_(Storage<int>& dest, const std::string& keyword,
                           const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>&
                           fieldPropIntOnLeafAssigner);
        unsigned imbRegion_(Storage<int>& array, unsigned elemIdx);
        void initArrays_(
                         Storage<Storage<int>*>& satnumArray,
                         Storage<Storage<int>*>& imbnumArray,
                         Storage<Storage<MaterialLawParams>*>& mlpArray);
        void initMaterialLawParamVectors_();
        void initOilWaterScaledEpsInfo_();
        // \brief Function argument 'fieldProptOnLeadAssigner' needed to lookup
        //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
        void initSatnumRegionArray_(const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>&
                                    fieldPropIntOnLeafAssigner);
        #if !HAVE_CUDA
        void initThreePhaseParams_(
                                   HystParams &hystParams,
                                   MaterialLawParams& materialParams,
                                   unsigned satRegionIdx,
                                   unsigned elemIdx);
        #else // HAVE_CUDA
        void initThreePhaseParams_(
                                   MaterialLawParams& materialParams,
                                   unsigned satRegionIdx,
                                   unsigned elemIdx);
        #endif
        void readEffectiveParameters_();
        void readUnscaledEpsPointsVectors_();
        template <class Container>
        void readUnscaledEpsPoints_(Container& dest, SharedPtr<EclEpsConfig> config, EclTwoPhaseSystemType system_type);
        unsigned satRegion_(Storage<int>& array, unsigned elemIdx);
        unsigned satOrImbRegion_(Storage<int>& array, Storage<int>& default_vec, unsigned elemIdx);

        #if !HAVE_CUDA
        // This class' implementation is defined in "EclMaterialLawManagerHystParams.cpp"
        class HystParams {
        public:
            explicit HystParams(EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>::InitParams& init_params);
            void finalize();
            SharedPtr<GasOilTwoPhaseHystParams> getGasOilParams();
            SharedPtr<OilWaterTwoPhaseHystParams> getOilWaterParams();
            SharedPtr<GasWaterTwoPhaseHystParams> getGasWaterParams();
            void setConfig(unsigned satRegionIdx);
            // Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
            // leaf gridview cell with index 'elemIdx', its 'lookupIdx' (index of the parent/equivalent cell on level zero).
            void setDrainageParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                                         const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            void setDrainageParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                                           const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            void setDrainageParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                                           const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            void setImbibitionParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                                           const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            void setImbibitionParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                                             const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            void setImbibitionParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                                             const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
        private:
            bool hasGasWater_();
            bool hasGasOil_();
            bool hasOilWater_();

            // Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
            // leaf gridview cell with index 'elemIdx', its 'lookupIdx' (index of the parent/equivalent cell on level zero).
            std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
            readScaledEpsPoints_(const EclEpsGridProperties& epsGridProperties, unsigned elemIdx, EclTwoPhaseSystemType type,
                                 const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
            readScaledEpsPointsDrainage_(unsigned elemIdx, EclTwoPhaseSystemType type,
                                         const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);
            std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
            readScaledEpsPointsImbibition_(unsigned elemIdx, EclTwoPhaseSystemType type,
                                           const std::function<unsigned(unsigned)>& lookupIdxOnLevelZeroAssigner);

            EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>::InitParams& init_params_;
            EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>& parent_;
            const EclipseState& eclState_;
            SharedPtr<GasOilTwoPhaseHystParams> gasOilParams_;
            SharedPtr<OilWaterTwoPhaseHystParams> oilWaterParams_;
            SharedPtr<GasWaterTwoPhaseHystParams> gasWaterParams_;
        };
        #endif // !HAVE_CUDA

        // This class' implementation is defined in "EclMaterialLawManagerReadEffectiveParams.cpp"
        class ReadEffectiveParams {
        public:
            explicit ReadEffectiveParams(EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>::InitParams& init_params);
            void read();
        private:
            Storage<double> normalizeKrValues_(const double tolcrit, const TableColumn& krValues) const;
            void readGasOilParameters_(GasOilEffectiveParamVector& dest, unsigned satRegionIdx);
            template <class TableType>
            void readGasOilFamily2_(
                                    GasOilEffectiveTwoPhaseParams& effParams,
                                    const Scalar Swco,
                                    const double tolcrit,
                                    const TableType& sofTable,
                                    const SgfnTable& sgfnTable,
                                    const std::string& columnName);
            void readGasOilSgof_(GasOilEffectiveTwoPhaseParams& effParams,
                                 const Scalar Swco,
                                 const double tolcrit,
                                 const SgofTable& sgofTable);

            void readGasOilSlgof_(GasOilEffectiveTwoPhaseParams& effParams,
                                  const Scalar Swco,
                                  const double tolcrit,
                                  const SlgofTable& slgofTable);
            void readGasWaterParameters_(GasWaterEffectiveParamVector& dest, unsigned satRegionIdx);
            void readOilWaterParameters_(OilWaterEffectiveParamVector& dest, unsigned satRegionIdx);

            EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>::InitParams& init_params_;
            EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>& parent_;
            const EclipseState& eclState_;
        }; // end of "class ReadEffectiveParams"

        EclMaterialLawManager<TraitsT, Storage, SharedPtr, UniquePtr>& parent_;
        const EclipseState& eclState_;
        size_t numCompressedElems_;

        UniquePtr<EclEpsGridProperties> epsImbGridProperties_; //imbibition
        UniquePtr<EclEpsGridProperties> epsGridProperties_;    // drainage

    };  // end of "class InitParams"

public:
    void initFromState(const EclipseState& eclState);

    // \brief Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
    //        field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    //        Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
    //        leaf gridview cell with index 'elemIdx', its 'lookupIdx' (index of the parent/equivalent cell on level zero).
    void initParamsForElements(const EclipseState& eclState, size_t numCompressedElems,
                               const std::function<Storage<int>(const FieldPropsManager&, const std::string&, bool)>&
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
    { return *hysteresisConfig_; }

    bool enableHysteresis() const
    { return hysteresisConfig_->enableHysteresis(); }

    bool enablePCHysteresis() const
    { return hysteresisConfig_->enablePCHysteresis(); }

    bool enableWettingHysteresis() const
    { return hysteresisConfig_->enableWettingHysteresis(); }

    bool enableNonWettingHysteresis() const
    { return hysteresisConfig_->enableNonWettingHysteresis(); }

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

    const MaterialLawParams& materialLawParams(unsigned elemIdx, FaceDir::DirEnum facedir) const
    {
        return materialLawParamsFunc_(elemIdx, facedir);
    }

    MaterialLawParams& materialLawParams(unsigned elemIdx, FaceDir::DirEnum facedir)
    {
        return const_cast<MaterialLawParams&>(materialLawParamsFunc_(elemIdx, facedir));
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

    bool hasDirectionalImbnum() const {
        if (imbnumXArray_.size() > 0 || imbnumYArray_.size() > 0 || imbnumZArray_.size() > 0) {
            return true;
        }
        return false;
    }

    int imbnumRegionIdx(unsigned elemIdx) const
    { return imbnumRegionArray_[elemIdx]; }

    template <class FluidState>
    bool updateHysteresis(const FluidState& fluidState, unsigned elemIdx)
    {
        OPM_TIMEFUNCTION_LOCAL();
        if (!enableHysteresis())
            return false;
        #if !HAVE_CUDA
        bool changed = MaterialLaw::updateHysteresis(materialLawParams(elemIdx), fluidState);
        if (hasDirectionalRelperms() || hasDirectionalImbnum()) {
            using Dir = FaceDir::DirEnum;
            constexpr int ndim = 3;
            const Dir facedirs[] = {Dir::XPlus, Dir::YPlus, Dir::ZPlus};
            for (int i = 0; i<ndim; i++) {
                bool ischanged =  MaterialLaw::updateHysteresis(materialLawParams(elemIdx, facedirs[i]), fluidState);
                changed = changed || ischanged;
            }
        }
        return changed;
        #endif // !HAVE_CUDA
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

    #if !HAVE_CUDA
    EclEpsScalingPoints<Scalar>& oilWaterScaledEpsPointsDrainage(unsigned elemIdx);

    const EclEpsScalingPointsInfo<Scalar>& oilWaterScaledEpsInfoDrainage(size_t elemIdx) const
    { return oilWaterScaledEpsInfoDrainage_[elemIdx]; }
    #endif // !HAVE_CUDA

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        // This is for restart serialization.
        // Only dynamic state in the parameters need to be stored.
        // For that reason we do not serialize the vector
        // as that would recreate the objects inside.
        for (auto& mat : materialLawParams_) {
            serializer(mat);
        }
    }

private:
    const MaterialLawParams& materialLawParamsFunc_(unsigned elemIdx, FaceDir::DirEnum facedir) const;

    void readGlobalEpsOptions_(const EclipseState& eclState);

    void readGlobalHysteresisOptions_(const EclipseState& state);

    void readGlobalThreePhaseOptions_(const Runspec& runspec);

    bool enableEndPointScaling_;
    SharedPtr<EclHysteresisConfig> hysteresisConfig_;
    Storage<SharedPtr<WagHysteresisConfig::WagHysteresisConfigRecord>> wagHystersisConfig_;

    #if !HAVE_CUDA
    SharedPtr<EclEpsConfig> oilWaterEclEpsConfig_;
    Storage<EclEpsScalingPointsInfo<Scalar>> unscaledEpsInfo_;
    OilWaterScalingInfoVector oilWaterScaledEpsInfoDrainage_;

    SharedPtr<EclEpsConfig> gasWaterEclEpsConfig_;

    GasOilScalingPointsVector gasOilUnscaledPointsVector_;
    OilWaterScalingPointsVector oilWaterUnscaledPointsVector_;
    GasWaterScalingPointsVector gasWaterUnscaledPointsVector_;

    GasOilEffectiveParamVector gasOilEffectiveParamVector_;
    OilWaterEffectiveParamVector oilWaterEffectiveParamVector_;
    GasWaterEffectiveParamVector gasWaterEffectiveParamVector_;
    #endif // !HAVE_CUDA
    EclMultiplexerApproach threePhaseApproach_ = EclMultiplexerApproach::Default;
    // this attribute only makes sense for twophase simulations!
    enum EclTwoPhaseApproach twoPhaseApproach_ = EclTwoPhaseApproach::GasOil;

    Storage<MaterialLawParams> materialLawParams_;
    DirectionalMaterialLawParamsPtr dirMaterialLawParams_;

    Storage<int> satnumRegionArray_;
    Storage<int> krnumXArray_;
    Storage<int> krnumYArray_;
    Storage<int> krnumZArray_;
    Storage<int> imbnumXArray_;
    Storage<int> imbnumYArray_;
    Storage<int> imbnumZArray_;
    Storage<int> imbnumRegionArray_;
    Storage<Scalar> stoneEtas_;

    bool enablePpcwmax_;
    Storage<Scalar> maxAllowPc_;
    Storage<bool> modifySwl_;

    bool hasGas;
    bool hasOil;
    bool hasWater;

    SharedPtr<EclEpsConfig> gasOilConfig_;
    SharedPtr<EclEpsConfig> oilWaterConfig_;
    SharedPtr<EclEpsConfig> gasWaterConfig_;
};
} // namespace Opm

#endif
