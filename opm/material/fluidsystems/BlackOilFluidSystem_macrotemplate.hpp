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
 * \copydoc Opm::BlackOilFluidSystem
 */

#include "BlackOilDefaultIndexTraits.hpp"
#include "blackoilpvt/OilPvtMultiplexer.hpp"
#include "blackoilpvt/GasPvtMultiplexer.hpp"
#include "blackoilpvt/WaterPvtMultiplexer.hpp"

#include <opm/common/TimingMacros.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/Constants.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/HasMemberGeneratorMacros.hpp>
#include <opm/material/fluidsystems/NullParameterCache.hpp>
#include <opm/material/fluidsystems/BlackOilFunctions.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif




/*!
 * \brief A fluid system which uses the black-oil model assumptions to calculate
 *        termodynamically meaningful quantities.
 *
 * \tparam Scalar The type used for scalar floating point values
 */
template <class Scalar, class IndexTraits = BlackOilDefaultIndexTraits, 
    template<typename> typename Storage = VectorWithDefaultAllocator, 
    template<typename> typename SmartPointer = std::shared_ptr>
class FLUIDSYSTEM_CLASSNAME : public BaseFluidSystem<Scalar, FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, Storage, SmartPointer> >
{
    using ThisType = FLUIDSYSTEM_CLASSNAME;

public:
    // The logic here is the following: regular CPU version of this class uses vector
    // If we are in a GPU version of the class, then we keep using smart pointers
    // if it is a buffer version, as it will allocate memory dynamically, and not be
    // used in a kernel, otherwise we must use a templated pointertype that is GPU compatible
    using GasPvt = std::conditional_t<
        std::is_same_v<Storage<double>, std::vector<double>>,
        GasPvtMultiplexer<Scalar, true>,
        std::conditional_t<
            std::is_same_v<SmartPointer<double>, std::shared_ptr<double>>,
            GasPvtMultiplexer<Scalar, true, Storage<double>, Storage<Scalar>, std::unique_ptr>,
            GasPvtMultiplexer<Scalar, true, Storage<double>, Storage<Scalar>, SmartPointer>
        >
    >;
    using OilPvt = OilPvtMultiplexer<Scalar>;
    using WaterPvt = std::conditional_t<
        std::is_same_v<Storage<double>, std::vector<double>>,
        WaterPvtMultiplexer<Scalar, true, true>,
        std::conditional_t<
            std::is_same_v<SmartPointer<double>, std::shared_ptr<double>>,
            WaterPvtMultiplexer<Scalar, true, true, Storage<double>, Storage<Scalar>, std::unique_ptr>,
            WaterPvtMultiplexer<Scalar, true, true, Storage<double>, Storage<Scalar>, SmartPointer>
        >
    >;

    #ifdef COMPILING_STATIC_FLUID_SYSTEM
    //! \copydoc BaseFluidSystem::ParameterCache
    template <class EvaluationT>
    struct ParameterCache : public NullParameterCache<EvaluationT>
    {
        using Evaluation = EvaluationT;

    public:
        explicit ParameterCache(Scalar maxOilSat = 1.0, unsigned regionIdx = 0)
            : maxOilSat_(maxOilSat)
            , regionIdx_(regionIdx)
        {
        }

        /*!
         * \brief Copy the data which is not dependent on the type of the Scalars from
         *        another parameter cache.
         *
         * For the black-oil parameter cache this means that the region index must be
         * copied.
         */
        template <class OtherCache>
        void assignPersistentData(const OtherCache& other)
        {
            regionIdx_ = other.regionIndex();
            maxOilSat_ = other.maxOilSat();
        }

        /*!
         * \brief Return the index of the region which should be used to determine the
         *        thermodynamic properties
         *
         * This is only required because "oil" and "gas" are pseudo-components, i.e. for
         * more comprehensive equations of state there would only be one "region".
         */
        unsigned regionIndex() const
        { return regionIdx_; }

        /*!
         * \brief Set the index of the region which should be used to determine the
         *        thermodynamic properties
         *
         * This is only required because "oil" and "gas" are pseudo-components, i.e. for
         * more comprehensive equations of state there would only be one "region".
         */
        void setRegionIndex(unsigned val)
        { regionIdx_ = val; }

        const Evaluation& maxOilSat() const
        { return maxOilSat_; }

        void setMaxOilSat(const Evaluation& val)
        { maxOilSat_ = val; }

    private:
        Evaluation maxOilSat_;
        unsigned regionIdx_;
    };

    #else

    // We want to use the exact same ParameterCache class for both the static and nonstatic versions
    template<class EvaluationT>
    using ParameterCache =
        typename FLUIDSYSTEM_CLASSNAME_STATIC<Scalar,
             IndexTraits,
             Storage,
             SmartPointer>::template ParameterCache<EvaluationT>;
    #endif

    #ifndef COMPILING_STATIC_FLUID_SYSTEM
    /**
     * \brief Default copy constructor.
     * 
     * This can be used to copy from one storage type (say std::vector) to another (say GPUBuffer).
     */
    template<template<typename> typename StorageT, template<typename> typename SmartPointerT>
    explicit FLUIDSYSTEM_CLASSNAME(const FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, StorageT, SmartPointerT>& other)
        : surfacePressure(other.surfacePressure)
        , surfaceTemperature(other.surfaceTemperature)
        , numActivePhases_(other.numActivePhases_)
        , phaseIsActive_(other.phaseIsActive_)
        , reservoirTemperature_(other.reservoirTemperature_)
        , gasPvt_(SmartPointerT<typename decltype(gasPvt_)::element_type>(other.gasPvt_))
        , oilPvt_(SmartPointerT<typename decltype(oilPvt_)::element_type>(other.oilPvt_))
        , waterPvt_(SmartPointerT<typename decltype(waterPvt_)::element_type>(other.waterPvt_))
        , enableDissolvedGas_(other.enableDissolvedGas_)
        , enableDissolvedGasInWater_(other.enableDissolvedGasInWater_)
        , enableVaporizedOil_(other.enableVaporizedOil_)
        , enableVaporizedWater_(other.enableVaporizedWater_)
        , enableDiffusion_(other.enableDiffusion_)
        , referenceDensity_(StorageT<typename decltype(referenceDensity_)::value_type>(other.referenceDensity_))
        , molarMass_(StorageT<typename decltype(molarMass_)::value_type>(other.molarMass_))
        , diffusionCoefficients_(StorageT<typename decltype(diffusionCoefficients_)::value_type>(other.diffusionCoefficients_))
        , activeToCanonicalPhaseIdx_(other.activeToCanonicalPhaseIdx_)
        , canonicalToActivePhaseIdx_(other.canonicalToActivePhaseIdx_)
        , isInitialized_(other.isInitialized_)
        , useSaturatedTables_(other.useSaturatedTables_)
        , enthalpy_eq_energy_(other.enthalpy_eq_energy_)
    {
    }

    FLUIDSYSTEM_CLASSNAME(Scalar _surfacePressure_,
                          Scalar _surfaceTemperature_,
                          unsigned _numActivePhases_,
                          std::array<bool, 3> _phaseIsActive_,
                          Scalar _reservoirTemperature_,
                          SmartPointer<GasPvt> _gasPvt_,
                          SmartPointer<OilPvt> _oilPvt_,
                          SmartPointer<WaterPvt> _waterPvt_,
                          bool _enableDissolvedGas_,
                          bool _enableDissolvedGasInWater_,
                          bool _enableVaporizedOil_,
                          bool _enableVaporizedWater_,
                          bool _enableDiffusion_,
                          Storage<std::array<Scalar, 3>> _referenceDensity_,
                          Storage<std::array<Scalar, 3>> _molarMass_,
                          Storage<std::array<Scalar, 3 * 3>> _diffusionCoefficients_,
                          std::array<short, 3> _activeToCanonicalPhaseIdx_,
                          std::array<short, 3> _canonicalToActivePhaseIdx_,
                          bool _isInitialized_,
                          bool _useSaturatedTables_,
                          bool _enthalpy_eq_energy_)
        : surfacePressure(_surfacePressure_)
        , surfaceTemperature(_surfaceTemperature_)
        , numActivePhases_(_numActivePhases_)
        , phaseIsActive_(_phaseIsActive_)
        , reservoirTemperature_(_reservoirTemperature_)
        , gasPvt_(_gasPvt_)
        , oilPvt_(_oilPvt_)
        , waterPvt_(_waterPvt_)
        , enableDissolvedGas_(_enableDissolvedGas_)
        , enableDissolvedGasInWater_(_enableDissolvedGasInWater_)
        , enableVaporizedOil_(_enableVaporizedOil_)
        , enableVaporizedWater_(_enableVaporizedWater_)
        , enableDiffusion_(_enableDiffusion_)
        , referenceDensity_(_referenceDensity_)
        , molarMass_(_molarMass_)
        , diffusionCoefficients_(_diffusionCoefficients_)
        , activeToCanonicalPhaseIdx_(_activeToCanonicalPhaseIdx_)
        , canonicalToActivePhaseIdx_(_canonicalToActivePhaseIdx_)
        , isInitialized_(_isInitialized_)
        , useSaturatedTables_(_useSaturatedTables_)
        , enthalpy_eq_energy_(_enthalpy_eq_energy_)
    {
    }

