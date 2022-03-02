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
 * \copydoc Opm::BinaryCoeff::Brine_H2
 */
#ifndef OPM_BINARY_COEFF_BRINE_H2_HPP
#define OPM_BINARY_COEFF_BRINE_H2_HPP

#include <opm/material/IdealGas.hpp>
#include <opm/material/binarycoefficients/FullerMethod.hpp>

namespace Opm {
namespace BinaryCoeff {

/*!
* \ingroup Binarycoefficients
* \brief Binary coefficients for brine and CO2.
*/
template<class Scalar, class H2O, class H2, bool verbose = true>
class Brine_H2 {
    using IdealGas = Opm::IdealGas<Scalar>;
    static const int liquidPhaseIdx = 0; // index of the liquid phase
    static const int gasPhaseIdx = 1; // index of the gas phase

public:
    /*!
    * \brief Returns the _mol_ (!) fraction of H2 in the liquid phase for a given temperature, pressure, H2 molality and
    * brine salinity. Implemented according to Li et al., Int. J. Hydrogen Energ., 2018.
    *
    * \param temperature temperature [K]
    * \param pg gas phase pressure [Pa]
    * \param salinity salinity [mol NaCl / kg solution]
    * \param knownPhaseIdx indicates which phases are present
    * \param xlH2 mole fraction of H2 in brine [mol/mol]
    */
    template <class Evaluation>
    static void calculateMoleFractions(const Evaluation& temperature,
                                       const Evaluation& pg,
                                       Scalar salinity,
                                       Evaluation& xH2)
    {
        // All intermediate calculations
        Evaluation lnYH2 = moleFractionGasH2_(temperature, pg);
        Evaluation lnPg = log(pg / 1e6);  // Pa --> MPa before ln
        Evaluation lnPhiH2 = fugacityCoefficientH2(temperature, pg);
        Evaluation lnKh = henrysConstant_(temperature);
        Evaluation PF = computePoyntingFactor_(temperature, pg);
        Evaluation lnGammaH2 = activityCoefficient_(temperature, salinity);

        // Eq. (6) to get molality of H2 in brine
        Evaluation solH2 = exp(lnYH2 + lnPg + lnPhiH2 - lnKh - PF - lnGammaH2 - 4.0166);

        // Convert to mole fraction
        xH2 = solH2 / (55.51 + solH2);
    }

    /*!
    * \brief Returns the Poynting Factor (PF) which is needed in calculation of H2 solubility in Li et al (2018).
    *
    * \param temperature temperature [K]
    * \param pg gas phase pressure [Pa]
    */
    template <class Evaluation>
    static Evaluation computePoyntingFactor_(const Evaluation& temperature, const Evaluation& pg)
    {
        // PF is approximated as a polynomial expansion in terms of temperature and pressure with the following
        // parameters (Table 4)
        static const Scalar a[4] = {6.156755, -2.502396e-2, 4.140593e-5, -1.322988e-3};

        // Eq. (16)
        Evaluation pg_mpa = pg / 1.0e6;  // convert from Pa to MPa
        Evaluation PF = a[0]*pg_mpa/temperature + a[1]*pg_mpa + a[2]*temperature*pg_mpa + a[3]*pg_mpa*pg_mpa/temperature;
        return PF;
    }

    /*!
    * \brief Returns the activity coefficient of H2 in brine which is needed in calculation of H2 solubility in Li et
    * al (2018). Note that we only include NaCl effects. Could be extended with other salts, e.g. from Duan & Sun,
    * Chem. Geol., 2003.
    * 
    * \param temperature temperature [K]
    * \param salinity salinity [mol NaCl / kg solution]
    */
    template <class Evaluation>
    static Evaluation activityCoefficient_(const Evaluation& temperature, Scalar salinity)
    {
        // Linear approximation in temperature with following parameters (Table 5)
        static const Scalar a[2] = {0.64485, 0.00142};

        // Eq. (17)
        Evaluation lnGamma = (a[0] - a[1]*temperature)*salinity;
        return lnGamma;
    }

    /*!
    * \brief Returns Henry's constant of H2 in brine which is needed in calculation of H2 solubility in Li et al (2018).
    * 
    * \param temperature temperature [K]
    */
    template <class Evaluation>
    static Evaluation henrysConstant_(const Evaluation& temperature)
    {
        // Polynomic approximation in temperature with following parameters (Table 2)
        static const Scalar a[5] = {2.68721e-5, -0.05121, 33.55196, -3411.0432, -31258.74683};

        // Eq. (13)
        Evaluation lnKh = a[0]*temperature*temperature + a[1]*temperature + a[2] + a[3]/temperature 
            + a[4]/(temperature*temperature);
        return lnKh;
    }

    /*!
    * \brief Returns mole fraction of H2 in gasous phase which is needed in calculation of H2 solubility in Li et al
    * (2018).
    * 
    * \param temperature temperature [K]
    * \param pg gas phase pressure [Pa]
    */
    template <class Evaluation>
    static Evaluation moleFractionGasH2_(const Evaluation& temperature, const Evaluation& pg)
    {
        // Need saturaturated vapor pressure of pure water
        Evaluation pw_sat = H2O::vaporPressure(temperature);

        // Eq. (12)
        Evaluation lnyH2 = log(1 - (pw_sat / pg));
        return lnyH2;
    }

