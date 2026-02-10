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
 * \copydoc Opm::OilPvtMultiplexer
 */
#ifndef OPM_OIL_PVT_MULTIPLEXER_HPP
#define OPM_OIL_PVT_MULTIPLEXER_HPP

#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/BrineH2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DeadOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/LiveOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/OilPvtThermal.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantRsDeadOilPvt.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

#define OPM_OIL_PVT_MULTIPLEXER_CALL(codeToCall, ...)                             \
    switch (approach_) {                                                          \
    case OilPvtApproach::ConstantCompressibilityOil: {                            \
        auto& pvtImpl = getRealPvt<OilPvtApproach::ConstantCompressibilityOil>(); \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::DeadOil: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::DeadOil>();                    \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::LiveOil: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::LiveOil>();                    \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::ThermalOil: {                                            \
        auto& pvtImpl = getRealPvt<OilPvtApproach::ThermalOil>();                 \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::BrineCo2: {                                              \
        auto& pvtImpl = getRealPvt<OilPvtApproach::BrineCo2>();                   \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::BrineH2: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::BrineH2>();                    \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    case OilPvtApproach::ConstantRsDeadOil: {                                     \
        auto& pvtImpl = getRealPvt<OilPvtApproach::ConstantRsDeadOil>();          \
        codeToCall;                                                               \
        __VA_ARGS__;                                                              \
    }                                                                             \
    default:                                                                      \
    case OilPvtApproach::NoOil:                                                   \
        throw std::logic_error("Not implemented: Oil PVT of this deck!");         \
    }

enum class OilPvtApproach {
    NoOil,
    LiveOil,
    DeadOil,
    ConstantCompressibilityOil,
    ThermalOil,
    BrineCo2,
    BrineH2,
    ConstantRsDeadOil
};

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the oil
 *        phase in the black-oil model.
 *
 * This is the base class which which provides an API for the actual PVT implementation
 * classes which based on dynamic polymorphism. The rationale to use dynamic polymorphism
 * here is that this enables the fluid system to easily switch the used PVT relations for
 * the individual fluid phases.
 *
 * Note that, since the application for this class is the black-oil fluid system, the API
 * exposed by this class is pretty specific to the black-oil model.
 */
template <class Scalar, bool enableThermal = true>
class OilPvtMultiplexer
{
public:
    OilPvtMultiplexer()
        : approach_(OilPvtApproach::NoOil)
        , realOilPvt_(nullptr)
    {
    }

    OilPvtMultiplexer(OilPvtApproach approach, void* realOilPvt)
        : approach_(approach)
        , realOilPvt_(realOilPvt)
    { }

    OilPvtMultiplexer(const OilPvtMultiplexer<Scalar,enableThermal>& data)
    {
        *this = data;
    }

    ~OilPvtMultiplexer();

    bool mixingEnergy() const
    {
        return approach_ == OilPvtApproach::ThermalOil;
    }
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL state.
     *
     * This method assumes that the deck features valid DENSITY and PVTO/PVDO/PVCDO keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT


    void initEnd();

    bool isActive() const
    {
        return approach_ != OilPvtApproach::NoOil;
    }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const;

    void setVapPars(const Scalar par1, const Scalar par2);

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    Scalar oilReferenceDensity(unsigned regionIdx) const;

    /*!
     * \brief Returns the specific enthalpy [J/kg] oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rs)); }

    Scalar hVap(unsigned regionIdx) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.hVap(regionIdx)); }
    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rs)); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rs)); }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx)
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactorAndViscosity(fluidState, regionIdx)); }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure)); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             const Evaluation& maxOilSaturation) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation)); }

    /*!
     * \brief Returns the saturation pressure [Pa] of oil given the mass fraction of the
     *        gas component in the oil phase.
     *
     * Calling this method only makes sense for live oil. All other implementations of
     * the black-oil PVT interface will just throw an exception...
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rs)); }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
      OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.diffusionCoefficient(temperature, pressure, compIdx));
    }

    void setApproach(OilPvtApproach appr);

    /*!
     * \brief Returns the concrete approach for calculating the PVT relations.
     *
     * (This is only determined at runtime.)
     */
    OilPvtApproach approach() const
    { return approach_; }

    // get the concrete parameter object for the oil phase
    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::LiveOil, LiveOilPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<LiveOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::LiveOil, const LiveOilPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<LiveOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::DeadOil, DeadOilPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<DeadOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::DeadOil, const DeadOilPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<DeadOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ConstantCompressibilityOil, ConstantCompressibilityOilPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ConstantCompressibilityOil, const ConstantCompressibilityOilPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ThermalOil, OilPvtThermal<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<OilPvtThermal<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ThermalOil, const OilPvtThermal<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const OilPvtThermal<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::BrineCo2, BrineCo2Pvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<BrineCo2Pvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::BrineCo2, const BrineCo2Pvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineCo2Pvt<Scalar>* >(realOilPvt_);
    }

    const void* realOilPvt() const { return realOilPvt_; }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::BrineH2, BrineH2Pvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<BrineH2Pvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::BrineH2, const BrineH2Pvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineH2Pvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ConstantRsDeadOil, ConstantRsDeadOilPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantRsDeadOilPvt<Scalar>* >(realOilPvt_);
    }

    template <OilPvtApproach approachV>
    typename std::enable_if<approachV == OilPvtApproach::ConstantRsDeadOil, const ConstantRsDeadOilPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const ConstantRsDeadOilPvt<Scalar>* >(realOilPvt_);
    }

    OilPvtMultiplexer<Scalar,enableThermal>&
    operator=(const OilPvtMultiplexer<Scalar,enableThermal>& data);

private:
    OilPvtApproach approach_{OilPvtApproach::NoOil};
    void* realOilPvt_{nullptr};
};

} // namespace Opm

#endif
