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
 * \copydoc Opm::BrineDynamic
 */
#ifndef OPM_BRINEDYNAMIC_HPP
#define OPM_BRINEDYNAMIC_HPP

#include <opm/material/components/Component.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/common/utility/gpuDecorators.hpp>

#include <string_view>

namespace Opm {

/*!
 * \ingroup Components
 *
 * \brief A class for the brine fluid properties.
 *
 * \tparam Scalar The type used for scalar values
 * \tparam H2O Static polymorphism: the Brine class can access all properties of the H2O class
 */
template <class Scalar, class H2O>
class BrineDynamic : public Component<Scalar, BrineDynamic<Scalar, H2O> >
{
public:

    /*!
     * \copydoc Component::name
     */
    static std::string_view name()
    { return "Brine"; }

    /*!
     * \copydoc H2O::gasIsIdeal
     */
    OPM_HOST_DEVICE static bool gasIsIdeal()
    { return H2O::gasIsIdeal(); }

    /*!
     * \copydoc H2O::gasIsCompressible
     */
    OPM_HOST_DEVICE static bool gasIsCompressible()
    { return H2O::gasIsCompressible(); }

    /*!
     * \copydoc H2O::liquidIsCompressible
     */
    OPM_HOST_DEVICE static bool liquidIsCompressible()
    { return H2O::liquidIsCompressible(); }

    /*!
     * \copydoc Component::molarMass
     *
     * This assumes that the salt is pure NaCl.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation molarMass(const Evaluation& salinity)
    {
        const Scalar M1 = H2O::molarMass();
        const Evaluation X2 = salinity; // mass fraction of salt in brine
        return M1*mM_salt()/(mM_salt() + X2*(M1 - mM_salt()));
    }

    /*!
     * \copydoc H2O::criticalTemperature
     */
    OPM_HOST_DEVICE static Scalar criticalTemperature()
    { return H2O::criticalTemperature(); /* [K] */ }

    /*!
     * \copydoc H2O::criticalPressure
     */
    OPM_HOST_DEVICE static Scalar criticalPressure()
    { return H2O::criticalPressure(); /* [N/m^2] */ }

    /*!
     * \copydoc H2O::criticalVolume
     */
    OPM_HOST_DEVICE static Scalar criticalVolume()
    { return H2O::criticalVolume(); /* [m3/kmol] */ }

    /*!
     * \copydoc H20::acentricFactor
     */
    OPM_HOST_DEVICE static Scalar acentricFactor()
    { return H2O::acentricFactor(); }

    /*!
     * \copydoc H2O::tripleTemperature
     */
    OPM_HOST_DEVICE static Scalar tripleTemperature()
    { return H2O::tripleTemperature(); /* [K] */ }

    /*!
     * \copydoc H2O::triplePressure
     */
    OPM_HOST_DEVICE static Scalar triplePressure()
    { return H2O::triplePressure(); /* [N/m^2] */ }

    /*!
     * \brief The vapor pressure in \f$\mathrm{[Pa]}\f$ of pure water
     *        at a given temperature.
     *
     * See:
     *
     * IAPWS: "Revised Release on the IAPWS Industrial Formulation
     * 1997 for the Thermodynamic Properties of Water and Steam",
     * http://www.iapws.org/relguide/IF97-Rev.pdf
     *
     * \param T Absolute temperature of the system in \f$\mathrm{[K]}\f$
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation vaporPressure(const Evaluation& T)
    { return H2O::vaporPressure(T); /* [N/m^2] */ }

