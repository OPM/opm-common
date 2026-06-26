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

#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/SaltArray.hpp>
#include <opm/material/components/Component.hpp>
#include <opm/material/components/CaIon.hpp>
#include <opm/material/components/ClIon.hpp>
#include <opm/material/components/KIon.hpp>
#include <opm/material/components/MgIon.hpp>
#include <opm/material/components/NaIon.hpp>
#include <opm/material/components/SO4Ion.hpp>
#include <opm/material/common/MathToolbox.hpp>

#include <span>
#include <string_view>

# include <fmt/format.h>

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
    static constexpr Scalar molalTolerance = 1e-6; ///< tolerance for molality calculations
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

    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    invAvgMolarMassFromMassFrac(const SaltArray<Evaluation, SaltMassFraction>& salinity)
    {
        const Scalar mH2O = H2O::molarMass();
        Evaluation s = 1.0 / mH2O;
        for (std::size_t i = 0; i < salinity.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            auto mIon = saltMolarMass<Scalar>(sIdx);
            s += salinity[sIdx] * ((mH2O - mIon) / (mH2O * mIon));
        }
        return s;
    }

    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    avgMolarMassFromMoleFrac(const SaltArray<Evaluation, SaltMoleFraction>& salinity)
    {
        const Scalar mH2O = H2O::molarMass();
        Evaluation s = mH2O;
        for (std::size_t i = 0; i < salinity.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            auto mIon = saltMolarMass<Scalar>(sIdx);
            s += salinity[sIdx] * (mIon - mH2O);
        }
        return s;
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
     * \brief The density \f$\mathrm{[kg/m^3]}\f$ of the liquid component at
     *        a given pressure in \f$\mathrm{[Pa]}\f$ and temperature in \f$\mathrm{[K]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity Concentration of dissolved salts
     * \param rhow Density of liquid
     *
     * Equations given in:
     * - Batzle & Wang (1992)
     * - cited by: Adams & Bachu in Geofluids (2002) 2, 257-271
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidDensity(const Evaluation& temperature,
                                                    const Evaluation& pressure,
                                                    const Evaluation& salinity,
                                                    const Evaluation& rhow)
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
     * Density of liquid brine using multicomponent salts from Laliberte & Cooper,
     * Journal of Chemical and Engineering Data, Vol. 49, No. 5, 2004.
     *
     * @param temperature Temperature [K]
     * @param salinity Array of salt ions [kg ion/kg water]
     * @return Brine density [kg/m3]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidDensityMulticompSalt(const Evaluation& temperature,
                               const SaltArray<Evaluation, SaltMassFraction>& salinity)
    {
        // Density of pure water
        const Evaluation rhow = liquidDensityPureWaterLaliberteCooper_(temperature);

        // Return pure water density if salinity is zero
        if (!salinity.any_nonzero()) {
            return rhow;
        }
        return liquidDensityLaliberteCooper(temperature, salinity, rhow);
    }

    /*!
     * Density of liquid brine using multicomponent salts from Laliberte & Cooper,
     * Journal of Chemical and Engineering Data, Vol. 49, No. 5, 2004 (all equations refs. are
     * from this paper).
     *
     * @param temperature Temperature [K]
     * @param salinity Array of salt ions [kg ion/kg water]
     * @param rhow Density pure water [kg/m3]
     * @return Brine density [kg/m3]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidDensityLaliberteCooper(const Evaluation& temperature,
                                 const SaltArray<Evaluation, SaltMassFraction>& salinity,
                                 const Evaluation& rhow)
    {
        // Generate (valid) electrolytes from salt components
        auto electrolytes = electrolytesFromSaltComponents_(salinity);

        // Calculate brine density with Eq. (6)
        Evaluation tempC = temperature - 273.15;
        auto sumSalinity = salinity.sum();
        Evaluation sumAppVolTimesMf = 0.0;
        for (const auto& salt : electrolytes) {
            // Get constants
            const auto c = LaliberteCooperCoeff_(salt.first);

            // Calculate apparent volume with Eq. (10) (where we use (1 - w_H2O) instead of w_i)
            // multiplied by mass fraction salt eletrolyte to get the sum term in Eq. (6)
            Evaluation tmpExponent = tempC + c[4];
            sumAppVolTimesMf +=
                salt.second * (sumSalinity + c[2] + c[3] * tempC)
                / ((c[0] * sumSalinity + c[1]) * exp(1e-6 * tmpExponent * tmpExponent));
        }

        // Calculate the rest of Eq. (6) to get brine density
        // OBS: Assume sum of ions mass fractions = sum of electrolyte mass fractions, which is
        // true if there are no residual molalities when calculating electrolytes
        auto wH2O = 1.0 - sumSalinity;
        Evaluation rho = 1.0 / (wH2O / rhow + sumAppVolTimesMf);

        return rho;
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

    /*!
     * Viscosity of liquid brine using multicomponent salts from Laliberte, Journal of Chemical
     * and Engineering Data, Vol. 52, No. 2, 2007 and corrections paper Laliberte, Journal of
     * Chemical and Engineering Data, Vol. 52, No. 4, 2007.
     *
     * @param temperature Temperature [K]
     * @param salinity Array of salt ions [kg ion/kg water]
     * @return Viscosity of brine [Pa*s]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidViscosityMulticompSalt(const Evaluation& temperature,
                                 const SaltArray<Evaluation, SaltMassFraction>& salinity)
    {
        // Pure water viscosity
        const Evaluation muW = liquidViscosityPureWaterLaliberte_(temperature);

        // Return pure water viscosity for zero salinity
        if (!salinity.any_nonzero()) {
            return muW;
        }

        return liquidViscosityLaliberte(temperature, salinity, muW);
    }

    /*!
     * Viscosity of liquid brine using multicomponent salts from Laliberte, Journal of Chemical
     * and Engineering Data, Vol. 52, No. 2, 2007 and corrections paper Laliberte, Journal of
    * Chemical and Engineering Data, Vol. 52, No. 4, 2007 (all equations refs. are from these
    * papers).
     *
     * @param temperature Temperature [K]
     * @param salinity Array of salt ions [kg ion/kg water]
     * @param muW Viscosity of pure water [mPa*s]
     * @return Viscosity of brine [Pa*s]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidViscosityLaliberte(const Evaluation& temperature,
                             const SaltArray<Evaluation, SaltMassFraction>& salinity,
                             const Evaluation& muW)
    {
        // Generate (valid) electrolytes from salt components
        auto electrolytes = electrolytesFromSaltComponents_(salinity);

        // ln-viscosity of mix from Eq. (8)
        Evaluation tempC = temperature - 273.15;
        auto sumSalinity = salinity.sum();
        Evaluation lnMuMix = 0.0;
        for (const auto& salt : electrolytes) {
            // Get constants
            const auto v = LaliberteCoeff_(salt.first);

            // Calculate ln of Eq. (12).
            // OBS: Different from Eq. (13) in correction paper (which seems to be incorrect)
            lnMuMix += salt.second
                * ((v[0] * pow(sumSalinity, v[1]) + v[2]) / (v[3] * tempC + 1.0)
                - log(v[4] * pow(sumSalinity, v[5]) + 1.0));
        }

        // Calculate the rest of Eq. (8)
        // OBS: Assume sum of ions mass fractions = sum of electrolyte mass fractions, which is
        // true if there are no residual molalities when calculating electrolytes
        lnMuMix += (1.0 - sumSalinity) * log(muW);

        // OBS: mPa*s to Pa*s
        return exp(lnMuMix) * 1e-3;
    }

private:
    //Molar mass salt (assumes pure NaCl) [kg/mol]
    static constexpr Scalar mM_salt()
    {
        return 58.44e-3;
    }

    /*!
     * Density of pure water from Laliberte & Cooper, Journal of Chemical and Engineering Data,
     * Vol. 49, No. 5, 2004.
     *
     * @param temperature Temperature [K]
     * @return Density [kg/m3]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidDensityPureWaterLaliberteCooper_(const Evaluation& temperature)
    {
        const auto tempC = temperature - 273.15;
        return (((((-2.8054253e-10 * tempC + 1.0556302e-7) * tempC - 4.6170461e-5) * tempC
                - 0.0079870401) * tempC + 16.945176) * tempC + 999.83952)
            / (1.0 + 0.01687985 * tempC);
    }

    /*!
     * Viscosity of pure water from Laliberte, Journal of Chemical and Engineering Data, Vol. 52,
     * No. 2, 2007.
     *
     * @param temperature Temperature [K]
     * @return Viscosity [mPa*s]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation
    liquidViscosityPureWaterLaliberte_(const Evaluation& temperature)
    {
        const auto tempC = temperature - 273.15;
        return (tempC + 246.0) / ((0.05594 * tempC + 5.2842) * tempC + 137.37);
    }

    /// Supported electrolytes for Laliberte viscosity and Laliberte-Cooper density
    enum class SaltElectrolytes
    {
        CaCl2, KCl, K2SO4, MgCl2, MgSO4, NaCl, Na2SO4
    };

    /*!
     * Molar mass of electrolyte
     *
     * @param e Electrolyte
     * @return Molar mass [mol/kg]
     */
    static Scalar electrolyteMolarMass(const SaltElectrolytes e)
    {
        switch (e) {
        case SaltElectrolytes::CaCl2:
            return CaIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass();
        case SaltElectrolytes::KCl:
            return KIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass();
        case SaltElectrolytes::K2SO4:
            return 2.0 * KIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        case SaltElectrolytes::MgCl2:
            return MgIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass();
        case SaltElectrolytes::MgSO4:
            return MgIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        case SaltElectrolytes::NaCl:
            return NaIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass();
        case SaltElectrolytes::Na2SO4:
            return 2.0 * NaIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass();
        default:
            throw std::runtime_error("Unknown SaltElectrolytes!");
        }
    }

    /*!
     * @brief Calculate electrolytes from cations and anions, and adjust molality of cations and
     * anions after generating electrolytes
     *
     * The electrolytes are made from the dissociation (or chemical equilibrium) equation
     * A_pB_q = pA+ + qB-. Hence, to make 1 molal of A_pB_q (electrolyte), p molal of A+ (cation)
     * and q molal of B- (anion) are needed. In this function we make as many molal electrolytes
     * as the maximum amount of cations or anions allow, based on the dissociation equation.
     *
     * @param cationIdx Salt index of cation
     * @param cation Molality of cation [mol cation/kg water]
     * @param anionIdx Salt index of anion
     * @param anion Molality of anion [mol anion/kg water]
     * @return Electrolyte and its molality [mol electrolyte/kg water]
     */
    template <class Evaluation>
    static std::pair<SaltElectrolytes, Evaluation>
    electrolytesAndRemainingMolal_(const SaltIndex cationIdx,
                                   Evaluation& cation,
                                   const SaltIndex anionIdx,
                                   Evaluation& anion)
    {
        // NOTE: All comments describe unit molal conversion between electrolyte and ions
        Evaluation electrolyteMolal;
        std::pair<SaltElectrolytes, Evaluation> electrolyte;
        if (cationIdx == SaltIndex::MG && anionIdx == SaltIndex::SO4) {
            // 1m Mg+ + 1m SO4-- = 1m MgSO4
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {SaltElectrolytes::MgSO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::NA && anionIdx == SaltIndex::SO4) {
            // 2m Na+ + 1m SO4-- = 1m Na2SO4
            electrolyteMolal = min(cation / 2.0, anion);
            cation -= electrolyteMolal * 2.0;
            anion -= electrolyteMolal;
            electrolyte = {SaltElectrolytes::Na2SO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::K && anionIdx == SaltIndex::SO4) {
            // 2m K+ + 1m SO4-- = 1m K2SO4
            electrolyteMolal = min(cation / 2.0, anion);
            cation -= electrolyteMolal * 2.0;
            anion -= electrolyteMolal;
            electrolyte = {SaltElectrolytes::K2SO4, electrolyteMolal};
        } else if (cationIdx == SaltIndex::MG && anionIdx == SaltIndex::CL) {
            // 1m Mg+ + 2m Cl- = 1m MgCl2
            electrolyteMolal = min(cation, anion / 2.0);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal * 2.0;
            electrolyte = {SaltElectrolytes::MgCl2, electrolyteMolal};
        } else if (cationIdx == SaltIndex::CA && anionIdx == SaltIndex::CL) {
            // 1m Ca+ + 2m Cl- = 1m CaCl2
            electrolyteMolal = min(cation, anion / 2.0);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal * 2.0;
            electrolyte = {SaltElectrolytes::CaCl2, electrolyteMolal};
        } else if (cationIdx == SaltIndex::NA && anionIdx == SaltIndex::CL) {
            // 1m Na+ + 1m Cl- = 1m NaCl
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {SaltElectrolytes::NaCl, electrolyteMolal};
        } else if (cationIdx == SaltIndex::K && anionIdx == SaltIndex::CL) {
            // 1m K+ + 1m Cl- = 1m KCl
            electrolyteMolal = min(cation, anion);
            cation -= electrolyteMolal;
            anion -= electrolyteMolal;
            electrolyte = {SaltElectrolytes::KCl, electrolyteMolal};
        } else {
            throw std::runtime_error("Unknown cation and/or anion SaltIndex!");
        }

        return electrolyte;
    }

    /*!
     * Convert electrolyte molality to mass fraction
     *
     * @param electrolyte Vector of electrolytes and their molality
     */
    template <class Evaluation>
    static void
    electrolytesMolalToMassFrac_(std::vector<std::pair<SaltElectrolytes, Evaluation> >& electrolyte)
    {
        Scalar sum = 1.0;
        for (auto& salt : electrolyte) {
            auto mMsalt = electrolyteMolarMass(salt.first);
            salt.second *= mMsalt;
            sum += decay<Scalar>(salt.second);
        }
        for (auto& salt : electrolyte) {
            salt.second /= sum;
        }
    }

    /*!
     * Generate electrolytes from salt ions
     *
     * @param salinity Array of salt ions
     * @return Vector of electrolytes and their mass fractions
     */
    template <class Evaluation>
    static std::vector<std::pair<SaltElectrolytes, Evaluation> >
    electrolytesFromSaltComponents_(const SaltArray<Evaluation, SaltMassFraction>& salinity)
    {
        // Generate (valid) electrolytes from salt components
        auto molalSalinity = salinity.template convert_to<SaltMolality>();
        auto [cations, anions] = salinity.cations_and_anions();
        std::vector<std::pair<SaltElectrolytes, Evaluation> > electrolytes;
        for (const auto& anionIdx : anions) {
            auto& molalAnion = molalSalinity[anionIdx];
            for (std::size_t i = 0; i < cations.size() && molalAnion >= molalTolerance; ++i) {
                const auto& cationIdx = cations[i];
                auto& molalCation = molalSalinity[cationIdx];
                if (molalCation < molalTolerance) {
                    continue;
                }

                // Generate salt electrolyte (in molal), and subtract used cation and anion
                // molalities. Note: Electrolytes are made from cation and anion pairs in
                // decreasing ion strength
                electrolytes.push_back(
                    electrolytesAndRemainingMolal_(cationIdx,
                                                   molalCation,
                                                   anionIdx,
                                                   molalAnion));
            }
        }

        // Warn if there is any leftover molality after generating electrolytes
        if (molalSalinity.sum() > molalTolerance) {
            OpmLog::debug(
                fmt::format(
                    fmt::runtime("Sum molality of salt components ({}) > tolerance, "
                        "and will be ignored in the property calculations!"),
                    decay<Scalar>(molalSalinity.sum())
                    )
                );
        }

        // Convert salt electrolytes from molality to mass fraction
        electrolytesMolalToMassFrac_(electrolytes);

        return electrolytes;
    }

    /*!
     * Get coefficients for calculating density with electrolyte
     *
     * @param electrolyte Electrolyte index
     * @return Coefficients for electrolyte
     */
    static std::span<const Scalar, 5>
    LaliberteCooperCoeff_(const SaltElectrolytes electrolyte)
    {
        if (electrolyte == SaltElectrolytes::CaCl2) {
            return cDensCaCl2;
        }
        if (electrolyte == SaltElectrolytes::KCl) {
            return cDensKCl;
        }
        if (electrolyte == SaltElectrolytes::K2SO4) {
            return cDensK2SO4;
        }
        if (electrolyte == SaltElectrolytes::MgCl2) {
            return cDensMgCl2;
        }
        if (electrolyte == SaltElectrolytes::MgSO4) {
            return cDensMgSO4;
        }
        if (electrolyte == SaltElectrolytes::NaCl) {
            return cDensNaCl;
        }
        if (electrolyte == SaltElectrolytes::Na2SO4) {
            return cDensNa2SO4;
        }
        throw std::runtime_error("Unknown SaltElectrolytes!");
    }

    /*!
     * Get coefficients for calculating viscosity with electrolyte
     *
     * @param electrolyte Electrolyte index
     * @return Coefficients for electrolyte
     */
    static std::span<const Scalar, 6>
    LaliberteCoeff_(const SaltElectrolytes electrolyte)
    {
        if (electrolyte == SaltElectrolytes::CaCl2) {
            return cViscCaCl2;
        }
        if (electrolyte == SaltElectrolytes::KCl) {
            return cViscKCl;
        }
        if (electrolyte == SaltElectrolytes::K2SO4) {
            return cViscK2SO4;
        }
        if (electrolyte == SaltElectrolytes::MgCl2) {
            return cViscMgCl2;
        }
        if (electrolyte == SaltElectrolytes::MgSO4) {
            return cViscMgSO4;
        }
        if (electrolyte == SaltElectrolytes::NaCl) {
            return cViscNaCl;
        }
        if (electrolyte == SaltElectrolytes::Na2SO4) {
            return cViscNa2SO4;
        }
        throw std::runtime_error("Unknown SaltElectrolytes!");
    }

    // Coefficients for density from Laliberte & Cooper, J. Chem. Eng. Data 49, 2004
    static constexpr Scalar cDensCaCl2[5] = {-0.63254, 0.93995, 4.2785, 0.048319, 3180.9};
    static constexpr Scalar cDensKCl[5] = {-0.46782, 4.30800, 2.3780, 0.022044, 2714.0};
    static constexpr Scalar cDensK2SO4[5] = {-2.6681e-5, 3.0412e-5, 0.97118, 0.019816, 4366.1};
    static constexpr Scalar cDensMgCl2[5] = {-0.00051500, 0.0013444, 0.58358, 0.0085832, 3843.6};
    static constexpr Scalar cDensMgSO4[5] = {0.0032447, 0.057246, 0.05136, 0.002146, 3287.8};
    static constexpr Scalar cDensNaCl[5] = {-0.00433, 0.06471, 1.01660, 0.014624, 3315.6};
    static constexpr Scalar cDensNa2SO4[5] = {-1.2095e-7, 4.3474e-7, 0.15364, 0.0072514, 4731.5};

    // Coefficients for viscosity from Laliberte, J. Chem. Eng. Data 52 (2), 2007 and the
    // correction paper Laliberte, J. Chem. Eng. Data 52(4), 2007
    static constexpr Scalar cViscCaCl2[6] = {32.028, 0.78792, -1.1495, 0.0026995, 780860, 5.8442};
    static constexpr Scalar cViscKCl[6] = {6.4883, 1.3175, -0.77785, 0.092722, -1.3, 2.0811};
    static constexpr Scalar cViscK2SO4[6] = {-983.76, 983.76, 984.52, 0.0038473, -9.5001, 2.1916};
    static constexpr Scalar cViscMgCl2[6] = {24.032, 2.2694, 3.7108, 0.021853, -1.1236, 0.14474};
    static constexpr Scalar cViscMgSO4[6] = {72.269, 2.2238, 6.6037, 0.0079004, 3340.1, 6.1304};
    static constexpr Scalar cViscNaCl[6] = {16.222, 1.3229, 1.4849, 0.0074691, 30.78, 2.0583};
    static constexpr Scalar cViscNa2SO4[6] = {26.519, 1.5746, 3.4966, 0.010388, 106.23, 2.9738};
};

} // namespace Opm

#endif
