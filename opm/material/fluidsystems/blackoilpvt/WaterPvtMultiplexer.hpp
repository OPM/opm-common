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

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/fluidsystems/blackoilpvt/WaterPvtThermal.hpp>
#include <opm/material/fluidsystems/blackoilpvt/PvtEnums.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#endif

#include <variant>

namespace Opm {

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the water
 *        phase in the black-oil model.
 */
template <class Scalar, bool enableThermal = true, bool enableBrine = true>
class WaterPvtMultiplexer
{
public:
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        if (!eclState.runspec().phases().active(Phase::WATER))
            return;

        if (enableThermal && eclState.getSimulationConfig().isThermal()) {
            setApproach(WaterPvtApproach::ThermalWater);
        } else {
            setApproach(WaterPvtThermal<Scalar,enableBrine>::chooseApproach(eclState));
        }

        std::visit(VisitorOverloadSet{[&](auto& pvt)
                                      {
                                          pvt.initFromState(eclState, schedule);
                                      }, monoHandler_}, waterPvt_);
    }
#endif // HAVE_ECL_INPUT

    void initEnd()
    {
        std::visit(VisitorOverloadSet{[](auto& pvt)
                                        {
                                            pvt.initEnd();
                                        }, monoHandler_}, waterPvt_);
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
                                      }, monoHandler_}, waterPvt_);
        return result;
    }

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    Scalar waterReferenceDensity(unsigned regionIdx) const
    {
        Scalar result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                         {
                                             result = pvt.waterReferenceDensity(regionIdx);
                                         }, monoHandler_}, waterPvt_);
        return result;
    }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& saltconcentration) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.internalEnergy(regionIdx,
                                                                      temperature,
                                                                      pressure,
                                                                      saltconcentration);
                                      }, monoHandler_}, waterPvt_);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& saltconcentration) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.viscosity(regionIdx,
                                                                 temperature,
                                                                 pressure,
                                                                 saltconcentration);
                                      }, monoHandler_}, waterPvt_);
        return result;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& saltconcentration) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.viscosity(regionIdx,
                                                                 temperature,
                                                                 pressure,
                                                                 saltconcentration);
                                      }, monoHandler_}, waterPvt_);
        return result;
    }

    void setApproach(WaterPvtApproach appr)
    {
        if (appr == WaterPvtApproach::ThermalWater) {
            waterPvt_ = WaterPvtThermal<Scalar,enableBrine>{};
        } else  {
            auto pvt = WaterPvtThermal<Scalar,enableBrine>::initialize(appr);
            std::visit([&](const auto& param)
                       {
                           waterPvt_ = param;
                       }, pvt);
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

    //! \brief Apply a visitor to the concrete pvt implementation.
    template<class Function>
    void visit(Function f)
    {
        std::visit(VisitorOverloadSet{f, monoHandler_, [](auto&) {}}, waterPvt_);
    }

private:
    WaterPvtApproach approach_ = WaterPvtApproach::NoWater;
    std::variant<std::monostate,
                 ConstantCompressibilityBrinePvt<Scalar>,
                 ConstantCompressibilityWaterPvt<Scalar>,
                 WaterPvtThermal<Scalar,enableBrine>> waterPvt_;

    MonoThrowHandler<std::logic_error>
    monoHandler_{"Not implemented: Water PVT of this deck!"}; // mono state handler
};

} // namespace Opm

#endif
