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
 *
 * \copydoc Opm::BlackOilFluidState
 */
#ifndef OPM_BLACK_OIL_FLUID_STATE_HH
#define OPM_BLACK_OIL_FLUID_STATE_HH

#include <type_traits>

#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>
#include <opm/material/common/HasMemberGeneratorMacros.hpp>

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/ConditionalStorage.hpp>

namespace Opm {

OPM_GENERATE_HAS_MEMBER(pvtRegionIndex, ) // Creates 'HasMember_pvtRegionIndex<T>'.

template <class FluidState>
OPM_HOST_DEVICE unsigned getPvtRegionIndex_(typename std::enable_if<HasMember_pvtRegionIndex<FluidState>::value,
                                            const FluidState&>::type fluidState)
{ return fluidState.pvtRegionIndex(); }

template <class FluidState>
OPM_HOST_DEVICE unsigned getPvtRegionIndex_(typename std::enable_if<!HasMember_pvtRegionIndex<FluidState>::value,
                                            const FluidState&>::type)
{ return 0; }

OPM_GENERATE_HAS_MEMBER(invB, /*phaseIdx=*/0) // Creates 'HasMember_invB<T>'.

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE
auto getInvB_(typename std::enable_if<HasMember_invB<FluidState>::value,
                                      const FluidState&>::type fluidState,
              unsigned phaseIdx,
              unsigned,
              [[maybe_unused]] const FluidSystem& fluidSystem = FluidSystem{})
    -> decltype(decay<LhsEval>(fluidState.invB(phaseIdx)))
{ return decay<LhsEval>(fluidState.invB(phaseIdx)); }

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE
LhsEval getInvB_(typename std::enable_if<!HasMember_invB<FluidState>::value,
                                          const FluidState&>::type fluidState,
                 unsigned phaseIdx,
                 unsigned pvtRegionIdx,
                 const FluidSystem& fluidSystem = FluidSystem{})
{
    const auto& rho = fluidState.density(phaseIdx);
    const auto& Xsolvent =
        fluidState.massFraction(phaseIdx, fluidSystem.solventComponentIndex(phaseIdx));

    return
        decay<LhsEval>(rho)
        *decay<LhsEval>(Xsolvent)
        /fluidSystem.referenceDensity(phaseIdx, pvtRegionIdx);
}

OPM_GENERATE_HAS_MEMBER(saltConcentration, ) // Creates 'HasMember_saltConcentration<T>'.

template <class FluidState>
OPM_HOST_DEVICE auto getSaltConcentration_(typename std::enable_if<HasMember_saltConcentration<FluidState>::value,
                                           const FluidState&>::type fluidState)
{ return fluidState.saltConcentration(); }

template <class FluidState>
OPM_HOST_DEVICE auto getSaltConcentration_(typename std::enable_if<!HasMember_saltConcentration<FluidState>::value,
                                           const FluidState&>::type)
{ return 0.0; }

OPM_GENERATE_HAS_MEMBER(saltSaturation, ) // Creates 'HasMember_saltSaturation<T>'.

template <class FluidState>
OPM_HOST_DEVICE auto getSaltSaturation_(typename std::enable_if<HasMember_saltSaturation<FluidState>::value,
                                        const FluidState&>::type fluidState)
{ return fluidState.saltSaturation(); }


template <class FluidState>
OPM_HOST_DEVICE auto getSaltSaturation_(typename std::enable_if<!HasMember_saltSaturation<FluidState>::value,
                                        const FluidState&>::type)
{ return 0.0; }

OPM_GENERATE_HAS_MEMBER(solventSaturation, ) // Creates 'HasMember_solventSaturation<T>'.

template <class FluidState>
OPM_HOST_DEVICE auto getSolventSaturation_(typename std::enable_if<HasMember_solventSaturation<FluidState>::value,
                                           const FluidState&>::type fluidState)
{ return fluidState.solventSaturation(); }

template <class FluidState>
OPM_HOST_DEVICE auto getSolventSaturation_(typename std::enable_if<!HasMember_solventSaturation<FluidState>::value,
                                           const FluidState&>::type)
{ return 0.0; }

/*!
 * \brief Implements a "tailor-made" fluid state class for the black-oil model.
 *
 * I.e., it uses exactly the same quantities which are used by the ECL blackoil
 * model. Further quantities are computed "on the fly" and are accessing them is thus
 * relatively slow.
 */
template <class ValueT,
          class FluidSystemT,
          bool storeTemperature = false,
          bool storeEnthalpy = false,
          bool enableDissolution = true,
          bool enableVapwat = false,
          bool enableBrine = false,
          bool enableSaltPrecipitation = false,
          bool enableDissolutionInWater = false,
          bool enableSolvent = false,
          unsigned numStoragePhases = FluidSystemT::numPhases>
class BlackOilFluidState
{
public:

template <class OtherScalarT,
          class OtherFluidSystemT,
          bool otherStoreTemperature,
          bool otherStoreEnthalpy,
          bool otherEnableDissolution,
          bool otherEnableVapwat,
          bool otherEnableBrine,
          bool otherEnableSaltPrecipitation,
          bool otherEnableDissolutionInWater,
          bool otherEnableSolvent,
          unsigned otherNumStoragePhases>
friend class BlackOilFluidState;