    template <template <class> class NewContainerType, class ScalarT, class IndexTraitsT, template <class> class OldContainerType>
    friend FLUIDSYSTEM_CLASSNAME<ScalarT, IndexTraitsT, NewContainerType, SmartPointer>
    gpuistl::copy_to_gpu(const FLUIDSYSTEM_CLASSNAME<ScalarT, IndexTraitsT, OldContainerType, SmartPointer>& oldFluidSystem);

    template <template <class> class ViewType, template <class> class PtrType, class ScalarT, class IndexTraitsT, template <class> class OldContainerType>
    friend FLUIDSYSTEM_CLASSNAME<ScalarT, IndexTraitsT, ViewType, PtrType>
    gpuistl::make_view(FLUIDSYSTEM_CLASSNAME<ScalarT, IndexTraitsT, OldContainerType>& oldFluidSystem);

    #endif

    #ifdef COMPILING_STATIC_FLUID_SYSTEM
    /**
     * \brief Get the non-static instance of the fluid system.
     * 
     * This function is for now primarily used when accessing the fluid system from the GPU.
     * 
     * \note This function works as a singleton. 
     */
    template<template<typename> typename StorageT = VectorWithDefaultAllocator, template<typename> typename SmartPointerT = std::shared_ptr>
    static FLUIDSYSTEM_CLASSNAME_NONSTATIC<Scalar, IndexTraits, StorageT, SmartPointerT>& getNonStaticInstance()
    {
        static FLUIDSYSTEM_CLASSNAME_NONSTATIC<Scalar, IndexTraits, StorageT, SmartPointerT> instance(FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, Storage, SmartPointer>{});
        return instance;
        
    }
    #endif

    /****************************************
     * Initialization
     ****************************************/
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the fluid system using an ECL deck object
     */
    STATIC_OR_NOTHING void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Begin the initialization of the black oil fluid system.
     *
     * After calling this method the reference densities, all dissolution and formation
     * volume factors, the oil bubble pressure, all viscosities and the water
     * compressibility must be set. Before the fluid system can be used, initEnd() must
     * be called to finalize the initialization.
     */
    STATIC_OR_NOTHING void initBegin(std::size_t numPvtRegions);

    /*!
     * \brief Specify whether the fluid system should consider that the gas component can
     *        dissolve in the oil phase
     *
     * By default, dissolved gas is considered.
     */
    STATIC_OR_DEVICE void setEnableDissolvedGas(bool yesno)
    { enableDissolvedGas_ = yesno; }

    /*!
     * \brief Specify whether the fluid system should consider that the oil component can
     *        dissolve in the gas phase
     *
     * By default, vaporized oil is not considered.
     */
    STATIC_OR_DEVICE void setEnableVaporizedOil(bool yesno)
    { enableVaporizedOil_ = yesno; }

     /*!
     * \brief Specify whether the fluid system should consider that the water component can
     *        dissolve in the gas phase
     *
     * By default, vaporized water is not considered.
     */
    STATIC_OR_DEVICE void setEnableVaporizedWater(bool yesno)
    { enableVaporizedWater_ = yesno; }

     /*!
     * \brief Specify whether the fluid system should consider that the gas component can
     *        dissolve in the water phase
     *
     * By default, dissovled gas in water is not considered.
     */
    STATIC_OR_DEVICE void setEnableDissolvedGasInWater(bool yesno)
    { enableDissolvedGasInWater_ = yesno; }
    /*!
     * \brief Specify whether the fluid system should consider diffusion
     *
     * By default, diffusion is not considered.
     */
    STATIC_OR_DEVICE void setEnableDiffusion(bool yesno)
    { enableDiffusion_ = yesno; }

    /*!
     * \brief Specify whether the saturated tables should be used
     *
     * By default, saturated tables are used
     */
    STATIC_OR_DEVICE void setUseSaturatedTables(bool yesno)
    { useSaturatedTables_ = yesno; }

    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the gas phase.
     */
    STATIC_OR_DEVICE void setGasPvt(SmartPointer<GasPvt> pvtObj)
    { gasPvt_ = pvtObj; }

    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the oil phase.
     */
    STATIC_OR_DEVICE void setOilPvt(SmartPointer<OilPvt> pvtObj)
    { oilPvt_ = pvtObj; }

    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the water phase.
     */
    STATIC_OR_DEVICE void setWaterPvt(SmartPointer<WaterPvt> pvtObj)
    { waterPvt_ = pvtObj; }

    STATIC_OR_DEVICE void setVapPars(const Scalar par1, const Scalar par2)
    {
        if (gasPvt_) {
            gasPvt_->setVapPars(par1, par2);
        }
        if (oilPvt_) {
            oilPvt_->setVapPars(par1, par2);
        }
        if (waterPvt_) {
            waterPvt_->setVapPars(par1, par2);
        }
    }

    /*!
     * \brief Initialize the values of the reference densities
     *
     * \param rhoOil The reference density of (gas saturated) oil phase.
     * \param rhoWater The reference density of the water phase.
     * \param rhoGas The reference density of the gas phase.
     */
    STATIC_OR_DEVICE void setReferenceDensities(Scalar rhoOil,
                                      Scalar rhoWater,
                                      Scalar rhoGas,
                                      unsigned regionIdx);

    /*!
     * \brief Finish initializing the black oil fluid system.
     */
    STATIC_OR_DEVICE void initEnd();

    STATIC_OR_DEVICE bool isInitialized()
    { return isInitialized_; }

    /****************************************
     * Generic phase properties
     ****************************************/

    //! \copydoc BaseFluidSystem::numPhases
    static constexpr unsigned numPhases = 3;

    //! Index of the water phase
    static constexpr unsigned waterPhaseIdx = IndexTraits::waterPhaseIdx;
    //! Index of the oil phase
    static constexpr unsigned oilPhaseIdx = IndexTraits::oilPhaseIdx;
    //! Index of the gas phase
    static constexpr unsigned gasPhaseIdx = IndexTraits::gasPhaseIdx;

    //! The pressure at the surface
    STATIC_OR_NOTHING Scalar surfacePressure;

    //! The temperature at the surface
    STATIC_OR_NOTHING Scalar surfaceTemperature;

    //! \copydoc BaseFluidSystem::phaseName
    STATIC_OR_NOTHING std::string_view phaseName(unsigned phaseIdx);



    //! \copydoc BaseFluidSystem::isLiquid
    STATIC_OR_DEVICE bool isLiquid(unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);
        return phaseIdx != gasPhaseIdx;
    }

    /****************************************
     * Generic component related properties
     ****************************************/

    //! \copydoc BaseFluidSystem::numComponents
    static constexpr unsigned numComponents = 3;

    //! Index of the oil component
    static constexpr int oilCompIdx = IndexTraits::oilCompIdx;
    //! Index of the water component
    static constexpr int waterCompIdx = IndexTraits::waterCompIdx;
    //! Index of the gas component
    static constexpr int gasCompIdx = IndexTraits::gasCompIdx;

protected:
    STATIC_OR_NOTHING unsigned char numActivePhases_;
    STATIC_OR_NOTHING std::array<bool,numPhases> phaseIsActive_;

