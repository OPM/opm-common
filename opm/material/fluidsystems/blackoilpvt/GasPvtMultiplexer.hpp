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
namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

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
template <class Scalar, bool enableThermal = true>
class GasPvtMultiplexer
{
public:
    GasPvtMultiplexer()
        : gasPvtApproach_(GasPvtApproach::NoGas)
        , realGasPvt_(nullptr, [](void*){})
    {
    }

    GasPvtMultiplexer(GasPvtApproach approach, void* realGasPvt)
        : gasPvtApproach_(approach)
        , realGasPvt_(realGasPvt, [this](void* ptr){ deleter(ptr); })
    { }

    GasPvtMultiplexer(const GasPvtMultiplexer<Scalar,enableThermal>& data)
    {
        *this = data;
    }

    ~GasPvtMultiplexer() = default;

    bool mixingEnergy() const
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

    void setApproach(GasPvtApproach gasPvtAppr);

    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const;

    void setVapPars(const Scalar par1, const Scalar par2);

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    Scalar gasReferenceDensity(unsigned regionIdx);

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rv,
                        const Evaluation& Rvw) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rv, Rvw)); }

    Scalar hVap(unsigned regionIdx) const;

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation = Scalar>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& Rvw ) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rv, Rvw)); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas given a set of parameters.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation = Scalar>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& Rvw) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rv, Rvw)); }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas given a set of parameters.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedOilVaporizationFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              const Evaluation& maxOilSaturation) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedOilVaporizationFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation)); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedWaterVaporizationFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
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
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rv) const
    { OPM_GAS_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rv)); }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
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
    GasPvtApproach gasPvtApproach() const
    { return gasPvtApproach_; }

    // get the parameter object for the dry gas case
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::DryGas, DryGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<DryGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::DryGas, const DryGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const DryGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the dry humid gas case
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::DryHumidGas, DryHumidGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<DryHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::DryHumidGas, const DryHumidGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const DryHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the wet humid gas case
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::WetHumidGas, WetHumidGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<WetHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::WetHumidGas, const WetHumidGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const WetHumidGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the wet gas case
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::WetGas, WetGasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<WetGasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::WetGas, const WetGasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const WetGasPvt<Scalar>* >(realGasPvt_.get());
    }

    // get the parameter object for the thermal gas case
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::ThermalGas, GasPvtThermal<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<GasPvtThermal<Scalar>* >(realGasPvt_.get());
    }
    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::ThermalGas, const GasPvtThermal<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const GasPvtThermal<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::Co2Gas, Co2GasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<Co2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::Co2Gas, const Co2GasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const Co2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::H2Gas, H2GasPvt<Scalar> >::type& getRealPvt()
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<H2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    template <GasPvtApproach approachV>
    typename std::enable_if<approachV == GasPvtApproach::H2Gas, const H2GasPvt<Scalar> >::type& getRealPvt() const
    {
        assert(gasPvtApproach() == approachV);
        return *static_cast<const H2GasPvt<Scalar>* >(realGasPvt_.get());
    }

    const void* realGasPvt() const { return realGasPvt_.get(); }

    GasPvtMultiplexer<Scalar,enableThermal>&
    operator=(const GasPvtMultiplexer<Scalar,enableThermal>& data);

private:
    using UniqueVoidPtrWithDeleter = std::unique_ptr<void, std::function<void(void*)>>;

    template <class ConcreteGasPvt> UniqueVoidPtrWithDeleter makeGasPvt();

    template <class ConcretePvt> UniqueVoidPtrWithDeleter copyPvt(const UniqueVoidPtrWithDeleter&);

    GasPvtApproach gasPvtApproach_{GasPvtApproach::NoGas};
    UniqueVoidPtrWithDeleter realGasPvt_;

    void deleter(void* ptr);
};

} // namespace Opm

#endif