    using FluidSystem = FluidSystemT;
    using ValueType = ValueT;

    static constexpr int waterPhaseIdx = FluidSystem::waterPhaseIdx;
    static constexpr int gasPhaseIdx = FluidSystem::gasPhaseIdx;
    static constexpr int oilPhaseIdx = FluidSystem::oilPhaseIdx;

    static constexpr int waterCompIdx = FluidSystem::waterCompIdx;
    static constexpr int gasCompIdx = FluidSystem::gasCompIdx;
    static constexpr int oilCompIdx = FluidSystem::oilCompIdx;

    static constexpr int numPhases = FluidSystem::numPhases;
    static constexpr int numComponents = FluidSystem::numComponents;

    static constexpr bool fluidSystemIsStatic = std::is_empty_v<FluidSystem>;

    /**
     * \brief Construct a fluid state object.
     *
     * \param fluidSystem The fluid system which is used to compute various quantities
     */
    explicit OPM_HOST_DEVICE BlackOilFluidState(const FluidSystem& fluidSystem)
    {
        if constexpr (!fluidSystemIsStatic) {
            fluidSystemPtr_ = &fluidSystem;
        }
    }

    /**
     * \brief Create a new fluid state object with a different fluid system.
     *
     * \param other The new fluid system to use.
     * \return A new BlackOilFluidState object with the specified fluid system.
     */
    template<class OtherFluidSystemType>
    auto withOtherFluidSystem(const OtherFluidSystemType& other) const
    {
        using FS = std::decay_t<OtherFluidSystemType>;

        auto bfstate = BlackOilFluidState<
            ValueType,
            FS,
            storeTemperature,
            storeEnthalpy,
            enableDissolution,
            enableVapwat,
            enableBrine,
            enableSaltPrecipitation,
            enableDissolutionInWater,
            enableSolvent,
            numStoragePhases
        >(other);

        bfstate.assign(*this);
        return bfstate;
    }

    /**
     * \brief Construct a fluid state object.
     *
     * The fluid system used is assumed to be stateless.
     */
    OPM_HOST_DEVICE BlackOilFluidState()
    {
        static_assert(fluidSystemIsStatic);
    }

    /*!
     * \brief Make sure that all attributes are defined.
     *
     * This method does not do anything if the program is not run
     * under valgrind. If it is, then valgrind will print an error
     * message if some attributes of the object have not been properly
     * defined.
     */
    void checkDefined() const
    {
#ifndef NDEBUG
        Valgrind::CheckDefined(pvtRegionIdx_);

        for (unsigned storagePhaseIdx = 0; storagePhaseIdx < numStoragePhases; ++ storagePhaseIdx) {
            Valgrind::CheckDefined(saturation_[storagePhaseIdx]);
            Valgrind::CheckDefined(pressure_[storagePhaseIdx]);
            Valgrind::CheckDefined(density_[storagePhaseIdx]);
            Valgrind::CheckDefined(invB_[storagePhaseIdx]);

            if constexpr (storeEnthalpy)
                Valgrind::CheckDefined((*enthalpy_)[storagePhaseIdx]);
        }

        if constexpr (enableDissolution) {
            Valgrind::CheckDefined(*Rs_);
            Valgrind::CheckDefined(*Rv_);
        }

        if constexpr (enableVapwat) {
            Valgrind::CheckDefined(*Rvw_);
        }

        if constexpr (enableDissolutionInWater) {
            Valgrind::CheckDefined(*Rsw_);
        }

        if constexpr (enableBrine) {
            Valgrind::CheckDefined(*saltConcentration_);
        }

        if constexpr (enableSaltPrecipitation) {
            Valgrind::CheckDefined(*saltSaturation_);
        }

        if constexpr (enableSolvent) {
            Valgrind::CheckDefined(*solventSaturation_);
            Valgrind::CheckDefined(*solventDensity_);
            Valgrind::CheckDefined(*solventInvB_);
            Valgrind::CheckDefined(*rsSolw_);
        }

        if constexpr (storeTemperature)
            Valgrind::CheckDefined(*temperature_);
#endif // NDEBUG
    }

