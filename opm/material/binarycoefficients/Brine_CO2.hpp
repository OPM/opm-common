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
 * \copydoc Opm::BinaryCoeff::Brine_CO2
 */
#ifndef OPM_BINARY_COEFF_BRINE_CO2_HPP
#define OPM_BINARY_COEFF_BRINE_CO2_HPP

#include <opm/material/IdealGas.hpp>
#include <opm/material/common/Valgrind.hpp>

#include <array>

namespace Opm {
namespace BinaryCoeff {

/*!
 * \ingroup Binarycoefficients
 * \brief Binary coefficients for brine and CO2.
 */
template<class Scalar, class H2O, class CO2, bool verbose = true>
class Brine_CO2 {
    typedef ::Opm::IdealGas<Scalar> IdealGas;
    static const int liquidPhaseIdx = 0; // index of the liquid phase
    static const int gasPhaseIdx = 1; // index of the gas phase

public:
    /*!
     * \brief Binary diffusion coefficent [m^2/s] of water in the CO2 phase.
     *
     * According to "Diffusion of Water in Liquid and Supercritical Carbon Dioxide: An NMR Study",Bin Xu et al., 2002
     * \param temperature the temperature [K]
     * \param pressure the phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation gasDiffCoeff(const Evaluation& temperature, const Evaluation& pressure, bool extrapolate = false)
    {
        //Diffusion coefficient of water in the CO2 phase
        Scalar k = 1.3806504e-23; // Boltzmann constant
        Scalar c = 4; // slip parameter, can vary between 4 (slip condition) and 6 (stick condition)
        Scalar R_h = 1.72e-10; // hydrodynamic radius of the solute
        const Evaluation& mu = CO2::gasViscosity(temperature, pressure, extrapolate); // CO2 viscosity
        return k / (c * M_PI * R_h) * (temperature / mu);
    }

    /*!
     * \brief Binary diffusion coefficent [m^2/s] of CO2 in the brine phase.
     *
     * \param temperature the temperature [K]
     * \param pressure the phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation liquidDiffCoeff(const Evaluation& /*temperature*/, const Evaluation& /*pressure*/)
    {
        //Diffusion coefficient of CO2 in the brine phase
        return 2e-9;
    }

    /*!
     * \brief Returns the _mol_ (!) fraction of CO2 in the liquid
     *        phase and the mol_ (!) fraction of H2O in the gas phase
     *        for a given temperature, pressure, CO2 density and brine
     *        salinity.
     *
     *        Implemented according to "Spycher and Pruess 2005"
     *        applying the activity coefficient expression of "Duan and Sun 2003"
     *        and the correlations for pure water given in "Spycher, Pruess and Ennis-King 2003"
     *
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     * \param salinity the salinity [kg NaCl / kg solution]
     * \param knownPhaseIdx indicates which phases are present
     * \param xlCO2 mole fraction of CO2 in brine [mol/mol]
     * \param ygH2O mole fraction of water in the gas phase [mol/mol]
     */
    template <class Evaluation>
    static void calculateMoleFractions(const Evaluation& temperature,
                                       const Evaluation& pg,
                                       const Evaluation& salinity,
                                       const int knownPhaseIdx,
                                       Evaluation& xlCO2,
                                       Evaluation& ygH2O,
                                       bool activity_model,
                                       bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
        // Evaluation A = computeA_(temperature, pg, extrapolate);

        /* salinity: conversion from mass fraction to mol fraction */
        Evaluation x_NaCl = salinityToMolFrac_(salinity);

        // High- or low-temperature case?
        bool highTemp;
        if (temperature > 372.15) {
            highTemp = true;
        }
        else {
            highTemp = false;
        }

        // if both phases are present the mole fractions in each phase can be calculate
        // with the mutual solubility function
        if (knownPhaseIdx < 0) {
            Evaluation molalityNaCl = moleFracToMolality_(x_NaCl); // molality of NaCl //CHANGED
            
            // High-temp. cases need iterations
            if (highTemp) {
                auto [xCO2, yH2O] = highTempSolubility_(temperature, pg, molalityNaCl, extrapolate);
                xlCO2 = xCO2;
                ygH2O = yH2O;
            }

            // While low-temp. cases don't need it
            else {
                auto [xCO2, yH2O] = lowTempSolubility_(temperature, pg, molalityNaCl, extrapolate);
                xlCO2 = xCO2;
                ygH2O = yH2O;
            }
        }

        // if only liquid phase is present the mole fraction of CO2 in brine is given and
        // and the virtual equilibrium mole fraction of water in the non-existing gas phase can be estimated
        // with the mutual solubility function
        // if (knownPhaseIdx == liquidPhaseIdx)
        //     ygH2O = A * (1 - xlCO2 - x_NaCl);

        // if only gas phase is present the mole fraction of water in the gas phase is given and
        // and the virtual equilibrium mole fraction of CO2 in the non-existing liquid phase can be estimated
        // with the mutual solubility function
        // if (knownPhaseIdx == gasPhaseIdx)
        //     //y_H2o = fluidstate.
        //     xlCO2 = 1 - x_NaCl - ygH2O / A;
    }