    /*!
     * \copydoc Component::gasEnthalpy
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasEnthalpy(const Evaluation& temperature,
                                  const Evaluation& pressure)
    { return H2O::gasEnthalpy(temperature, pressure); /* [J/kg] */ }

    /*!
     * \brief Specific enthalpy \f$\mathrm{[J/kg]}\f$ of the pure component in liquid.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity Salinity of component
     *
     *
     * Equations given in:
     * - Palliser & McKibbin 1997
     * - Michaelides 1981
     * - Daubert & Danner 1989
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidEnthalpy(const Evaluation& temperature,
                                     const Evaluation& pressure,
                                     const Evaluation& salinity)
    {
        // Numerical coefficents from Palliser and McKibbin
        static constexpr Scalar f[] = {
            2.63500e-1, 7.48368e-6, 1.44611e-6, -3.80860e-10
        };

        // Numerical coefficents from Michaelides for the enthalpy of brine
        static constexpr Scalar a[4][3] = {
            { -9633.6, -4080.0, +286.49 },
            { +166.58, +68.577, -4.6856 },
            { -0.90963, -0.36524, +0.249667e-1 },
            { +0.17965e-2, +0.71924e-3, -0.4900e-4 }
        };

        const Evaluation theta = temperature - 273.15;

        Evaluation S = salinity;
        const Evaluation S_lSAT =
            f[0]
            + f[1]*theta
            + f[2]*pow(theta, 2)
            + f[3]*pow(theta, 3);

        // Regularization
        if (S > S_lSAT)
            S = S_lSAT;

        const Evaluation hw = H2O::liquidEnthalpy(temperature, pressure)/1e3; // [kJ/kg]

        // From Daubert and Danner
        const Evaluation h_NaCl =
            (3.6710e4*temperature
             + (6.2770e1/2)*temperature*temperature
             - (6.6670e-2/3)*temperature*temperature*temperature
             + (2.8000e-5/4)*pow(temperature, 4.0))/58.44e3
            - 2.045698e+02; // [kJ/kg]

        const Evaluation m = S/(1-S)/58.44e-3;

        Evaluation d_h = 0;
        for (int i = 0; i<=3; ++i) {
            for (int j = 0; j <= 2; ++j) {
                d_h += a[i][j] * pow(theta, i) * pow(m, j);
            }
        }

        const Evaluation delta_h = 4.184/(1e3 + (58.44 * m))*d_h;

        // Enthalpy of brine
        const Evaluation h_ls = (1-S)*hw + S*h_NaCl + S*delta_h; // [kJ/kg]
        return h_ls*1e3; // convert to [J/kg]
    }


    /*!
     * \copydoc H2O::liquidHeatCapacity
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidHeatCapacity(const Evaluation& temperature,
                                         const Evaluation& pressure)
    {
        Scalar eps = scalarValue(temperature)*1e-8;
        return (liquidEnthalpy(temperature + eps, pressure) - liquidEnthalpy(temperature, pressure))/eps;
    }

    /*!
     * \copydoc H2O::gasHeatCapacity
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasHeatCapacity(const Evaluation& temperature,
                                      const Evaluation& pressure)
    { return H2O::gasHeatCapacity(temperature, pressure); }

    /*!
     * \copydoc H2O::gasInternalEnergy
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasInternalEnergy(const Evaluation& temperature,
                                        const Evaluation& pressure)
    {
        return
            gasEnthalpy(temperature, pressure) -
            pressure/gasDensity(temperature, pressure);
    }

    /*!
     * \copydoc H2O::liquidInternalEnergy
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidInternalEnergy(const Evaluation& temperature,
                                           const Evaluation& pressure)
    {
        return
            liquidEnthalpy(temperature, pressure) -
            pressure/liquidDensity(temperature, pressure);
    }

    /*!
     * \copydoc H2O::gasDensity
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasDensity(const Evaluation& temperature, const Evaluation& pressure)
    { return H2O::gasDensity(temperature, pressure); }

    /*!
     * \brief The density \f$\mathrm{[kg/m^3]}\f$ of the liquid component at a given pressure in \f$\mathrm{[Pa]}\f$ and temperature in \f$\mathrm{[K]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity Salinity of component
     * \param extrapolate True to use extrapolation
     *
     * Equations given in:
     * - Batzle & Wang (1992)
     * - cited by: Adams & Bachu in Geofluids (2002) 2, 257-271
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidDensity(const Evaluation& temperature, const Evaluation& pressure,
                                                    const Evaluation& salinity, bool extrapolate = false)
    {
        const Evaluation rhow = H2O::liquidDensity(temperature, pressure, extrapolate);
        return liquidDensity(temperature, pressure, salinity, rhow);
    }

    /*!
     * \copydoc Component::liquidDensity
     *
     * Equations given in:
     * - Batzle & Wang (1992)
     * - cited by: Adams & Bachu in Geofluids (2002) 2, 257-271
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidDensity(const Evaluation& temperature, const Evaluation& pressure, const Evaluation& salinity, const Evaluation& rhow)
    {
        Evaluation tempC = temperature - 273.15;
        Evaluation pMPa = pressure/1.0E6;
        return
            rhow +
            1000*salinity*(
                0.668 +
                0.44*salinity +
                1.0E-6*(
                    300*pMPa -
                    2400*pMPa*salinity +
                    tempC*(
                        80.0 +
                        3*tempC -
                        3300*salinity -
                        13*pMPa +
                        47*pMPa*salinity)));
    }

    /*!
     * \copydoc H2O::gasPressure
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasPressure(const Evaluation& temperature, const Evaluation& density)
    { return H2O::gasPressure(temperature, density); }

    /*!
     * \copydoc H2O::liquidPressure
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidPressure(const Evaluation& temperature, const Evaluation& density)
    {
        // We use the newton method for this. For the initial value we
        // assume the pressure to be 10% higher than the vapor
        // pressure
        Evaluation pressure = 1.1*vaporPressure(temperature);
        Scalar eps = scalarValue(pressure)*1e-7;

        Evaluation deltaP = pressure*2;
        for (int i = 0;
             i < 5
                 && std::abs(scalarValue(pressure)*1e-9) < std::abs(scalarValue(deltaP));
             ++i)
        {
            const Evaluation f = liquidDensity(temperature, pressure) - density;

            Evaluation df_dp = liquidDensity(temperature, pressure + eps);
            df_dp -= liquidDensity(temperature, pressure - eps);
            df_dp /= 2*eps;

            deltaP = - f/df_dp;

            pressure += deltaP;
        }

        return pressure;
    }

    /*!
     * \copydoc H2O::gasViscosity
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation gasViscosity(const Evaluation& temperature, const Evaluation& pressure)
    { return H2O::gasViscosity(temperature, pressure); }

    /*!
     * \brief The dynamic liquid viscosity \f$\mathrm{[Pa*s]}\f$ of the pure component.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param salinity Salinity of component
     *
     * Equation given in:
     * - Batzle & Wang (1992)
     * - cited by: Bachu & Adams (2002)
     *   "Equations of State for basin geofluids"
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidViscosity(const Evaluation& temperature,
                                                      const Evaluation& /*pressure*/,
                                                      const Evaluation& salinity)
    {
        Evaluation T_C = temperature - 273.15;
        if(temperature <= 275.) // regularization
            T_C = 275.0;

        Evaluation A = (0.42*Opm::pow((Opm::pow(salinity, 0.8)-0.17), 2) + 0.045)*pow(T_C, 0.8);
        Evaluation mu_brine = 0.1 + 0.333*salinity + (1.65+91.9*salinity*salinity*salinity)*exp(-A);

        return mu_brine/1000.0; // convert to [Pa s] (todo: check if correct cP->Pa s is times 10...)
    }

private:
    //Molar mass salt (assumes pure NaCl) [kg/mol]
    static constexpr Scalar mM_salt()
    {
        return 58.44e-3;
    }

};

} // namespace Opm

#endif
