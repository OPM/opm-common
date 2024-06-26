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
#ifndef OPM_BLACK_OIL_FLUID_SYSTEM_HPP
#define OPM_BLACK_OIL_FLUID_SYSTEM_HPP

#include "BlackOilDefaultIndexTraits.hpp"
#include "blackoilpvt/OilPvtMultiplexer.hpp"
#include "blackoilpvt/GasPvtMultiplexer.hpp"
#include "blackoilpvt/WaterPvtMultiplexer.hpp"
#include "blackoilpvt/BrineCo2Pvt.hpp"
#include "blackoilpvt/BrineH2Pvt.hpp"

#include <opm/common/TimingMacros.hpp>

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/Constants.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/HasMemberGeneratorMacros.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

namespace BlackOil {
OPM_GENERATE_HAS_MEMBER(Rs, ) // Creates 'HasMember_Rs<T>'.
OPM_GENERATE_HAS_MEMBER(Rv, ) // Creates 'HasMember_Rv<T>'.
OPM_GENERATE_HAS_MEMBER(Rvw, ) // Creates 'HasMember_Rvw<T>'.
OPM_GENERATE_HAS_MEMBER(Rsw, ) // Creates 'HasMember_Rsw<T>'.
OPM_GENERATE_HAS_MEMBER(saltConcentration, )
OPM_GENERATE_HAS_MEMBER(saltSaturation, )

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getRs_(typename std::enable_if<!HasMember_Rs<FluidState>::value, const FluidState&>::type fluidState,
               unsigned regionIdx)
{
    const auto& XoG =
        decay<LhsEval>(fluidState.massFraction(FluidSystem::oilPhaseIdx, FluidSystem::gasCompIdx));
    return FluidSystem::convertXoGToRs(XoG, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto getRs_(typename std::enable_if<HasMember_Rs<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rs()))
{ return decay<LhsEval>(fluidState.Rs()); }

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getRv_(typename std::enable_if<!HasMember_Rv<FluidState>::value, const FluidState&>::type fluidState,
               unsigned regionIdx)
{
    const auto& XgO =
        decay<LhsEval>(fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::oilCompIdx));
    return FluidSystem::convertXgOToRv(XgO, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto getRv_(typename std::enable_if<HasMember_Rv<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rv()))
{ return decay<LhsEval>(fluidState.Rv()); }

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getRvw_(typename std::enable_if<!HasMember_Rvw<FluidState>::value, const FluidState&>::type fluidState,
               unsigned regionIdx)
{
    const auto& XgW =
        decay<LhsEval>(fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::waterCompIdx));
    return FluidSystem::convertXgWToRvw(XgW, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto getRvw_(typename std::enable_if<HasMember_Rvw<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rvw()))
{ return decay<LhsEval>(fluidState.Rvw()); }

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getRsw_(typename std::enable_if<!HasMember_Rsw<FluidState>::value, const FluidState&>::type fluidState,
               unsigned regionIdx)
{
    const auto& XwG =
        decay<LhsEval>(fluidState.massFraction(FluidSystem::waterPhaseIdx, FluidSystem::gasCompIdx));
    return FluidSystem::convertXwGToRsw(XwG, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto getRsw_(typename std::enable_if<HasMember_Rsw<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rsw()))
{ return decay<LhsEval>(fluidState.Rsw()); }

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getSaltConcentration_(typename std::enable_if<!HasMember_saltConcentration<FluidState>::value,
                              const FluidState&>::type,
                              unsigned)
{return 0.0;}

template <class FluidSystem, class FluidState, class LhsEval>
auto getSaltConcentration_(typename std::enable_if<HasMember_saltConcentration<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.saltConcentration()))
{ return decay<LhsEval>(fluidState.saltConcentration()); }

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval getSaltSaturation_(typename std::enable_if<!HasMember_saltSaturation<FluidState>::value,
                              const FluidState&>::type,
                              unsigned)
{return 0.0;}

template <class FluidSystem, class FluidState, class LhsEval>
auto getSaltSaturation_(typename std::enable_if<HasMember_saltSaturation<FluidState>::value, const FluidState&>::type fluidState,
            unsigned)
    -> decltype(decay<LhsEval>(fluidState.saltSaturation()))
{ return decay<LhsEval>(fluidState.saltSaturation()); }

}

/*!
 * \brief A fluid system which uses the black-oil model assumptions to calculate
 *        termodynamically meaningful quantities.
 *
 * \tparam Scalar The type used for scalar floating point values
 */
template <class Scalar, class IndexTraits = BlackOilDefaultIndexTraits>
class BlackOilFluidSystem : public BaseFluidSystem<Scalar, BlackOilFluidSystem<Scalar, IndexTraits> >
{
    using ThisType = BlackOilFluidSystem;

public:
    using GasPvt = GasPvtMultiplexer<Scalar>;
    using OilPvt = OilPvtMultiplexer<Scalar>;
    using WaterPvt = WaterPvtMultiplexer<Scalar>;

