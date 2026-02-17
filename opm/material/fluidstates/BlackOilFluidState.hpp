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

/*!
 * \brief Implements a "tailor-made" fluid state class for the black-oil model.
 *
 * I.e., it uses exactly the same quantities which are used by the ECL blackoil
 * model. Further quantities are computed "on the fly" and are accessing them is thus
 * relatively slow.
 */
template <class ScalarT,
          class FluidSystemT,
          bool storeTemperature = false,
          bool storeEnthalpy = false,
          bool enableDissolution = true,
          bool enableVapwat = false,
          bool enableBrine = false,
          bool enableSaltPrecipitation = false,
          bool enableDissolutionInWater = false,
          unsigned numStoragePhases = FluidSystemT::numPhases>
class BlackOilFluidState
{
public:
    using FluidSystem = FluidSystemT;
    using Scalar = ScalarT;

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
    explicit OPM_HOST_DEVICE BlackOilFluidState(const FluidSystem* fluidSystem)
    {
        if constexpr (fluidSystemIsStatic) {
            fluidSystemPtr_ = fluidSystem;
        }
    }

    // Constructor that initializes all member variables
    // Only members enabled in SPE11C are initialized
    OPM_HOST_DEVICE BlackOilFluidState(
        const FluidSystem* fluidSystem,
        Scalar temperature,
        std::array<Scalar, numStoragePhases> enthalpy,
        Scalar totalSaturation,
        std::array<Scalar, numStoragePhases> pressure,
        std::array<Scalar, numStoragePhases> saturation,
        std::array<Scalar, numStoragePhases> invB,
        std::array<Scalar, numStoragePhases> density,
        Scalar Rvw,
        Scalar Rsw,
        unsigned short pvtRegionIdx
    ) : temperature_(temperature),
        enthalpy_(enthalpy),
        totalSaturation_(totalSaturation),
        pressure_(pressure),
        saturation_(saturation),
        invB_(invB),
        density_(density),
        Rvw_(Rvw),
        Rsw_(Rsw),
        pvtRegionIdx_(pvtRegionIdx),
        fluidSystemPtr_(fluidSystem)
    {
    }

    // This is intended to be used when we are converting fluid
    // state from a version that uses the static fluidsystem to
    // a version that uses a dynamic fluid system.
    template<class OtherFluidSystemType>
    auto withOtherFluidSystem(const OtherFluidSystemType* other) const
    {
        return BlackOilFluidState<Scalar, OtherFluidSystemType,
                                  enableTemperature,
                                  enableEnergy,
                                  enableDissolution,
                                  enableVapwat,
                                  enableBrine,
                                  enableSaltPrecipitation,
                                  enableDissolutionInWater,
                                  numStoragePhases>(
            other,
            *temperature_,
            *enthalpy_,
            totalSaturation_,
            pressure_,
            saturation_,
            invB_,
            density_,
            *Rvw_,
            *Rsw_,
            pvtRegionIdx_
        );
    }

    // Constructor that initializes all member variables
    // Only members enabled in SPE11C are initialized
    // OPM_HOST_DEVICE BlackOilFluidState(
    //     const FluidSystem* fluidSystem,
    //     Scalar totalSaturation,
    //     std::array<Scalar, numStoragePhases> pressure,
    //     std::array<Scalar, numStoragePhases> saturation,
    //     std::array<Scalar, numStoragePhases> invB,
    //     std::array<Scalar, numStoragePhases> density,
    //     Scalar Rvw,
    //     Scalar Rsw,
    //     unsigned short pvtRegionIdx
    // ) : totalSaturation_(totalSaturation),
    //     pressure_(pressure),
    //     saturation_(saturation),
    //     invB_(invB),
    //     density_(density),
    //     Rvw_(Rvw),
    //     Rsw_(Rsw),
    //     pvtRegionIdx_(pvtRegionIdx),
    //     fluidSystemPtr_(fluidSystem)
    // {
    // }

    // // This is intended to be used when we are converting fluid
    // // state from a version that uses the static fluidsystem to
    // // a version that uses a dynamic fluid system.
    // template<class OtherFluidSystemType>
    // auto withOtherFluidSystem(const OtherFluidSystemType* other) const
    // {
    //     return BlackOilFluidState<Scalar, OtherFluidSystemType,
    //                               enableTemperature,
    //                               enableEnergy,
    //                               enableDissolution,
    //                               enableVapwat,
    //                               enableBrine,
    //                               enableSaltPrecipitation,
    //                               enableDissolutionInWater,
    //                               numStoragePhases>(
    //         other,
    //         totalSaturation_,
    //         pressure_,
    //         saturation_,
    //         invB_,
    //         density_,
    //         *Rvw_,
    //         *Rsw_,
    //         pvtRegionIdx_
    //     );
    // }

