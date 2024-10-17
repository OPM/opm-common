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
 * \copydoc Opm::CO2
 */
#ifndef OPM_CO2_HPP
#define OPM_CO2_HPP
#include <opm/common/TimingMacros.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <opm/material/Constants.hpp>
#include <opm/material/IdealGas.hpp>
#include <opm/material/components/Component.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/components/CO2Tables.hpp>

#include <cmath>
#include <string_view>

namespace Opm {

/*!
 * \brief A class for the CO2 fluid properties
 *
 * Under reservoir conditions, CO2 is typically in supercritical
 * state. These properties can be provided in tabulated form, which is
 * necessary for this component. The template is used by the
 * fluidsystem \c BrineCO2FluidSystem. If thermodynamic precision
 * is not a top priority, the much simpler component \c Opm::SimpleCO2 can be
 * used instead
 */
template <class Scalar, class ParamsT = Opm::CO2Tables>
class CO2 : public Component<Scalar, CO2<Scalar>>
{
    static constexpr Scalar R = Constants<Scalar>::R;

public:
    using Params = ParamsT;

    static constexpr const Scalar brineSalinity = 1.000000000000000e-01;

    /*!
     * \brief A human readable name for the CO2.
     */
    static std::string_view name()
    { return "CO2"; }

    /*!
     * \brief The mass in [kg] of one mole of CO2.
     */
    static Scalar molarMass()
    { return 44e-3; }

    /*!
     * \brief Returns the critical temperature [K] of CO2
     */
    static Scalar criticalTemperature()
    { return 273.15 + 30.95; /* [K] */ }

    /*!
     * \brief Returns the critical pressure [Pa] of CO2
     */
    static Scalar criticalPressure()
    { return 73.8e5; /* [N/m^2] */ }

    /*!
     * \brief Returns the temperature [K]at CO2's triple point.
     */
    static Scalar tripleTemperature()
    { return 273.15 - 56.35; /* [K] */ }

    /*!
     * \brief Returns the pressure [Pa] at CO2's triple point.
     */
    static Scalar triplePressure()
    { return 5.11e5; /* [N/m^2] */ }

    /*!
     * \brief Returns the pressure [Pa] at CO2's triple point.
     */
//    static Scalar minTabulatedPressure()
//    { return tabulatedEnthalpy.minPress(); /* [N/m^2] */ }

    /*!
     * \brief Returns the pressure [Pa] at CO2's triple point.
     */
//    static Scalar maxTabulatedPressure()
//    { return tabulatedEnthalpy.maxPress(); /* [N/m^2] */ }

    /*!
     * \brief Returns the pressure [Pa] at CO2's triple point.
     */
//    static Scalar minTabulatedTemperature()
//    { return tabulatedEnthalpy.minTemp(); /* [N/m^2] */ }

    /*!
     * \brief Returns the pressure [Pa] at CO2's triple point.
     */
//    static Scalar maxTabulatedTemperature()
//    { return tabulatedEnthalpy.maxTemp(); /* [N/m^2] */ }

    /*!
     * \brief The vapor pressure in [N/m^2] of pure CO2
     *        at a given temperature.
     *
     * See:
     *
     * R. Span and W. Wagner: A New Equation of State for Carbon
     * Dioxide Covering the Fluid Region from the Triple‐Point
     * Temperature to 1100 K at Pressures up to 800 MPa. Journal of
     * Physical and Chemical Reference Data, 25 (6), pp. 1509-1596,
     * 1996
     */
    template <class Evaluation>
    static Evaluation vaporPressure(const Evaluation& T)
    {
        static constexpr Scalar a[4] =
            { -7.0602087, 1.9391218, -1.6463597, -3.2995634 };
        static constexpr Scalar t[4] =
            { 1.0, 1.5, 2.0, 4.0 };

        // this is on page 1524 of the reference
        Evaluation exponent = 0;
        Evaluation Tred = T/criticalTemperature();
        for (int i = 0; i < 4; ++i)
            exponent += a[i]*pow(1 - Tred, t[i]);
        exponent *= 1.0/Tred;

        return exp(exponent)*criticalPressure();
    }


    /*!
     * \brief Returns true iff the gas phase is assumed to be compressible
     */
    static bool gasIsCompressible()
    { return true; }

    /*!
     * \brief Returns true iff the gas phase is assumed to be ideal
     */
    static bool gasIsIdeal()
    { return false; }

    /*!
     * \brief Specific enthalpy of gaseous CO2 [J/kg].
     */
    template <class Evaluation>
    static Evaluation gasEnthalpy(const Params& params,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure,
                                  bool extrapolate = false)
    {
        return params.tabulatedEnthalpy.eval(temperature, pressure, extrapolate);
    }

    /*!
     * \brief Specific enthalpy of gaseous CO2 [J/kg].
     * this is only here so that the API is valid
     */
    template <class Evaluation>
    static Evaluation gasEnthalpy([[maybe_unused]] const Evaluation& temperature,
                                  [[maybe_unused]] const Evaluation& pressure,
                                  [[maybe_unused]] bool extrapolate = false)
    {
        OPM_THROW(std::runtime_error, "This function should not be called");
    }

