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

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/fluidsystems/blackoilpvt/GasPvtThermal.hpp>
#include <opm/material/fluidsystems/blackoilpvt/PvtEnums.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#endif

#include <variant>

namespace Opm {

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
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        if (!eclState.runspec().phases().active(Phase::GAS))
            return;

        if (!eclState.runspec().co2Storage() &&
            enableThermal && eclState.getSimulationConfig().isThermal()) {
            setApproach(GasPvtApproach::ThermalGas);
        } else {
            setApproach(GasPvtThermal<Scalar>::chooseApproach(eclState));
        }

        std::visit(VisitorOverloadSet{[&](auto& pvt)
                                      {
                                          pvt.initFromState(eclState, schedule);
                                      }, monoHandler_}, gasPvt_);
    }
#endif // HAVE_ECL_INPUT

    void setApproach(GasPvtApproach gasPvtAppr)
    {
        if (gasPvtAppr == GasPvtApproach::ThermalGas)
            gasPvt_ = GasPvtThermal<Scalar>{};
        else {
            auto pvt = GasPvtThermal<Scalar>::initialize(gasPvtAppr);
            std::visit([&](auto& param)
                       {
                           gasPvt_ = param;
                       }, pvt);
        }

        gasPvtApproach_ = gasPvtAppr;
    }

    void initEnd()
    {
        std::visit(VisitorOverloadSet{[](auto& pvt) { pvt.initEnd(); }, monoHandler_}, gasPvt_);
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
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    Scalar gasReferenceDensity(unsigned regionIdx) const
    {
        Scalar result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.gasReferenceDensity(regionIdx);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation& pressure,
                              const Evaluation& Rv) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.internalEnergy(regionIdx,
                                                                      temperature,
                                                                      pressure,
                                                                      Rv);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& Rvw) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.viscosity(regionIdx,
                                                                 temperature,
                                                                 pressure,
                                                                 Rv, Rvw);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas given a set of parameters.
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
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& Rvw) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.inverseFormationVolumeFactor(regionIdx,
                                                                                    temperature,
                                                                                    pressure,
                                                                                    Rv, Rvw);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas given a set of parameters.
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
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedOilVaporizationFactor(regionIdx,
                                                                                      temperature,
                                                                                      pressure);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of oil saturated gas.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              const Evaluation& maxOilSaturation) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedOilVaporizationFactor(regionIdx,
                                                                                      temperature,
                                                                                      pressure,
                                                                                      oilSaturation,
                                                                                      maxOilSaturation);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                                const Evaluation& temperature,
                                                const Evaluation& pressure) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedWaterVaporizationFactor(regionIdx,
                                                                                        temperature,
                                                                                        pressure);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                                const Evaluation& temperature,
                                                const Evaluation& pressure,
                                                const Evaluation& saltConcentration) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedWaterVaporizationFactor(regionIdx,
                                                                                        temperature,
                                                                                        pressure,
                                                                                        saltConcentration);
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * \param Rv The surface volume of oil component dissolved in what will yield one cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rv) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturationPressure(regionIdx,
                                                                          temperature,
                                                                          Rv);
                                      }, monoHandler_}, gasPvt_);
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
                                      }, monoHandler_}, gasPvt_);
        return result;
    }

    /*!
     * \brief Returns the concrete approach for calculating the PVT relations.
     *
     * (This is only determined at runtime.)
     */
    GasPvtApproach gasPvtApproach() const
    { return gasPvtApproach_; }

    //! \brief Apply a visitor to the concrete pvt implementation.
    template<class Function>
    void visit(Function f)
    {
        std::visit(VisitorOverloadSet{f, monoHandler_, [](auto&) {}}, gasPvt_);
    }

private:
    GasPvtApproach gasPvtApproach_ = GasPvtApproach::NoGas;
    std::variant<std::monostate,
                 DryGasPvt<Scalar>,
                 DryHumidGasPvt<Scalar>,
                 WetHumidGasPvt<Scalar>,
                 WetGasPvt<Scalar>,
                 GasPvtThermal<Scalar>,
                 Co2GasPvt<Scalar>> gasPvt_;

    MonoThrowHandler<std::logic_error>
    monoHandler_{"Not implemented: Gas PVT of this deck!"}; // mono state handler
};

} // namespace Opm

#endif
