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
 * \copydoc Opm::GasPvtMultiplexer
 */
#ifndef OPM_GAS_PVT_MULTIPLEXER_HPP
#define OPM_GAS_PVT_MULTIPLEXER_HPP

#include <opm/material/fluidsystems/blackoilpvt/Co2GasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DryGasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DryHumidGasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/GasPvtThermal.hpp>
#include <opm/material/fluidsystems/blackoilpvt/H2GasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WetGasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WetHumidGasPvt.hpp>

#include <functional>
#include <opm/common/utility/gpuDecorators.hpp>

#include <iostream>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

#if OPM_IS_COMPILING_WITH_GPU_COMPILER
// Testing whether hardcoding the PvtType supported on GPU helps
#define OPM_GAS_PVT_MULTIPLEXER_CALL(codeToCall, ...)                     \
    if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) { \
        auto& pvtImpl = getRealPvt<GasPvtApproach::Co2Gas>();             \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    } else {                                                              \
        auto& pvtImpl = realGasPvt_;                                      \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }

#else
#define OPM_GAS_PVT_MULTIPLEXER_CALL(codeToCall, ...)                     \
    switch (gasPvtApproach_) {                                            \
    case GasPvtApproach::DryGas: {                                        \
        auto& pvtImpl = getRealPvt<GasPvtApproach::DryGas>();             \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::DryHumidGas: {                                   \
        auto& pvtImpl = getRealPvt<GasPvtApproach::DryHumidGas>();        \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::WetHumidGas: {                                   \
        auto& pvtImpl = getRealPvt<GasPvtApproach::WetHumidGas>();        \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::WetGas: {                                        \
        auto& pvtImpl = getRealPvt<GasPvtApproach::WetGas>();             \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::ThermalGas: {                                    \
        auto& pvtImpl = getRealPvt<GasPvtApproach::ThermalGas>();         \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::Co2Gas: {                                        \
        auto& pvtImpl = getRealPvt<GasPvtApproach::Co2Gas>();             \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    case GasPvtApproach::H2Gas: {                                         \
        auto& pvtImpl = getRealPvt<GasPvtApproach::H2Gas>();              \
        codeToCall;                                                       \
        __VA_ARGS__;                                                      \
    }                                                                     \
    default:                                                              \
    case GasPvtApproach::NoGas:                                           \
        throw std::logic_error("Not implemented: Gas PVT of this deck!"); \
    }
#endif

enum class GasPvtApproach {
    NoGas,
    DryGas,
    DryHumidGas,
    WetHumidGas,
    WetGas,
    ThermalGas,
    Co2Gas,
    H2Gas
};

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas
 *        phase in the black-oil model.
 *
 * This is a multiplexer class which forwards all calls to the real implementation.
 *
 * Note that, since the main application for this class is the black oil fluid system,
 * the API exposed by this class is pretty specific to the assumptions made by the black
 * oil model.
 */
template <class Scalar, bool enableThermal = true, class ParamsContainer = std::vector<double>, class ContainerT = std::vector<Scalar>, template <class...> class PtrType = std::unique_ptr>
class GasPvtMultiplexer
{
public:

using ParamsT = CO2Tables<double, ParamsContainer>;
using UniqueVoidPtrWithDeleter =
        std::conditional_t<
            std::is_same_v<PtrType<void>, std::unique_ptr<void>>,
            std::unique_ptr<void, std::function<void(void*)>>,
            Co2GasPvt<Scalar, ParamsT, ContainerT>
        >;

    GasPvtMultiplexer()
        : gasPvtApproach_(GasPvtApproach::NoGas)
        , realGasPvt_(nullptr, [](void*){})
    {
    }

    GasPvtMultiplexer(GasPvtApproach approach, void* realGasPvt)
        : gasPvtApproach_(approach)
        , realGasPvt_(realGasPvt, [this](void* ptr){ deleter(ptr); })
    { }

    template<class ConcretePvt>
    GasPvtMultiplexer(GasPvtApproach approach, const ConcretePvt& realGasPvt)
    : gasPvtApproach_(approach)
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            realGasPvt_ = UniqueVoidPtrWithDeleter(new ConcretePvt(realGasPvt), [this](void* ptr){ deleter(ptr); });
        }
        else {
            realGasPvt_ = realGasPvt; //UniqueVoidPtrWithDeleter(new ConcretePvt(realGasPvt));
        }
    }

    template <class T = PtrType<void>, typename std::enable_if<!std::is_same_v<T, std::unique_ptr<void>>, int>::type = 0>
    GasPvtMultiplexer(GasPvtApproach approach, const Co2GasPvt<Scalar, ParamsT, ContainerT>& realGasPvt)
    : gasPvtApproach_(approach), realGasPvt_(realGasPvt)
    {
    }

    //template <class T = PtrType<void>, typename std::enable_if<std::is_same_v<T, std::unique_ptr<void>>, int>::type = 0>
    GasPvtMultiplexer(const GasPvtMultiplexer<Scalar, enableThermal, ParamsContainer, ContainerT, PtrType>& data)
    : gasPvtApproach_(data.gasPvtApproach_)
    , realGasPvt_(initializeCopyConstructor(data))
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            *this = data;
        }
    }

    ~GasPvtMultiplexer() = default;

    OPM_HOST_DEVICE bool mixingEnergy() const
    {
        return gasPvtApproach_ == GasPvtApproach::ThermalGas;
    }

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    OPM_HOST_DEVICE void setApproach(GasPvtApproach gasPvtAppr);

    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE unsigned numRegions() const {
        OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.numRegions());
    }

    OPM_HOST_DEVICE void setVapPars(const Scalar par1, const Scalar par2);

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE Scalar gasReferenceDensity(unsigned regionIdx) const {
        OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.gasReferenceDensity(regionIdx));
    }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rv,
                        const Evaluation& Rvw) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rv, Rvw)); }

    OPM_HOST_DEVICE Scalar hVap(unsigned regionIdx) const;

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& Rvw ) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rv, Rvw)); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas given a set of parameters.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& Rvw) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rv, Rvw)); }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas given a set of parameters.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedOilVaporizationFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              const Evaluation& maxOilSaturation) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedOilVaporizationFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation)); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedWaterVaporizationFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& saltConcentration) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedWaterVaporizationFactor(regionIdx, temperature, pressure, saltConcentration)); }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * \param Rv The surface volume of oil component dissolved in what will yield one cubic meter of gas at the surface [-]
     */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rv) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rv)); }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
      OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.diffusionCoefficient(temperature, pressure, compIdx));
    }

    /*!
     * \brief Returns the concrete approach for calculating the PVT relations.
     *
     * (This is only determined at runtime.)
     */
    OPM_HOST_DEVICE GasPvtApproach gasPvtApproach() const
    { return gasPvtApproach_; }


    // get the parameter object for the dry gas case
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::DryGas, DryGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<DryGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::DryGas, const DryGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const DryGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the dry humid gas case
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::DryHumidGas, DryHumidGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<DryHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::DryHumidGas, const DryHumidGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const DryHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the wet humid gas case
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::WetHumidGas, WetHumidGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<WetHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::WetHumidGas, const WetHumidGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const WetHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the wet gas case
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::WetGas, WetGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<WetGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::WetGas, const WetGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const WetGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the thermal gas case
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::ThermalGas, GasPvtThermal<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<GasPvtThermal<Scalar>* >(realGasPvt_.get());
    }
    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::ThermalGas, const GasPvtThermal<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const GasPvtThermal<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::Co2Gas, Co2GasPvt<Scalar, ParamsT, ContainerT> >::type& getRealPvt()
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            assert(gasPvtApproach() == approachV);
            return *static_cast<Co2GasPvt<Scalar, ParamsT, ContainerT>* >(realGasPvt_.get());
        } else {
            assert(gasPvtApproach() == approachV);
            return realGasPvt_;
        }
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::Co2Gas, const Co2GasPvt<Scalar, ParamsT, ContainerT> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const Co2GasPvt<Scalar, ParamsT, ContainerT>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::H2Gas, H2GasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<H2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == GasPvtApproach::H2Gas, const H2GasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const H2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    OPM_HOST_DEVICE const void* realGasPvt() const { return realGasPvt_.get(); }

    GasPvtMultiplexer<Scalar,enableThermal, ParamsContainer, ContainerT, PtrType>&
    operator=(const GasPvtMultiplexer<Scalar,enableThermal, ParamsContainer, ContainerT, PtrType>& data){
        gasPvtApproach_ = data.gasPvtApproach_;

        copyPointer(data.realGasPvt_);
        return *this;
    }