    //! \copydoc BaseFluidSystem::ParameterCache
    template <class EvaluationT>
    struct ParameterCache : public NullParameterCache<EvaluationT>
    {
        using Evaluation = EvaluationT;

    public:
        ParameterCache(Scalar maxOilSat = 1.0, unsigned regionIdx=0)
        {
            maxOilSat_ = maxOilSat;
            regionIdx_ = regionIdx;
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

    /****************************************
     * Initialization
     ****************************************/
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the fluid system using an ECL deck object
     */
    static void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Begin the initialization of the black oil fluid system.
     *
     * After calling this method the reference densities, all dissolution and formation
     * volume factors, the oil bubble pressure, all viscosities and the water
     * compressibility must be set. Before the fluid system can be used, initEnd() must
     * be called to finalize the initialization.
     */
    static void initBegin(std::size_t numPvtRegions);

    /*!
     * \brief Specify whether the fluid system should consider that the gas component can
     *        dissolve in the oil phase
     *
     * By default, dissolved gas is considered.
     */
    static void setEnableDissolvedGas(bool yesno)
    { enableDissolvedGas_ = yesno; }

    /*!
     * \brief Specify whether the fluid system should consider that the oil component can
     *        dissolve in the gas phase
     *
     * By default, vaporized oil is not considered.
     */
    static void setEnableVaporizedOil(bool yesno)
    { enableVaporizedOil_ = yesno; }

     /*!
     * \brief Specify whether the fluid system should consider that the water component can
     *        dissolve in the gas phase
     *
     * By default, vaporized water is not considered.
     */
    static void setEnableVaporizedWater(bool yesno)
    { enableVaporizedWater_ = yesno; }

     /*!
     * \brief Specify whether the fluid system should consider that the gas component can
     *        dissolve in the water phase
     *
     * By default, dissovled gas in water is not considered.
     */
    static void setEnableDissolvedGasInWater(bool yesno)
    { enableDissolvedGasInWater_ = yesno; }
    /*!
     * \brief Specify whether the fluid system should consider diffusion
     *
     * By default, diffusion is not considered.
     */
    static void setEnableDiffusion(bool yesno)
    { enableDiffusion_ = yesno; }


    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the gas phase.
     */
    static void setGasPvt(std::shared_ptr<GasPvt> pvtObj)
    { gasPvt_ = pvtObj; }

    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the oil phase.
     */
    static void setOilPvt(std::shared_ptr<OilPvt> pvtObj)
    { oilPvt_ = pvtObj; }

    /*!
     * \brief Set the pressure-volume-saturation (PVT) relations for the water phase.
     */
    static void setWaterPvt(std::shared_ptr<WaterPvt> pvtObj)
    { waterPvt_ = pvtObj; }

    static void setVapPars(const Scalar par1, const Scalar par2)
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
    static void setReferenceDensities(Scalar rhoOil,
                                      Scalar rhoWater,
                                      Scalar rhoGas,
                                      unsigned regionIdx);

    /*!
     * \brief Finish initializing the black oil fluid system.
     */
    static void initEnd();

    static bool isInitialized()
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
    static Scalar surfacePressure;

    //! The temperature at the surface
    static Scalar surfaceTemperature;

    //! \copydoc BaseFluidSystem::phaseName
    static std::string_view phaseName(unsigned phaseIdx);

    //! \copydoc BaseFluidSystem::isLiquid
    static bool isLiquid(unsigned phaseIdx)
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
    static constexpr unsigned oilCompIdx = IndexTraits::oilCompIdx;
    //! Index of the water component
    static constexpr unsigned waterCompIdx = IndexTraits::waterCompIdx;
    //! Index of the gas component
    static constexpr unsigned gasCompIdx = IndexTraits::gasCompIdx;

protected:
    static unsigned char numActivePhases_;
    static std::array<bool,numPhases> phaseIsActive_;

public:
    //! \brief Returns the number of active fluid phases (i.e., usually three)
    static unsigned numActivePhases()
    { return numActivePhases_; }

    //! \brief Returns whether a fluid phase is active
    static bool phaseIsActive(unsigned phaseIdx)
    {
        assert(phaseIdx < numPhases);
        return phaseIsActive_[phaseIdx];
    }

    //! \brief returns the index of "primary" component of a phase (solvent)
    static unsigned solventComponentIndex(unsigned phaseIdx);

    //! \brief returns the index of "secondary" component of a phase (solute)
    static unsigned soluteComponentIndex(unsigned phaseIdx);

    //! \copydoc BaseFluidSystem::componentName
    static std::string_view componentName(unsigned compIdx);

    //! \copydoc BaseFluidSystem::molarMass
    static Scalar molarMass(unsigned compIdx, unsigned regionIdx = 0)
    { return molarMass_[regionIdx][compIdx]; }

    //! \copydoc BaseFluidSystem::isIdealMixture
    static bool isIdealMixture(unsigned /*phaseIdx*/)
    {
        // fugacity coefficients are only pressure dependent -> we
        // have an ideal mixture
        return true;
    }

    //! \copydoc BaseFluidSystem::isCompressible
    static bool isCompressible(unsigned /*phaseIdx*/)
    { return true; /* all phases are compressible */ }

    //! \copydoc BaseFluidSystem::isIdealGas
    static bool isIdealGas(unsigned /*phaseIdx*/)
    { return false; }


    /****************************************
     * Black-oil specific properties
     ****************************************/
    /*!
     * \brief Returns the number of PVT regions which are considered.
     *
     * By default, this is 1.
     */
    static std::size_t numRegions()
    { return molarMass_.size(); }

    /*!
     * \brief Returns whether the fluid system should consider that the gas component can
     *        dissolve in the oil phase
     *
     * By default, dissolved gas is considered.
     */
    static bool enableDissolvedGas()
    { return enableDissolvedGas_; }


    /*!
     * \brief Returns whether the fluid system should consider that the gas component can
     *        dissolve in the water phase
     *
     * By default, dissolved gas is considered.
     */
    static bool enableDissolvedGasInWater()
    { return enableDissolvedGasInWater_; }

    /*!
     * \brief Returns whether the fluid system should consider that the oil component can
     *        dissolve in the gas phase
     *
     * By default, vaporized oil is not considered.
     */
    static bool enableVaporizedOil()
    { return enableVaporizedOil_; }

    /*!
     * \brief Returns whether the fluid system should consider that the water component can
     *        dissolve in the gas phase
     *
     * By default, vaporized water is not considered.
     */
    static bool enableVaporizedWater()
    { return enableVaporizedWater_; }

    /*!
     * \brief Returns whether the fluid system should consider diffusion
     *
     * By default, diffusion is not considered.
     */
    static bool enableDiffusion()
    { return enableDiffusion_; }

    /*!
     * \brief Returns the density of a fluid phase at surface pressure [kg/m^3]
     *
     * \copydoc Doxygen::phaseIdxParam
     */
    static Scalar referenceDensity(unsigned phaseIdx, unsigned regionIdx)
    { return referenceDensity_[regionIdx][phaseIdx]; }

    /****************************************
     * thermodynamic quantities (generic version)
     ****************************************/
    //! \copydoc BaseFluidSystem::density
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    static LhsEval density(const FluidState& fluidState,
                           const ParameterCache<ParamCacheEval>& paramCache,
                           unsigned phaseIdx)
    { return density<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    //! \copydoc BaseFluidSystem::fugacityCoefficient
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    static LhsEval fugacityCoefficient(const FluidState& fluidState,
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
    static LhsEval viscosity(const FluidState& fluidState,
                             const ParameterCache<ParamCacheEval>& paramCache,
                             unsigned phaseIdx)
    { return viscosity<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    //! \copydoc BaseFluidSystem::enthalpy
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    static LhsEval enthalpy(const FluidState& fluidState,
                            const ParameterCache<ParamCacheEval>& paramCache,
                            unsigned phaseIdx)
    { return enthalpy<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    static LhsEval internalEnergy(const FluidState& fluidState,
                                  const ParameterCache<ParamCacheEval>& paramCache,
                                  unsigned phaseIdx)
    { return internalEnergy<FluidState, LhsEval>(fluidState, phaseIdx, paramCache.regionIndex()); }

    /****************************************
     * thermodynamic quantities (black-oil specific version: Note that the PVT region
     * index is explicitly passed instead of a parameter cache object)
     ****************************************/
    //! \copydoc BaseFluidSystem::density
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    static LhsEval density(const FluidState& fluidState,
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

        throw std::logic_error("Unhandled phase index "+std::to_string(phaseIdx));
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
    static LhsEval saturatedDensity(const FluidState& fluidState,
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
    static LhsEval inverseFormationVolumeFactor(const FluidState& fluidState,
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
                if (fluidState.saturation(gasPhaseIdx) > 0.0
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
                 if (fluidState.saturation(waterPhaseIdx) > 0.0
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
                if (fluidState.saturation(oilPhaseIdx) > 0.0
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
                if (fluidState.saturation(waterPhaseIdx) > 0.0
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
                if (fluidState.saturation(gasPhaseIdx) > 0.0
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
    static LhsEval saturatedInverseFormationVolumeFactor(const FluidState& fluidState,
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
    static LhsEval fugacityCoefficient(const FluidState& fluidState,
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
    static LhsEval viscosity(const FluidState& fluidState,
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
                if (fluidState.saturation(gasPhaseIdx) > 0.0
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
                 if (fluidState.saturation(waterPhaseIdx) > 0.0
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
                if (fluidState.saturation(oilPhaseIdx) > 0.0
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
                if (fluidState.saturation(waterPhaseIdx) > 0.0
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
                if (fluidState.saturation(gasPhaseIdx) > 0.0
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
    static LhsEval internalEnergy(const FluidState& fluidState,
                                  unsigned phaseIdx,
                                  unsigned regionIdx){
        bool is_mixing = false;
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));

        switch (phaseIdx) {
        case oilPhaseIdx: {
            if(!oilPvt_->mixingEnergy()){
                const auto& oilEnergy =
                    oilPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRs_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));

                return oilEnergy;
            }
            is_mixing = true;
            break;
        }
        case waterPhaseIdx: {
            if(!waterPvt_->mixingEnergy()){
                const auto waterEnergy =
                    waterPvt_->internalEnergy(regionIdx, T, p,
                                              BlackOil::template getRsw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                              BlackOil::template getSaltConcentration_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));

                return waterEnergy;
            }
            is_mixing = true;
            break;
        }
        case gasPhaseIdx: {
            if(!gasPvt_->mixingEnergy()){
                const auto gasEnergy =
                    gasPvt_->internalEnergy(regionIdx, T, p,
                                            BlackOil::template getRv_<ThisType, FluidState, LhsEval>(fluidState, regionIdx),
                                            BlackOil::template getRvw_<ThisType, FluidState, LhsEval>(fluidState, regionIdx));

                return gasEnergy;
            }
            is_mixing = true;
            break;
        }
        }
        assert(is_mixing==true);
        const auto& energy = internalMixingTotalEnergy<FluidState,LhsEval>(fluidState, phaseIdx, regionIdx);
        const auto& phase_density = density<FluidState,LhsEval>(fluidState, phaseIdx, regionIdx);
        return energy/phase_density;
    }


    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    static LhsEval internalMixingTotalEnergy(const FluidState& fluidState,
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
    static LhsEval enthalpy(const FluidState& fluidState,
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
    static LhsEval saturatedVaporizationFactor(const FluidState& fluidState,
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
    static LhsEval saturatedDissolutionFactor(const FluidState& fluidState,
                                              unsigned phaseIdx,
                                              unsigned regionIdx,
                                              const LhsEval& maxOilSaturation)
    {
        OPM_TIMEBLOCK_LOCAL(saturatedDissolutionFactor);
        assert(phaseIdx <= numPhases);
        assert(regionIdx <= numRegions());

        const auto& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& So = decay<LhsEval>(fluidState.saturation(oilPhaseIdx));

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
    static LhsEval saturatedDissolutionFactor(const FluidState& fluidState,
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
    static LhsEval bubblePointPressure(const FluidState& fluidState,
                                       unsigned regionIdx)
    {
        return saturationPressure(fluidState, oilPhaseIdx, regionIdx);
    }


    /*!
     * \brief Returns the dew point pressure $P_d$ using the current Rv
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    static LhsEval dewPointPressure(const FluidState& fluidState,
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
    static LhsEval saturationPressure(const FluidState& fluidState,
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
    static LhsEval convertXoGToRs(const LhsEval& XoG, unsigned regionIdx)
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
    static LhsEval convertXwGToRsw(const LhsEval& XwG, unsigned regionIdx)
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
    static LhsEval convertXgOToRv(const LhsEval& XgO, unsigned regionIdx)
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
    static LhsEval convertXgWToRvw(const LhsEval& XgW, unsigned regionIdx)
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
    static LhsEval convertRsToXoG(const LhsEval& Rs, unsigned regionIdx)
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
    static LhsEval convertRswToXwG(const LhsEval& Rsw, unsigned regionIdx)
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
    static LhsEval convertRvToXgO(const LhsEval& Rv, unsigned regionIdx)
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
    static LhsEval convertRvwToXgW(const LhsEval& Rvw, unsigned regionIdx)
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
    static LhsEval convertXgWToxgW(const LhsEval& XgW, unsigned regionIdx)
    {
        Scalar MW = molarMass_[regionIdx][waterCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XgW*MG / (MW*(1 - XgW) + XgW*MG);
    }

    /*!
     * \brief Convert a gas mass fraction in the water phase the corresponding mole fraction.
     */
    template <class LhsEval>
    static LhsEval convertXwGToxwG(const LhsEval& XwG, unsigned regionIdx)
    {
        Scalar MW = molarMass_[regionIdx][waterCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XwG*MW / (MG*(1 - XwG) + XwG*MW);
    }

    /*!
     * \brief Convert a gas mass fraction in the oil phase the corresponding mole fraction.
     */
    template <class LhsEval>
    static LhsEval convertXoGToxoG(const LhsEval& XoG, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XoG*MO / (MG*(1 - XoG) + XoG*MO);
    }

    /*!
     * \brief Convert a gas mole fraction in the oil phase the corresponding mass fraction.
     */
    template <class LhsEval>
    static LhsEval convertxoGToXoG(const LhsEval& xoG, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return xoG*MG / (xoG*(MG - MO) + MO);
    }

    /*!
     * \brief Convert a oil mass fraction in the gas phase the corresponding mole fraction.
     */
    template <class LhsEval>
    static LhsEval convertXgOToxgO(const LhsEval& XgO, unsigned regionIdx)
    {
        Scalar MO = molarMass_[regionIdx][oilCompIdx];
        Scalar MG = molarMass_[regionIdx][gasCompIdx];

        return XgO*MG / (MO*(1 - XgO) + XgO*MG);
    }

    /*!
     * \brief Convert a oil mole fraction in the gas phase the corresponding mass fraction.
     */
    template <class LhsEval>
    static LhsEval convertxgOToXgO(const LhsEval& xgO, unsigned regionIdx)
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
    static const GasPvt& gasPvt()
    { return *gasPvt_; }

    /*!
     * \brief Return a reference to the low-level object which calculates the oil phase
     *        quantities.
     *
     * \note It is not recommended to use this method directly, but the black-oil
     *       specific methods of the fluid systems from above should be used instead.
     */
    static const OilPvt& oilPvt()
    { return *oilPvt_; }

    /*!
     * \brief Return a reference to the low-level object which calculates the water phase
     *        quantities.
     *
     * \note It is not recommended to use this method directly, but the black-oil
     *       specific methods of the fluid systems from above should be used instead.
     */
    static const WaterPvt& waterPvt()
    { return *waterPvt_; }

    /*!
     * \brief Set the temperature of the reservoir.
     *
     * This method is black-oil specific and only makes sense for isothermal simulations.
     */
    static Scalar reservoirTemperature(unsigned = 0)
    { return reservoirTemperature_; }

    /*!
     * \brief Return the temperature of the reservoir.
     *
     * This method is black-oil specific and only makes sense for isothermal simulations.
     */
    static void setReservoirTemperature(Scalar value)
    { reservoirTemperature_ = value; }

    static short activeToCanonicalPhaseIdx(unsigned activePhaseIdx);

    static short canonicalToActivePhaseIdx(unsigned phaseIdx);

    //! \copydoc BaseFluidSystem::diffusionCoefficient
    static Scalar diffusionCoefficient(unsigned compIdx, unsigned phaseIdx, unsigned regionIdx = 0)
    { return diffusionCoefficients_[regionIdx][numPhases*compIdx + phaseIdx]; }

    //! \copydoc BaseFluidSystem::setDiffusionCoefficient
    static void setDiffusionCoefficient(Scalar coefficient, unsigned compIdx, unsigned phaseIdx, unsigned regionIdx = 0)
    { diffusionCoefficients_[regionIdx][numPhases*compIdx + phaseIdx] = coefficient ; }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
    static LhsEval diffusionCoefficient(const FluidState& fluidState,
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
    static void setEnergyEqualEnthalpy(bool enthalpy_eq_energy){
        enthalpy_eq_energy_ = enthalpy_eq_energy;
    }

    static bool enthalpyEqualEnergy(){
        return enthalpy_eq_energy_;
    }

private:
    static void resizeArrays_(std::size_t numRegions);

    static Scalar reservoirTemperature_;

    static std::shared_ptr<GasPvt> gasPvt_;
    static std::shared_ptr<OilPvt> oilPvt_;
    static std::shared_ptr<WaterPvt> waterPvt_;

    static bool enableDissolvedGas_;
    static bool enableDissolvedGasInWater_;
    static bool enableVaporizedOil_;
    static bool enableVaporizedWater_;
    static bool enableDiffusion_;

    // HACK for GCC 4.4: the array size has to be specified using the literal value '3'
    // here, because GCC 4.4 seems to be unable to determine the number of phases from
    // the BlackOil fluid system in the attribute declaration below...
    static std::vector<std::array<Scalar, /*numPhases=*/3> > referenceDensity_;
    static std::vector<std::array<Scalar, /*numComponents=*/3> > molarMass_;
    static std::vector<std::array<Scalar, /*numComponents=*/3 * /*numPhases=*/3> > diffusionCoefficients_;

    static std::array<short, numPhases> activeToCanonicalPhaseIdx_;
    static std::array<short, numPhases> canonicalToActivePhaseIdx_;

    static bool isInitialized_;
    inline static bool enthalpy_eq_energy_ = false;
};

template <typename T> using BOFS = BlackOilFluidSystem<T, BlackOilDefaultIndexTraits>;

#define DECLARE_INSTANCE(T) \
template<> unsigned char BOFS<T>::numActivePhases_; \
template<> std::array<bool, BOFS<T>::numPhases> BOFS<T>::phaseIsActive_; \
template<> std::array<short, BOFS<T>::numPhases> BOFS<T>::activeToCanonicalPhaseIdx_; \
template<> std::array<short, BOFS<T>::numPhases> BOFS<T>::canonicalToActivePhaseIdx_; \
template<> T BOFS<T>::surfaceTemperature; \
template<> T BOFS<T>::surfacePressure; \
template<> T BOFS<T>::reservoirTemperature_; \
template<> bool BOFS<T>::enableDissolvedGas_; \
template<> bool BOFS<T>::enableDissolvedGasInWater_; \
template<> bool BOFS<T>::enableVaporizedOil_; \
template<> bool BOFS<T>::enableVaporizedWater_; \
template<> bool BOFS<T>::enableDiffusion_; \
template<> std::shared_ptr<OilPvtMultiplexer<T>> BOFS<T>::oilPvt_; \
template<> std::shared_ptr<GasPvtMultiplexer<T>> BOFS<T>::gasPvt_; \
template<> std::shared_ptr<WaterPvtMultiplexer<T>> BOFS<T>::waterPvt_; \
template<> std::vector<std::array<T, 3>> BOFS<T>::referenceDensity_; \
template<> std::vector<std::array<T, 3>> BOFS<T>::molarMass_; \
template<> std::vector<std::array<T, 9>> BOFS<T>::diffusionCoefficients_; \
template<> bool BOFS<T>::isInitialized_;

DECLARE_INSTANCE(float)
DECLARE_INSTANCE(double)

#undef DECLARE_INSTANCE

} // namespace Opm

#endif
