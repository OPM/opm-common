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
* \ingroup Components
*
* \copydoc Opm::H2
*
*/
#ifndef OPM_H2_HPP
#define OPM_H2_HPP

#include <opm/material/IdealGas.hpp>
#include <opm/material/components/Component.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/densead/Math.hpp>

#include <cmath>

namespace Opm {

// this class collects all the H2tabulated quantities in one convenient place
struct H2Tables {
	static const Opm::UniformTabulated2DFunction<double>   tabulatedEnthalpy;
	static const Opm::UniformTabulated2DFunction<double>   tabulatedDensity;
	static constexpr double brineSalinity = 1.000000000000000e-01;
};

struct H2TabulatedDensityTraits {
    typedef double Scalar;
    static const char  *name;
    static const int    numX = 200;
    static const Scalar xMin;
    static const Scalar xMax;
    static const int    numY = 500;
    static const Scalar yMin;
    static const Scalar yMax;

    static const Scalar vals[200][500];
};

struct H2TabulatedEnthalpyTraits {
    typedef double Scalar;
    static const char  *name;
    static const int    numX = 200;
    static const Scalar xMin;
    static const Scalar xMax;
    static const int    numY = 500;
    static const Scalar yMin;
    static const Scalar yMax;
    static const Scalar vals[200][500];
};

/*!
 * \ingroup Components
 * \brief Properties of pure molecular hydrogen \f$H_2\f$. Most of the properties are calculated based on Leachman JW,
 * Jacobsen RT, Penoncello SG, Lemmon EW. Fundamental equations of state for parahydrogen, normal hydrogen, and
 * orthohydrogen. J Phys Chem Reference Data 2009;38:721e48. Their approach uses the fundamental Helmholtz free energy
 * thermodynamic EOS, which is better suited for many gases such as H2. See also Span R, Lemmon EW, Jacobsen RT, Wagner
 * W, Yokozeki A. A Reference Equation of State for the Thermodynamic Properties of Nitrogen for Temperatures from
 * 63.151 to 1000 K and Pressures to 2200 MPa for explicit equations derived from the fundamental EOS formula.
 * 
 * OBS: All equation and table references here are taken from Leachman et al. (2009) unless otherwise stated!
 * 
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class H2 : public Component<Scalar, H2<Scalar> >
{
    using IdealGas = Opm::IdealGas<Scalar>;

    static const UniformTabulated2DFunction<double>& tabulatedEnthalpy;
    static const UniformTabulated2DFunction<double>& tabulatedDensity;

public:
    // For H2Tables class
    static const Scalar brineSalinity;

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
    { return 33.145; /* [K] */ }

    /*!
    * \brief Returns the critical pressure \f$\mathrm{[Pa]}\f$ of molecular hydrogen.
    */
    static Scalar criticalPressure()
    { return 1.2964e6; /* [N/m^2] */ }

    /*!
    * \brief Returns the critical density \f$\mathrm{[mol/cm^3]}\f$ of molecular hydrogen.
    */
    static Scalar criticalDensity()
    { return 15.508e-3; /* [mol/cm^3] */ }

    /*!
    * \brief Returns the temperature \f$\mathrm{[K]}\f$ at molecular hydrogen's triple point.
    */
    static Scalar tripleTemperature()
    { return 13.957; /* [K] */ }

    /*!
    * \brief Returns the pressure \f$\mathrm{[Pa]}\f$ of molecular hydrogen's triple point.
    */
    static Scalar triplePressure()
    { return 0.00736e6; /* [N/m^2] */ }

    /*!
    * \brief Returns the density \f$\mathrm{[mol/cm^3]}\f$ of molecular hydrogen's triple point.
    */
    static Scalar tripleDensity()
    { return 38.2e-3; /* [mol/cm^3] */ }

    /*!
     * \brief Critical volume of \f$H_2\f$ [m2/kmol].
     */
    static Scalar criticalVolume() {return 6.45e-2; }

    /*!
     * \brief Acentric factor of \f$H_2\f$.
     */
    static Scalar acentricFactor() { return -0.22; }

    /*!
    * \brief The vapor pressure in \f$\mathrm{[Pa]}\f$ of pure molecular hydrogen
    *        at a given temperature.
    *
    *\param temperature temperature of component in \f$\mathrm{[K]}\f$
    *
    */
    template <class Evaluation>
    static Evaluation vaporPressure(Evaluation temperature)
    {
        if (temperature > criticalTemperature())
            return criticalPressure();
        if (temperature < tripleTemperature())
            return 0; // H2 is solid: We don't take sublimation into
                      // account

        // Intermediate calculations involving temperature
        Evaluation sigma = 1 - temperature/criticalTemperature();
        Evaluation T_recp = criticalTemperature() / temperature;

        // Parameters for normal hydrogen in Table 8
        static const Scalar N[4] = {-4.89789, 0.988558, 0.349689, 0.499356};
        static const Scalar k[4] = {1.0, 1.5, 2.0, 2.85};

        // Eq. (33)
        Evaluation s = 0.0;  // sum calculation
        for (int i = 0; i < 4; ++i) {
            s += N[i] * pow(sigma, k[i]);
        }
        Evaluation lnPsigmaPc = T_recp * s;
        
        return exp(lnPsigmaPc) * criticalPressure();
    }

    /*!
    * \brief The density \f$\mathrm{[kg/m^3]}\f$ of \f$H_2\f$ at a given pressure and temperature.
    *
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
    */
    template <class Evaluation>
    static Evaluation gasDensity(Evaluation temperature, Evaluation pressure, bool extrapolate = false)
    {
        return tabulatedDensity.eval(temperature, pressure, extrapolate);
    }

    /*!
     * \brief The molar density of \f$H_2\f$ in \f$\mathrm{[mol/m^3]}\f$,
     *   depending on pressure and temperature.
     * \param temperature The temperature of the gas
     * \param pressure The pressure of the gas
     */
    template <class Evaluation>
    static Evaluation gasMolarDensity(Evaluation temperature, Evaluation pressure, bool extrapolate = false)
    { return gasDensity(temperature, pressure, extrapolate) / molarMass(); }

    /*!
     * \brief Returns true if the gas phase is assumed to be compressible
     */
    static constexpr bool gasIsCompressible()
    { return true; }

    /*!
     * \brief Returns true if the gas phase is assumed to be ideal
     */
    static constexpr bool gasIsIdeal()
    { return false; }

    /*!
     * \brief The pressure of gaseous \f$H_2\f$ in \f$\mathrm{[Pa]}\f$ at a given density and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param density density of component in \f$\mathrm{[kg/m^3]}\f$
     */
    template <class Evaluation>
    static Evaluation gasPressure(Evaluation temperature, Evaluation density)
    {
        // Eq. (56) in Span et al. (2000)
        Scalar R = IdealGas::R;
        Evaluation rho_red = density / (molarMass() * criticalDensity() * 1e6);
        Evaluation T_red = H2::criticalTemperature() / temperature;
        return rho_red * criticalDensity() * R * temperature 
            * (1 + rho_red * derivResHelmholtzWrtRedRho(T_red, rho_red));
    }

    /*!
    * \brief Specific internal energy of H2 [J/kg].
    */
    template <class Evaluation>
    static Evaluation gasInternalEnergy(const Evaluation& temperature,
                                        const Evaluation& pressure,
                                        bool extrapolate = false)
    {
        const Evaluation h = gasEnthalpy(temperature, pressure, extrapolate);
        const Evaluation rho = gasDensity(temperature, pressure, extrapolate);

        return h - (pressure / rho);
    }

    /*!
    * \brief Specific enthalpy \f$\mathrm{[J/kg]}\f$ of pure hydrogen gas.
    *
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
    */
    template <class Evaluation>
    static const Evaluation gasEnthalpy(Evaluation temperature,
                                        Evaluation pressure,
                                        bool extrapolate = false)
    {
        return tabulatedEnthalpy.eval(temperature, pressure, extrapolate);
    }

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of \f$H_2\f$ at a given pressure and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     *
     * See: Muzney et al., J. Chem. Eng. Data 2013, 58, 4, 969–979 (and the corrections!)
     *
     */
    template <class Evaluation>
    static Evaluation gasViscosity(const Evaluation& temperature, 
                                   const Evaluation& pressure,
                                   bool extrapolate = false)
    {
        // Some needed parameters and variables
        const Scalar M = molarMass() * 1e3;  // g/mol
        const Scalar epsilon_div_kb = 30.41;
        const Scalar sigma = 0.297;  // nm
        const Scalar Na = 6.022137e23;  // Avogadro's number

        Evaluation T_star = temperature / epsilon_div_kb;
        Evaluation ln_T_star = log(T_star);
        Evaluation T_r = temperature / criticalTemperature();
        Evaluation rho = gasDensity(temperature, pressure, extrapolate);
        Evaluation rho_r = rho / 90.909090909; // see corrections

        // 
        // Eta_0 terms (zero-density viscocity)
        //
        // Parameters in Table 2 for Eq. (4) 
        static constexpr Scalar a[5] = 
            {2.0963e-1, -4.55274e-1, 1.43602e-1, -3.35325e-2, 2.76981e-3};

        // Eq. (4)
        Evaluation ln_S_star = 0.0;
        for (int i = 0; i < 5; ++i) {
            ln_S_star += a[i] * pow(ln_T_star, i);
        }

        // Eq. (3)
        Evaluation eta_0 = 0.021357 * sqrt(M * temperature) / (sigma * sigma * exp(ln_S_star));

        // 
        // Eta_1 terms (excess contribution)
        // 
        // 
        // Parameters in Table 3 for Eq. (7)
        static constexpr Scalar b[7] = 
            {-0.187, 2.4871, 3.7151, -11.0972, 9.0965, -3.8292, 0.5166};
        
        // Eq. (7) with corrections
        Evaluation B_star = 0.0;
        for (int i = 0; i < 7; ++i) {
            B_star += b[i] * pow(T_star, -i);
        }

        // Eq. (9) (eta_1 part) using Eq. (5-7) with corrections and sigma in m (instead of nm). Note that Na * sigma_m
        // ^ 3 have unit [m3/mol], so we need to multiply with molar density (= rho / molarMass) and not mass density in
        // Eq. (9)
        const Scalar sigma_m = sigma * 1e-9;
        Evaluation eta_1 = B_star * Na * sigma_m * sigma_m * sigma_m * eta_0 * rho / M;

        //
        // delta eta_h terms (higher-order effects)
        // 
        // Parameters in Table 4 for Eq. (9)
        static constexpr Scalar c[6] =
            {6.43449673, 4.56334068e-2, 2.32797868e-1, 9.58326120e-1, 1.27941189e-1, 3.63576595e-1};

        // delta eta_h terms in Eq. (9)
        Evaluation delta_eta_h = c[0] * rho_r * rho_r * exp(c[1] * T_r + c[2] / T_r + 
        (c[3] * rho_r * rho_r) / (c[4] + T_r) + c[5] * pow(rho_r, 6));

        // return all terms in Eq. (9) converted from muPa*s to Pa*s
        return (eta_0 + eta_1 + delta_eta_h) * 1e-6;
    }

    /*!
    * \brief Specific isobaric heat capacity \f$\mathrm{[J/(kg*K)]}\f$ of pure hydrogen gas. This is equivalent to the
    * partial derivative of the specific enthalpy to the temperature.
    * 
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
    *
    */
    template <class Evaluation>
    static const Evaluation gasHeatCapacity(Evaluation temperature,
                                            Evaluation pressure)
    {
        // Reduced variables
        Evaluation rho_red = reducedMolarDensity(temperature, pressure);
        Evaluation T_red = criticalTemperature() / temperature;
        
        // Need Eq. (62) in Span et al. (2000)
        Evaluation cv = gasIsochoricHeatCapacity(temperature, pressure);  // [J/(kg*K)]

        // Some intermediate calculations 
        Evaluation numerator = pow(1 + rho_red * derivResHelmholtzWrtRedRho(T_red, rho_red) 
            - rho_red * T_red * secDerivResHelmholtzWrtRecipRedTempAndRedRho(T_red, rho_red), 2);

        Evaluation denominator = 1 + 2 * rho_red * derivResHelmholtzWrtRedRho(T_red, rho_red)
            + pow(rho_red, 2) * secDerivResHelmholtzWrtRedRho(T_red, rho_red);
        
        // Eq. (63) in Span et al. (2000).
        Scalar R = IdealGas::R;
        Evaluation cp = cv + R * (numerator / denominator) / molarMass();  // divide by M to get [J/(kg*K)]

        // Return
        return cp;
    }

    /*!
    * \brief Specific isochoric heat capacity \f$\mathrm{[J/(kg*K)]}\f$ of pure hydrogen gas.
    * 
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
    *
    */
    template <class Evaluation>
    static const Evaluation gasIsochoricHeatCapacity(Evaluation temperature,
                                                     Evaluation pressure)
    {
        // Reduced variables
        Evaluation rho_red = reducedMolarDensity(temperature, pressure);
        Evaluation T_red = criticalTemperature() / temperature;

        // Eq. (62) in Span et al. (2000)
        Scalar R = IdealGas::R;
        Evaluation cv = R * (-pow(T_red, 2) * (secDerivIdealHelmholtzWrtRecipRedTemp(T_red) 
            + secDerivResHelmholtzWrtRecipRedTemp(T_red, rho_red)));  // [J/(mol*K)]
        
        return cv / molarMass();
    }
    /*!
    * \brief Calculate reduced density (rho/rho_crit) from pressure and temperature. The conversion is done using the
    * simplest root-finding algorithm, i.e. the bisection method.
    * 
    * \param pg gas phase pressure [Pa]
    * \param temperature temperature [K]
    */
    template <class Evaluation> 
    static Evaluation reducedMolarDensity(const Evaluation& temperature, 
                                          const Evaluation& pg, 
                                          bool extrapolate = false)
    {
        return gasDensity(temperature, pg, extrapolate) / (molarMass() * criticalDensity() * 1e6);
    }
    
    /*!
    * \brief The ideal-gas part of Helmholtz energy.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation idealGasPartHelmholtz(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Eq. (31), which can be compared with Eq. (53) in Span et al. (2000)
        // Terms not in sum
        Evaluation s1 = log(rho_red) + 1.5*log(T_red) + a_[0] + a_[1] * T_red;

        // Sum term
        Evaluation s2 = 0.0;
        for (int i = 2; i < 7; ++i) {
            s1 += a_[i] * log(1 - exp(b_[i-2] * T_red));
        }

        // Return total
        Evaluation s = s1 + s2;
        return s;
    }

    /*!
    * \brief Derivative of the ideal-gas part of Helmholtz energy wrt to reciprocal reduced temperature.
    * 
    * \param T_red reduced temperature [-]
    */
    template <class Evaluation> 
    static Evaluation derivIdealHelmholtzWrtRecipRedTemp(const Evaluation& T_red)
    {
        // Derivative of Eq. (31) wrt. reciprocal reduced temperature, which can be compared with Eq. (79) in Span et
        // al. (2000)
        // Terms not in sum
        Evaluation s1 = (1.5 / T_red) + a_[1];

        // Sum term
        Evaluation s2 = 0.0;
        for (int i = 2; i < 7; ++i) {
            s2 += (-a_[i] * b_[i-2] * exp(b_[i-2] * T_red)) / (1 - exp(b_[i-2] * T_red));
        }

        // Return total
        Evaluation s = s1 + s2;
        return s;
    }

    /*!
    * \brief Second derivative of the ideal-gas part of Helmholtz energy wrt to reciprocal reduced temperature.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation secDerivIdealHelmholtzWrtRecipRedTemp(const Evaluation& T_red)
    {
        // Second derivative of Eq. (31) wrt. reciprocal reduced temperature, which can be compared with Eq. (80) in
        // Span et al. (2000)
        // Sum term
        Evaluation s1 = 0.0;
        for (int i = 2; i < 7; ++i) {
            s1 += (-a_[i] * pow(b_[i-2], 2) * exp(b_[i-2] * T_red)) / pow(1 - exp(b_[i-2] * T_red), 2);
        }

        // Return total
        Evaluation s = (-1.5 / pow(T_red, 2)) + s1;
        return s;
    }

    /*!
    * \brief The residual part of Helmholtz energy.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation residualPartHelmholtz(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Eq. (32), which can be compared with Eq. (55) in Span et al. (2000)
        // First sum term
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += N_[i] * pow(rho_red, d_[i]) * pow(T_red, t_[i]);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]) * exp(-pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

    /*!
    * \brief Derivative of the residual part of Helmholtz energy wrt. reduced density.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation derivResHelmholtzWrtRedRho(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Derivative of Eq. (32) wrt to reduced density, which can be compared with Eq. (81) in Span et al. (2000)
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += d_[i] * N_[i] * pow(rho_red, d_[i]-1) * pow(T_red, t_[i]);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]-1) * exp(-pow(rho_red, p_[i-7])) *
                (d_[i] - p_[i-7]*pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]-1) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2)) *
                    (d_[i] + 2 * phi_[i-9] * rho_red * (rho_red - D_[i-9]));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

    /*!
    * \brief Second derivative of the residual part of Helmholtz energy wrt. reduced density.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation secDerivResHelmholtzWrtRedRho(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Second derivative of Eq. (32) wrt to reduced density, which can be compared with Eq. (82) in Span et al.
        // (2000)
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += d_[i] * (d_[i] - 1) * N_[i] * pow(rho_red, d_[i]-2) * pow(T_red, t_[i]);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]-2) * exp(-pow(rho_red, p_[i-7])) *
                ((d_[i] - p_[i-7] * pow(rho_red, p_[i-7])) * (d_[i] - p_[i-7] * pow(rho_red, p_[i-7]) - 1.0) 
                    - pow(p_[i-7], 2) * pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]) * pow(rho_red, d_[i]-2) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2)) *
                    (pow(d_[i] + 2 * phi_[i-9] * rho_red * (rho_red - D_[i-9]), 2) 
                        - d_[i] + 2 * phi_[i-9] * pow(rho_red, 2));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }
    
    /*!
    * \brief Derivative of the residual part of Helmholtz energy wrt. reciprocal reduced temperature.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation derivResHelmholtzWrtRecipRedTemp(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Derivative of Eq. (32) wrt to reciprocal reduced temperature, which can be compared with Eq. (84) in Span et
        // al. (2000).
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += t_[i] * N_[i] * pow(rho_red, d_[i]) * pow(T_red, t_[i]-1);
        }
        
        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += t_[i] * N_[i] * pow(T_red, t_[i]-1) * pow(rho_red, d_[i]) * exp(-pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]-1) * pow(rho_red, d_[i]) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2)) *
                    (t_[i] + 2 * beta_[i-9] * T_red * (T_red - gamma_[i-9]));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

    /*!
    * \brief Second derivative of the residual part of Helmholtz energy wrt. reciprocal reduced temperature.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation secDerivResHelmholtzWrtRecipRedTemp(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Second derivative of Eq. (32) wrt to reciprocal reduced temperature, which can be compared with Eq. (85) in
        // Span et al. (2000).
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += t_[i] * (t_[i] - 1) * N_[i] * pow(rho_red, d_[i]) * pow(T_red, t_[i]-2);
        }
        
        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += t_[i] * (t_[i] - 1) * N_[i] * pow(T_red, t_[i]-2) * pow(rho_red, d_[i]) * exp(-pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]-2) * pow(rho_red, d_[i]) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2)) *
                    (pow(t_[i] + 2 * beta_[i-9] * T_red * (T_red - gamma_[i-9]), 2) 
                        - t_[i] + 2 * beta_[i-9] * pow(T_red, 2));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

    /*!
    * \brief Second derivative of the residual part of Helmholtz energy first wrt. reciprocal reduced temperature, and
    * second wrt. reduced density (i.e. d^2 H / drho_red dT_red).
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation secDerivResHelmholtzWrtRecipRedTempAndRedRho(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Second derivative of Eq. (32) wrt to reciprocal reduced temperature and reduced density, which can be
        // compared with Eq. (86) in Span et al. (2000).
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += t_[i] * d_[i] * N_[i] * pow(rho_red, d_[i]-1) * pow(T_red, t_[i]-1);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += t_[i] * N_[i] * pow(T_red, t_[i]-1) * pow(rho_red, d_[i]-1) * exp(-pow(rho_red, p_[i-7]))
                * (d_[i] - p_[i-7] * pow(rho_red, p_[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 14; ++i) {
            s3 += N_[i] * pow(T_red, t_[i]-1) * pow(rho_red, d_[i]-1) * 
                exp(phi_[i-9] * pow(rho_red - D_[i-9], 2) + beta_[i-9] * pow(T_red - gamma_[i-9], 2)) *
                    (t_[i] + 2 * beta_[i-9] * T_red * (T_red - gamma_[i-9])) 
                        * (d_[i] + 2 * phi_[i-9] * rho_red * (rho_red - D_[i-9]));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

private:

    // Parameter values need in the ideal-gas contribution to the reduced Helmholtz free energy given in Table 4
    static constexpr Scalar a_[7] = {-1.4579856475, 1.888076782, 1.616, -0.4117, -0.792, 0.758, 1.217};
    static constexpr Scalar b_[5] = {-16.0205159149, -22.6580178006, -60.0090511389, -74.9434303817, -206.9392065168};

    // Parameter values needed in the residual contribution to the reduced Helmholtz free energy given in Table 5.
    static constexpr Scalar N_[14] = {-6.93643, 0.01, 2.1101, 4.52059, 0.732564, -1.34086, 0.130985, -0.777414, 
            0.351944, -0.0211716, 0.0226312, 0.032187, -0.0231752, 0.0557346};
    static constexpr Scalar t_[14] = {0.6844, 1.0, 0.989, 0.489, 0.803, 1.1444, 1.409, 1.754, 1.311, 4.187, 5.646, 
        0.791, 7.249, 2.986};
    static constexpr Scalar d_[14] = {1, 4, 1, 1, 2, 2, 3, 1, 3, 2, 1, 3, 1, 1};
    static constexpr Scalar p_[2] = {1, 1};
    static constexpr Scalar phi_[5] = {-1.685, -0.489, -0.103, -2.506, -1.607};
    static constexpr Scalar beta_[5] = {-0.1710, -0.2245, -0.1304, -0.2785, -0.3967};
    static constexpr Scalar gamma_[5] = {0.7164, 1.3444, 1.4517, 0.7204, 1.5445};
    static constexpr Scalar D_[5] = {1.506, 0.156, 1.736, 0.670, 1.662};

#if 0
    /*!
    * \brief Objective function in root-finding done in convertPgToReducedRho.
    * 
    * \param rho_red reduced density [-]
    * \param pg gas phase pressure [Pa]
    * \param temperature temperature [K]
    */
    template <class Evaluation> 
    static Evaluation rootFindingObj_(const Evaluation& rho_red, const Evaluation& temperature, const Evaluation& pg)
    {
        // Temporary calculations
        Evaluation T_red = criticalTemperature() / temperature;  // reciprocal reduced temp.
        Evaluation p_MPa = pg / 1.0e6;  // Pa --> MPa
        Scalar R = IdealGas::R;
        Evaluation rho_cRT = criticalDensity() * R * temperature;

        // Eq. (56) in Span et al. (2000)
        Evaluation dResHelm_dRedRho = derivResHelmholtzWrtRedRho(T_red, rho_red);
        Evaluation obj = rho_red * rho_cRT * (1 + rho_red * dResHelm_dRedRho) - p_MPa;
        return obj;
    }
#endif
};

} // end namespace Opm

#endif