public:
    //! \brief Returns the number of active fluid phases (i.e., usually three)
    STATIC_OR_DEVICE unsigned numActivePhases()
    { return numActivePhases_; }

    //! \brief Returns whether a fluid phase is active
    STATIC_OR_DEVICE bool phaseIsActive(unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);
        return phaseIsActive_[phaseIdx];
    }

    //! \brief returns the index of "primary" component of a phase (solvent)
    STATIC_OR_DEVICE unsigned solventComponentIndex(unsigned phaseIdx);

    //! \brief returns the index of "secondary" component of a phase (solute)
    STATIC_OR_DEVICE unsigned soluteComponentIndex(unsigned phaseIdx);

    //! \copydoc BaseFluidSystem::componentName
    STATIC_OR_NOTHING std::string_view componentName(unsigned compIdx);

    //! \copydoc BaseFluidSystem::molarMass
    STATIC_OR_DEVICE Scalar molarMass(unsigned compIdx, unsigned regionIdx = 0)
    { return molarMass_[regionIdx][compIdx]; }

    //! \copydoc BaseFluidSystem::isIdealMixture
    STATIC_OR_DEVICE bool isIdealMixture(unsigned /*phaseIdx*/)
    {
        // fugacity coefficients are only pressure dependent -> we
        // have an ideal mixture
        return true;
    }

    //! \copydoc BaseFluidSystem::isCompressible
    STATIC_OR_DEVICE bool isCompressible(unsigned /*phaseIdx*/)
    { return true; /* all phases are compressible */ }

    //! \copydoc BaseFluidSystem::isIdealGas
    STATIC_OR_DEVICE bool isIdealGas(unsigned /*phaseIdx*/)
    { return false; }


    /****************************************
     * Black-oil specific properties
     ****************************************/
    /*!
     * \brief Returns the number of PVT regions which are considered.
     *
     * By default, this is 1.
     */
    STATIC_OR_DEVICE std::size_t numRegions()
    { return molarMass_.size(); }

    /*!
     * \brief Returns whether the fluid system should consider that the gas component can
     *        dissolve in the oil phase
     *
     * By default, dissolved gas is considered.
     */
    STATIC_OR_DEVICE bool enableDissolvedGas()
    { return enableDissolvedGas_; }


    /*!
     * \brief Returns whether the fluid system should consider that the gas component can
     *        dissolve in the water phase
     *
     * By default, dissolved gas is considered.
     */
    STATIC_OR_DEVICE bool enableDissolvedGasInWater()
    { return enableDissolvedGasInWater_; }

    /*!
     * \brief Returns whether the fluid system should consider that the oil component can
     *        dissolve in the gas phase
     *
     * By default, vaporized oil is not considered.
     */
    STATIC_OR_DEVICE bool enableVaporizedOil()
    { return enableVaporizedOil_; }

    /*!
     * \brief Returns whether the fluid system should consider that the water component can
     *        dissolve in the gas phase
     *
     * By default, vaporized water is not considered.
     */
    STATIC_OR_DEVICE bool enableVaporizedWater()
    { return enableVaporizedWater_; }

    /*!
     * \brief Returns whether the fluid system should consider diffusion
     *
     * By default, diffusion is not considered.
     */
    STATIC_OR_DEVICE bool enableDiffusion()
    { return enableDiffusion_; }

    /*!
     * \brief Returns whether the saturated tables should be used
     *
     * By default, saturated tables are used. If false the unsaturated tables are extrapolated
     */
    STATIC_OR_DEVICE bool useSaturatedTables()
    { return useSaturatedTables_; }

    /*!
     * \brief Returns the density of a fluid phase at surface pressure [kg/m^3]
     *
     * \copydoc Doxygen::phaseIdxParam
     */
    STATIC_OR_DEVICE Scalar referenceDensity(unsigned phaseIdx, unsigned regionIdx)
    { return referenceDensity_[regionIdx][phaseIdx]; }

    /****************************************
     * thermodynamic quantities (generic version)
     ****************************************/
    //! \copydoc BaseFluidSystem::density
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval density(const FluidState& fluidState,
                           const ParameterCache<ParamCacheEval>& paramCache,
                           unsigned phaseIdx)
    { return density<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    //! \copydoc BaseFluidSystem::fugacityCoefficient
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval fugacityCoefficient(const FluidState& fluidState,
                                       const ParameterCache<ParamCacheEval>& paramCache,
                                       unsigned phaseIdx,
                                       unsigned compIdx)
    {
        return fugacityCoefficient<FluidState, LhsEval>(fluidState,
                                                        phaseIdx,
                                                        compIdx,
                                                        paramCache.regionIndex());
    }

    //! \copydoc BaseFluidSystem::viscosity
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval viscosity(const FluidState& fluidState,
                             const ParameterCache<ParamCacheEval>& paramCache,
                             unsigned phaseIdx)
    { return viscosity<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    //! \copydoc BaseFluidSystem::enthalpy
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval enthalpy(const FluidState& fluidState,
                            const ParameterCache<ParamCacheEval>& paramCache,
                            unsigned phaseIdx)
    { return enthalpy<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval internalEnergy(const FluidState& fluidState,
                                  const ParameterCache<ParamCacheEval>& paramCache,
                                  unsigned phaseIdx)
    { return internalEnergy<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    /****************************************
     * thermodynamic quantities (black-oil specific version: Note that the PVT region
     * index is explicitly passed instead of a parameter cache object)
     ****************************************/
    //! \copydoc BaseFluidSystem::density
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval density(const FluidState& fluidState,
                           unsigned phaseIdx,
                           unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const LhsEval& saltConcentration = BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);

        switch (phaseIdx) {
        case oilPhaseIdx: {
            if (enableDissolvedGas()) {
                // miscible oil
                const LhsEval& Rs = BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);

                return
                    bo*referenceDensity(oilPhaseIdx, regionIdx)
                    + Rs*bo*referenceDensity(gasPhaseIdx, regionIdx);
            }

            // immiscible oil
            const LhsEval Rs(0.0);
            const auto& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);

            return referenceDensity(phaseIdx, regionIdx)*bo;
        }

        case gasPhaseIdx: {
             if (enableVaporizedOil() && enableVaporizedWater()) {
                // gas containing vaporized oil and vaporized water
                const LhsEval& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rv*bg*referenceDensity(oilPhaseIdx, regionIdx)
                    + Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx);
            }
            if (enableVaporizedOil()) {
                // miscible gas
                const LhsEval Rvw(0.0);
                const LhsEval& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rv*bg*referenceDensity(oilPhaseIdx, regionIdx);
            }
            if (enableVaporizedWater()) {
                // gas containing vaporized water
                const LhsEval Rv(0.0);
                const LhsEval& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx);
            }

            // immiscible gas
            const LhsEval Rv(0.0);
            const LhsEval Rvw(0.0);
            const auto& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
            return bg*referenceDensity(phaseIdx, regionIdx);
        }

        case waterPhaseIdx:
            if (enableDissolvedGasInWater()) {
                 // gas miscible in water
                const LhsEval& Rsw =BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bw = waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
                return
                    bw*referenceDensity(waterPhaseIdx, regionIdx)
                    + Rsw*bw*referenceDensity(gasPhaseIdx, regionIdx);
            }
            const LhsEval Rsw(0.0);
            return
                referenceDensity(waterPhaseIdx, regionIdx)
                * waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
        }

        throw std::logic_error("Unhandled phase index " + std::to_string(phaseIdx));
    }

    /*!
     * \brief Compute the density of a saturated fluid phase.
     *
     * This means the density of the given fluid phase if the dissolved component (gas
     * for the oil phase and oil for the gas phase) is at the thermodynamically possible
     * maximum. For the water phase, there's no difference to the density() method
     * for the standard blackoil model. If enableDissolvedGasInWater is enabled
     * the water density takes into account the amount of dissolved gas
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturatedDensity(const FluidState& fluidState,
                                    unsigned phaseIdx,
                                    unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = fluidState.pressure(phaseIdx);
        const auto& T = fluidState.temperature(phaseIdx);

        switch (phaseIdx) {
        case oilPhaseIdx: {
            if (enableDissolvedGas()) {
                // miscible oil
                const LhsEval& Rs = saturatedDissolutionFactor<FluidState, LhsEval>(fluidState, oilPhaseIdx, regionIdx);
                const LhsEval& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);

                return
                    bo*referenceDensity(oilPhaseIdx, regionIdx)
                    + Rs*bo*referenceDensity(gasPhaseIdx, regionIdx);
            }

            // immiscible oil
            const LhsEval Rs(0.0);
            const LhsEval& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);
            return referenceDensity(phaseIdx, regionIdx)*bo;
        }

        case gasPhaseIdx: {
            if (enableVaporizedOil() && enableVaporizedWater()) {
                // gas containing vaporized oil and vaporized water
                const LhsEval& Rv = saturatedDissolutionFactor<FluidState, LhsEval>(fluidState, gasPhaseIdx, regionIdx);
                const LhsEval& Rvw = saturatedVaporizationFactor<FluidState, LhsEval>(fluidState, gasPhaseIdx, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rv*bg*referenceDensity(oilPhaseIdx, regionIdx)
                    + Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx) ;
            }

            if (enableVaporizedOil()) {
                // miscible gas
                const LhsEval Rvw(0.0);
                const LhsEval& Rv = saturatedDissolutionFactor<FluidState, LhsEval>(fluidState, gasPhaseIdx, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rv*bg*referenceDensity(oilPhaseIdx, regionIdx);
            }

            if (enableVaporizedWater()) {
                // gas containing vaporized water
                const LhsEval Rv(0.0);
                const LhsEval& Rvw = saturatedVaporizationFactor<FluidState, LhsEval>(fluidState, gasPhaseIdx, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

                return
                    bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx);
            }

            // immiscible gas
            const LhsEval Rv(0.0);
            const LhsEval Rvw(0.0);
            const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);

            return referenceDensity(phaseIdx, regionIdx)*bg;

        }

        case waterPhaseIdx:
        {
            if (enableDissolvedGasInWater()) {
                 // miscible in water
                const auto& saltConcentration = decay<LhsEval>(fluidState.saltConcentration());
                const LhsEval& Rsw = saturatedDissolutionFactor<FluidState, LhsEval>(fluidState, waterPhaseIdx, regionIdx);
                const LhsEval& bw = waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
                return
                    bw*referenceDensity(waterPhaseIdx, regionIdx)
                    + Rsw*bw*referenceDensity(gasPhaseIdx, regionIdx);
            }
            return
                referenceDensity(waterPhaseIdx, regionIdx)
                *inverseFormationVolumeFactor<FluidState, LhsEval>(fluidState, waterPhaseIdx, regionIdx);
        }
        }

        throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
    }

    /*!
     * \brief Returns the formation volume factor \f$B_\alpha\f$ of an "undersaturated"
     *        fluid phase
     *
     * For the oil (gas) phase, "undersaturated" means that the concentration of the gas
     * (oil) component is not assumed to be at the thermodynamically possible maximum at
     * the given temperature and pressure.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval inverseFormationVolumeFactor(const FluidState& fluidState,
                                                unsigned phaseIdx,
                                                unsigned regionIdx)
    {
        OPM_TIMEBLOCK_LOCAL(inverseFormationVolumeFactor);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: {
            if (enableDissolvedGas()) {
                const auto& Rs = BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(gasPhaseIdx) > 0.0
                    && Rs >= (1.0 - 1e-10)*oilPvt_->saturatedGasDissolutionFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return oilPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
                } else {
                    return oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);
                }
            }

            const LhsEval Rs(0.0);
            return oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);
        }
        case gasPhaseIdx: {
            if (enableVaporizedOil() && enableVaporizedWater()) {
                 const auto& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                 const auto& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                 if (useSaturatedTables() && fluidState.saturation(waterPhaseIdx) > 0.0
                    && Rvw >= (1.0 - 1e-10)*gasPvt_->saturatedWaterVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p))
                    && fluidState.saturation(oilPhaseIdx) > 0.0
                    && Rv >= (1.0 - 1e-10)*gasPvt_->saturatedOilVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                 {
                    return gasPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
                 } else {
                    return gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                 }
            }

            if (enableVaporizedOil()) {
                const auto& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(oilPhaseIdx) > 0.0
                    && Rv >= (1.0 - 1e-10)*gasPvt_->saturatedOilVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return gasPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
                } else {
                    const LhsEval Rvw(0.0);
                    return gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                }
            }

            if (enableVaporizedWater()) {
                const auto& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(waterPhaseIdx) > 0.0
                    && Rvw >= (1.0 - 1e-10)*gasPvt_->saturatedWaterVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return gasPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
                } else {
                    const LhsEval Rv(0.0);
                    return gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                }
            }

            const LhsEval Rv(0.0);
            const LhsEval Rvw(0.0);
            return gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
        }
        case waterPhaseIdx:
        {
            const auto& saltConcentration = BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
            if (enableDissolvedGasInWater()) {
                const auto& Rsw = BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(gasPhaseIdx) > 0.0
                    && Rsw >= (1.0 - 1e-10)*waterPvt_->saturatedGasDissolutionFactor(regionIdx, scalarValue(T), scalarValue(p), scalarValue(saltConcentration)))
                {
                    return waterPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p, saltConcentration);
                } else {
                    return waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
                }
            }
            const LhsEval Rsw(0.0);
            return waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
        }
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    /*!
     * \brief Returns the formation volume factor \f$B_\alpha\f$ of a "saturated" fluid
     *        phase
     *
     * For the oil phase, this means that it is gas saturated, the gas phase is oil
     * saturated and for the water phase, there is no difference to formationVolumeFactor()
     * for the standard blackoil model. If enableDissolvedGasInWater is enabled
     * the water density takes into account the amount of dissolved gas
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturatedInverseFormationVolumeFactor(const FluidState& fluidState,
                                                         unsigned phaseIdx,
                                                         unsigned regionIdx)
    {
        OPM_TIMEBLOCK_LOCAL(saturatedInverseFormationVolumeFactor);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& saltConcentration = BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);

        switch (phaseIdx) {
        case oilPhaseIdx: return oilPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
        case gasPhaseIdx: return gasPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p);
        case waterPhaseIdx: return waterPvt_->saturatedInverseFormationVolumeFactor(regionIdx, T, p, saltConcentration);
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    //! \copydoc BaseFluidSystem::fugacityCoefficient
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval fugacityCoefficient(const FluidState& fluidState,
                                       unsigned phaseIdx,
                                       unsigned compIdx,
                                       unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(compIdx <= numComponents);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        // for the fugacity coefficient of the oil component in the oil phase, we use
        // some pseudo-realistic value for the vapor pressure to ease physical
        // interpretation of the results
        const LhsEval phi_oO = 20e3/p;

        // for the gas component in the gas phase, assume it to be an ideal gas
        constexpr const Scalar phi_gG = 1.0;

        // for the fugacity coefficient of the water component in the water phase, we use
        // the same approach as for the oil component in the oil phase
        const LhsEval phi_wW = 30e3/p;

        switch (phaseIdx) {
        case gasPhaseIdx: // fugacity coefficients for all components in the gas phase
            switch (compIdx) {
            case gasCompIdx:
                return phi_gG;

            // for the oil component, we calculate the Rv value for saturated gas and Rs
            // for saturated oil, and compute the fugacity coefficients at the
            // equilibrium. for this, we assume that in equilibrium the fugacities of the
            // oil component is the same in both phases.
            case oilCompIdx: {
                if (!enableVaporizedOil())
                    // if there's no vaporized oil, the gas phase is assumed to be
                    // immiscible with the oil component
                    return phi_gG*1e6;

                const auto& R_vSat = gasPvt_->saturatedOilVaporizationFactor(regionIdx, T, p);
                const auto& X_gOSat = convertRvToXgO(R_vSat, regionIdx);
                const auto& x_gOSat = convertXgOToxgO(X_gOSat, regionIdx);

                const auto& R_sSat = oilPvt_->saturatedGasDissolutionFactor(regionIdx, T, p);
                const auto& X_oGSat = convertRsToXoG(R_sSat, regionIdx);
                const auto& x_oGSat = convertXoGToxoG(X_oGSat, regionIdx);
                const auto& x_oOSat = 1.0 - x_oGSat;

                const auto& p_o = decay<LhsEval>(fluidState.pressure(oilPhaseIdx));
                const auto& p_g = decay<LhsEval>(fluidState.pressure(gasPhaseIdx));

                return phi_oO*p_o*x_oOSat / (p_g*x_gOSat);
            }

            case waterCompIdx:
                // the water component is assumed to be never miscible with the gas phase
                return phi_gG*1e6;

            default:
                throw std::logic_error("Invalid component index "+std::to_string(compIdx));
            }

        case oilPhaseIdx: // fugacity coefficients for all components in the oil phase
            switch (compIdx) {
            case oilCompIdx:
                return phi_oO;

            // for the oil and water components, we proceed analogous to the gas and
            // water components in the gas phase
            case gasCompIdx: {
                if (!enableDissolvedGas())
                    // if there's no dissolved gas, the oil phase is assumed to be
                    // immiscible with the gas component
                    return phi_oO*1e6;

                const auto& R_vSat = gasPvt_->saturatedOilVaporizationFactor(regionIdx, T, p);
                const auto& X_gOSat = convertRvToXgO(R_vSat, regionIdx);
                const auto& x_gOSat = convertXgOToxgO(X_gOSat, regionIdx);
                const auto& x_gGSat = 1.0 - x_gOSat;

                const auto& R_sSat = oilPvt_->saturatedGasDissolutionFactor(regionIdx, T, p);
                const auto& X_oGSat = convertRsToXoG(R_sSat, regionIdx);
                const auto& x_oGSat = convertXoGToxoG(X_oGSat, regionIdx);

                const auto& p_o = decay<LhsEval>(fluidState.pressure(oilPhaseIdx));
                const auto& p_g = decay<LhsEval>(fluidState.pressure(gasPhaseIdx));

                return phi_gG*p_g*x_gGSat / (p_o*x_oGSat);
            }

            case waterCompIdx:
                return phi_oO*1e6;

            default:
                throw std::logic_error("Invalid component index "+std::to_string(compIdx));
            }

        case waterPhaseIdx: // fugacity coefficients for all components in the water phase
            // the water phase fugacity coefficients are pretty simple: because the water
            // phase is assumed to consist entirely from the water component, we just
            // need to make sure that the fugacity coefficients for the other components
            // are a few orders of magnitude larger than the one of the water
            // component. (i.e., the affinity of the gas and oil components to the water
            // phase is lower by a few orders of magnitude)
            switch (compIdx) {
            case waterCompIdx: return phi_wW;
            case oilCompIdx: return 1.1e6*phi_wW;
            case gasCompIdx: return 1e6*phi_wW;
            default:
                throw std::logic_error("Invalid component index "+std::to_string(compIdx));
            }

        default:
            throw std::logic_error("Invalid phase index "+std::to_string(phaseIdx));
        }

        throw std::logic_error("Unhandled phase or component index");
    }

    //! \copydoc BaseFluidSystem::viscosity
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval viscosity(const FluidState& fluidState,
                             unsigned phaseIdx,
                             unsigned regionIdx)
    {
        OPM_TIMEBLOCK_LOCAL(viscosity);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: {
            if (enableDissolvedGas()) {
                const auto& Rs = BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(gasPhaseIdx) > 0.0
                    && Rs >= (1.0 - 1e-10)*oilPvt_->saturatedGasDissolutionFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return oilPvt_->saturatedViscosity(regionIdx, T, p);
                } else {
                    return oilPvt_->viscosity(regionIdx, T, p, Rs);
                }
            }

            const LhsEval Rs(0.0);
            return oilPvt_->viscosity(regionIdx, T, p, Rs);
        }

        case gasPhaseIdx: {
             if (enableVaporizedOil() && enableVaporizedWater()) {
                 const auto& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                 const auto& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                 if (useSaturatedTables() && fluidState.saturation(waterPhaseIdx) > 0.0
                    && Rvw >= (1.0 - 1e-10)*gasPvt_->saturatedWaterVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p))
                    && fluidState.saturation(oilPhaseIdx) > 0.0
                    && Rv >= (1.0 - 1e-10)*gasPvt_->saturatedOilVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                 {
                     return gasPvt_->saturatedViscosity(regionIdx, T, p);
                 } else {
                     return gasPvt_->viscosity(regionIdx, T, p, Rv, Rvw);
                 }
            }
            if (enableVaporizedOil()) {
                const auto& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(oilPhaseIdx) > 0.0
                    && Rv >= (1.0 - 1e-10)*gasPvt_->saturatedOilVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return gasPvt_->saturatedViscosity(regionIdx, T, p);
                } else {
                    const LhsEval Rvw(0.0);
                    return gasPvt_->viscosity(regionIdx, T, p, Rv, Rvw);
                }
            }
            if (enableVaporizedWater()) {
                const auto& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(waterPhaseIdx) > 0.0
                    && Rvw >= (1.0 - 1e-10)*gasPvt_->saturatedWaterVaporizationFactor(regionIdx, scalarValue(T), scalarValue(p)))
                {
                    return gasPvt_->saturatedViscosity(regionIdx, T, p);
                } else {
                    const LhsEval Rv(0.0);
                    return gasPvt_->viscosity(regionIdx, T, p, Rv, Rvw);
                }
            }

            const LhsEval Rv(0.0);
            const LhsEval Rvw(0.0);
            return gasPvt_->viscosity(regionIdx, T, p, Rv, Rvw);
        }

        case waterPhaseIdx:
        {
            const LhsEval& saltConcentration = BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
            if (enableDissolvedGasInWater()) {
                const auto& Rsw = BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                if (useSaturatedTables() && fluidState.saturation(gasPhaseIdx) > 0.0
                    && Rsw >= (1.0 - 1e-10)*waterPvt_->saturatedGasDissolutionFactor(regionIdx, scalarValue(T), scalarValue(p), scalarValue(saltConcentration)))
                {
                    return waterPvt_->saturatedViscosity(regionIdx, T, p, saltConcentration);
                } else {
                    return waterPvt_->viscosity(regionIdx, T, p, Rsw, saltConcentration);
                }
            }
            const LhsEval Rsw(0.0);
            return waterPvt_->viscosity(regionIdx, T, p, Rsw, saltConcentration);
        }
        }

        throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
    }

    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval internalEnergy(const FluidState& fluidState,
                                  const unsigned phaseIdx,
                                  const unsigned regionIdx)
    {
        const auto p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx:
            if (!oilPvt_->mixingEnergy()) {
                return oilPvt_->internalEnergy
                    (regionIdx, T, p,
                     BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            }
            break;

        case waterPhaseIdx:
            if (!waterPvt_->mixingEnergy()) {
                return waterPvt_->internalEnergy
                    (regionIdx, T, p,
                     BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                     BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            }
            break;

        case gasPhaseIdx:
            if (!gasPvt_->mixingEnergy()) {
                return gasPvt_->internalEnergy
                    (regionIdx, T, p,
                     BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                     BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            }
            break;

        default:
            throw std::logic_error {
                "Phase index " + std::to_string(phaseIdx) + " does not support internal energy"
            };
        }

        return internalMixingTotalEnergy<FluidState,LhsEval>(fluidState, phaseIdx, regionIdx)
            /  density<FluidState,LhsEval>(fluidState, phaseIdx, regionIdx);
    }


    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval internalMixingTotalEnergy(const FluidState& fluidState,
                                             unsigned phaseIdx,
                                             unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const LhsEval& saltConcentration = BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
        // to avoid putting all thermal into the interface of the multiplexer
        switch (phaseIdx) {
        case oilPhaseIdx: {
            auto oilEnergy = oilPvt_->internalEnergy(regionIdx, T, p,
                                                     BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            assert(oilPvt_->mixingEnergy());
            //mixing energy adsed
            if (enableDissolvedGas()) {
                // miscible oil
                const LhsEval& Rs = BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);
                const auto& gasEnergy =
                    gasPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                            BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                const auto hVapG = gasPvt_->hVap(regionIdx);// pressure correction ? assume equal to energy change
                return
                    oilEnergy*bo*referenceDensity(oilPhaseIdx, regionIdx)
                    + (gasEnergy-hVapG)*Rs*bo*referenceDensity(gasPhaseIdx, regionIdx);
            }

            // immiscible oil
            const LhsEval Rs(0.0);
            const auto& bo = oilPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rs);

            return oilEnergy*referenceDensity(phaseIdx, regionIdx)*bo;
        }

        case gasPhaseIdx: {
            const auto& gasEnergy =
                gasPvt_->internalEnergy(regionIdx, T, p,
                                        BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                        BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            assert(gasPvt_->mixingEnergy());
            if (enableVaporizedOil() && enableVaporizedWater()) {
                const auto& oilEnergy =
                    oilPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                const auto waterEnergy =
                    waterPvt_->internalEnergy(regionIdx, T, p,
                                              BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                              BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                // gas containing vaporized oil and vaporized water
                const LhsEval& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                const auto hVapO = oilPvt_->hVap(regionIdx);
                const auto hVapW = waterPvt_->hVap(regionIdx);
                return
                    gasEnergy*bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + (oilEnergy+hVapO)*Rv*bg*referenceDensity(oilPhaseIdx, regionIdx)
                    + (waterEnergy+hVapW)*Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx);
            }
            if (enableVaporizedOil()) {
                const auto& oilEnergy =
                    oilPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                // miscible gas
                const LhsEval Rvw(0.0);
                const LhsEval& Rv = BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                const auto hVapO = oilPvt_->hVap(regionIdx);
                return
                    gasEnergy*bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + (oilEnergy+hVapO)*Rv*bg*referenceDensity(oilPhaseIdx, regionIdx);
            }
            if (enableVaporizedWater()) {
                // gas containing vaporized water
                const LhsEval Rv(0.0);
                const LhsEval& Rvw = BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx);
                const LhsEval& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
                const auto waterEnergy =
                    waterPvt_->internalEnergy(regionIdx, T, p,
                                              BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                              BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                const auto hVapW = waterPvt_->hVap(regionIdx);
                return
                    gasEnergy*bg*referenceDensity(gasPhaseIdx, regionIdx)
                    + (waterEnergy+hVapW)*Rvw*bg*referenceDensity(waterPhaseIdx, regionIdx);
            }

            // immiscible gas
            const LhsEval Rv(0.0);
            const LhsEval Rvw(0.0);
            const auto& bg = gasPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw);
            return gasEnergy*bg*referenceDensity(phaseIdx, regionIdx);
        }

        case waterPhaseIdx:
            const auto waterEnergy =
                waterPvt_->internalEnergy(regionIdx, T, p,
                                          BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                          BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
            assert(waterPvt_->mixingEnergy());
            if (enableDissolvedGasInWater()) {
                const auto& gasEnergy =
                    gasPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                            BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
                // gas miscible in water
                const LhsEval& Rsw = saturatedDissolutionFactor<FluidState, LhsEval>(fluidState, waterPhaseIdx, regionIdx);
                const LhsEval& bw = waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
                return
                    waterEnergy*bw*referenceDensity(waterPhaseIdx, regionIdx)
                    + gasEnergy*Rsw*bw*referenceDensity(gasPhaseIdx, regionIdx);
            }
            const LhsEval Rsw(0.0);
            return
                waterEnergy*referenceDensity(waterPhaseIdx, regionIdx)
                * waterPvt_->inverseFormationVolumeFactor(regionIdx, T, p, Rsw, saltConcentration);
        }
        throw std::logic_error("Unhandled phase index " + std::to_string(phaseIdx));
    }



    //! \copydoc BaseFluidSystem::enthalpy
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval enthalpy(const FluidState& fluidState,
                            unsigned phaseIdx,
                            unsigned regionIdx)
    {
        // should preferably not be used values should be taken from intensive quantities fluid state.
        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        auto energy = internalEnergy<FluidState, LhsEval>(fluidState, phaseIdx, regionIdx);
        if(!enthalpy_eq_energy_){
            // used for simplified models
            energy += p/density<FluidState, LhsEval>(fluidState, phaseIdx, regionIdx);
        }
        return energy;
    }

    /*!
     * \brief Returns the water vaporization factor \f$R_\alpha\f$ of saturated phase
     *
     * For the gas phase, this means the R_vw factor, for the water and oil phase,
     * it is always 0.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturatedVaporizationFactor(const FluidState& fluidState,
                                              unsigned phaseIdx,
                                              unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& saltConcentration = decay<LhsEval>(fluidState.saltConcentration());

        switch (phaseIdx) {
        case oilPhaseIdx: return 0.0;
        case gasPhaseIdx: return gasPvt_->saturatedWaterVaporizationFactor(regionIdx, T, p, saltConcentration);
        case waterPhaseIdx: return 0.0;
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    /*!
     * \brief Returns the dissolution factor \f$R_\alpha\f$ of a saturated fluid phase
     *
     * For the oil (gas) phase, this means the R_s and R_v factors, for the water phase,
     * it is always 0.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturatedDissolutionFactor(const FluidState& fluidState,
                                              unsigned phaseIdx,
                                              unsigned regionIdx,
                                              const LhsEval& maxOilSaturation)
    {
        OPM_TIMEBLOCK_LOCAL(saturatedDissolutionFactor);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& So = (phaseIdx == waterPhaseIdx) ? 0 : decay<LhsEval>(fluidState.saturation(oilPhaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: return oilPvt_->saturatedGasDissolutionFactor(regionIdx, T, p, So, maxOilSaturation);
        case gasPhaseIdx: return gasPvt_->saturatedOilVaporizationFactor(regionIdx, T, p, So, maxOilSaturation);
        case waterPhaseIdx: return waterPvt_->saturatedGasDissolutionFactor(regionIdx, T, p,
        BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    /*!
     * \brief Returns the dissolution factor \f$R_\alpha\f$ of a saturated fluid phase
     *
     * For the oil (gas) phase, this means the R_s and R_v factors, for the water phase,
     * it is always 0. The difference of this method compared to the previous one is that
     * this method does not prevent dissolving a given component if the corresponding
     * phase's saturation is small-
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturatedDissolutionFactor(const FluidState& fluidState,
                                              unsigned phaseIdx,
                                              unsigned regionIdx)
    {
        OPM_TIMEBLOCK_LOCAL(saturatedDissolutionFactor);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: return oilPvt_->saturatedGasDissolutionFactor(regionIdx, T, p);
        case gasPhaseIdx: return gasPvt_->saturatedOilVaporizationFactor(regionIdx, T, p);
        case waterPhaseIdx: return waterPvt_->saturatedGasDissolutionFactor(regionIdx, T, p,
        BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    /*!
     * \brief Returns the bubble point pressure $P_b$ using the current Rs
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval bubblePointPressure(const FluidState& fluidState,
                                       unsigned regionIdx)
    {
        return saturationPressure(fluidState, oilPhaseIdx, regionIdx);
    }


    /*!
     * \brief Returns the dew point pressure $P_d$ using the current Rv
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval dewPointPressure(const FluidState& fluidState,
                                       unsigned regionIdx)
    {
        return saturationPressure(fluidState, gasPhaseIdx, regionIdx);
    }

    /*!
     * \brief Returns the saturation pressure of a given phase [Pa] depending on its
     *        composition.
     *
     * In the black-oil model, the saturation pressure it the pressure at which the fluid
     * phase is in equilibrium with the gas phase, i.e., it is the inverse of the
     * "dissolution factor". Note that a-priori this quantity is undefined for the water
     * phase (because water is assumed to be immiscible with everything else). This method
     * here just returns 0, though.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    STATIC_OR_DEVICE LhsEval saturationPressure(const FluidState& fluidState,
                                      unsigned phaseIdx,
                                      unsigned regionIdx)
    {
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: return oilPvt_->saturationPressure(regionIdx, T, BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
        case gasPhaseIdx: return gasPvt_->saturationPressure(regionIdx, T, BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
        case waterPhaseIdx: return waterPvt_->saturationPressure(regionIdx, T,
        BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
        BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }

    /****************************************
     * Auxiliary and convenience methods for the black-oil model
     ****************************************/
    /*!
     * \brief Convert the mass fraction of the gas component in the oil phase to the
     *        corresponding gas dissolution factor.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXoGToRs(const LhsEval& XoG, unsigned regionIdx)
    {
        Scalar rho_oRef = referenceDensity_[regionIdx][oilPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        return XoG/(1.0 - XoG)*(rho_oRef/rho_gRef);
    }

    /*!
     * \brief Convert the mass fraction of the gas component in the water phase to the
     *        corresponding gas dissolution factor.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXwGToRsw(const LhsEval& XwG, unsigned regionIdx)
    {
        Scalar rho_wRef = referenceDensity_[regionIdx][waterPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        return XwG/(1.0 - XwG)*(rho_wRef/rho_gRef);
    }

    /*!
     * \brief Convert the mass fraction of the oil component in the gas phase to the
     *        corresponding oil vaporization factor.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXgOToRv(const LhsEval& XgO, unsigned regionIdx)
    {
        Scalar rho_oRef = referenceDensity_[regionIdx][oilPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        return XgO/(1.0 - XgO)*(rho_gRef/rho_oRef);
    }

    /*!
     * \brief Convert the mass fraction of the water component in the gas phase to the
     *        corresponding water vaporization factor.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXgWToRvw(const LhsEval& XgW, unsigned regionIdx)
    {
        Scalar rho_wRef = referenceDensity_[regionIdx][waterPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        return XgW/(1.0 - XgW)*(rho_gRef/rho_wRef);
    }


    /*!
     * \brief Convert a gas dissolution factor to the the corresponding mass fraction
     *        of the gas component in the oil phase.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertRsToXoG(const LhsEval& Rs, unsigned regionIdx)
    {
        Scalar rho_oRef = referenceDensity_[regionIdx][oilPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        const LhsEval& rho_oG = Rs*rho_gRef;
        return rho_oG/(rho_oRef + rho_oG);
    }

    /*!
     * \brief Convert a gas dissolution factor to the the corresponding mass fraction
     *        of the gas component in the water phase.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertRswToXwG(const LhsEval& Rsw, unsigned regionIdx)
    {
        Scalar rho_wRef = referenceDensity_[regionIdx][waterPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        const LhsEval& rho_wG = Rsw*rho_gRef;
        return rho_wG/(rho_wRef + rho_wG);
    }

    /*!
     * \brief Convert an oil vaporization factor to the corresponding mass fraction
     *        of the oil component in the gas phase.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertRvToXgO(const LhsEval& Rv, unsigned regionIdx)
    {
        Scalar rho_oRef = referenceDensity_[regionIdx][oilPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        const LhsEval& rho_gO = Rv*rho_oRef;
        return rho_gO/(rho_gRef + rho_gO);
    }

    /*!
     * \brief Convert an water vaporization factor to the corresponding mass fraction
     *        of the water component in the gas phase.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertRvwToXgW(const LhsEval& Rvw, unsigned regionIdx)
    {
        Scalar rho_wRef = referenceDensity_[regionIdx][waterPhaseIdx];
        Scalar rho_gRef = referenceDensity_[regionIdx][gasPhaseIdx];

        const LhsEval& rho_gW = Rvw*rho_wRef;
        return rho_gW/(rho_gRef + rho_gW);
    }

    /*!
     * \brief Convert a water mass fraction in the gas phase the corresponding mole fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXgWToxgW(const LhsEval& XgW, unsigned regionIdx)
    {
        Scalar MW = molarMass_[regionIdx][waterCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XgW*MG / (MW*(1 - XgW) + XgW*MG);
    }

    /*!
     * \brief Convert a gas mass fraction in the water phase the corresponding mole fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXwGToxwG(const LhsEval& XwG, unsigned regionIdx)
    {
        Scalar MW = molarMass_[regionIdx][waterCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XwG*MW / (MG*(1 - XwG) + XwG*MW);
    }

    /*!
     * \brief Convert a gas mass fraction in the oil phase the corresponding mole fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXoGToxoG(const LhsEval& XoG, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XoG*MO / (MG*(1 - XoG) + XoG*MO);
    }

    /*!
     * \brief Convert a gas mole fraction in the oil phase the corresponding mass fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertxoGToXoG(const LhsEval& xoG, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return xoG*MG / (xoG*(MG - MO) + MO);
    }

    /*!
     * \brief Convert a oil mass fraction in the gas phase the corresponding mole fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertXgOToxgO(const LhsEval& XgO, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XgO*MG / (MO*(1 - XgO) + XgO*MG);
    }

    /*!
     * \brief Convert a oil mole fraction in the gas phase the corresponding mass fraction.
     */
    template <class LhsEval>
    STATIC_OR_DEVICE LhsEval convertxgOToXgO(const LhsEval& xgO, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return xgO*MO / (xgO*(MO - MG) + MG);
    }

    /*!
     * \brief Return a reference to the low-level object which calculates the gas phase
     *        quantities.
     *
     * \note It is not recommended to use this method directly, but the black-oil
     *       specific methods of the fluid systems from above should be used instead.
     */
    STATIC_OR_DEVICE const GasPvt& gasPvt()
    { return *gasPvt_; }

    /*!
     * \brief Return a reference to the low-level object which calculates the oil phase
     *        quantities.
     *
     * \note It is not recommended to use this method directly, but the black-oil
     *       specific methods of the fluid systems from above should be used instead.
     */
    STATIC_OR_DEVICE const OilPvt& oilPvt()
    { return *oilPvt_; }

    /*!
     * \brief Return a reference to the low-level object which calculates the water phase
     *        quantities.
     *
     * \note It is not recommended to use this method directly, but the black-oil
     *       specific methods of the fluid systems from above should be used instead.
     */
    STATIC_OR_DEVICE const WaterPvt& waterPvt()
    { return *waterPvt_; }

    /*!
     * \brief Set the temperature of the reservoir.
     *
     * This method is black-oil specific and only makes sense for isothermal simulations.
     */
    STATIC_OR_DEVICE Scalar reservoirTemperature(unsigned = 0)
    { return reservoirTemperature_; }

    /*!
     * \brief Return the temperature of the reservoir.
     *
     * This method is black-oil specific and only makes sense for isothermal simulations.
     */
    STATIC_OR_DEVICE void setReservoirTemperature(Scalar value)
    { reservoirTemperature_ = value; }

    STATIC_OR_DEVICE short activeToCanonicalPhaseIdx(unsigned activePhaseIdx);

    STATIC_OR_DEVICE short canonicalToActivePhaseIdx(unsigned phaseIdx);

    //! \copydoc BaseFluidSystem::diffusionCoefficient
    STATIC_OR_DEVICE Scalar diffusionCoefficient(unsigned compIdx, unsigned phaseIdx, unsigned regionIdx = 0)
    { return diffusionCoefficients_[regionIdx][numPhases*compIdx + phaseIdx]; }

    //! \copydoc BaseFluidSystem::setDiffusionCoefficient
    STATIC_OR_DEVICE void setDiffusionCoefficient(Scalar coefficient, unsigned compIdx, unsigned phaseIdx, unsigned regionIdx = 0)
    { diffusionCoefficients_[regionIdx][numPhases*compIdx + phaseIdx] = coefficient ; }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    STATIC_OR_DEVICE LhsEval diffusionCoefficient(const FluidState& fluidState,
                                        const ParameterCache<ParamCacheEval>& paramCache,
                                        unsigned phaseIdx,
                                        unsigned compIdx)
    {
        // diffusion is disabled by the user
        if(!enableDiffusion())
            return 0.0;

        // diffusion coefficients are set, and we use them
        if(!diffusionCoefficients_.empty()) {
            return diffusionCoefficient(compIdx, phaseIdx, paramCache.regionIndex());
        }

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: return oilPvt().diffusionCoefficient(T, p, compIdx);
        case gasPhaseIdx: return gasPvt().diffusionCoefficient(T, p, compIdx);
        case waterPhaseIdx: return waterPvt().diffusionCoefficient(T, p, compIdx);
        default: throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
        }
    }
    STATIC_OR_DEVICE void setEnergyEqualEnthalpy(bool enthalpy_eq_energy){
        enthalpy_eq_energy_ = enthalpy_eq_energy;
    }

    STATIC_OR_DEVICE bool enthalpyEqualEnergy(){
        return enthalpy_eq_energy_;
    }