    /*!
     * \brief Specific internal energy of CO2 [J/kg].
     */
    template <class Evaluation>
    static Evaluation gasInternalEnergy(const Params& params,
                                        const Evaluation& temperature,
                                        const Evaluation& pressure,
                                        bool extrapolate = false)
    {
        const Evaluation h = gasEnthalpy(params, temperature, pressure, extrapolate);
        const Evaluation rho = gasDensity(params, temperature, pressure, extrapolate);

        return h - (pressure / rho);
    }

    /*!
     * \brief Specific internal energy of CO2 [J/kg].
     * this is only here so that the API is valid
     */
    template <class Evaluation>
    static Evaluation gasInternalEnergy([[maybe_unused]] const Evaluation& temperature,
                                        [[maybe_unused]] const Evaluation& pressure,
                                        [[maybe_unused]] bool extrapolate = false)
    {
        OPM_THROW(std::runtime_error, "This function should not be called");
    }

    /*!
     * \brief The density of CO2 at a given pressure and temperature [kg/m^3].
     */
    template <class Evaluation>
    static Evaluation gasDensity(const Params& params,
                                 const Evaluation& temperature,
                                 const Evaluation& pressure,
                                 bool extrapolate = false)
    {
        return params.tabulatedDensity.eval(temperature, pressure, extrapolate);
    }

    /*!
     * \brief The density of CO2 at a given pressure and temperature [kg/m^3].
     * this is only here so that the API is valid
     */
    template <class Evaluation>
    static Evaluation gasDensity([[maybe_unused]] const Evaluation& temperature,
                                 [[maybe_unused]] const Evaluation& pressure,
                                 [[maybe_unused]] bool extrapolate = false)
    {
        OPM_THROW(std::runtime_error, "This function should not be called");
    }

    /*!
     * \brief The dynamic viscosity [Pa s] of CO2.
     *
     * Equations given in: - Vesovic et al., 1990
     *                        - Fenhour etl al., 1998
     */
    template <class Evaluation>
    static Evaluation gasViscosity(const Params& params,
                                   Evaluation temperature,
                                   const Evaluation& pressure,
                                   bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
        constexpr Scalar a0 = 0.235156;
        constexpr Scalar a1 = -0.491266;
        constexpr Scalar a2 = 5.211155e-2;
        constexpr Scalar a3 = 5.347906e-2;
        constexpr Scalar a4 = -1.537102e-2;

        constexpr Scalar d11 = 0.4071119e-2;
        constexpr Scalar d21 = 0.7198037e-4;
        constexpr Scalar d64 = 0.2411697e-16;
        constexpr Scalar d81 = 0.2971072e-22;
        constexpr Scalar d82 = -0.1627888e-22;

        constexpr Scalar ESP = 251.196;

        if(temperature < 275.) // regularization
            temperature = 275.0;
        Evaluation TStar = temperature/ESP;

        // mu0: viscosity in zero-density limit
        const Evaluation logTStar = log(TStar);
        Evaluation SigmaStar = exp(a0 + logTStar*(a1 + logTStar*(a2 + logTStar*(a3 + logTStar*a4))));

        Evaluation mu0 = 1.00697*sqrt(temperature) / SigmaStar;

        const Evaluation rho = gasDensity(params, temperature, pressure, extrapolate); // CO2 mass density [kg/m^3]

        // dmu : excess viscosity at elevated density
        Evaluation dmu =
            d11*rho
            + d21*rho*rho
            + d64*pow(rho, 6.0)/(TStar*TStar*TStar)
            + d81*pow(rho, 8.0)
            + d82*pow(rho, 8.0)/TStar;

        return (mu0 + dmu)/1.0e6; // conversion to [Pa s]
    }

    /*!
     * \brief The dynamic viscosity [Pa s] of CO2.
     *
     * This is only here so that the API is valid
     */
    template <class Evaluation>
    static Evaluation gasViscosity([[maybe_unused]] Evaluation temperature,
                                   [[maybe_unused]] const Evaluation& pressure,
                                   [[maybe_unused]] bool extrapolate = false)
    {
        OPM_THROW(std::runtime_error, "This function should not be called");
    }

    /*!
     * \brief Specific isobaric heat capacity of the component [J/kg]
     *        as a liquid.
     *
     * This function uses the fact that heat capacity is the partial
     * derivative of enthalpy function with respect to temperature.
     *
     * \param temperature Temperature of component \f$\mathrm{[K]}\f$
     * \param pressure Pressure of component \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasHeatCapacity(const Params& params, const Evaluation& temperature, const Evaluation& pressure)
    {
        OPM_TIMEFUNCTION_LOCAL();
        constexpr Scalar eps = 1e-6;
        // NB!! should be changed to using the derivative from the table (if piecwise linear double derivate is zero).
        // use central differences here because one-sided methods do
        // not come with a performance improvement. (central ones are
        // more accurate, though...)
        const Evaluation h1 = gasEnthalpy(params, temperature - eps, pressure);
        const Evaluation h2 = gasEnthalpy(params, temperature + eps, pressure);

        return (h2 - h1) / (2*eps) ;
    }

    /*!
     * \brief Specific isobaric heat capacity of the component [J/kg]
     *        as a liquid.
     * This is only here so that the API is valid
     */
    template <class Evaluation>
    static Evaluation gasHeatCapacity([[maybe_unused]] const Evaluation& temperature, [[maybe_unused]] const Evaluation& pressure)
    {
        OPM_THROW(std::runtime_error, "This function should not be called");
    }
};

} // namespace Opm

#endif
