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

#include "ConstantCompressibilityOilPvt.hpp"
#include "DeadOilPvt.hpp"
#include "LiveOilPvt.hpp"
#include "OilPvtThermal.hpp"
#include "BrineCo2Pvt.hpp"
#include "BrineH2Pvt.hpp"

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

#define OPM_OIL_PVT_MULTIPLEXER_CALL(codeToCall)                                  \
    switch (approach_) {                                                          \
    case OilPvtApproach::ConstantCompressibilityOil: {                            \
        auto& pvtImpl = getRealPvt<OilPvtApproach::ConstantCompressibilityOil>(); \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
    case OilPvtApproach::DeadOil: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::DeadOil>();                    \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
    case OilPvtApproach::LiveOil: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::LiveOil>();                    \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
    case OilPvtApproach::ThermalOil: {                                            \
        auto& pvtImpl = getRealPvt<OilPvtApproach::ThermalOil>();                 \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
    case OilPvtApproach::BrineCo2: {                                              \
        auto& pvtImpl = getRealPvt<OilPvtApproach::BrineCo2>();                   \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
    case OilPvtApproach::BrineH2: {                                               \
        auto& pvtImpl = getRealPvt<OilPvtApproach::BrineH2>();                    \
        codeToCall;                                                               \
        break;                                                                    \
    }                                                                             \
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
    BrineH2
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
    {
        approach_ = OilPvtApproach::NoOil;
        realOilPvt_ = nullptr;
    }

    OilPvtMultiplexer(OilPvtApproach approach, void* realOilPvt)
        : approach_(approach)
        , realOilPvt_(realOilPvt)
    { }

    OilPvtMultiplexer(const OilPvtMultiplexer<Scalar,enableThermal>& data)
    {
        *this = data;
    }

    ~OilPvtMultiplexer()
    {
        switch (approach_) {
        case OilPvtApproach::LiveOil: {
            delete &getRealPvt<OilPvtApproach::LiveOil>();
            break;
        }
        case OilPvtApproach::DeadOil: {
            delete &getRealPvt<OilPvtApproach::DeadOil>();
            break;
        }
        case OilPvtApproach::ConstantCompressibilityOil: {
            delete &getRealPvt<OilPvtApproach::ConstantCompressibilityOil>();
            break;
        }
        case OilPvtApproach::ThermalOil: {
            delete &getRealPvt<OilPvtApproach::ThermalOil>();
            break;
        }
        case OilPvtApproach::BrineCo2: {
            delete &getRealPvt<OilPvtApproach::BrineCo2>();
            break;
        }
	    case OilPvtApproach::BrineH2: {
            delete &getRealPvt<OilPvtApproach::BrineH2>();
            break;
        }
        case OilPvtApproach::NoOil:
            break;
        }
    }

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL state.
     *
     * This method assumes that the deck features valid DENSITY and PVTO/PVDO/PVCDO keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT


    void initEnd()
    { OPM_OIL_PVT_MULTIPLEXER_CALL(pvtImpl.initEnd()); }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.numRegions()); return 1; }

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    const Scalar oilReferenceDensity(unsigned regionIdx) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.oilReferenceDensity(regionIdx)); return 700.; }

    /*!
     * \brief Returns the specific enthalpy [J/kg] oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rs)); return 0; }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rs)); return 0; }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure)); return 0; }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rs)); return 0; }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure)); return 0; }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure)); return 0; }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             const Evaluation& maxOilSaturation) const
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation)); return 0; }

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
    { OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rs)); return 0; }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
      OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.diffusionCoefficient(temperature, pressure, compIdx)); return 0;
    }

    void setApproach(OilPvtApproach appr)
    {
        switch (appr) {
        case OilPvtApproach::LiveOil:
            realOilPvt_ = new LiveOilPvt<Scalar>;
            break;

        case OilPvtApproach::DeadOil:
            realOilPvt_ = new DeadOilPvt<Scalar>;
            break;

        case OilPvtApproach::ConstantCompressibilityOil:
            realOilPvt_ = new ConstantCompressibilityOilPvt<Scalar>;
            break;

        case OilPvtApproach::ThermalOil:
            realOilPvt_ = new OilPvtThermal<Scalar>;
            break;

        case OilPvtApproach::BrineCo2:
            realOilPvt_ = new BrineCo2Pvt<Scalar>;
            break;

        case OilPvtApproach::BrineH2:
            realOilPvt_ = new BrineH2Pvt<Scalar>;
            break;

        case OilPvtApproach::NoOil:
            throw std::logic_error("Not implemented: Oil PVT of this deck!");
        }

        approach_ = appr;
    }

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

    OilPvtMultiplexer<Scalar,enableThermal>& operator=(const OilPvtMultiplexer<Scalar,enableThermal>& data)
    {
        approach_ = data.approach_;
        switch (approach_) {
        case OilPvtApproach::ConstantCompressibilityOil:
            realOilPvt_ = new ConstantCompressibilityOilPvt<Scalar>(*static_cast<const ConstantCompressibilityOilPvt<Scalar>*>(data.realOilPvt_));
            break;
        case OilPvtApproach::DeadOil:
            realOilPvt_ = new DeadOilPvt<Scalar>(*static_cast<const DeadOilPvt<Scalar>*>(data.realOilPvt_));
            break;
        case OilPvtApproach::LiveOil:
            realOilPvt_ = new LiveOilPvt<Scalar>(*static_cast<const LiveOilPvt<Scalar>*>(data.realOilPvt_));
            break;
        case OilPvtApproach::ThermalOil:
            realOilPvt_ = new OilPvtThermal<Scalar>(*static_cast<const OilPvtThermal<Scalar>*>(data.realOilPvt_));
            break;
        case OilPvtApproach::BrineCo2:
            realOilPvt_ = new BrineCo2Pvt<Scalar>(*static_cast<const BrineCo2Pvt<Scalar>*>(data.realOilPvt_));
            break;
	    case OilPvtApproach::BrineH2:
            realOilPvt_ = new BrineH2Pvt<Scalar>(*static_cast<const BrineH2Pvt<Scalar>*>(data.realOilPvt_));
            break;
        default:
            break;
        }

        return *this;
    }

private:
    OilPvtApproach approach_;
    void* realOilPvt_;
};

} // namespace Opm

#endif