    /*!
     * \brief Retrieve all parameters from an arbitrary fluid
     *        state.
     */
    template <class FluidState>
    OPM_HOST_DEVICE void assign(const FluidState& fs)
    {
        if constexpr (storeTemperature)
            setTemperature(fs.temperature(/*phaseIdx=*/0));

        unsigned pvtRegionIdx = getPvtRegionIndex_<FluidState>(fs);
        setPvtRegionIndex(pvtRegionIdx);

        setTotalSaturation(fs.totalSaturation());

        if constexpr (enableDissolution) {
            setRs(BlackOil::getRs_<FluidSystem, FluidState, ValueType>(fs, pvtRegionIdx));
            setRv(BlackOil::getRv_<FluidSystem, FluidState, ValueType>(fs, pvtRegionIdx));
        }
        if constexpr (enableVapwat) {
            setRvw(BlackOil::getRvw_<FluidSystem, FluidState, ValueType>(fs, pvtRegionIdx));
        }
        if constexpr (enableDissolutionInWater) {
            setRsw(BlackOil::getRsw_<FluidSystem, FluidState, ValueType>(fs, pvtRegionIdx));
        }
        if constexpr (enableBrine){
            setSaltConcentration(BlackOil::getSaltConcentration_<FluidState, ValueType>(fs, pvtRegionIdx));
        }
        if constexpr (enableSaltPrecipitation){
            setSaltSaturation(BlackOil::getSaltSaturation_<FluidSystem, FluidState, ValueType>(fs, pvtRegionIdx));
        }
        if constexpr (enableSolvent) {
            setSolventSaturation(BlackOil::getSolventSaturation_<FluidState, ValueType>(fs, pvtRegionIdx));
            setSolventDensity(BlackOil::getSolventDensity_<FluidState, ValueType>(fs, pvtRegionIdx));
            setSolventInvB(BlackOil::getSolventInvB_<FluidState, ValueType>(fs, pvtRegionIdx));
            setRsSolw(BlackOil::getRsSolw_<FluidState, ValueType>(fs, pvtRegionIdx));
        }
        for (unsigned int storagePhaseIdx = 0; storagePhaseIdx < numStoragePhases; ++storagePhaseIdx) {
            // No need to use setXXX as we would just have to convert index to canonical index and then back.
            pressure_[storagePhaseIdx] = fs.pressure_[storagePhaseIdx];
            saturation_[storagePhaseIdx] = fs.saturation_[storagePhaseIdx];
            density_[storagePhaseIdx] = fs.density_[storagePhaseIdx];
            invB_[storagePhaseIdx] = fs.invB_[storagePhaseIdx];
            if constexpr (storeEnthalpy)
                (*enthalpy_)[storagePhaseIdx] = (*fs.enthalpy_)[storagePhaseIdx];
        }
    }

    /*!
     * \brief Set the index of the fluid region
     *
     * This determines which tables are used to compute the quantities that are computed
     * on the fly.
     */
    OPM_HOST_DEVICE void setPvtRegionIndex(unsigned newPvtRegionIdx)
    { pvtRegionIdx_ = static_cast<unsigned short>(newPvtRegionIdx); }

    /*!
     * \brief Set the pressure of a fluid phase [-].
     */
    OPM_HOST_DEVICE void setPressure(unsigned phaseIdx, const ValueType& p)
    { pressure_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = p; }

    /*!
     * \brief Set the saturation of a fluid phase [-].
     */
    OPM_HOST_DEVICE void setSaturation(unsigned phaseIdx, const ValueType& S)
    { saturation_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = S; }

