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
 * \copydoc Opm::WaterPvtMultiplexer
 */
#ifndef OPM_WATER_PVT_MULTIPLEXER_HPP
#define OPM_WATER_PVT_MULTIPLEXER_HPP

#include "ConstantCompressibilityWaterPvt.hpp"
#include "ConstantCompressibilityBrinePvt.hpp"
#include "WaterPvtThermal.hpp"
#include "BrineCo2Pvt.hpp"
#include "BrineH2Pvt.hpp"

#define OPM_WATER_PVT_MULTIPLEXER_CALL(codeToCall)                      \
    switch (approach_) {                                                \
    case WaterPvtApproach::ConstantCompressibilityWater: {           \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ConstantCompressibilityWater>();  \
        codeToCall;                                                     \
        break;                                                          \
    }                                                                   \
    case WaterPvtApproach::ConstantCompressibilityBrine: {           \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ConstantCompressibilityBrine>();  \
        codeToCall;                                                     \
        break;                                                          \
    }                                                                   \
    case WaterPvtApproach::ThermalWater: {                           \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ThermalWater>();   \
        codeToCall;                                                     \
        break;                                                          \
    }                                                                    \
    case WaterPvtApproach::BrineCo2: {                                              \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::BrineCo2>();                   \
        codeToCall;                                                                  \
        break;                                                                       \
    }                                                                    \
    case WaterPvtApproach::BrineH2: {                                              \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::BrineH2>();                   \
        codeToCall;                                                                  \
        break;                                                                       \
    }                                                                    \
    case WaterPvtApproach::NoWater:                                  \
        throw std::logic_error("Not implemented: Water PVT of this deck!"); \
    }

namespace Opm {

enum class WaterPvtApproach {
    NoWater,
    ConstantCompressibilityBrine,
    ConstantCompressibilityWater,
    ThermalWater,
    BrineCo2,
    BrineH2
};

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the water
 *        phase in the black-oil model.
 */
template <class Scalar, bool enableThermal = true, bool enableBrine = true>
class WaterPvtMultiplexer
{
public:
    WaterPvtMultiplexer()
    {
        approach_ = WaterPvtApproach::NoWater;
        realWaterPvt_ = nullptr;
    }

    WaterPvtMultiplexer(WaterPvtApproach approach, void* realWaterPvt)
        : approach_(approach)
        , realWaterPvt_(realWaterPvt)
    { }

    WaterPvtMultiplexer(const WaterPvtMultiplexer<Scalar,enableThermal,enableBrine>& data)
    {
        *this = data;
    }