    /*!
     * \brief Henry coefficent \f$\mathrm{[N/m^2]}\f$ for CO2 in brine.
     */
    template <class Evaluation>
    static Evaluation henry(const Evaluation& temperature, bool extrapolate = false)
    { return fugacityCoefficientCO2(temperature, /*pressure=*/1e5, extrapolate)*1e5; }

    /*!
     * \brief Returns the fugacity coefficient of the CO2 component in a water-CO2 mixture
     *
     * (given in Spycher, Pruess and Ennis-King (2003))
     *
     * \param T the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation fugacityCoefficientCO2(const Evaluation& temperature, 
                                             const Evaluation& pg,
                                             const Evaluation& yH2O, 
                                             const bool highTemp, 
                                             bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
        Valgrind::CheckDefined(temperature);
        Valgrind::CheckDefined(pg);

        Evaluation V = 1 / (CO2::gasDensity(temperature, pg, extrapolate) / CO2::molarMass()) * 1.e6; // molar volume in cm^3/mol
        Evaluation pg_bar = pg / 1.e5; // gas phase pressure in bar
        Scalar R = IdealGas::R * 10.; // ideal gas constant with unit bar cm^3 /(K mol)

        // Parameters in Redlich-Kwong equation
        Evaluation a_CO2 = aCO2_(temperature, highTemp);
        Evaluation a_CO2_H2O = aCO2_H2O_(temperature, yH2O, highTemp);
        Evaluation a_mix = aMix_(temperature, yH2O, highTemp);
        Scalar b_CO2 = bCO2_(highTemp); 
        Evaluation b_mix = bMix_(yH2O, highTemp);

        Evaluation lnPhiCO2;
        lnPhiCO2 = log(V / (V - b_mix));
        lnPhiCO2 += b_CO2 / (V - b_mix);
        lnPhiCO2 -= 2 * (yH2O * a_CO2_H2O + (1 - yH2O) * a_CO2) / (R * pow(temperature, 1.5) * b_mix) 
                    * log((V + b_mix) / V);
        lnPhiCO2 += a_mix * b_CO2 / (R * pow(temperature, 1.5) * b_mix * b_mix) * (log((V + b_CO2) / V) 
                    - b_mix / (V + b_mix));
        lnPhiCO2 -= log(pg_bar * V / (R * temperature));

        return exp(lnPhiCO2); // fugacity coefficient of CO2
    }

    /*!
     * \brief Returns the fugacity coefficient of the H2O component in a water-CO2 mixture
     *
     * (given in Spycher, Pruess and Ennis-King (2003))
     *
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation fugacityCoefficientH2O(const Evaluation& temperature, 
                                             const Evaluation& pg,
                                             const Evaluation& yH2O, 
                                             const bool highTemp, 
                                             bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
        Valgrind::CheckDefined(temperature);
        Valgrind::CheckDefined(pg);

        const Evaluation& V = 1 / (CO2::gasDensity(temperature, pg, extrapolate) / CO2::molarMass()) * 1.e6; // molar volume in cm^3/mol
        const Evaluation& pg_bar = pg / 1.e5; // gas phase pressure in bar
        Scalar R = IdealGas::R * 10.; // ideal gas constant with unit bar cm^3 /(K mol)

        // Mixture parameter of  Redlich-Kwong equation
        Evaluation a_H2O = aH2O_(temperature, highTemp);
        Evaluation a_CO2_H2O = aCO2_H2O_(temperature, yH2O, highTemp);
        Evaluation a_mix = aMix_(temperature, yH2O, highTemp);
        Scalar b_H2O = bH2O_(highTemp); 
        Evaluation b_mix = bMix_(yH2O, highTemp);

        Evaluation lnPhiH2O;
        lnPhiH2O = log(V / (V - b_mix));
        lnPhiH2O += b_H2O / (V - b_mix);
        lnPhiH2O -= 2 * (yH2O * a_H2O + (1 - yH2O) * a_CO2_H2O) / (R * pow(temperature, 1.5) * b_mix) 
                    * log((V + b_mix) / V);
        lnPhiH2O += a_mix * b_H2O / (R * pow(temperature, 1.5) * b_mix * b_mix) * (log((V + b_H2O) / V) 
                    - b_mix / (V + b_mix));
        lnPhiH2O -= log(pg_bar * V / (R * temperature));

        return exp(lnPhiH2O); // fugacity coefficient of H2O
    }

private:
    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation aCO2_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp) {
            return 8.008e7 - 4.984e4 * temperature;
        }
        else {
            return 7.54e7 - 4.13e4 * temperature;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation aH2O_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp) {
            return 1.337e8 - 1.4e4 * temperature;
        }
        else {
            return 0.0;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation aCO2_H2O_(const Evaluation& temperature, const Evaluation& yH2O, const bool& highTemp)
    {
        if (highTemp) {
            // Pure parameters
            Evaluation aCO2 = aCO2_(temperature, highTemp);
            Evaluation aH2O = aH2O_(temperature, highTemp);

            // Mixture Eq. (A-6)
            Evaluation K_CO2_H2O = 0.4228 - 7.422e-4 * temperature;
            Evaluation K_H2O_CO2 = 1.427e-2 - 4.037e-4 * temperature;
            Evaluation k_CO2_H2O = yH2O * K_H2O_CO2 + (1 - yH2O) * K_CO2_H2O;

            // Eq. (A-5)
            return sqrt(aCO2 * aH2O) * (1 - k_CO2_H2O);
        }
        else {
            return 7.89e7;
        }
    }
    
    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation aMix_(const Evaluation& temperature, const Evaluation& yH2O, const bool& highTemp)
    {
        if (highTemp) {
            // Parameters
            Evaluation aCO2 = aCO2_(temperature, highTemp);
            Evaluation aH2O = aH2O_(temperature, highTemp);
            Evaluation a_CO2_H2O = aCO2_H2O_(temperature, yH2O, highTemp);

            return yH2O * yH2O * aH2O + 2 * yH2O * (1 - yH2O) * a_CO2_H2O + (1 - yH2O) * (1 - yH2O) * aCO2;
        }
        else {
            return aCO2_(temperature, highTemp);
        }
    }


    /*!
    * \brief
    */
    static Scalar bCO2_(const bool& highTemp)
    {
        if (highTemp) {
            return 28.25;
        }
        else {
            return 27.8;
        }
    }