private:

    UniqueVoidPtrWithDeleter initializeCopyConstructor(
        const GasPvtMultiplexer<Scalar, enableThermal, ParamsContainer, ContainerT, PtrType>& data)
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            if (data.realGasPvt_.get() == nullptr) {
                if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
                    return UniqueVoidPtrWithDeleter(nullptr, [](void*){});
                } else {
                    return PtrType<void>(nullptr);
                }
            }
            switch (data.gasPvtApproach_) {
            case GasPvtApproach::DryGas:
                return copyPvt<DryGasPvt<Scalar>>(data.realGasPvt_);
            case GasPvtApproach::DryHumidGas:
                return copyPvt<DryHumidGasPvt<Scalar>>(data.realGasPvt_);
            case GasPvtApproach::WetHumidGas:
                return copyPvt<WetHumidGasPvt<Scalar>>(data.realGasPvt_);
            case GasPvtApproach::WetGas:
                return copyPvt<WetGasPvt<Scalar>>(data.realGasPvt_);
            case GasPvtApproach::ThermalGas:
                return copyPvt<GasPvtThermal<Scalar>>(data.realGasPvt_);
            case GasPvtApproach::Co2Gas:
                return copyPvt<Co2GasPvt<Scalar, ParamsT, ContainerT>>(data.realGasPvt_);
            case GasPvtApproach::H2Gas:
                return copyPvt<H2GasPvt<Scalar>>(data.realGasPvt_);
            default:
                if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
                    return UniqueVoidPtrWithDeleter(nullptr, [](void*){});
                } else {
                    return PtrType<void>(nullptr); // Assuming default constructor works
                }
            }
        }
        else {
            return data.realGasPvt_;
        }
    }

    void copyPointer(const UniqueVoidPtrWithDeleter& pointer) {
        switch (gasPvtApproach_) {
            case GasPvtApproach::DryGas:
                realGasPvt_ = copyPvt<DryGasPvt<Scalar>>(pointer);
                break;
            case GasPvtApproach::DryHumidGas:
                realGasPvt_ = copyPvt<DryHumidGasPvt<Scalar>>(pointer);
                break;
            case GasPvtApproach::WetHumidGas:
                realGasPvt_ = copyPvt<WetHumidGasPvt<Scalar>>(pointer);
                break;
            case GasPvtApproach::WetGas:
                realGasPvt_ = copyPvt<WetGasPvt<Scalar>>(pointer);
                break;
            case GasPvtApproach::ThermalGas:
                realGasPvt_ = copyPvt<GasPvtThermal<Scalar>>(pointer);
                break;
            case GasPvtApproach::Co2Gas:
                realGasPvt_ = copyPvt<Co2GasPvt<Scalar, ParamsT, ContainerT>>(pointer);
                break;
            case GasPvtApproach::H2Gas:
                realGasPvt_ = copyPvt<H2GasPvt<Scalar>>(pointer);
                break;
            default:
                break;
            }
    }

    template <class ConcreteGasPvt> UniqueVoidPtrWithDeleter makeGasPvt();

    template <class ConcretePvt> UniqueVoidPtrWithDeleter copyPvt(const UniqueVoidPtrWithDeleter& sourcePvt){
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            return UniqueVoidPtrWithDeleter(
                new ConcretePvt(*static_cast<const ConcretePvt*>(sourcePvt.get())),
                [this](void* ptr) { deleter(ptr); }
            );
        }
        else {
            return realGasPvt_;
        }
    }

    void deleter(void* ptr){
        switch (gasPvtApproach_) {
            case GasPvtApproach::DryGas: {
                delete static_cast<DryGasPvt<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::DryHumidGas: {
                delete static_cast<DryHumidGasPvt<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::WetHumidGas: {
                delete static_cast<WetHumidGasPvt<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::WetGas: {
                delete static_cast<WetGasPvt<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::ThermalGas: {
                delete static_cast<GasPvtThermal<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::Co2Gas: {
                delete static_cast<Co2GasPvt<Scalar, ParamsT, ContainerT>*>(ptr);
                break;
            }
            case GasPvtApproach::H2Gas: {
                delete static_cast<H2GasPvt<Scalar>*>(ptr);
                break;
            }
            case GasPvtApproach::NoGas:
                break;
        }
    }

    GasPvtApproach gasPvtApproach_{GasPvtApproach::NoGas};
    UniqueVoidPtrWithDeleter realGasPvt_;
};

namespace gpuistl{
    template<class GPUContainerDouble, class GPUContainerScalar, class Scalar>
    GasPvtMultiplexer<Scalar, true, GPUContainerDouble, GPUContainerScalar>
    copy_to_gpu(GasPvtMultiplexer<Scalar, true, std::vector<double>, std::vector<Scalar>>& gasMultiplexer)
    {
        using Params = CO2Tables<Scalar, GPUContainerDouble>;

        assert(gasMultiplexer.gasPvtApproach() == GasPvtApproach::Co2Gas);

        return GasPvtMultiplexer<Scalar, true, GPUContainerDouble, GPUContainerScalar>(
            GasPvtApproach::Co2Gas,
            copy_to_gpu<GPUContainerScalar, Params>(gasMultiplexer.template getRealPvt<GasPvtApproach::Co2Gas>())
        );
    }

    template <template <class> class ViewPtr, class ViewDouble, class ViewScalar, class GPUContainerDouble, class GPUContainerScalar, class Scalar>
    GasPvtMultiplexer<Scalar, true, ViewDouble, ViewScalar, ViewPtr>
    make_view(GasPvtMultiplexer<Scalar, true, GPUContainerDouble, GPUContainerScalar, std::unique_ptr>& gasMultiplexer)
    {
        using ParamsView = CO2Tables<Scalar, ViewDouble>;

        assert(gasMultiplexer.gasPvtApproach() == GasPvtApproach::Co2Gas);

        auto gpuPvtView = make_view<ViewScalar, ParamsView>(gasMultiplexer.template getRealPvt<GasPvtApproach::Co2Gas>());

        return GasPvtMultiplexer<Scalar, true, ViewDouble, ViewScalar, ViewPtr>(GasPvtApproach::Co2Gas, gpuPvtView);
    }
}
} // namespace Opm

#endif
