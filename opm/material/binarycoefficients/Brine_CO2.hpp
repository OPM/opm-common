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
#include <opm/common/TimingMacros.hpp>
#include <opm/common/utility/gpuDecorators.hpp>

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
     * According to "Diffusion of Water in Liquid and Supercritical Carbon Dioxide:
     * An NMR Study",Bin Xu et al., 2002
     * \param params Parameters to use
     * \param temperature the temperature [K]
     * \param pressure the phase pressure [Pa]
     * \param extrapolate True to use extrapolation
     */
    template <class Evaluation, class CO2Params>
    OPM_HOST_DEVICE static Evaluation gasDiffCoeff(const CO2Params& params,
                                                   const Evaluation& temperature,
                                                   const Evaluation& pressure,
                                                   bool extrapolate = false)
    {
        //Diffusion coefficient of water in the CO2 phase
        Scalar k = 1.3806504e-23; // Boltzmann constant
        Scalar c = 4; // slip parameter, can vary between 4 (slip condition) and 6 (stick condition)
        Scalar R_h = 1.72e-10; // hydrodynamic radius of the solute
        const Evaluation& mu = CO2::gasViscosity(params, temperature, pressure, extrapolate); // CO2 viscosity
        return k / (c * M_PI * R_h) * (temperature / mu);
    }

    /*!
     * \brief Binary diffusion coefficent [m^2/s] of CO2 in the brine phase.
     *
     * \param temperature the temperature [K]
     * \param pressure the phase pressure [Pa]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation liquidDiffCoeff(const Evaluation& /*temperature*/, const Evaluation& /*pressure*/)
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
     * \param params Parameters to use
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     * \param salinity the salinity [kg NaCl / kg solution]
     * \param knownPhaseIdx indicates which phases are present
     * \param xlCO2 mole fraction of CO2 in brine [mol/mol]
     * \param ygH2O mole fraction of water in the gas phase [mol/mol]
     * \param activityModel Activity model to use
     * \param extrapolate True to use extrapolation
     */
    template <class Evaluation, class CO2Params>
    OPM_HOST_DEVICE static void
    calculateMoleFractions(const CO2Params& params,
                           const Evaluation& temperature,
                           const Evaluation& pg,
                           const Evaluation& salinity,
                           const int knownPhaseIdx,
                           Evaluation& xlCO2,
                           Evaluation& ygH2O,
                           const int& activityModel,
                           bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);

        // Iterate or not?
        bool iterate = false;
        if ((activityModel == 1 && salinity > 0.0) || (activityModel == 2 && temperature > 372.15)) {
            iterate = true;
        }

        // If both phases are present the mole fractions in each phase can be calculate with the mutual solubility
        // function
        if (knownPhaseIdx < 0) {
            Evaluation molalityNaCl = massFracToMolality_(salinity); // mass fraction to molality of NaCl

            // Duan-Sun model as given in Spycher & Pruess (2005) have a different fugacity coefficient formula and
            // activity coefficient definition (not a true activity coefficient but a ratio).
            // Technically only valid below T = 100 C, but we use low-temp. parameters and formulas even above 100 C as
            // an approximation.
            if (activityModel == 3) {
                auto [xCO2, yH2O] = mutualSolubilitySpycherPruess2005_(params, temperature, pg, molalityNaCl, extrapolate);
                xlCO2 = xCO2;
                ygH2O = yH2O;

            }
            else {
                // Fixed-point iterations to calculate solubility
                if (iterate) {
                    auto [xCO2, yH2O] = fixPointIterSolubility_(params, temperature, pg, molalityNaCl, activityModel, extrapolate);
                    xlCO2 = xCO2;
                    ygH2O = yH2O;
                }

                // Solve mutual solubility equation with back substitution (no need for iterations)
                else {
                    auto [xCO2, yH2O] = nonIterSolubility_(params, temperature, pg, molalityNaCl, activityModel, extrapolate);
                    xlCO2 = xCO2;
                    ygH2O = yH2O;
                }
            }
        }

        // if only liquid phase is present the mole fraction of CO2 in brine is given and
        // and the virtual equilibrium mole fraction of water in the non-existing gas phase can be estimated
        // with the mutual solubility function
        else if (knownPhaseIdx == liquidPhaseIdx && activityModel == 3) {
            Evaluation x_NaCl = salinityToMolFrac_(salinity);
            const Evaluation& A = computeA_(params, temperature, pg, Evaluation(0.0), Evaluation(0.0), false, extrapolate, true);
            ygH2O = A * (1 - xlCO2 - x_NaCl);
        }

        // if only gas phase is present the mole fraction of water in the gas phase is given and
        // and the virtual equilibrium mole fraction of CO2 in the non-existing liquid phase can be estimated
        // with the mutual solubility function
        else if (knownPhaseIdx == gasPhaseIdx && activityModel == 3) {
            //y_H2o = fluidstate.
            Evaluation x_NaCl = salinityToMolFrac_(salinity);
            const Evaluation& A = computeA_(params, temperature, pg, Evaluation(0.0), Evaluation(0.0), false, extrapolate, true);
            xlCO2 = 1 - x_NaCl - ygH2O / A;
        }
    }

    /*!
     * \brief Henry coefficent \f$\mathrm{[N/m^2]}\f$ for CO2 in brine.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation henry(const Evaluation& temperature, bool extrapolate = false)
    { return fugacityCoefficientCO2(temperature, /*pressure=*/1e5, extrapolate)*1e5; }

    /*!
     * \brief Returns the fugacity coefficient of the CO2 component in a water-CO2 mixture
     *
     * (given in Spycher, Pruess and Ennis-King (2003))
     *
     * \param params Parameters to use
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     * \param yH2O mole fraction of water
     * \param highTemp True to use high temperature
     * \param extrapolate True to use extrapolation
     * \param spycherPruess2005 True to Spycher-Pruess (2005) model
     */
    template <class Evaluation, class CO2Params>
    OPM_HOST_DEVICE static Evaluation fugacityCoefficientCO2(const CO2Params& params,
                                             const Evaluation& temperature,
                                             const Evaluation& pg,
                                             const Evaluation& yH2O,
                                             const bool highTemp,
                                             bool extrapolate = false,
                                             bool spycherPruess2005 = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Valgrind::CheckDefined(temperature);
        Valgrind::CheckDefined(pg);

        const Evaluation V = 1 / (CO2::gasDensity(params, temperature, pg, extrapolate) / CO2::molarMass()) * 1.e6; // molar volume in cm^3/mol
        const Evaluation pg_bar = pg / 1.e5; // gas phase pressure in bar
        const Scalar R = IdealGas::R * 10.; // ideal gas constant with unit bar cm^3 /(K mol)

        // Parameters in Redlich-Kwong equation
        const Evaluation a_CO2 = aCO2_(temperature, highTemp);
        const Evaluation a_CO2_H2O = aCO2_H2O_(temperature, yH2O, highTemp);
        const Evaluation a_mix = aMix_(temperature, yH2O, highTemp);
        const Scalar b_CO2 = bCO2_(highTemp);
        const Evaluation b_mix = bMix_(yH2O, highTemp);
        const Evaluation Rt15 = R * pow(temperature, 1.5);

        Evaluation lnPhiCO2;
        if (spycherPruess2005) {
            const Evaluation logVpb_V = log((V + b_CO2) / V);
            lnPhiCO2 = log(V / (V - b_CO2));
            lnPhiCO2 += b_CO2 / (V - b_CO2);
            lnPhiCO2 -= 2 * a_CO2 / (Rt15 * b_CO2) * logVpb_V;
            lnPhiCO2 +=
                a_CO2 * b_CO2
                / (Rt15
                   * b_CO2
                   * b_CO2)
                * (logVpb_V
                   - b_CO2 / (V + b_CO2));
            lnPhiCO2 -= log(pg_bar * V / (R * temperature));
        }
        else {
            lnPhiCO2 = (b_CO2 / b_mix) * (pg_bar * V / (R * temperature) - 1);
            lnPhiCO2 -= log(pg_bar * (V - b_mix) / (R * temperature));
            lnPhiCO2 += (2 * (yH2O * a_CO2_H2O + (1 - yH2O) * a_CO2) / a_mix - (b_CO2 / b_mix)) *
                        a_mix / (b_mix * Rt15) * log(V / (V + b_mix));
        }
        return exp(lnPhiCO2); // fugacity coefficient of CO2
    }

    /*!
     * \brief Returns the fugacity coefficient of the H2O component in a water-CO2 mixture
     *
     * (given in Spycher, Pruess and Ennis-King (2003))
     *
     * \param params Parameters to use
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     * \param yH2O mole fraction of water
     * \param highTemp True to use high temperature
     * \param extrapolate True to use extrapolation
     * \param spycherPruess2005 True to Spycher-Pruess (2005) model
     */
    template <class Evaluation, class CO2Params>
    OPM_HOST_DEVICE static Evaluation fugacityCoefficientH2O(const CO2Params& params,
                                             const Evaluation& temperature,
                                             const Evaluation& pg,
                                             const Evaluation& yH2O,
                                             const bool highTemp,
                                             bool extrapolate = false,
                                             bool spycherPruess2005 = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Valgrind::CheckDefined(temperature);
        Valgrind::CheckDefined(pg);

        const Evaluation& V = 1 / (CO2::gasDensity(params, temperature, pg, extrapolate) / CO2::molarMass()) * 1.e6; // molar volume in cm^3/mol
        const Evaluation& pg_bar = pg / 1.e5; // gas phase pressure in bar
        const Scalar R = IdealGas::R * 10.; // ideal gas constant with unit bar cm^3 /(K mol)

        // Mixture parameter of  Redlich-Kwong equation
        const Evaluation a_H2O = aH2O_(temperature, highTemp);
        const Evaluation a_CO2_H2O = aCO2_H2O_(temperature, yH2O, highTemp);
        const Evaluation a_mix = aMix_(temperature, yH2O, highTemp);
        const Scalar b_H2O = bH2O_(highTemp);
        const Evaluation b_mix = bMix_(yH2O, highTemp);
        const Evaluation Rt15 = R * pow(temperature, 1.5);

        Evaluation lnPhiH2O;
        if (spycherPruess2005) {
            const Evaluation logVpb_V = log((V + b_mix) / V);
            lnPhiH2O =
                log(V/(V - b_mix))
                + b_H2O/(V - b_mix) - 2*a_CO2_H2O
                / (Rt15*b_mix)*logVpb_V
                + a_mix*b_H2O/(Rt15*b_mix*b_mix)
                *(logVpb_V - b_mix/(V + b_mix))
                - log(pg_bar*V/(R*temperature));
        }
        else {
            lnPhiH2O = (b_H2O / b_mix) * (pg_bar * V / (R * temperature) - 1);
            lnPhiH2O -= log(pg_bar * (V - b_mix) / (R * temperature));
            lnPhiH2O += (2 * (yH2O * a_H2O + (1 - yH2O) * a_CO2_H2O) / a_mix - (b_H2O / b_mix)) *
                        a_mix / (b_mix * Rt15) * log(V / (V + b_mix));
        }
        return exp(lnPhiH2O); // fugacity coefficient of H2O
    }

