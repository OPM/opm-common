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
 * \copydoc Opm:H2
 * \brief Properties of pure molecular hydrogen \f$H_2\f$.
 */
#ifndef OPM_H2_HPP
#define OPM_H2_HPP

#include <opm/material/IdealGas.hpp>
#include <opm/material/components/Component.hpp>

#include <opm/material/densead/Math.hpp>

#include <cmath>

namespace Opm {

/*!
 * \ingroup Components
 * \brief Properties of pure molecular hydrogen \f$H_2\f$.
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class H2 : public Component<Scalar, H2<Scalar> >
{
    using IdealGas = Opm::IdealGas<Scalar>;

public:
    /*!
     * \brief A human readable name for the \f$H_2\f$.
     */
    static std::string name()
    { return "H2"; }

    /*!
     * \brief The molar mass in \f$\mathrm{[kg/mol]}\f$ of molecular hydrogen.
     */
    static constexpr Scalar molarMass()
    { return 2.01588e-3; }

    /*!
     * \brief Returns the critical temperature \f$\mathrm{[K]}\f$ of molecular hydrogen.
     */
    static Scalar criticalTemperature()
    { return 33.2; /* [K] */ }

    /*!
     * \brief Returns the critical pressure \f$\mathrm{[Pa]}\f$ of molecular hydrogen.
     */
    static Scalar criticalPressure()
    { return 13.0e5; /* [N/m^2] */ }

    /*!
     * \brief Returns the critical density \f$\mathrm{[mol/cm^3]}\f$ of molecular hydrogen.
     */
    static Scalar criticalDensity()
    { return 15.508e-3; /* [mol/cm^3] */ }

    /*!
     * \brief Returns the temperature \f$\mathrm{[K]}\f$ at molecular hydrogen's triple point.
     */
    static Scalar tripleTemperature()
    { return 14.0; /* [K] */ }

    /*!
     * \brief The vapor pressure in \f$\mathrm{[Pa]}\f$ of pure molecular hydrogen
     *        at a given temperature.
     *
     *\param temperature temperature of component in \f$\mathrm{[K]}\f$
     *
     * Taken from:
     *
     * See: R. Reid, et al. (1987, pp 208-209, 669) \cite reid1987
     *
     * \todo implement the Gomez-Thodos approach...
     */
    template <class Evaluation>
    static Evaluation vaporPressure(Evaluation temperature)
    {
        if (temperature > criticalTemperature())
            return criticalPressure();
        if (temperature < tripleTemperature())
            return 0; // H2 is solid: We don't take sublimation into
                      // account

        // antoine equation
        const Scalar A = -7.76451;
        const Scalar B = 1.45838;
        const Scalar C = -2.77580;

        return 1e5 * exp(A - B/(temperature + C));
    }

    /*!
     * \brief The density \f$\mathrm{[kg/m^3]}\f$ of \f$H_2\f$ at a given pressure and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasDensity(Evaluation temperature, Evaluation pressure)
    {
        // Assume an ideal gas
        return IdealGas::density(Evaluation(molarMass()), temperature, pressure);
    }

    /*!
     * \brief The molar density of \f$H_2\f$ in \f$\mathrm{[mol/m^3]}\f$,
     *   depending on pressure and temperature.
     * \param temperature The temperature of the gas
     * \param pressure The pressure of the gas
     */
    template <class Evaluation>
    static Evaluation gasMolarDensity(Evaluation temperature, Evaluation pressure)
    { return IdealGas::molarDensity(temperature, pressure); }

    /*!
     * \brief Returns true if the gas phase is assumed to be compressible
     */
    static constexpr bool gasIsCompressible()
    { return true; }

    /*!
     * \brief Returns true if the gas phase is assumed to be ideal
     */
    static constexpr bool gasIsIdeal()
    { return true; }

    /*!
     * \brief The pressure of gaseous \f$H_2\f$ in \f$\mathrm{[Pa]}\f$ at a given density and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param density density of component in \f$\mathrm{[kg/m^3]}\f$
     */
    template <class Evaluation>
    static Evaluation gasPressure(Evaluation temperature, Evaluation density)
    {
        // Assume an ideal gas
        return IdealGas::pressure(temperature, density/molarMass());
    }

    /*!
    * \brief Specific internal energy of H2 [J/kg].
    */
    template <class Evaluation>
    static Evaluation gasInternalEnergy(const Evaluation& temperature,
                                        const Evaluation& pressure)
    {
        const Evaluation& h = gasEnthalpy(temperature, pressure);
        const Evaluation& rho = gasDensity(temperature, pressure);

        return h - (pressure / rho);
    }

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of \f$H_2\f$ at a given pressure and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     *
     * See:
     *
     * See: R. Reid, et al.: The Properties of Gases and Liquids,
     * 4th edition, McGraw-Hill, 1987, pp 396-397,
     * 5th edition, McGraw-Hill, 2001  pp 9.7-9.8 (omega and V_c taken from p. A.19)
     *
     */
    template <class Evaluation>
    static Evaluation gasViscosity(const Evaluation& temperature, const Evaluation& /*pressure*/)
    {
        const Scalar Tc = criticalTemperature();
        const Scalar Vc = 64.2; // critical specific volume [cm^3/mol]
        const Scalar omega = -0.217; // accentric factor
        const Scalar M = molarMass() * 1e3; // molar mas [g/mol]
        const Scalar dipole = 0.0; // dipole moment [debye]

        Scalar mu_r4 = 131.3 * dipole / std::sqrt(Vc * Tc);
        mu_r4 *= mu_r4;
        mu_r4 *= mu_r4;

        Scalar Fc = 1 - 0.2756*omega + 0.059035*mu_r4;
        const Evaluation& Tstar = 1.2593 * temperature/Tc;
        const Evaluation& Omega_v =
            1.16145*pow(Tstar, -0.14874) +
            0.52487*exp(- 0.77320*Tstar) +
            2.16178*exp(- 2.43787*Tstar);
        const Evaluation& mu = 40.785*Fc*sqrt(M*temperature)/(std::pow(Vc, 2./3)*Omega_v);

        // convertion from micro poise to Pa s
        return mu/1e6 / 10;
    }

    /*!
     * \brief Specific enthalpy \f$\mathrm{[J/kg]}\f$ of pure hydrogen gas.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static const Evaluation gasEnthalpy(Evaluation temperature,
                                        Evaluation pressure)
    {
        return gasHeatCapacity(temperature, pressure) * temperature;
    }

    /*!
     * \brief Specific isobaric heat capacity \f$\mathrm{[J/(kg*K)]}\f$ of pure
     *        hydrogen gas.
     *
     * This is equivalent to the partial derivative of the specific
     * enthalpy to the temperature.
     * \param T temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     *
     * See: R. Reid, et al. (1987, pp 154, 657, 665) \cite reid1987
     */
    template <class Evaluation>
    static const Evaluation gasHeatCapacity(Evaluation T,
                                            Evaluation pressure)
    {
        // method of Joback
        const Scalar cpVapA = 27.14;
        const Scalar cpVapB = 9.273e-3;
        const Scalar cpVapC = -1.381e-5;
        const Scalar cpVapD = 7.645e-9;

        return
            1/molarMass()* // conversion from [J/(mol*K)] to [J/(kg*K)]
            (cpVapA + T*
              (cpVapB/2 + T*
                (cpVapC/3 + T*
                 (cpVapD/4))));
    }
};

} // end namespace Opm

#endif