    /*!
     * \brief Set the total saturation used for sequential methods
     */
    OPM_HOST_DEVICE void setTotalSaturation(const ValueType& value)
    {
        totalSaturation_ = value;
    }

    /*!
     * \brief Set the temperature [K]
     *
     * If storeTemperature arguments are not set
     * to true, this method will throw an exception!
     */
    OPM_HOST_DEVICE void setTemperature(const ValueType& value)
    {
        assert(storeTemperature);

        (*temperature_) = value;
    }

    /*!
     * \brief Set the specific enthalpy [J/kg] of a given fluid phase.
     *
     * If the storeEnthalpy template argument is not set to true, this method will throw
     * an exception!
     */
    OPM_HOST_DEVICE void setEnthalpy(unsigned phaseIdx, const ValueType& value)
    {
        assert(storeEnthalpy);

        (*enthalpy_)[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = value;
    }

    /*!
     * \ brief Set the inverse formation volume factor of a fluid phase
     */
    OPM_HOST_DEVICE void setInvB(unsigned phaseIdx, const ValueType& b)
    { invB_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = b; }

    /*!
     * \ brief Set the density of a fluid phase
     */
    OPM_HOST_DEVICE void setDensity(unsigned phaseIdx, const ValueType& rho)
    { density_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = rho; }

    /*!
     * \brief Set the gas dissolution factor [m^3/m^3] of the oil phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRs(const ValueType& newRs)
    { *Rs_ = newRs; }

    /*!
     * \brief Set the oil vaporization factor [m^3/m^3] of the gas phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRv(const ValueType& newRv)
    { *Rv_ = newRv; }

    /*!
     * \brief Set the water vaporization factor [m^3/m^3] of the gas phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRvw(const ValueType& newRvw)
    { *Rvw_ = newRvw; }

    /*!
     * \brief Set the gas dissolution factor [m^3/m^3] of the water phase..
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRsw(const ValueType& newRsw)
    { *Rsw_ = newRsw; }

    /*!
     * \brief Set the salt concentration.
     */
    OPM_HOST_DEVICE void setSaltConcentration(const ValueType& newSaltConcentration)
    { *saltConcentration_ = newSaltConcentration; }

    /*!
     * \brief Set the solid salt saturation.
     */
    OPM_HOST_DEVICE void setSaltSaturation(const ValueType& newSaltSaturation)
    { *saltSaturation_ = newSaltSaturation; }

    /*!
     * \brief Set the solvent saturation.
     */
    OPM_HOST_DEVICE void setSolventSaturation(const ValueType& newSolventSaturation)
    { *solventSaturation_ = newSolventSaturation; }

    /*!
     * \brief Set the solvent density [kg/m^3].
     */
    OPM_HOST_DEVICE void setSolventDensity(const ValueType& newSolventDensity)
    { *solventDensity_ = newSolventDensity; }

    /*!
     * \brief Set the solvent inverse formation volume factor [-].
     */
    OPM_HOST_DEVICE void setSolventInvB(const ValueType& newSolventInvB)
    { *solventInvB_ = newSolventInvB; }

    /*!
     * \brief Set the solvent dissolution factor in water [m^3/m^3].
     */
    OPM_HOST_DEVICE void setRsSolw(const ValueType& newRsSolw)
    { *rsSolw_ = newRsSolw; }

    /*!
     * \brief Return the pressure of a fluid phase [Pa]
     */
    OPM_HOST_DEVICE const ValueType& pressure(unsigned phaseIdx) const
    { return pressure_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the saturation of a fluid phase [-]
     */
    OPM_HOST_DEVICE const ValueType& saturation(unsigned phaseIdx) const
    { return saturation_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the total saturation needed for sequential
     */
    OPM_HOST_DEVICE const ValueType& totalSaturation() const
    {
        return totalSaturation_;
    }

    /*!
     * \brief Return the temperature [K]
     */
    OPM_HOST_DEVICE ValueType temperature(unsigned) const
    {
        if constexpr (storeTemperature) {
            return *temperature_;
        } else {
            return fluidSystem().reservoirTemperature(pvtRegionIdx_);
        }
    }

    /*!
     * \brief Return the inverse formation volume factor of a fluid phase [-].
     *
     * This factor expresses the change of density of a pure phase due to increased
     * pressure and temperature at reservoir conditions compared to surface conditions.
     */
    OPM_HOST_DEVICE const ValueType& invB(unsigned phaseIdx) const
    { return invB_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the gas dissolution factor of oil [m^3/m^3].
     *
     * I.e., the amount of gas which is present in the oil phase in terms of cubic meters
     * of gas at surface conditions per cubic meter of liquid oil at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE ValueType Rs() const
    {
        if constexpr (enableDissolution) {
            return *Rs_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the oil vaporization factor of gas [m^3/m^3].
     *
     * I.e., the amount of oil which is present in the gas phase in terms of cubic meters
     * of liquid oil at surface conditions per cubic meter of gas at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE ValueType Rv() const
    {
        if constexpr (!enableDissolution) {
            return ValueType{0.0};
        } else {
            return *Rv_;
        }
    }

    /*!
     * \brief Return the water vaporization factor of gas [m^3/m^3].
     *
     * I.e., the amount of water which is present in the gas phase in terms of cubic meters
     * of liquid water at surface conditions per cubic meter of gas at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE ValueType Rvw() const
    {
        if constexpr (enableVapwat) {
            return *Rvw_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the gas dissolution factor of water [m^3/m^3].
     *
     * I.e., the amount of gas which is present in the water phase in terms of cubic meters
     * of gas at surface conditions per cubic meter of water at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE ValueType Rsw() const
    {
        if constexpr (enableDissolutionInWater) {
            return *Rsw_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the concentration of salt in water
     */
    OPM_HOST_DEVICE ValueType saltConcentration() const
    {
        if constexpr (enableBrine) {
            return *saltConcentration_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the saturation of solid salt
     */
    OPM_HOST_DEVICE ValueType saltSaturation() const
    {
        if constexpr (enableSaltPrecipitation) {
            return *saltSaturation_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the solvent saturation [-]
     */
    OPM_HOST_DEVICE ValueType solventSaturation() const
    {
        if constexpr (enableSolvent) {
            return *solventSaturation_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the solvent density [kg/m^3]
     */
    OPM_HOST_DEVICE ValueType solventDensity() const
    {
        if constexpr (enableSolvent) {
            return *solventDensity_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the solvent inverse formation volume factor [-]
     */
    OPM_HOST_DEVICE ValueType solventInvB() const
    {
        if constexpr (enableSolvent) {
            return *solventInvB_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the solvent dissolution factor in water [m^3/m^3]
     */
    OPM_HOST_DEVICE ValueType rsSolw() const
    {
        if constexpr (enableSolvent) {
            return *rsSolw_;
        } else {
            return ValueType{0.0};
        }
    }

    /*!
     * \brief Return the PVT region where the current fluid state is assumed to be part of.
     *
     */
    OPM_HOST_DEVICE unsigned short pvtRegionIndex() const
    { return pvtRegionIdx_; }

    /*!
     * \brief Return the density [kg/m^3] of a given fluid phase.
      */
    OPM_HOST_DEVICE ValueType density(unsigned phaseIdx) const
    { return density_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the specific enthalpy [J/kg] of a given fluid phase.
     *
     * If the EnableEnergy property is not set to true, this method will throw an
     * exception!
     */
    OPM_HOST_DEVICE const ValueType& enthalpy(unsigned phaseIdx) const
    { return (*enthalpy_)[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the specific internal energy [J/kg] of a given fluid phase.
     *
     * If the EnableEnergy property is not set to true, this method will throw an
     * exception!
     */
    OPM_HOST_DEVICE ValueType internalEnergy(unsigned phaseIdx) const
    {   auto energy = (*enthalpy_)[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())];
        if(!fluidSystem().enthalpyEqualEnergy()){
            energy -= pressure(phaseIdx)/density(phaseIdx);
        }
        return energy;
    }

    //////
    // slow methods
    //////

    /*!
     * \brief Return the molar density of a fluid phase [mol/m^3].
     */
    OPM_HOST_DEVICE ValueType molarDensity(unsigned phaseIdx) const
    {
        const auto& rho = density(phaseIdx);

        if (phaseIdx == waterPhaseIdx)
            return rho/fluidSystem().molarMass(waterCompIdx, pvtRegionIdx_);

        return
            rho*(moleFraction(phaseIdx, gasCompIdx)/fluidSystem().molarMass(gasCompIdx, pvtRegionIdx_)
                 + moleFraction(phaseIdx, oilCompIdx)/fluidSystem().molarMass(oilCompIdx, pvtRegionIdx_));

    }

    /*!
     * \brief Return the molar volume of a fluid phase [m^3/mol].
     *
     * This is equivalent to the inverse of the molar density.
     */
    OPM_HOST_DEVICE ValueType molarVolume(unsigned phaseIdx) const
    { return 1.0/molarDensity(phaseIdx); }

    /*!
     * \brief Return the dynamic viscosity of a fluid phase [Pa s].
     */
    OPM_HOST_DEVICE ValueType viscosity(unsigned phaseIdx) const
    {
        typename FluidSystem::template ParameterCache<ValueType> paramCache;
        paramCache.setRegionIndex(pvtRegionIdx_);
        paramCache.setDepth(0.0);
        paramCache.updateAll(*this);
        return fluidSystem().viscosity(*this, paramCache, phaseIdx);
    }

    /*!
     * \brief Return the mass fraction of a component in a fluid phase [-].
     */
    OPM_HOST_DEVICE ValueType massFraction(unsigned phaseIdx, unsigned compIdx) const
    {
        switch (phaseIdx) {
        case waterPhaseIdx:
            if (compIdx == waterCompIdx)
                return 1.0;
            return 0.0;

        case oilPhaseIdx:
            if (compIdx == waterCompIdx)
                return 0.0;
            else if (compIdx == oilCompIdx)
                return 1.0 - fluidSystem().convertRsToXoG(Rs(), pvtRegionIdx_);
            else {
                assert(compIdx == gasCompIdx);
                return fluidSystem().convertRsToXoG(Rs(), pvtRegionIdx_);
            }
            break;

        case gasPhaseIdx:
            if (compIdx == waterCompIdx)
                return 0.0;
            else if (compIdx == oilCompIdx)
                return fluidSystem().convertRvToXgO(Rv(), pvtRegionIdx_);
            else {
                assert(compIdx == gasCompIdx);
                return 1.0 - fluidSystem().convertRvToXgO(Rv(), pvtRegionIdx_);
            }
            break;
        }

        OPM_THROW(std::logic_error, "Invalid phase or component index!");
    }

    /*!
     * \brief Return the mole fraction of a component in a fluid phase [-].
     */
    OPM_HOST_DEVICE ValueType moleFraction(unsigned phaseIdx, unsigned compIdx) const
    {
        switch (phaseIdx) {
        case waterPhaseIdx:
            if (compIdx == waterCompIdx)
                return 1.0;
            return 0.0;

        case oilPhaseIdx:
            if (compIdx == waterCompIdx)
                return 0.0;
            else if (compIdx == oilCompIdx)
                return 1.0 - fluidSystem().convertXoGToxoG(fluidSystem().convertRsToXoG(Rs(), pvtRegionIdx_),
                                                          pvtRegionIdx_);
            else {
                assert(compIdx == gasCompIdx);
                return fluidSystem().convertXoGToxoG(fluidSystem().convertRsToXoG(Rs(), pvtRegionIdx_),
                                                     pvtRegionIdx_);
            }
            break;

        case gasPhaseIdx:
            if (compIdx == waterCompIdx)
                return 0.0;
            else if (compIdx == oilCompIdx)
                return fluidSystem().convertXgOToxgO(fluidSystem().convertRvToXgO(Rv(), pvtRegionIdx_),
                                                    pvtRegionIdx_);
            else {
                assert(compIdx == gasCompIdx);
                return 1.0 - fluidSystem().convertXgOToxgO(fluidSystem().convertRvToXgO(Rv(), pvtRegionIdx_),
                                                          pvtRegionIdx_);
            }
            break;
        }

        OPM_THROW(std::logic_error, "Invalid phase or component index!");
    }

    /*!
     * \brief Return the partial molar density of a component in a fluid phase [mol / m^3].
     */
    OPM_HOST_DEVICE ValueType molarity(unsigned phaseIdx, unsigned compIdx) const
    { return moleFraction(phaseIdx, compIdx)*molarDensity(phaseIdx); }

    /*!
     * \brief Return the partial molar density of a fluid phase [kg / mol].
     */
    OPM_HOST_DEVICE ValueType averageMolarMass(unsigned phaseIdx) const
    {
        ValueType result{0.0};
        for (unsigned compIdx = 0; compIdx < numComponents; ++ compIdx)
            result += fluidSystem().molarMass(compIdx, pvtRegionIdx_)*moleFraction(phaseIdx, compIdx);
        return result;
    }

    /*!
     * \brief Return the fugacity coefficient of a component in a fluid phase [-].
     */
    OPM_HOST_DEVICE ValueType fugacityCoefficient(unsigned phaseIdx, unsigned compIdx) const
    {
        typename FluidSystem::template ParameterCache<ValueType> paramCache;
        paramCache.setRegionIndex(pvtRegionIdx_);
        paramCache.setDepth(0.0);
        paramCache.updateAll(*this);
        return fluidSystem().fugacityCoefficient(*this, paramCache, phaseIdx, compIdx);
    }

    /*!
     * \brief Return the fugacity of a component in a fluid phase [Pa].
     */
    OPM_HOST_DEVICE ValueType fugacity(unsigned phaseIdx, unsigned compIdx) const
    {
        return
            fugacityCoefficient(phaseIdx, compIdx)
            *moleFraction(phaseIdx, compIdx)
            *pressure(phaseIdx);
    }

    /*!
     * \brief Return if a phase is active (via the FluidSystem).
     *
     * Note: this could be a static function, but for future GPU
     *       usage we must avoid static, so making it a regular
     *       member function to simplify future refactoring.
     */
    OPM_HOST_DEVICE bool phaseIsActive(int phaseIdx) const
    {
        return fluidSystem().phaseIsActive(phaseIdx);
    }

    /*!
     * \brief Return the fluid system used by this fluid state.
     *
     * If the fluid system is static (i.e., if the fluid system
     * has no state), this method will always return a reference
     * to the same static object.
     */
    OPM_HOST_DEVICE const FluidSystem& fluidSystem() const
    {
        if constexpr (fluidSystemIsStatic) {
            static FluidSystem instance;
            return instance;
        } else {
            return **fluidSystemPtr_;
        }
    }

private:
    OPM_HOST_DEVICE static unsigned storageToCanonicalPhaseIndex_(unsigned storagePhaseIdx, const FluidSystem& fluidSystem)
    {
        if constexpr (numStoragePhases == 3)
            return storagePhaseIdx;
        else
            return fluidSystem.activeToCanonicalPhaseIdx(storagePhaseIdx);
    }

    OPM_HOST_DEVICE static unsigned canonicalToStoragePhaseIndex_(unsigned canonicalPhaseIdx, const FluidSystem& fluidSystem)
    {
        if constexpr (numStoragePhases == 3)
            return canonicalPhaseIdx;
        else
            return fluidSystem.canonicalToActivePhaseIdx(canonicalPhaseIdx);
    }

    ConditionalStorage<storeTemperature, ValueType> temperature_{};
    ConditionalStorage<storeEnthalpy, std::array<ValueType, numStoragePhases> > enthalpy_{};
    ValueType totalSaturation_{};
    std::array<ValueType, numStoragePhases> pressure_{};
    std::array<ValueType, numStoragePhases> saturation_{};
    std::array<ValueType, numStoragePhases> invB_{};
    std::array<ValueType, numStoragePhases> density_{};
    ConditionalStorage<enableDissolution,ValueType> Rs_{};
    ConditionalStorage<enableDissolution, ValueType> Rv_{};
    ConditionalStorage<enableVapwat,ValueType> Rvw_{};
    ConditionalStorage<enableDissolutionInWater,ValueType> Rsw_{};
    ConditionalStorage<enableBrine, ValueType> saltConcentration_{};
    ConditionalStorage<enableSaltPrecipitation, ValueType> saltSaturation_{};
    ConditionalStorage<enableSolvent, ValueType> solventSaturation_{};
    ConditionalStorage<enableSolvent, ValueType> solventDensity_{};
    ConditionalStorage<enableSolvent, ValueType> solventInvB_{};
    ConditionalStorage<enableSolvent, ValueType> rsSolw_{};

    unsigned short pvtRegionIdx_{};

    // If we have a non-static fluid system, we need to store a pointer
    // to it. Otherwise, we do not need to store anything.
    ConditionalStorage<!fluidSystemIsStatic, FluidSystem const*> fluidSystemPtr_;
};

} // namespace Opm

#endif