private:
    /*!
    * \brief
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation aCO2_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation aH2O_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation aCO2_H2O_(const Evaluation& temperature, const Evaluation& yH2O, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation aMix_(const Evaluation& temperature, const Evaluation& yH2O, const bool& highTemp)
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
    OPM_HOST_DEVICE static Scalar bCO2_(const bool& highTemp)
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
    OPM_HOST_DEVICE static Scalar bH2O_(const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation bMix_(const Evaluation& yH2O, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation V_avg_CO2_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation V_avg_H2O_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation AM_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation Pref_(const Evaluation& temperature, const bool& highTemp)
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
    OPM_HOST_DEVICE static Evaluation activityCoefficientCO2_(const Evaluation& temperature,
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
    OPM_HOST_DEVICE static Evaluation activityCoefficientH2O_(const Evaluation& temperature,
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
    OPM_HOST_DEVICE static Evaluation salinityToMolFrac_(const Evaluation& salinity) {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
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
#if 0
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation moleFracToMolality_(const Evaluation& x_NaCl)
    {
        // conversion from mol fraction to molality (dissolved CO2 neglected)
        return 55.508 * x_NaCl / (1 - x_NaCl);
    }
#endif

    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation massFracToMolality_(const Evaluation& X_NaCl)
    {
        const Scalar MmNaCl = 58.44e-3;
        return X_NaCl / (MmNaCl * (1 - X_NaCl));
    }

    /*!
    * \brief Returns the mole fraction NaCl; inverse of moleFracToMolality
    *
    * \param x_NaCl mole fraction of NaCL in brine [mol/mol]
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation molalityToMoleFrac_(const Evaluation& m_NaCl)
    {
        // conversion from molality to mole fractio (dissolved CO2 neglected)
        return m_NaCl / (55.508 + m_NaCl);
    }

    /*!
    * \brief Fixed-point iterations for high-temperature cases
    */
    template <class Evaluation, class CO2Parameters>
    OPM_HOST_DEVICE static std::pair<Evaluation, Evaluation> fixPointIterSolubility_(const CO2Parameters& params,
                                                                     const Evaluation& temperature,
                                                                     const Evaluation& pg,
                                                                     const Evaluation& m_NaCl,
                                                                     const int& activityModel,
                                                                     bool extrapolate = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
	    // Start point for fixed-point iterations as recommended below in section 2.2
        Evaluation yH2O = H2O::vaporPressure(temperature) / pg;  // ideal mixing
        Evaluation xCO2 = 0.009;  // same as ~0.5 mol/kg
        Evaluation gammaNaCl = 1.0;  // default salt activity coeff = 1.0

        // We can pre-calculate Duan-Sun, Spycher & Pruess (2009) salt activity coeff.
        if (m_NaCl > 0.0 && activityModel == 2) {
            gammaNaCl = activityCoefficientSalt_(temperature, pg, m_NaCl, Evaluation(0.0), activityModel);
        }

        // Options
        int max_iter = 100;
        Scalar tol = 1e-8;
        bool highTemp = true;
        if (activityModel == 1) {
            highTemp = false;
        }
        const bool iterate = true;

        // Fixed-point loop x_i+1 = F(x_i)
        for (int i = 0; i < max_iter; ++i) {
            // Calculate activity coefficient for Rumpf et al (1994) model
            if (m_NaCl > 0.0 && activityModel == 1) {
                gammaNaCl = activityCoefficientSalt_(temperature, pg, m_NaCl, xCO2, activityModel);
            }

            // F(x_i) is the mutual solubilities
            auto [xCO2_new, yH2O_new] = mutualSolubility_(params, temperature, pg, xCO2, yH2O, m_NaCl, gammaNaCl, highTemp,
                                                          iterate, extrapolate);

            // Check for convergence
            if (abs(xCO2_new - xCO2) < tol && abs(yH2O_new - yH2O) < tol) {
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
    template <class Evaluation, class CO2Parameters>
    OPM_HOST_DEVICE static std::pair<Evaluation, Evaluation> nonIterSolubility_(const CO2Parameters& params,
                                                                const Evaluation& temperature,
                                                                const Evaluation& pg,
                                                                const Evaluation& m_NaCl,
                                                                const int& activityModel,
                                                                bool extrapolate = false)
    {
        // Calculate activity coefficient for salt
        Evaluation gammaNaCl = 1.0;
        if (m_NaCl > 0.0 && activityModel > 0 && activityModel < 3) {
            gammaNaCl = activityCoefficientSalt_(temperature, pg, m_NaCl, Evaluation(0.0), activityModel);
        }

        // Calculate mutual solubility.
        // Note that we don't use xCO2 and yH2O input in low-temperature case, so we set them to 0.0
        const bool highTemp = false;
        const bool iterate = false;
        auto [xCO2, yH2O] = mutualSolubility_(params, temperature, pg, Evaluation(0.0), Evaluation(0.0), m_NaCl, gammaNaCl,
                                              highTemp, iterate, extrapolate);

        return {xCO2, yH2O};
    }

    /*!
    * \brief Mutual solubility according to Spycher & Pruess (2009)
    */
    template <class Evaluation, class CO2Parameters>
    OPM_HOST_DEVICE static std::pair<Evaluation, Evaluation> mutualSolubility_(const CO2Parameters& params,
                                                               const Evaluation& temperature,
                                                               const Evaluation& pg,
                                                               const Evaluation& xCO2,
                                                               const Evaluation& yH2O,
                                                               const Evaluation& m_NaCl,
                                                               const Evaluation& gammaNaCl,
                                                               const bool& highTemp,
                                                               const bool& iterate,
                                                               bool extrapolate = false)
    {
        // Calculate A and B (without salt effect); Eqs. (8) and (9)
        const Evaluation& A = computeA_(params, temperature, pg, yH2O, xCO2, highTemp, extrapolate);
        Evaluation B = computeB_(params, temperature, pg, yH2O, xCO2, highTemp, extrapolate);

        // Add salt effect to B, Eq. (17)
        B /= gammaNaCl;

        // Compute yH2O and xCO2, Eqs. (B-7) and (B-2)
        Evaluation yH2O_new = (1. - B) * 55.508 / ((1. / A - B) * (2 * m_NaCl + 55.508) + 2 * m_NaCl * B);
        Evaluation xCO2_new;
        if (iterate) {
            xCO2_new = B * (1 - yH2O);
        }
        else {
            xCO2_new = B * (1 - yH2O_new);
        }

        return {xCO2_new, yH2O_new};
    }

    /*!
    * \brief Mutual solubility according to Spycher & Pruess (2009)
    */
    template <class Evaluation, class CO2Parameters>
    OPM_HOST_DEVICE static std::pair<Evaluation, Evaluation> mutualSolubilitySpycherPruess2005_(const CO2Parameters& params,
                                                                                const Evaluation& temperature,
                                                                                const Evaluation& pg,
                                                                                const Evaluation& m_NaCl,
                                                                                bool extrapolate = false)
    {
        // Calculate A and B (without salt effect); Eqs. (8) and (9)
        const Evaluation& A = computeA_(params, temperature, pg, Evaluation(0.0), Evaluation(0.0), false, extrapolate, true);
        const Evaluation& B = computeB_(params, temperature, pg, Evaluation(0.0), Evaluation(0.0), false, extrapolate, true);

        // Mole fractions and molality in pure water
        Evaluation yH2O = (1 - B) / (1. / A - B);
        Evaluation xCO2 = B * (1 - yH2O);

        // Modifiy mole fractions with Duan-Sun "activity coefficient" if salt is involved
        if (m_NaCl > 0.0) {
            const Evaluation& gammaNaCl = activityCoefficientSalt_(temperature, pg, m_NaCl, Evaluation(0.0), 3);

            // Molality with salt
            Evaluation mCO2 = (xCO2 * 55.508) / (1 - xCO2);  // pure water
            mCO2 /= gammaNaCl;
            xCO2 = mCO2 / (m_NaCl + 55.508 + mCO2);

            // new yH2O with salt
            const Evaluation& xNaCl = molalityToMoleFrac_(m_NaCl);
            yH2O = A * (1 - xCO2 - xNaCl);
        }

        return {xCO2, yH2O};
    }

    /*!
     * \brief Returns the paramater A for the calculation of
     * them mutual solubility in the water-CO2 system.
     * Given in Spycher, Pruess and Ennis-King (2003)
     *
     * \param T the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation, class CO2Params>
    OPM_HOST_DEVICE static Evaluation computeA_(const CO2Params& params,
                                const Evaluation& temperature,
                                const Evaluation& pg,
                                const Evaluation& yH2O,
                                const Evaluation& xCO2,
                                const bool& highTemp,
                                bool extrapolate = false,
                                bool spycherPruess2005 = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
	    // Intermediate calculations
        const Evaluation& deltaP = pg / 1e5 - Pref_(temperature, highTemp); // pressure range [bar] from pref to pg[bar]
        Evaluation v_av_H2O = V_avg_H2O_(temperature, highTemp); // average partial molar volume of H2O [cm^3/mol]
        Evaluation k0_H2O = equilibriumConstantH2O_(temperature, highTemp); // equilibrium constant for H2O at 1 bar
        Evaluation phi_H2O = fugacityCoefficientH2O(params, temperature, pg, yH2O, highTemp, extrapolate, spycherPruess2005); // fugacity coefficient of H2O for the water-CO2 system
        Evaluation gammaH2O = activityCoefficientH2O_(temperature, xCO2, highTemp);

        // In the intermediate temperature range 99-109 C, equilibrium constants and fugacity coeff. are linearly
        // weighted
        if ( temperature > 372.15 && temperature < 382.15 && !spycherPruess2005) {
            const Evaluation weight = (382.15 - temperature) / 10.;
            const Evaluation& k0_H2O_low = equilibriumConstantH2O_(temperature, false);
            const Evaluation& phi_H2O_low = fugacityCoefficientH2O(params, temperature, pg, Evaluation(0.0), false, extrapolate);
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
    template <class Evaluation, class CO2Parameters>
    OPM_HOST_DEVICE static Evaluation computeB_(const CO2Parameters& params,
                                const Evaluation& temperature,
                                const Evaluation& pg,
                                const Evaluation& yH2O,
                                const Evaluation& xCO2,
                                const bool& highTemp,
                                bool extrapolate = false,
                                bool spycherPruess2005 = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
	    // Intermediate calculations
        const Evaluation& deltaP = pg / 1e5 - Pref_(temperature, highTemp); // pressure range [bar] from pref to pg[bar]
        Evaluation v_av_CO2 = V_avg_CO2_(temperature, highTemp); // average partial molar volume of CO2 [cm^3/mol]
        Evaluation k0_CO2 = equilibriumConstantCO2_(temperature, pg, highTemp, spycherPruess2005); // equilibrium constant for CO2 at 1 bar
        Evaluation phi_CO2 = fugacityCoefficientCO2(params, temperature, pg, yH2O, highTemp, extrapolate, spycherPruess2005); // fugacity coefficient of CO2 for the water-CO2 system
        Evaluation gammaCO2 = activityCoefficientCO2_(temperature, xCO2, highTemp);

        // In the intermediate temperature range 99-109 C, equilibrium constants and fugacity coeff. are linearly
        // weighted
        if ( temperature > 372.15 && temperature < 382.15 && !spycherPruess2005) {
            const Evaluation weight = (382.15 - temperature) / 10.;
            const Evaluation& k0_CO2_low = equilibriumConstantCO2_(temperature, pg, false, spycherPruess2005);
            const Evaluation& phi_CO2_low = fugacityCoefficientCO2(params, temperature, pg, Evaluation(0.0), false, extrapolate);
            k0_CO2 = k0_CO2 * (1 - weight) + k0_CO2_low * weight;
            phi_CO2 = phi_CO2 * (1 - weight) + phi_CO2_low * weight;
        }

        // Eq. (11)
        const Evaluation& pg_bar = pg / 1.e5;
        const Scalar R = IdealGas::R * 10;
        return phi_CO2 * pg_bar / (55.508 * k0_CO2 * gammaCO2) * exp(-deltaP * v_av_CO2 / (R * temperature));
    }

    /*!
    * \brief Activity model of salt in Spycher & Pruess (2009)
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation activityCoefficientSalt_(const Evaluation& temperature,
                                               const Evaluation& pg,
                                               const Evaluation& m_NaCl,
                                               const Evaluation& xCO2,
                                               const int& activityModel)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
	    // Lambda and xi parameter for either Rumpf et al (1994) (activityModel = 1) or Duan-Sun as modified by Spycher
        // & Pruess (2009) (activityModel = 2) or Duan & Sun (2003) as given in Spycher & Pruess (2005) (activityModel =
        // 3)
        Evaluation lambda;
        Evaluation xi;
        Evaluation convTerm;
        if (activityModel == 1) {
            lambda = computeLambdaRumpfetal_(temperature);
            xi = -0.0028 * 3.0;
            Evaluation m_CO2 = xCO2 * (2 * m_NaCl + 55.508) / (1 - xCO2);
            convTerm = (1 + (m_CO2 + 2 * m_NaCl) / 55.508) / (1 + m_CO2 / 55.508);
        }
        else if (activityModel == 2) {
            lambda = computeLambdaSpycherPruess2009_(temperature);
            xi = computeXiSpycherPruess2009_(temperature);
            convTerm = 1 + 2 * m_NaCl / 55.508;
        }
        else if (activityModel == 3) {
            lambda = computeLambdaDuanSun_(temperature, pg);
            xi = computeXiDuanSun_(temperature, pg);
            convTerm = 1.0;
        }
        else {
            assert(false && "Activity model for salt-out effect has not been implemented!"); // temporary assert
            // throw std::runtime_error("Activity model for salt-out effect has not been implemented!");
        }

        // Eq. (18)
        const Evaluation& lnGamma = 2 * lambda * m_NaCl + xi * m_NaCl * m_NaCl;

        // Eq. (18), return activity coeff. on mole-fraction scale
        return convTerm * exp(lnGamma);
    }

    /*!
    * \brief Lambda parameter in Duan & Sun model, as modified and detailed in Spycher & Pruess (2009)
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation computeLambdaSpycherPruess2009_(const Evaluation& temperature)
    {
        // Table 1
        static const Scalar c[3] = { 2.217e-4, 1.074, 2648. };

        // Eq. (19)
        return c[0] * temperature + c[1] / temperature + c[2] / (temperature * temperature);
    }

    /*!
    * \brief Xi parameter in Duan & Sun model, as modified and detailed in Spycher & Pruess (2009)
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation computeXiSpycherPruess2009_(const Evaluation& temperature)
    {
        // Table 1
        static const Scalar c[3] = { 1.3e-5, -20.12, 5259. };

        // Eq. (19)
        return c[0] * temperature + c[1] / temperature + c[2] / (temperature * temperature);
    }

    /*!
    * \brief Lambda parameter in Rumpf et al. (1994), as detailed in Spycher & Pruess (2005)
    */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation computeLambdaRumpfetal_(const Evaluation& temperature)
    {
        // B^(0) below Eq. (A-6)
        static const Scalar c[4] = { 0.254, -76.82, -10656, 6312e3 };

        return c[0] + c[1] / temperature + c[2] / (temperature * temperature) +
            c[3] / (temperature * temperature * temperature);
    }

    /*!
     * \brief Returns the parameter lambda, which is needed for the
     * calculation of the CO2 activity coefficient in the brine-CO2 system.
     * Given in Spycher and Pruess (2005)
     * \param temperature the temperature [K]
     * \param pg the gas phase pressure [Pa]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation computeLambdaDuanSun_(const Evaluation& temperature, const Evaluation& pg)
    {
        static const Scalar c[6] =
            { -0.411370585, 6.07632013E-4, 97.5347708, -0.0237622469, 0.0170656236, 1.41335834E-5 };

        Evaluation pg_bar = pg / 1.0E5; /* conversion from Pa to bar */
        return c[0] + c[1]*temperature + c[2]/temperature + c[3]*pg_bar/temperature + c[4]*pg_bar/(630.0 - temperature)
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
    OPM_HOST_DEVICE static Evaluation computeXiDuanSun_(const Evaluation& temperature, const Evaluation& pg)
    {
        static const Scalar c[4] =
            { 3.36389723E-4, -1.98298980E-5, 2.12220830E-3, -5.24873303E-3 };

        Evaluation pg_bar = pg / 1.0E5; /* conversion from Pa to bar */
        return c[0] + c[1]*temperature + c[2]*pg_bar/temperature + c[3]*pg_bar/(630.0 - temperature);
    }

    /*!
     * \brief Returns the equilibrium constant for CO2, which is needed for the
     * calculation of the mutual solubility in the water-CO2 system
     * Given in Spycher, Pruess and Ennis-King (2003)
     * \param temperature the temperature [K]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE static Evaluation equilibriumConstantCO2_(const Evaluation& temperature,
                                              const Evaluation& pg,
                                              const bool& highTemp,
                                              bool spycherPruess2005 = false)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Evaluation temperatureCelcius = temperature - 273.15;
        std::array<Scalar, 4> c;
        if (highTemp) {
            c = { 1.668, 3.992e-3, -1.156e-5, 1.593e-9 };
        }
        else {
            // For temperature below critical temperature and pressures above saturation pressure, separate parameters are needed
            bool model1 = temperature < CO2::criticalTemperature() && !spycherPruess2005;
            if (model1) {
                // Computing the vapor pressure is not trivial and is also not defined for T > criticalTemperature
                Evaluation psat = CO2::vaporPressure(temperature);
                model1 = pg > psat;
            }
            if (model1) {
                c = { 1.169, 1.368e-2, -5.38e-5, 0.0 };
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
    OPM_HOST_DEVICE static Evaluation equilibriumConstantH2O_(const Evaluation& temperature, const bool& highTemp)
    {
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