private:
    STATIC_OR_NOTHING void resizeArrays_(std::size_t numRegions);

    STATIC_OR_NOTHING Scalar reservoirTemperature_;

    STATIC_OR_NOTHING SmartPointer<GasPvt> gasPvt_;
    STATIC_OR_NOTHING SmartPointer<OilPvt> oilPvt_;
    STATIC_OR_NOTHING SmartPointer<WaterPvt> waterPvt_;

    STATIC_OR_NOTHING bool enableDissolvedGas_;
    STATIC_OR_NOTHING bool enableDissolvedGasInWater_;
    STATIC_OR_NOTHING bool enableVaporizedOil_;
    STATIC_OR_NOTHING bool enableVaporizedWater_;
    STATIC_OR_NOTHING bool enableDiffusion_;

    // HACK for GCC 4.4: the array size has to be specified using the literal value '3'
    // here, because GCC 4.4 seems to be unable to determine the number of phases from
    // the BlackOil fluid system in the attribute declaration below...
    STATIC_OR_NOTHING Storage<std::array<Scalar, /*numPhases=*/3> > referenceDensity_;
    STATIC_OR_NOTHING Storage<std::array<Scalar, /*numComponents=*/3> > molarMass_;
    STATIC_OR_NOTHING Storage<std::array<Scalar, /*numComponents=*/3 * /*numPhases=*/3> > diffusionCoefficients_;

    STATIC_OR_NOTHING std::array<short, numPhases> activeToCanonicalPhaseIdx_;
    STATIC_OR_NOTHING std::array<short, numPhases> canonicalToActivePhaseIdx_;

    STATIC_OR_NOTHING bool isInitialized_;
    STATIC_OR_NOTHING bool useSaturatedTables_;
    STATIC_OR_NOTHING bool enthalpy_eq_energy_;

    #ifndef COMPILING_STATIC_FLUID_SYSTEM
    template<template<typename> typename StorageT, template<typename> typename SmartPointerT>
    explicit FLUIDSYSTEM_CLASSNAME(const FLUIDSYSTEM_CLASSNAME_STATIC<Scalar, IndexTraits, StorageT, SmartPointerT>& other)
        : surfacePressure(other.surfacePressure)
        , surfaceTemperature(other.surfaceTemperature)
        , numActivePhases_(other.numActivePhases_)
        , phaseIsActive_(other.phaseIsActive_)
        , reservoirTemperature_(other.reservoirTemperature_)
        , gasPvt_(SmartPointerT<typename decltype(gasPvt_)::element_type>(other.gasPvt_))
        , oilPvt_(SmartPointerT<typename decltype(oilPvt_)::element_type>(other.oilPvt_))
        , waterPvt_(SmartPointerT<typename decltype(waterPvt_)::element_type>(other.waterPvt_))
        , enableDissolvedGas_(other.enableDissolvedGas_)
        , enableDissolvedGasInWater_(other.enableDissolvedGasInWater_)
        , enableVaporizedOil_(other.enableVaporizedOil_)
        , enableVaporizedWater_(other.enableVaporizedWater_)
        , enableDiffusion_(other.enableDiffusion_)
        , referenceDensity_(StorageT<typename decltype(referenceDensity_)::value_type>(other.referenceDensity_))
        , molarMass_(StorageT<typename decltype(molarMass_)::value_type>(other.molarMass_))
        , diffusionCoefficients_(StorageT<typename decltype(diffusionCoefficients_)::value_type>(other.diffusionCoefficients_))
        , activeToCanonicalPhaseIdx_(other.activeToCanonicalPhaseIdx_)
        , canonicalToActivePhaseIdx_(other.canonicalToActivePhaseIdx_)
        , isInitialized_(other.isInitialized_)
        , useSaturatedTables_(other.useSaturatedTables_)
        , enthalpy_eq_energy_(other.enthalpy_eq_energy_)
    {
            OPM_ERROR_IF(!other.isInitialized(), "The fluid system must be initialized before it can be copied.");
    }

    template<class ScalarT, class IndexTraitsT, template<typename> typename StorageT, template<typename> typename SmartPointerT>
    friend class FLUIDSYSTEM_CLASSNAME_STATIC;
    #else
    template<class ScalarT, class IndexTraitsT, template<typename> typename StorageT, template<typename> typename SmartPointerT>
    friend class FLUIDSYSTEM_CLASSNAME_NONSTATIC;
    #endif
};

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
void FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits, Storage, SmartPointer>::
initBegin(std::size_t numPvtRegions)
{
    isInitialized_ = false;
    useSaturatedTables_ = true;

    enableDissolvedGas_ = true;
    enableDissolvedGasInWater_ = false;
    enableVaporizedOil_ = false;
    enableVaporizedWater_ = false;
    enableDiffusion_ = false;

    oilPvt_ = nullptr;
    gasPvt_ = nullptr;
    waterPvt_ = nullptr;

    surfaceTemperature = 273.15 + 15.56; // [K]
    surfacePressure = 1.01325e5; // [Pa]
    setReservoirTemperature(surfaceTemperature);

    numActivePhases_ = numPhases;
    std::fill_n(&phaseIsActive_[0], numPhases, true);

    resizeArrays_(numPvtRegions);
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE void FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits,Storage, SmartPointer>::
setReferenceDensities(Scalar rhoOil,
                      Scalar rhoWater,
                      Scalar rhoGas,
                      unsigned regionIdx)
{
    referenceDensity_[regionIdx][oilPhaseIdx] = rhoOil;
    referenceDensity_[regionIdx][waterPhaseIdx] = rhoWater;
    referenceDensity_[regionIdx][gasPhaseIdx] = rhoGas;
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE void FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits, Storage, SmartPointer>::initEnd()
{
    // calculate the final 2D functions which are used for interpolation.
    const std::size_t num_regions = molarMass_.size();
    for (unsigned regionIdx = 0; regionIdx < num_regions; ++regionIdx) {
        // calculate molar masses

        // water is simple: 18 g/mol
        molarMass_[regionIdx][waterCompIdx] = 18e-3;

        if (phaseIsActive(gasPhaseIdx)) {
            // for gas, we take the density at standard conditions and assume it to be ideal
            Scalar p = surfacePressure;
            Scalar T = surfaceTemperature;
            Scalar rho_g = referenceDensity_[/*regionIdx=*/0][gasPhaseIdx];
            molarMass_[regionIdx][gasCompIdx] = Constants<Scalar>::R*T*rho_g / p;
        }
        else
            // hydrogen gas. we just set this do avoid NaNs later
            molarMass_[regionIdx][gasCompIdx] = 2e-3;

        // finally, for oil phase, we take the molar mass from the spe9 paper
        molarMass_[regionIdx][oilCompIdx] = 175e-3; // kg/mol
    }


    int activePhaseIdx = 0;
    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        if(phaseIsActive(phaseIdx)){
            canonicalToActivePhaseIdx_[phaseIdx] = activePhaseIdx;
            activeToCanonicalPhaseIdx_[activePhaseIdx] = phaseIdx;
            activePhaseIdx++;
        }
    }
    isInitialized_ = true;
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
std::string_view FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits, Storage, SmartPointer>::
phaseName(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        return "water";
    case oilPhaseIdx:
        return "oil";
    case gasPhaseIdx:
        return "gas";

    default:
        throw std::logic_error(std::string("Phase index ") + std::to_string(phaseIdx) + " is unknown");
    }
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE unsigned FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits,Storage, SmartPointer>::
solventComponentIndex(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        return waterCompIdx;
    case oilPhaseIdx:
        return oilCompIdx;
    case gasPhaseIdx:
        return gasCompIdx;

    default:
        throw std::logic_error(std::string("Phase index ") + std::to_string(phaseIdx) + " is unknown");
    }
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE unsigned FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits,Storage, SmartPointer>::
soluteComponentIndex(unsigned phaseIdx)
{
    switch (phaseIdx) {
    case waterPhaseIdx:
        if (enableDissolvedGasInWater())
            return gasCompIdx;
        throw std::logic_error("The water phase does not have any solutes in the black oil model!");
    case oilPhaseIdx:
        return gasCompIdx;
    case gasPhaseIdx:
        if (enableVaporizedWater()) {
            return waterCompIdx;
        }
        return oilCompIdx;

    default:
        throw std::logic_error(std::string("Phase index ") + std::to_string(phaseIdx) + " is unknown");
    }
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
std::string_view FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits,Storage, SmartPointer>::
componentName(unsigned compIdx)
{
    switch (compIdx) {
    case waterCompIdx:
        return "Water";
    case oilCompIdx:
        return "Oil";
    case gasCompIdx:
        return "Gas";

    default:
        throw std::logic_error(std::string("Component index ") + std::to_string(compIdx) + " is unknown");
    }
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE short FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits, Storage, SmartPointer>::
activeToCanonicalPhaseIdx(unsigned activePhaseIdx)
{
    assert(activePhaseIdx<numActivePhases());
    return activeToCanonicalPhaseIdx_[activePhaseIdx];
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
NOTHING_OR_DEVICE short FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits, Storage, SmartPointer>::
canonicalToActivePhaseIdx(unsigned phaseIdx)
{
    assert(phaseIdx<numPhases);
    assert(phaseIsActive(phaseIdx));
    return canonicalToActivePhaseIdx_[phaseIdx];
}

template <class Scalar, class IndexTraits, template<typename> typename Storage, template<typename> typename SmartPointer>
void FLUIDSYSTEM_CLASSNAME<Scalar,IndexTraits,Storage, SmartPointer>::
resizeArrays_(std::size_t numRegions)
{
    molarMass_.resize(numRegions);
    referenceDensity_.resize(numRegions);
}

#ifdef COMPILING_STATIC_FLUID_SYSTEM
template <typename T> using BOFS = FLUIDSYSTEM_CLASSNAME<T, BlackOilDefaultIndexTraits, VectorWithDefaultAllocator, std::shared_ptr>;

#define DECLARE_INSTANCE(T)                                                           \
template<> unsigned char BOFS<T>::numActivePhases_;                                   \
template<> std::array<bool, BOFS<T>::numPhases> BOFS<T>::phaseIsActive_;              \
template<> std::array<short, BOFS<T>::numPhases> BOFS<T>::activeToCanonicalPhaseIdx_; \
template<> std::array<short, BOFS<T>::numPhases> BOFS<T>::canonicalToActivePhaseIdx_; \
template<> T BOFS<T>::surfaceTemperature;                                             \
template<> T BOFS<T>::surfacePressure;                                                \
template<> T BOFS<T>::reservoirTemperature_;                                          \
template<> bool BOFS<T>::enableDissolvedGas_;                                         \
template<> bool BOFS<T>::enableDissolvedGasInWater_;                                  \
template<> bool BOFS<T>::enableVaporizedOil_;                                         \
template<> bool BOFS<T>::enableVaporizedWater_;                                       \
template<> bool BOFS<T>::enableDiffusion_;                                            \
template<> std::shared_ptr<OilPvtMultiplexer<T>> BOFS<T>::oilPvt_;                    \
template<> std::shared_ptr<GasPvtMultiplexer<T>> BOFS<T>::gasPvt_;                    \
template<> std::shared_ptr<WaterPvtMultiplexer<T>> BOFS<T>::waterPvt_;                \
template<> std::vector<std::array<T, 3>> BOFS<T>::referenceDensity_;                  \
template<> std::vector<std::array<T, 3>> BOFS<T>::molarMass_;                         \
template<> std::vector<std::array<T, 9>> BOFS<T>::diffusionCoefficients_;             \
template<> bool BOFS<T>::isInitialized_;                                              \
template<> bool BOFS<T>::useSaturatedTables_;                                         \
template<> bool BOFS<T>::enthalpy_eq_energy_;

DECLARE_INSTANCE(float)
DECLARE_INSTANCE(double)

#undef DECLARE_INSTANCE
#endif

#ifndef COMPILING_STATIC_FLUID_SYSTEM
namespace gpuistl
{

template <template <class> class NewContainerType, class Scalar, class IndexTraits, template <class> class OldContainerType>
FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, NewContainerType>
copy_to_gpu(const FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, OldContainerType>& oldFluidSystem) {

    using GpuBuffer3Array = NewContainerType<std::array<Scalar, 3>>;
    using GpuBuffer9Array = NewContainerType<std::array<Scalar, 9>>;

    auto newGasPvt = std::make_shared<GasPvtMultiplexer<Scalar, true, NewContainerType<double>, NewContainerType<Scalar>>>(
        copy_to_gpu<NewContainerType<double>, NewContainerType<Scalar>>(*oldFluidSystem.gasPvt_.get())
    );

    auto newOilPvt = std::make_shared<OilPvtMultiplexer<Scalar>>();

    auto newWaterPvt = std::make_shared<WaterPvtMultiplexer<Scalar, true, true, NewContainerType<double>, NewContainerType<Scalar>>>(
        copy_to_gpu<NewContainerType<double>, NewContainerType<Scalar>>(*oldFluidSystem.waterPvt_)
    );

    static_assert(std::is_same_v<decltype(newGasPvt), std::shared_ptr<GasPvtMultiplexer<Scalar, true, NewContainerType<double>, NewContainerType<Scalar>>>>);

    auto newReferenceDensity = GpuBuffer3Array(oldFluidSystem.referenceDensity_);
    auto newMolarMass = GpuBuffer3Array(oldFluidSystem.molarMass_);
    auto newDiffusionCoefficients = GpuBuffer9Array(oldFluidSystem.diffusionCoefficients_);

    return FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, NewContainerType>(
        oldFluidSystem.surfacePressure,
        oldFluidSystem.surfaceTemperature,
        oldFluidSystem.numActivePhases_,
        oldFluidSystem.phaseIsActive_,
        oldFluidSystem.reservoirTemperature_,
        newGasPvt,
        newOilPvt,
        newWaterPvt,
        oldFluidSystem.enableDissolvedGas_,
        oldFluidSystem.enableDissolvedGasInWater_,
        oldFluidSystem.enableVaporizedOil_,
        oldFluidSystem.enableVaporizedWater_,
        oldFluidSystem.enableDiffusion_,
        newReferenceDensity,
        newMolarMass,
        newDiffusionCoefficients,
        oldFluidSystem.activeToCanonicalPhaseIdx_,
        oldFluidSystem.canonicalToActivePhaseIdx_,
        oldFluidSystem.isInitialized_,
        oldFluidSystem.useSaturatedTables_,
        oldFluidSystem.enthalpy_eq_energy_
    );
}

template <template <class> class ViewType, template <class> class PtrType, class Scalar, class IndexTraits, template <class> class OldContainerType>
FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, ViewType, PtrType>
make_view(FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, OldContainerType>& oldFluidSystem)
{
    using Array3 = std::array<Scalar, 3>;
    using Array9 = std::array<Scalar, 9>;
    using GasMultiplexerView = GasPvtMultiplexer<Scalar, true, ViewType<double>, ViewType<Scalar>, PtrType>;
    using WaterMultiplexerView = WaterPvtMultiplexer<Scalar, true, true, ViewType<double>, ViewType<Scalar>, PtrType>;

    // TODO: avoid newing this here
    auto* allocatedGasPvt = new GasMultiplexerView(
        make_view<PtrType, ViewType<double>, ViewType<Scalar>>(*oldFluidSystem.gasPvt_)
    );

    auto* allocatedOilPvt = new OilPvtMultiplexer<Scalar>();

    auto* allocatedWaterPvt = new WaterMultiplexerView(
        make_view<PtrType, ViewType<double>, ViewType<Scalar>>(*oldFluidSystem.waterPvt_)
    );

    auto newGasPvt = PtrType<GasMultiplexerView>(*allocatedGasPvt);
    auto newOilPvt = PtrType<OilPvtMultiplexer<Scalar>>(*allocatedOilPvt);
    auto newWaterPvt = PtrType<WaterMultiplexerView>(*allocatedWaterPvt);

    auto newReferenceDensity = make_view<Array3>(oldFluidSystem.referenceDensity_);
    auto newMolarMass = make_view<Array3>(oldFluidSystem.molarMass_);
    auto newDiffusionCoefficients = make_view<Array9>(oldFluidSystem.diffusionCoefficients_);

    return FLUIDSYSTEM_CLASSNAME<Scalar, IndexTraits, ViewType, PtrType>(
        oldFluidSystem.surfacePressure,
        oldFluidSystem.surfaceTemperature,
        oldFluidSystem.numActivePhases_,
        oldFluidSystem.phaseIsActive_,
        oldFluidSystem.reservoirTemperature_,
        newGasPvt,
        newOilPvt,
        newWaterPvt,
        oldFluidSystem.enableDissolvedGas_,
        oldFluidSystem.enableDissolvedGasInWater_,
        oldFluidSystem.enableVaporizedOil_,
        oldFluidSystem.enableVaporizedWater_,
        oldFluidSystem.enableDiffusion_,
        newReferenceDensity,
        newMolarMass,
        newDiffusionCoefficients,
        oldFluidSystem.activeToCanonicalPhaseIdx_,
        oldFluidSystem.canonicalToActivePhaseIdx_,
        oldFluidSystem.isInitialized_,
        oldFluidSystem.useSaturatedTables_,
        oldFluidSystem.enthalpy_eq_energy_
    );
}
}
#endif

} // namespace Opm
