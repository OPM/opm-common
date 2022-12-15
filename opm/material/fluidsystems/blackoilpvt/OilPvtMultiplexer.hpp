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

#include <opm/common/utility/Visitor.hpp>

#include "ConstantCompressibilityOilPvt.hpp"
#include "DeadOilPvt.hpp"
#include "LiveOilPvt.hpp"
#include "OilPvtThermal.hpp"
#include "BrineCo2Pvt.hpp"
#include <opm/material/fluidsystems/blackoilpvt/PvtEnums.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#endif

#include <variant>

namespace Opm {

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
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL state.
     *
     * This method assumes that the deck features valid DENSITY and PVTO/PVDO/PVCDO keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        if (!eclState.runspec().phases().active(Phase::OIL))
            return;
        // TODO move the BrineCo2 approach to the waterPvtMultiplexer
        // when a proper gas-water simulator is supported
        if (eclState.runspec().co2Storage())
            setApproach(OilPvtApproach::BrineCo2);
        else if (enableThermal && eclState.getSimulationConfig().isThermal())
            setApproach(OilPvtApproach::ThermalOil);
        else if (!eclState.getTableManager().getPvcdoTable().empty())
            setApproach(OilPvtApproach::ConstantCompressibilityOil);
        else if (eclState.getTableManager().hasTables("PVDO"))
            setApproach(OilPvtApproach::DeadOil);
        else if (!eclState.getTableManager().getPvtoTables().empty())
            setApproach(OilPvtApproach::LiveOil);

        std::visit(VisitorOverloadSet{[&](auto& pvt)
                                     {
                                         pvt.initFromState(eclState, schedule);
                                     }, monoHandler_}, oilPvt_);
    }
#endif // HAVE_ECL_INPUT


    void initEnd()
    {
        std::visit(VisitorOverloadSet{[](auto& pvt)
                                        {
                                            pvt.initEnd();
                                        }, monoHandler_}, oilPvt_);
    }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    {
        unsigned result;
        std::visit(VisitorOverloadSet{[&result](const auto& pvt)
                                      {
                                          result = pvt.numRegions();
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    Scalar oilReferenceDensity(unsigned regionIdx) const
    {
        Scalar result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                         {
                                             result = pvt.oilReferenceDensity(regionIdx);
                                         }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the specific enthalpy [J/kg] oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rs) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.internalEnergy(regionIdx,
                                                                      temperature,
                                                                      pressure,
                                                                      Rs);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rs) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.viscosity(regionIdx,
                                                                 temperature,
                                                                 pressure,
                                                                 Rs);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedViscosity(regionIdx,
                                                                          temperature,
                                                                          pressure);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.inverseFormationVolumeFactor(regionIdx,
                                                                                    temperature,
                                                                                    pressure,
                                                                                    Rs);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedInverseFormationVolumeFactor(regionIdx,
                                                                                             temperature,
                                                                                             pressure);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedGasDissolutionFactor(regionIdx,
                                                                                     temperature,
                                                                                     pressure);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated oil.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             const Evaluation& maxOilSaturation) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedGasDissolutionFactor(regionIdx,
                                                                                     temperature,
                                                                                     pressure,
                                                                                     oilSaturation,
                                                                                     maxOilSaturation);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

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
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturationPressure(regionIdx,
                                                                          temperature,
                                                                          Rs);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.diffusionCoefficient(temperature,
                                                                            pressure,
                                                                            compIdx);
                                      }, monoHandler_}, oilPvt_);
        return result;
    }

    void setApproach(OilPvtApproach appr)
    {
        switch (appr) {
        case OilPvtApproach::LiveOil:
            oilPvt_ = LiveOilPvt<Scalar>{};
            break;

        case OilPvtApproach::DeadOil:
            oilPvt_ = DeadOilPvt<Scalar>{};
            break;

        case OilPvtApproach::ConstantCompressibilityOil:
            oilPvt_ = ConstantCompressibilityOilPvt<Scalar>{};
            break;

        case OilPvtApproach::ThermalOil:
            oilPvt_ = OilPvtThermal<Scalar>{};
            break;

        case OilPvtApproach::BrineCo2:
            oilPvt_ = BrineCo2Pvt<Scalar>{};
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

    //! \brief Apply a visitor to the concrete pvt implementation.
    template<class Function>
    void visit(Function f)
    {
        std::visit(VisitorOverloadSet{f, monoHandler_, [](auto&) {}}, oilPvt_);
    }

private:
    OilPvtApproach approach_ = OilPvtApproach::NoOil;
    std::variant<std::monostate,
                 LiveOilPvt<Scalar>,
                 DeadOilPvt<Scalar>,
                 ConstantCompressibilityOilPvt<Scalar>,
                 OilPvtThermal<Scalar>,
                 BrineCo2Pvt<Scalar>> oilPvt_;

    MonoThrowHandler<std::logic_error>
    monoHandler_{"Not implemented: Oil PVT of this deck!"}; // mono state handler
};

} // namespace Opm

#endif