    /**
     * \brief Construct a fluid state object.
     *
     * The fluid system used is assumed to be stateless.
     */
    // TODO: Uncomment and implement default constructor
     OPM_HOST_DEVICE BlackOilFluidState()
     {
     //    static_assert(fluidSystemIsStatic);
    
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

        if constexpr (storeTemperature)
            Valgrind::CheckDefined(*temperature_);
#endif // NDEBUG
    }

    /*!
     * \brief Retrieve all parameters from an arbitrary fluid
     *        state.
     */
    template <class FluidState, class FluidSystemType = FluidSystem>
    OPM_HOST_DEVICE void assign(const FluidState& fs, const FluidSystemType& fluidSystemLocal = FluidSystem())
    {
        if constexpr (storeTemperature)
            setTemperature(fs.temperature(/*phaseIdx=*/0));

        unsigned pvtRegionIdx = getPvtRegionIndex_<FluidState>(fs);
        setPvtRegionIndex(pvtRegionIdx);

        if constexpr (enableDissolution) {
            setRs(BlackOil::getRs_<FluidSystem, FluidState, Scalar>(fs, pvtRegionIdx));
            setRv(BlackOil::getRv_<FluidSystem, FluidState, Scalar>(fs, pvtRegionIdx));
        }
        if constexpr (enableVapwat) {
            setRvw(BlackOil::getRvw_<FluidSystem, FluidState, Scalar>(fs, pvtRegionIdx));
        }
        if constexpr (enableDissolutionInWater) {
            setRsw(BlackOil::getRsw_<FluidSystem, FluidState, Scalar>(fs, pvtRegionIdx));
        }
        if constexpr (enableBrine){
            setSaltConcentration(BlackOil::getSaltConcentration_<FluidState, Scalar>(fs, pvtRegionIdx));
        }
        if constexpr (enableSaltPrecipitation){
            setSaltSaturation(BlackOil::getSaltSaturation_<FluidSystem, FluidState, Scalar>(fs, pvtRegionIdx));
        }
        for (unsigned storagePhaseIdx = 0; storagePhaseIdx < numStoragePhases; ++storagePhaseIdx) {
            unsigned phaseIdx = storageToCanonicalPhaseIndex_(storagePhaseIdx, fluidSystem());
            setSaturation(phaseIdx, fs.saturation(phaseIdx));
            setPressure(phaseIdx, fs.pressure(phaseIdx));
            setDensity(phaseIdx, fs.density(phaseIdx));

            if constexpr (storeEnthalpy)
                setEnthalpy(phaseIdx, fs.enthalpy(phaseIdx));

            setInvB(phaseIdx, getInvB_<FluidSystem, FluidState, Scalar>(fs, phaseIdx, pvtRegionIdx, fluidSystem()));
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
    OPM_HOST_DEVICE void setPressure(unsigned phaseIdx, const Scalar& p)
    { pressure_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = p; }

    /*!
     * \brief Set the saturation of a fluid phase [-].
     */
    OPM_HOST_DEVICE void setSaturation(unsigned phaseIdx, const Scalar& S)
    { saturation_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = S; }

    OPM_HOST_DEVICE void setFluidSystemPtr(FluidSystem* fluidSystemPtr)
    {
        if constexpr (!fluidSystemIsStatic) {
            fluidSystemPtr_ = fluidSystemPtr;
        }
    }

    /*!
     * \brief Set the total saturation used for sequential methods
     */
    OPM_HOST_DEVICE void setTotalSaturation(const Scalar& value)
    {
        totalSaturation_ = value;
    }

    /*!
     * \brief Set the temperature [K]
     *
     * If storeTemperature arguments are not set
     * to true, this method will throw an exception!
     */
    OPM_HOST_DEVICE void setTemperature(const Scalar& value)
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
    OPM_HOST_DEVICE void setEnthalpy(unsigned phaseIdx, const Scalar& value)
    {
        assert(storeEnthalpy);

        (*enthalpy_)[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = value;
    }

    /*!
     * \ brief Set the inverse formation volume factor of a fluid phase
     */
    OPM_HOST_DEVICE void setInvB(unsigned phaseIdx, const Scalar& b)
    { invB_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = b; }

    /*!
     * \ brief Set the density of a fluid phase
     */
    OPM_HOST_DEVICE void setDensity(unsigned phaseIdx, const Scalar& rho)
    { density_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())] = rho; }

    /*!
     * \brief Set the gas dissolution factor [m^3/m^3] of the oil phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRs(const Scalar& newRs)
    { *Rs_ = newRs; }

    /*!
     * \brief Set the oil vaporization factor [m^3/m^3] of the gas phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRv(const Scalar& newRv)
    { *Rv_ = newRv; }

    /*!
     * \brief Set the water vaporization factor [m^3/m^3] of the gas phase.
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRvw(const Scalar& newRvw)
    { *Rvw_ = newRvw; }

    /*!
     * \brief Set the gas dissolution factor [m^3/m^3] of the water phase..
     *
     * This quantity is very specific to the black-oil model.
     */
    OPM_HOST_DEVICE void setRsw(const Scalar& newRsw)
    { *Rsw_ = newRsw; }

    /*!
     * \brief Set the salt concentration.
     */
    OPM_HOST_DEVICE void setSaltConcentration(const Scalar& newSaltConcentration)
    { *saltConcentration_ = newSaltConcentration; }

    /*!
     * \brief Set the solid salt saturation.
     */
    OPM_HOST_DEVICE void setSaltSaturation(const Scalar& newSaltSaturation)
    { *saltSaturation_ = newSaltSaturation; }

    /*!
     * \brief Return the pressure of a fluid phase [Pa]
     */
    OPM_HOST_DEVICE const Scalar& pressure(unsigned phaseIdx) const
    { return pressure_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the saturation of a fluid phase [-]
     */
    OPM_HOST_DEVICE const Scalar& saturation(unsigned phaseIdx) const
    { return saturation_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the total saturation needed for sequential
     */
    OPM_HOST_DEVICE const Scalar& totalSaturation() const
    {
        return totalSaturation_;
    }

    /*!
     * \brief Return the temperature [K]
     */
    OPM_HOST_DEVICE Scalar temperature(unsigned) const
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
    OPM_HOST_DEVICE const Scalar& invB(unsigned phaseIdx) const
    { return invB_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the gas dissolution factor of oil [m^3/m^3].
     *
     * I.e., the amount of gas which is present in the oil phase in terms of cubic meters
     * of gas at surface conditions per cubic meter of liquid oil at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE Scalar Rs() const
    {
        if constexpr (enableDissolution) {
            return *Rs_;
        } else {
            return Scalar(0.0);
        }
    }

    /*!
     * \brief Return the oil vaporization factor of gas [m^3/m^3].
     *
     * I.e., the amount of oil which is present in the gas phase in terms of cubic meters
     * of liquid oil at surface conditions per cubic meter of gas at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE Scalar Rv() const
    {
        if constexpr (!enableDissolution) {
            return Scalar(0.0);
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
    OPM_HOST_DEVICE Scalar Rvw() const
    {
        if constexpr (enableVapwat) {
            return *Rvw_;
        } else {
            return Scalar(0.0);
        }
    }

    /*!
     * \brief Return the gas dissolution factor of water [m^3/m^3].
     *
     * I.e., the amount of gas which is present in the water phase in terms of cubic meters
     * of gas at surface conditions per cubic meter of water at surface
     * conditions. This method is specific to the black-oil model.
     */
    OPM_HOST_DEVICE Scalar Rsw() const
    {
        if constexpr (enableDissolutionInWater) {
            return *Rsw_;
        } else {
            return Scalar(0.0);
        }
    }

    /*!
     * \brief Return the concentration of salt in water
     */
    OPM_HOST_DEVICE Scalar saltConcentration() const
    {
        if constexpr (enableBrine) {
            return *saltConcentration_;
        } else {
            return Scalar(0.0);
        }
    }

    /*!
     * \brief Return the saturation of solid salt
     */
    OPM_HOST_DEVICE Scalar saltSaturation() const
    {
        if constexpr (enableSaltPrecipitation) {
            return *saltSaturation_;
        } else {
            return Scalar(0.0);
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
    OPM_HOST_DEVICE Scalar density(unsigned phaseIdx) const
    { return density_[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the specific enthalpy [J/kg] of a given fluid phase.
     *
     * If the EnableEnergy property is not set to true, this method will throw an
     * exception!
     */
    OPM_HOST_DEVICE const Scalar& enthalpy(unsigned phaseIdx) const
    { return (*enthalpy_)[canonicalToStoragePhaseIndex_(phaseIdx, fluidSystem())]; }

    /*!
     * \brief Return the specific internal energy [J/kg] of a given fluid phase.
     *
     * If the EnableEnergy property is not set to true, this method will throw an
     * exception!
     */
    OPM_HOST_DEVICE Scalar internalEnergy(unsigned phaseIdx) const
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
    OPM_HOST_DEVICE Scalar molarDensity(unsigned phaseIdx) const
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
    OPM_HOST_DEVICE Scalar molarVolume(unsigned phaseIdx) const
    { return 1.0/molarDensity(phaseIdx); }

    /*!
     * \brief Return the dynamic viscosity of a fluid phase [Pa s].
     */
    OPM_HOST_DEVICE Scalar viscosity(unsigned phaseIdx) const
    { return fluidSystem().viscosity(*this, phaseIdx, pvtRegionIdx_); }

    /*!
     * \brief Return the mass fraction of a component in a fluid phase [-].
     */
    OPM_HOST_DEVICE Scalar massFraction(unsigned phaseIdx, unsigned compIdx) const
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
    OPM_HOST_DEVICE Scalar moleFraction(unsigned phaseIdx, unsigned compIdx) const
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
    OPM_HOST_DEVICE Scalar molarity(unsigned phaseIdx, unsigned compIdx) const
    { return moleFraction(phaseIdx, compIdx)*molarDensity(phaseIdx); }

    /*!
     * \brief Return the partial molar density of a fluid phase [kg / mol].
     */
    OPM_HOST_DEVICE Scalar averageMolarMass(unsigned phaseIdx) const
    {
        Scalar result(0.0);
        for (unsigned compIdx = 0; compIdx < numComponents; ++ compIdx)
            result += fluidSystem().molarMass(compIdx, pvtRegionIdx_)*moleFraction(phaseIdx, compIdx);
        return result;
    }

    /*!
     * \brief Return the fugacity coefficient of a component in a fluid phase [-].
     */
    OPM_HOST_DEVICE Scalar fugacityCoefficient(unsigned phaseIdx, unsigned compIdx) const
    { return fluidSystem().fugacityCoefficient(*this, phaseIdx, compIdx, pvtRegionIdx_); }

    /*!
     * \brief Return the fugacity of a component in a fluid phase [Pa].
     */
    OPM_HOST_DEVICE Scalar fugacity(unsigned phaseIdx, unsigned compIdx) const
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
    bool phaseIsActive(int phaseIdx) const
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

    OPM_HOST_DEVICE FluidSystem const* fluidSystemPtr() const
    {
        if constexpr (fluidSystemIsStatic) {
            static FluidSystem instance;
            return &instance;
        } else {
            return fluidSystemPtr_;
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

    ConditionalStorage<storeTemperature, Scalar> temperature_{};
    ConditionalStorage<storeEnthalpy, std::array<Scalar, numStoragePhases> > enthalpy_{};
    Scalar totalSaturation_{};
    std::array<Scalar, numStoragePhases> pressure_{};
    std::array<Scalar, numStoragePhases> saturation_{};
    std::array<Scalar, numStoragePhases> invB_{};
    std::array<Scalar, numStoragePhases> density_{};
    ConditionalStorage<enableDissolution,Scalar> Rs_{};
    ConditionalStorage<enableDissolution, Scalar> Rv_{};
    ConditionalStorage<enableVapwat,Scalar> Rvw_{};
    ConditionalStorage<enableDissolutionInWater,Scalar> Rsw_{};
    ConditionalStorage<enableBrine, Scalar> saltConcentration_{};
    ConditionalStorage<enableSaltPrecipitation, Scalar> saltSaturation_{};

    unsigned short pvtRegionIdx_{};

    // If we have a non-static fluid system, we need to store a pointer
    // to it. Otherwise, we do not need to store anything.
    ConditionalStorage<!fluidSystemIsStatic, FluidSystem const*> fluidSystemPtr_;
};

} // namespace Opm

#endif