    ~WaterPvtMultiplexer()
    {
        switch (approach_) {
        case WaterPvtApproach::ConstantCompressibilityWater: {
            delete &getRealPvt<WaterPvtApproach::ConstantCompressibilityWater>();
            break;
        }
        case WaterPvtApproach::ConstantCompressibilityBrine: {
            delete &getRealPvt<WaterPvtApproach::ConstantCompressibilityBrine>();
            break;
        }
        case WaterPvtApproach::ThermalWater: {
            delete &getRealPvt<WaterPvtApproach::ThermalWater>();
            break;
        }
        case WaterPvtApproach::BrineCo2: {
            delete &getRealPvt<WaterPvtApproach::BrineCo2>();
            break;
        }
        case WaterPvtApproach::BrineH2: {
            delete &getRealPvt<WaterPvtApproach::BrineH2>();
            break;
        }
        case WaterPvtApproach::NoWater:
            break;
        }
    }
    bool mixingEnergy(){
        switch (approach_) {
        case WaterPvtApproach::ThermalWater: {
            return false;
            break;
        }
        default: {
            return false;
        }
        }
    }
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    void initEnd()
    { OPM_WATER_PVT_MULTIPLEXER_CALL(pvtImpl.initEnd()); }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.numRegions()); return 1; }

    void setVapPars(const Scalar par1, const Scalar par2)
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(pvtImpl.setVapPars(par1, par2));
    }

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    const Scalar waterReferenceDensity(unsigned regionIdx) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.waterReferenceDensity(regionIdx)); return 1000.; }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rsw,
                        const Evaluation& saltconcentration) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rsw, saltconcentration)); return 0; }

    Scalar hVap(unsigned regionIdx) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.hVap(regionIdx)); return 0; }
    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rsw,
                         const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rsw, saltconcentration));
        return 0;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure,
                                  const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure, saltconcentration));
        return 0;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rsw,
                                            const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rsw, saltconcentration));
        return 0;
    }

        /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure,
                                                     const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure, saltconcentration));
        return 0;
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated water.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure, saltconcentration));
        return 0;
    }

    /*!
     * \brief Returns the saturation pressure [Pa] of water given the mass fraction of the
     *        gas component in the water phase.
     *
     * Calling this method only makes sense for water that allows for dissolved gas. All other implementations of
     * the black-oil PVT interface will just throw an exception...
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rs,
                                  const Evaluation& saltconcentration) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rs, saltconcentration)); return 0; }


    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
      OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.diffusionCoefficient(temperature, pressure, compIdx));
      return 0;
    }

    void setApproach(WaterPvtApproach appr)
    {
        switch (appr) {
        case WaterPvtApproach::ConstantCompressibilityWater:
            realWaterPvt_ = new ConstantCompressibilityWaterPvt<Scalar>;
            break;

        case WaterPvtApproach::ConstantCompressibilityBrine:
            realWaterPvt_ = new ConstantCompressibilityBrinePvt<Scalar>;
            break;

        case WaterPvtApproach::ThermalWater:
            realWaterPvt_ = new WaterPvtThermal<Scalar, enableBrine>;
            break;

        case WaterPvtApproach::BrineCo2:
            realWaterPvt_ = new BrineCo2Pvt<Scalar>;
            break;

        case WaterPvtApproach::BrineH2:
            realWaterPvt_ = new BrineH2Pvt<Scalar>;
            break;

        case WaterPvtApproach::NoWater:
            throw std::logic_error("Not implemented: Water PVT of this deck!");
        }

        approach_ = appr;
    }

    /*!
     * \brief Returns the concrete approach for calculating the PVT relations.
     *
     * (This is only determined at runtime.)
     */
    WaterPvtApproach approach() const
    { return approach_; }

    // get the concrete parameter object for the water phase
    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityWater, ConstantCompressibilityWaterPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityWaterPvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityWater, const ConstantCompressibilityWaterPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityWaterPvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityBrine, ConstantCompressibilityBrinePvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityBrinePvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityBrine, const ConstantCompressibilityBrinePvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityBrinePvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ThermalWater, WaterPvtThermal<Scalar, enableBrine> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<WaterPvtThermal<Scalar, enableBrine>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::ThermalWater, const WaterPvtThermal<Scalar, enableBrine> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<WaterPvtThermal<Scalar, enableBrine>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::BrineCo2, BrineCo2Pvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<BrineCo2Pvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::BrineCo2, const BrineCo2Pvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineCo2Pvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::BrineH2, BrineH2Pvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<BrineH2Pvt<Scalar>* >(realWaterPvt_);
    }

    template <WaterPvtApproach approachV>
    typename std::enable_if<approachV == WaterPvtApproach::BrineH2, const BrineH2Pvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineH2Pvt<Scalar>* >(realWaterPvt_);
    }

    const void* realWaterPvt() const { return realWaterPvt_; }

    WaterPvtMultiplexer<Scalar,enableThermal,enableBrine>& operator=(const WaterPvtMultiplexer<Scalar,enableThermal,enableBrine>& data)
    {
        approach_ = data.approach_;
        switch (approach_) {
        case WaterPvtApproach::ConstantCompressibilityWater:
            realWaterPvt_ = new ConstantCompressibilityWaterPvt<Scalar>(*static_cast<const ConstantCompressibilityWaterPvt<Scalar>*>(data.realWaterPvt_));
            break;
        case WaterPvtApproach::ConstantCompressibilityBrine:
            realWaterPvt_ = new ConstantCompressibilityBrinePvt<Scalar>(*static_cast<const ConstantCompressibilityBrinePvt<Scalar>*>(data.realWaterPvt_));
            break;
        case WaterPvtApproach::ThermalWater:
            realWaterPvt_ = new WaterPvtThermal<Scalar, enableBrine>(*static_cast<const WaterPvtThermal<Scalar, enableBrine>*>(data.realWaterPvt_));
            break;
        case WaterPvtApproach::BrineCo2:
            realWaterPvt_ = new BrineCo2Pvt<Scalar>(*static_cast<const BrineCo2Pvt<Scalar>*>(data.realWaterPvt_));
            break;
        case WaterPvtApproach::BrineH2:
            realWaterPvt_ = new BrineH2Pvt<Scalar>(*static_cast<const BrineH2Pvt<Scalar>*>(data.realWaterPvt_));
            break;
        default:
            break;
        }

        return *this;
    }

private:
    WaterPvtApproach approach_;
    void* realWaterPvt_;
};

} // namespace Opm

#endif