    /*!
    * \brief Calculate fugacity coefficient for H2 which is needed in calculation of H2 solubility in Li et al (2018).
    * The equation used is based on Helmoltz free energy EOS. The formulas here are taken from Span et al., J. Phys.
    * Chem. Ref. Data 29, 2000 and adapted to H2 in Li et al (2018).
    * 
    * \param temperature temperature [K]
    * \param pg gas phase pressure [Pa] 
    */
    template <class Evaluation> 
    static Evaluation fugacityCoefficientH2(const Evaluation& temperature, const Evaluation& pg)
    {
        // Convert pressure to reduced density and temperature to reduced temperature
        Evaluation rho_red = convertPgToReducedRho_(temperature, pg);
        Evaluation T_red = temperature / H2::criticalTemperature();

        // Residual Helmholtz energy, Eq. (7) in Li et al. (2018)
        Evaluation resHelm = residualHelmholtz_(T_red, rho_red);

        // Derivative of residual Helmholtz energy wrt to reduced density, Eq. (73) in Span et al. (2018)
        Evaluation dResdHelm = derivResidualHelmholtz_(T_red, rho_red);

        // Fugacity coefficient, Eq. (8) in Li et al. (2018)
        Evaluation lnPhiH2 = resHelm + rho_red * dResdHelm - log(rho_red * dResdHelm + 1);

        return lnPhiH2;
    }

    /*!
    * \brief Convert pressure to reduced density (rho/rho_crit) for further calculation of fugacity coefficient in Li et
    * al. (2018) and Span et al. (2000). The conversion is done using the simplest root-finding algorithm, i.e. the
    * bisection method.
    * 
    * \param pg gas phase pressure [Pa]
    * \param temperature temperature [K]
    */
    template <class Evaluation> 
    static Evaluation convertPgToReducedRho_(const Evaluation& temperature, const Evaluation& pg)
    {
        // Interval for search
        Evaluation rho_red_min = 0.0;
        Evaluation rho_red_max = 1.0;

        // Obj. value at min, fmin=f(xmin) for first comparison with fmid=f(xmid)
        Evaluation fmin = -pg / 1.0e6;  // at 0.0 we don't need to envoke function (see also why in rootFindingObj_)

        // Bisection loop
        for (int iteration=1; iteration<100; ++iteration) {
            // New midpoint and its obj. value
            Evaluation rho_red = (rho_red_min + rho_red_max) / 2;
            Evaluation fmid = rootFindingObj_(rho_red, temperature, pg);

            // Check if midpoint fulfills f=0 or x-xmin is sufficiently small
            if (Opm::abs(fmid) < 1e-8 || Opm::abs((rho_red_max - rho_red_min) / 2) < 1e-8) {
                return rho_red;
            }

            // Else we repeat with midpoint being either xmin or xmax (depending on the signs)
            else if ((Opm::getValue(fmid) > 0.0 && Opm::getValue(fmin) < 0.0) ||
                (Opm::getValue(fmid) < 0.0 && Opm::getValue(fmin) > 0.0)) {
                // fmid has same sign as fmax so we set xmid as the new xmax
                rho_red_max = rho_red;
            }
            else {
                // fmid has same sign as fmin so we set xmid as the new xmin
                rho_red_min = rho_red;
                fmin = fmid;
            }
        }
    }

    /*!
    * \brief Objective function in root-finding done in convertPgToReducedRho_ taken from Li et al. (2018).
    * 
    * \param rho_red reduced density [-]
    * \param pg gas phase pressure [Pa]
    * \param temperature temperature [K]
    */
    template <class Evaluation> 
    static Evaluation rootFindingObj_(const Evaluation& rho_red, const Evaluation& temperature, const Evaluation& pg)
    {
        // Temporary calculations
        Evaluation T_red = temperature / H2::criticalTemperature();  // reduced temp.
        Evaluation p_MPa = pg / 1.0e6;  // Pa --> MPa
        Scalar R = IdealGas::R;
        Evaluation rho_cRT = H2::criticalDensity() * R * temperature;

        // Eq. (9)
        Evaluation dResdH = derivResidualHelmholtz_(T_red, rho_red);
        Evaluation obj = rho_red * rho_cRT * (1 + rho_red * dResdH) - p_MPa;
        return obj;
    }

