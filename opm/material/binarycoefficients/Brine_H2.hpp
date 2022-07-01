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

#include <opm/material/binarycoefficients/FullerMethod.hpp>

namespace Opm {
namespace BinaryCoeff {

/*!
* \ingroup Binarycoefficients
* \brief Binary coefficients for brine and H2.
*/
template<class Scalar, class H2O, class H2, bool verbose = true>
class Brine_H2 {
    static const int liquidPhaseIdx = 0; // index of the liquid phase
    static const int gasPhaseIdx = 1; // index of the gas phase

public:
    /*!
    * \brief Returns the _mol_ (!) fraction of H2 in the liquid phase for a given temperature, pressure, and brine
    * salinity. Implemented according to Li et al., Int. J. Hydrogen Energ., 2018.
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
                                       Evaluation& xH2,
                                       bool extrapolate = false)
    {
        // All intermediate calculations
        Evaluation lnYH2 = moleFractionGasH2_(temperature, pg);
        Evaluation lnPg = log(pg / 1e6);  // Pa --> MPa before ln
        Evaluation lnPhiH2 = fugacityCoefficientH2(temperature, pg, extrapolate);
        Evaluation lnKh = henrysConstant_(temperature);
        Evaluation PF = computePoyntingFactor_(temperature, pg);
        Evaluation lnGammaH2 = activityCoefficient_(temperature, salinity);

        // Eq. (4) to get mole fraction of H2 in brine
        xH2 = exp(lnYH2 + lnPg + lnPhiH2 - lnKh - PF - lnGammaH2);
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
    * Chem. Ref. Data 29, 2000 and Leachman et al., J. Phys. Chem. Ref. Data 38, 2009, and Li et al. (2018).
    * 
    * \param temperature temperature [K]
    * \param pg gas phase pressure [Pa] 
    */
    template <class Evaluation> 
    static Evaluation fugacityCoefficientH2(const Evaluation& temperature, 
                                            const Evaluation& pg, 
                                            bool extrapolate = false)
    {
        // Convert pressure to reduced density and temperature to reduced temperature
        Evaluation rho_red = H2::reducedMolarDensity(temperature, pg, extrapolate);
        Evaluation T_red = H2::criticalTemperature() / temperature;

        // Residual Helmholtz energy, Eq. (7) in Li et al. (2018)
        Evaluation resHelm = H2::residualPartHelmholtz(T_red, rho_red);

        // Derivative of residual Helmholtz energy wrt to reduced density, Eq. (73) in Span et al. (2018)
        Evaluation dResHelm_dRedRho = H2::derivResHelmholtzWrtRedRho(T_red, rho_red);

        // Fugacity coefficient, Eq. (8) in Li et al. (2018)
        Evaluation lnPhiH2 = resHelm + rho_red * dResHelm_dRedRho - log(rho_red * dResHelm_dRedRho + 1);

        return lnPhiH2;
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
        const Scalar SigmaNu[2] = { 13.1 /* H2O */,  7.07 /* H2 */ };
        // molar masses [g/mol]
        const Scalar M[2] = { H2O::molarMass()*1e3, H2::molarMass()*1e3 };

        return fullerMethod(M, SigmaNu, temperature, pressure);
    }
};  // end class Brine_H2

} // end namespace BinaryCoeff
}  // end namespace Opm
#endif