    /*!
    * \brief
    */
    static Scalar bH2O_(const bool& highTemp)
    {
        if (highTemp) {
            return 15.7;
        }
        else {
            return 18.18;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation bMix_(const Evaluation& yH2O, const bool& highTemp)
    {
        if (highTemp) {
            // Parameters
            Scalar bCO2 = bCO2_(highTemp);
            Scalar bH2O = bH2O_(highTemp);

            return yH2O * bH2O + (1 - yH2O) * bCO2;
        }
        else {
            return bCO2_(highTemp);
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation V_avg_CO2_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp && (temperature > 373.15)) {
            return 32.6 + 3.413e-2 * (temperature - 373.15);
        }
        else {
            return 32.6;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation V_avg_H2O_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp && (temperature > 373.15)) {
            return 18.1 + 3.137e-2 * (temperature - 373.15);
        }
        else {
            return 18.1;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation AM_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp && temperature > 373.15) {
            Evaluation deltaTk = temperature - 373.15;
            return deltaTk * (-3.084e-2 + 1.927e-5 * deltaTk);
        }
        else {
            return 0.0;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation Pref_(const Evaluation& temperature, const bool& highTemp)
    {
        if (highTemp && temperature > 373.15) {
            const Evaluation& temperatureCelcius = temperature - 273.15;
            static const Scalar c[5] = { -1.9906e-1, 2.0471e-3, 1.0152e-4, -1.4234e-6, 1.4168e-8 };
            return c[0] + temperatureCelcius * (c[1] + temperatureCelcius * (c[2] + 
                temperatureCelcius * (c[3] + temperatureCelcius * c[4])));
        }
        else {
            return 1.0;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation activityCoefficientCO2_(const Evaluation& temperature, 
                                              const Evaluation& xCO2, 
                                              const bool& highTemp)
    {
        if (highTemp) {
            // Eq. (13)
            Evaluation AM = AM_(temperature, highTemp);
            Evaluation lnGammaCO2 = 2 * AM * xCO2 * (1 - xCO2) * (1 - xCO2);
            return exp(lnGammaCO2);
        }
        else {
            return 1.0;
        }
    }

    /*!
    * \brief
    */
    template <class Evaluation>
    static Evaluation activityCoefficientH2O_(const Evaluation& temperature, 
                                              const Evaluation& xCO2, 
                                              const bool& highTemp)
    {
        if (highTemp) {
            // Eq. (12)
            Evaluation AM = AM_(temperature, highTemp);
            Evaluation lnGammaH2O = (1 - 2 * (1 - xCO2)) * AM * xCO2 * xCO2;
            return exp(lnGammaH2O);
        }
        else {
            return 1.0;
        }
    }

    /*!
     * \brief Returns the molality of NaCl (mol NaCl / kg water) for a given mole fraction
     *
     * \param salinity the salinity [kg NaCl / kg solution]
     */
    template <class Evaluation>
    static Evaluation salinityToMolFrac_(const Evaluation& salinity) {
        OPM_TIMEFUNCTION_LOCAL();
        const Scalar Mw = H2O::molarMass(); /* molecular weight of water [kg/mol] */
        const Scalar Ms = 58.44e-3; /* molecular weight of NaCl  [kg/mol] */

        const Evaluation X_NaCl = salinity;
        /* salinity: conversion from mass fraction to mol fraction */
        const Evaluation x_NaCl = -Mw * X_NaCl / ((Ms - Mw) * X_NaCl - Ms);
        return x_NaCl;
    }

    /*!
     * \brief Returns the molality of NaCl (mol NaCl / kg water) for a given mole fraction (mol NaCl / mol solution)
     *
     * \param x_NaCl mole fraction of NaCL in brine [mol/mol]
     */
    template <class Evaluation>
    static Evaluation moleFracToMolality_(const Evaluation& x_NaCl)
    {
        // conversion from mol fraction to molality (dissolved CO2 neglected)
        return 55.508 * x_NaCl / (1 - x_NaCl);
    }

    /*!
     * \brief Returns the equilibrium molality of CO2 (mol CO2 / kg water) for a
     * CO2-water mixture at a given pressure and temperature
     *
     * \param temperature The temperature [K]
     * \param pg The gas phase pressure [Pa]
     */
    // template <class Evaluation>
    // static Evaluation molalityCO2inPureWater_(const Evaluation& temperature, const Evaluation& pg, bool extrapolate = false)
    // {
    //     const Evaluation& A = computeA_(temperature, pg, extrapolate); // according to Spycher, Pruess and Ennis-King (2003)
    //     const Evaluation& B = computeB_(temperature, pg, extrapolate); // according to Spycher, Pruess and Ennis-King (2003)
    //     const Evaluation& yH2OinGas = (1 - B) / (1. / A - B); // equilibrium mol fraction of H2O in the gas phase
    //     const Evaluation& xCO2inWater = B * (1 - yH2OinGas); // equilibrium mol fraction of CO2 in the water phase
    //     return (xCO2inWater * 55.508) / (1 - xCO2inWater); // CO2 molality
    // }

    /*!
    * \brief Fixed-point iterations for high-temperature cases
    */
    template <class Evaluation>
    static std::pair<Evaluation, Evaluation> highTempSolubility_(const Evaluation& temperature, 
                                                          const Evaluation& pg,
                                                          const Evaluation& m_NaCl,
                                                          bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
	// Start point for fixed-point iterations as recommended below in section 2.2
        Evaluation yH2O = H2O::vaporPressure(temperature) / pg;  // ideal mixing
        Evaluation xCO2 = 0.009;  // same as ~0.5 mol/kg

        // Calculate activity coefficient for salt
        Evaluation gammaNaCl = 1.0;
        if (m_NaCl > 0.0) {
            gammaNaCl = activityCoefficientDuanSun_(temperature, m_NaCl);
        }

        // Options
        int max_iter = 100;
        Scalar tol = 1e-8;
        // bool success = false;
        const bool highTemp = true;

        // Fixed-point loop x_i+1 = F(x_i)
        for (int i = 0; i < max_iter; ++i) {
            // F(x_i) is the mutual solubilities
            auto [yH2O_new, xCO2_new] = mutualSolubility_(temperature, pg, xCO2, yH2O, m_NaCl, gammaNaCl, highTemp, 
                                                          extrapolate);
            
            // Check for convergence
            if (abs(xCO2_new - xCO2) < tol && abs(yH2O_new - yH2O) < tol) {
                // success = true;
                xCO2 = xCO2_new;
                yH2O = yH2O_new;
                break;
            }

            // Else update mole fractions for next iteration
            else {
                xCO2 = xCO2_new;
                yH2O = yH2O_new;
            }
        }

        return {xCO2, yH2O};
    }

    /*!
    * \brief Fixed-point iterations for high-temperature cases
    */
    template <class Evaluation>
    static std::pair<Evaluation, Evaluation> lowTempSolubility_(const Evaluation& temperature, 
                                                         const Evaluation& pg,
                                                         const Evaluation& m_NaCl,
                                                         bool extrapolate = false)
    {
        // Calculate activity coefficient for salt
        Evaluation gammaNaCl = 1.0;
        if (m_NaCl > 0.0) {
            gammaNaCl = activityCoefficientDuanSun_(temperature, m_NaCl);
        }

        // Calculate mutual solubility.
        // Note that we don't use xCO2 and yH2O input in low-temperature case, so we set them to 0.0
        const bool highTemp = false;
        auto [xCO2, yH2O] = mutualSolubility_(temperature, pg, Evaluation(0.0), Evaluation(0.0), m_NaCl, gammaNaCl, 
                                              highTemp, extrapolate);

        return {xCO2, yH2O};
    }

    /*!
    * \brief Mutual solubility according to Spycher & Preuss (2009)
    */
    template <class Evaluation>
    static std::pair<Evaluation, Evaluation> mutualSolubility_(const Evaluation& temperature, 
                                                        const Evaluation& pg,
                                                        const Evaluation& xCO2,
                                                        const Evaluation& yH2O,
                                                        const Evaluation& m_NaCl,
                                                        const Evaluation& gammaNaCl,
                                                        const bool& highTemp,
                                                        bool extrapolate = false)
    {
        // Calculate A and B (without salt effect); Eqs. (8) and (9)
        const Evaluation& A = computeA_(temperature, pg, yH2O, xCO2, highTemp, extrapolate);
        Evaluation B = computeB_(temperature, pg, yH2O, xCO2, highTemp, extrapolate);

        // Add salt effect to B, Eq. (17)
        B /= gammaNaCl;

        // Compute yH2O and xCO2, Eqs. (B-7) and (B-2)
        Evaluation yH2O_new = (1. - B) * 55.508 / ((1. / A - B) * (2 * m_NaCl + 55.508) + 2 * m_NaCl * B);
        Evaluation xCO2_new = B * (1 - yH2O);

        return {xCO2_new, yH2O_new};
    }

    /*!
     * \brief Returns the activity coefficient of CO2 in brine for a
     *           molal description. According to "Duan and Sun 2003"
     *           given in "Spycher and Pruess 2005"
     *
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     * \param molalityNaCl molality of NaCl (mol NaCl / kg water)
     */
    template <class Evaluation>
    static Evaluation activityCoefficient_(const Evaluation& temperature,
                                           const Evaluation& pg,
                                           const Evaluation& molalityNaCl)
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation& lambda = computeLambda_(temperature, pg); // lambda_{CO2-Na+}
        const Evaluation& xi = computeXi_(temperature, pg); // Xi_{CO2-Na+-Cl-}
        const Evaluation& lnGammaStar =
            2*molalityNaCl*lambda + xi*molalityNaCl*molalityNaCl;
        return exp(lnGammaStar);
    }

    /*!
     * \brief Returns the paramater A for the calculation of
     * them mutual solubility in the water-CO2 system.
     * Given in Spycher, Pruess and Ennis-King (2003)
     *
     * \param T the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation computeA_(const Evaluation& temperature, 
                                const Evaluation& pg, 
                                const Evaluation& yH2O,
                                const Evaluation& xCO2,
                                const bool& highTemp,
                                bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
	// Intermediate calculations
        const Evaluation& deltaP = pg / 1e5 - Pref_(temperature, highTemp); // pressure range [bar] from pref to pg[bar]
        Scalar v_av_H2O = V_avg_H2O_(temperature, highTemp); // average partial molar volume of H2O [cm^3/mol]
        Evaluation k0_H2O = equilibriumConstantH2O_(temperature, highTemp); // equilibrium constant for H2O at 1 bar
        Evaluation phi_H2O = fugacityCoefficientH2O(temperature, pg, yH2O, highTemp, extrapolate); // fugacity coefficient of H2O for the water-CO2 system
        Evaluation gammaH2O = activityCoefficientH2O_(temperature, xCO2, highTemp);

        // In the intermediate temperature range 99-109 C, equilibrium constants and fugacity coeff. are linearly
        // weighted
        if ( temperature > 372.15 && temperature < 382.15) {
            const Evaluation weight = (382.15 - temperature) / 10.;
            const Evaluation& k0_H2O_low = equilibriumConstantH2O_(temperature, false);
            const Evaluation& phi_H2O_low = fugacityCoefficientH2O(temperature, pg, Evaluation(0.0), false, extrapolate);
            k0_H2O = k0_H2O * (1 - weight) + k0_H2O_low * weight;
            phi_H2O = phi_H2O * (1 - weight) + phi_H2O_low * weight;
        }

        // Eq. (10)
        const Evaluation& pg_bar = pg / 1.e5;
        Scalar R = IdealGas::R * 10;
        return k0_H2O * gammaH2O / (phi_H2O * pg_bar) * exp(deltaP * v_av_H2O / (R * temperature));
    }

    /*!
     * \brief Returns the paramater B for the calculation of
     * the mutual solubility in the water-CO2 system.
     * Given in Spycher, Pruess and Ennis-King (2003)
     *
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation computeB_(const Evaluation& temperature, 
                                const Evaluation& pg, 
                                const Evaluation& yH2O,
                                const Evaluation& xCO2,
                                const bool& highTemp,
                                bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL();
	// Intermediate calculations
        const Evaluation& deltaP = pg / 1e5 - Pref_(temperature, highTemp); // pressure range [bar] from pref to pg[bar]
        const Scalar& v_av_CO2 = V_avg_CO2_(temperature, highTemp); // average partial molar volume of CO2 [cm^3/mol]
        Evaluation k0_CO2 = equilibriumConstantCO2_(temperature, pg, highTemp); // equilibrium constant for CO2 at 1 bar
        Evaluation phi_CO2 = fugacityCoefficientCO2(temperature, pg, yH2O, highTemp, extrapolate); // fugacity coefficient of CO2 for the water-CO2 system
        Evaluation gammaCO2 = activityCoefficientCO2_(temperature, xCO2, highTemp);

        // In the intermediate temperature range 99-109 C, equilibrium constants and fugacity coeff. are linearly
        // weighted
        if ( temperature > 372.15 && temperature < 382.15) {
            const Evaluation weight = (382.15 - temperature) / 10.;
            const Evaluation& k0_CO2_low = equilibriumConstantCO2_(temperature, pg, false);
            const Evaluation& phi_CO2_low = fugacityCoefficientCO2(temperature, pg, Evaluation(0.0), false, extrapolate);
            k0_CO2 = k0_CO2 * (1 - weight) + k0_CO2_low * weight;
            phi_CO2 = phi_CO2 * (1 - weight) + phi_CO2_low * weight;
        }

        // Eq. (11)
        const Evaluation& pg_bar = pg / 1.e5;
        const Scalar R = IdealGas::R * 10;
        return phi_CO2 * pg_bar / (55.508 * k0_CO2 * gammaCO2) * exp(-deltaP * v_av_CO2 / (R * temperature));
    }

    /*!
     * \brief Returns the parameter lambda, which is needed for the
     * calculation of the CO2 activity coefficient in the brine-CO2 system.
     * Given in Spycher and Pruess (2005)
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation computeLambda_(const Evaluation& temperature, const Evaluation& pg)
    {
        OPM_TIMEFUNCTION_LOCAL();
        static const Scalar c[6] =
            { -0.411370585, 6.07632013E-4, 97.5347708, -0.0237622469, 0.0170656236, 1.41335834E-5 };

        Evaluation pg_bar = pg / 1.0E5; /* conversion from Pa to bar */
        return
            c[0]
            + c[1]*temperature
            + c[2]/temperature
            + c[3]*pg_bar/temperature
            + c[4]*pg_bar/(630.0 - temperature)
            + c[5]*temperature*log(pg_bar);
    }

    /*!
     * \brief Returns the parameter xi, which is needed for the
     * calculation of the CO2 activity coefficient in the brine-CO2 system.
     * Given in Spycher and Pruess (2005)
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    static Evaluation computeXi_(const Evaluation& temperature, const Evaluation& pg)
    {
        OPM_TIMEFUNCTION_LOCAL();
        static const Scalar c[4] =
            { 3.36389723E-4, -1.98298980E-5, 2.12220830E-3, -5.24873303E-3 };

        Evaluation pg_bar = pg / 1.0E5; /* conversion from Pa to bar */
        return c[0] + c[1]*temperature + c[2]*pg_bar/temperature + c[3]*pg_bar/(630.0 - temperature);
    }

    /*!
    * \brief Activity model from Duan & Sun as modified and detailed in Spycher & Preuss (2009)
    */
    template <class Evaluation>
    static Evaluation activityCoefficientDuanSun_(const Evaluation& temperature, const Evaluation& m_NaCl)
    {
        // Lambda and xi parameters
        const Evaluation& lambda = computeLambdaDuanSun_(temperature);
        const Evaluation& xi = computeXiDuanSun_(temperature);
        const Evaluation& lnGamma = 2 * lambda * m_NaCl + xi * m_NaCl * m_NaCl;

        // Eq. (18), return activity coeff. on mole-fraction scale
        return (1 + m_NaCl / 55.508) * exp(lnGamma);
    }

    /*!
    * \brief Lambda parameter in Duan & Sun model, as modified and detailed in Spycher & Preuss (2009)
    */
    template <class Evaluation>
    static Evaluation computeLambdaDuanSun_(const Evaluation& temperature)
    {
        // Table 1
        static const Scalar c[3] = { 2.217e-4, 1.074, 2648. };

        // Eq. (19)
        return c[0] * temperature + c[1] / temperature + c[2] / (temperature * temperature);
    }

    /*!
    * \brief Xi parameter in Duan & Sun model, as modified and detailed in Spycher & Preuss (2009)
    */
    template <class Evaluation>
    static Evaluation computeXiDuanSun_(const Evaluation& temperature)
    {
        // Table 1
        static const Scalar c[3] = { 1.3e-5, -20.12, 5259. };

        // Eq. (19)
        return c[0] * temperature + c[1] / temperature + c[2] / (temperature * temperature);
    }

    /*!
     * \brief Returns the equilibrium constant for CO2, which is needed for the
     * calculation of the mutual solubility in the water-CO2 system
     * Given in Spycher, Pruess and Ennis-King (2003)
     * \param temperature the temperature [K]
     */
    template <class Evaluation>
    static Evaluation equilibriumConstantCO2_(const Evaluation& temperature, const Evaluation& pg, const bool& highTemp)
    {
        OPM_TIMEFUNCTION_LOCAL();
        Evaluation temperatureCelcius = temperature - 273.15;
        std::array<Scalar, 4> c;
        if (highTemp) {
            c = { 1.668, 3.992e-3, -1.156e-5, 1.593e-9 };
        }
        else {
            // For temperature below 31 C and pressures above saturation pressure, separate parameters are needed
            if (temperatureCelcius < 31) {
                Evaluation psat = CO2::vaporPressure(temperature);
                if (pg > psat) {
                    c = { 1.169, 1.368e-2, -5.38e-5, 0.0 };
                }
            }
            else {
                c = { 1.189, 1.304e-2, -5.446e-5, 0.0 };
            }
        }
        Evaluation logk0_CO2 = c[0] + temperatureCelcius * (c[1] + temperatureCelcius * 
                              (c[2] + temperatureCelcius * c[3]));
        Evaluation k0_CO2 = pow(10.0, logk0_CO2);
        return k0_CO2;
    }

    /*!
     * \brief Returns the equilibrium constant for H2O, which is needed for the
     * calculation of the mutual solubility in the water-CO2 system
     * Given in Spycher, Pruess and Ennis-King (2003)
     * \param temperature the temperature [K]
     */
    template <class Evaluation>
    static Evaluation equilibriumConstantH2O_(const Evaluation& temperature, const bool& highTemp)
    {
        OPM_TIMEFUNCTION_LOCAL();
        Evaluation temperatureCelcius = temperature - 273.15;
        std::array<Scalar, 5> c;
        if (highTemp){
            c = { -2.1077, 2.8127e-2, -8.4298e-5, 1.4969e-7, -1.1812e-10 };
        }
        else {
            c = { -2.209, 3.097e-2, -1.098e-4, 2.048e-7, 0.0 };
        }
        Evaluation logk0_H2O = c[0] + temperatureCelcius * (c[1] + temperatureCelcius * (c[2] + 
            temperatureCelcius * (c[3] + temperatureCelcius * c[4])));
        return pow(10.0, logk0_H2O);
    }

};

} // namespace BinaryCoeff
} // namespace Opm

#endif