    /*!
    * \brief Derivative of the residual part of Helmholtz energy wrt. reduced density. Used primarily to calculate
    * fugacity coefficient for H2.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation derivResidualHelmholtz_(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Various parameter values needed in calculations (Table 1 in Li et al. (2018))
        static const Scalar N[14] = {-6.93643, 0.01, 2.1101, 4.52059, 0.732564, -1.34086, 0.130985, -0.777414, 
            0.351944, -0.0211716, 0.0226312, 0.032187, -0.0231752, 0.0557346};
        
        static const Scalar t[14] = {0.6844, 1.0, 0.989, 0.489, 0.803, 1.1444, 1.409, 1.754, 1.311, 4.187, 5.646, 
            0.791, 7.249, 2.986};
        
        static const int d[14] = {1, 4, 1, 1, 2, 2, 3, 1, 3, 2, 1, 3, 1, 1};

        static const int p[2] = {1, 1};

        static const Scalar phi[5] = {-1.685, -0.489, -0.103, -2.506, -1.607};

        static const Scalar beta[5] = {-0.1710, -0.2245, -0.1304, -0.2785, -0.3967};

        static const Scalar gamma[5] = {0.7164, 1.3444, 1.4517, 0.7204, 1.5445};
        
        static const Scalar D[5] = {1.506, 0.156, 1.736, 0.670, 1.662};

        // Derivative of Eq. (7) in Li et al. (2018), which can be compared with Eq. (73) in Span et al. (2000)
        // First sum term 
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += d[i] * N[i] * pow(rho_red, d[i]-1) * pow(T_red, t[i]);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += N[i] * pow(T_red, t[i]) * pow(rho_red, d[i]-1) * exp(-pow(rho_red, p[i-7])) *
                (d[i] - p[i-7]*pow(rho_red, p[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 15; ++i) {
            s3 += N[i] * pow(T_red, t[i]) * pow(rho_red, d[i]-1) * 
                exp(phi[i-9] * pow(rho_red - D[i-9], 2) + beta[i-9] * pow(T_red - gamma[i-9], 2)) *
                    (d[i] + 2 * phi[i-9] * rho_red * (rho_red - D[i-9]));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }
    /*!
    * \brief The residual part of Helmholtz energy wrt. reduced density. Used primarily to calculate fugacity
    * coefficient for H2.
    * 
    * \param T_red reduced temperature [-]
    * \param rho_red reduced density [-]
    */
    template <class Evaluation> 
    static Evaluation residualHelmholtz_(const Evaluation& T_red, const Evaluation& rho_red)
    {
        // Various parameter values needed in calculations (Table 1 in Li et al. (2018))
        static const Scalar N[14] = {-6.93643, 0.01, 2.1101, 4.52059, 0.732564, -1.34086, 0.130985, -0.777414, 
            0.351944, -0.0211716, 0.0226312, 0.032187, -0.0231752, 0.0557346};
        
        static const Scalar t[14] = {0.6844, 1.0, 0.989, 0.489, 0.803, 1.1444, 1.409, 1.754, 1.311, 4.187, 5.646, 
            0.791, 7.249, 2.986};
        
        static const int d[14] = {1, 4, 1, 1, 2, 2, 3, 1, 3, 2, 1, 3, 1, 1};

        static const int p[2] = {1, 1};

        static const Scalar phi[5] = {-1.685, -0.489, -0.103, -2.506, -1.607};

        static const Scalar beta[5] = {-0.1710, -0.2245, -0.1304, -0.2785, -0.3967};

        static const Scalar gamma[5] = {0.7164, 1.3444, 1.4517, 0.7204, 1.5445};
        
        static const Scalar D[5] = {1.506, 0.156, 1.736, 0.670, 1.662};

        // Eq. (7) in Li et al. (2018), which can be compared with Eq. (55) in Span et al. (2000)
        // First sum term
        Evaluation s1 = 0.0;
        for (int i = 0; i < 7; ++i) {
            s1 += N[i] * pow(rho_red, d[i]) * pow(T_red, t[i]);
        }

        // Second sum term
        Evaluation s2 = 0.0;
        for (int i = 7; i < 9; ++i) {
            s2 += N[i] * pow(T_red, t[i]) * pow(rho_red, d[i]) * exp(-pow(rho_red, p[i-7]));
        }

        // Third, and last, sum term
        Evaluation s3 = 0.0;
        for (int i = 9; i < 15; ++i) {
            s3 += N[i] * pow(T_red, t[i]) * pow(rho_red, d[i]) * 
                exp(phi[i-9] * pow(rho_red - D[i-9], 2) + beta[i-9] * pow(T_red - gamma[i-9], 2));
        }

        // Return total sum
        Evaluation s = s1 + s2 + s3;
        return s;
    }

    /*!
    * \brief Binary diffusion coefficent [m^2/s] for molecular water and H2 as an approximation for brine-H2 diffusion.
    *
    * To calculate the values, the \ref fullerMethod is used.
    */
    template <class Evaluation>
    static Evaluation gasDiffCoeff(const Evaluation& temperature, const Evaluation& pressure)
    {
        // atomic diffusion volumes
        const Scalar SigmaNu[2] = { 13.1 /* H2O */,  7.07 /* CO2 */ };
        // molar masses [g/mol]
        const Scalar M[2] = { H2O::molarMass()*1e3, H2::molarMass()*1e3 };

        return fullerMethod(M, SigmaNu, temperature, pressure);
    }
};  // end class Brine_H2

} // end namespace BinaryCoeff
}  // end namespace Opm
#endif
