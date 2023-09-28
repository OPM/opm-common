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
 * \brief This test makes sure that mandated API is adhered to by all component classes
 */
#include "config.h"

#define BOOST_TEST_MODULE Binarycoefficients
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include <opm/material/binarycoefficients/Brine_CO2.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>

template<class Scalar>
bool close_at_tolerance(Scalar n1, Scalar n2, Scalar tolerance)
{
    auto comp = boost::math::fpc::close_at_tolerance<Scalar>(tolerance);
    return comp(n1, n2);
}

using Types = std::tuple<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Brine_CO2, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    using H2O = Opm::SimpleHuDuanH2O<Scalar>;
    using CO2 = Opm::CO2<Scalar>;

    using BinaryCoeffBrineCO2 = Opm::BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    // Molar mass of NaCl [kg/mol]
    const Scalar MmNaCl = 58.44e-3;

    // Extrapolate density?
    const bool extrapolate = true;

    // Activity model for salt
    // 1 = Rumpf et al. (1994) as given in Spycher & Pruess (2005)
    // 2 = Duan-Sun model as modified in Spycher & Pruess (2009)
    // 3 = Duan-Sun model as given in Spycher & Pruess (2005)
    const int activityModel = 2;

    // Init. water gas mole fraction (which we don't care about here) and dissolved CO2 mole fraction
    Evaluation xgH2O;
    Evaluation xlCO2;

    // Init pressure, temperature, and salinity variables
    Evaluation T;
    Evaluation p;
    Evaluation s;

    // Tolerance for Zhao et al (2015) data
    const Scalar tol_zhao = 0.5e-2;

    // Reference data Zhao et al., Geochimica et Cosmochimica Acta 149, 2015
    // Experimental data in Table 3
    // static constexpr Scalar xSol_Zhao[3][7] = {
    //     {1.245, 1.016, 0.859, 0.734, 0.623, 0.551, 0.498},
    //     {1.020, 0.836, 0.706, 0.618, 0.527, 0.469, 0.427},
    //     {1.001, 0.800, 0.647, 0.566, 0.498, 0.440, 0.390}
    // };
    // Spycher & Pruess (2009) (i.e., activityModel = 2 in our case) data given in Table 3
    static constexpr Scalar xSol_Zhao[3][7] = {
        {1.233, 1.012, 0.845, 0.716, 0.618, 0.541, 0.481},
        {1.001, 0.819, 0.686, 0.588, 0.515, 0.463, 0.425},
        {1.028, 0.824, 0.679, 0.577, 0.503, 0.450, 0.414}
    };

    // Temperature, pressure and salinity for Zhao et al (2015) data; Table 3
    T = 323.15;  // K
    const int numT = 3;
    p = 150e5;  // Pa
    const int numS = 7;

    // Test against Zhao et al (2015) data
    Evaluation salinity;
    Evaluation mCO2;
    for (int iT = 0; iT < numT; ++iT) {
        // Salinity in mol/kg
        s = 0.0;

        for (int iS = 0; iS < numS; ++iS) {
            // convert to mass fraction
            salinity = 1 / ( 1 + 1 / (s * MmNaCl));

            // Calculate solubility as mole fraction
            BinaryCoeffBrineCO2::calculateMoleFractions(T, p, salinity, -1, xlCO2, xgH2O, activityModel, extrapolate);

            // Convert to molality CO2
            mCO2 = xlCO2 * (s + 55.508) / (1 - xlCO2);

            // Compare to experimental data
            // BOOST_CHECK_CLOSE(mCO2.value(), xSol_Zhao[iT][iS], tol);
            BOOST_CHECK_MESSAGE(close_at_tolerance(mCO2.value(), xSol_Zhao[iT][iS], tol_zhao),
                "relative difference between solubility {"<<mCO2.value()<<"} and Zhao et al. (2015) {"<<
                xSol_Zhao[iT][iS]<<"} exceeds tolerance {"<<tol_zhao<<"} at (T, p, S) = ("<<T.value()<<", "<<
                p.value()<<", "<<s.value()<<")");

            // Increment salinity (mol/kg)
            s += 1.0;
        }
        // Increment T
        T += 50.0;
    }

    // Reference (experimental?) data Koschel et al., Fluid Phase Equilibria 247, 2006; Table 6
    // Data for T = 323.1 K
    static constexpr Scalar xSol_Koschel_1[2][4] = {
        {0.0111, 0.0164, 0.0177, 0.0181},
        {0.0083, 0.0114, 0.0124, 0.0137}
    };
    // Data for T = 373.1 K
    static constexpr Scalar xSol_Koschel_2[2][3] = {
        {0.006, 0.0112, 0.0161},
        {0.0043, 0.008, 0.0112}
    };

    // Pressure data in Koschel et al. (2006) data
    std::vector< std::vector<Evaluation> > p_1 = {
        {5.1e6, 10.03e6, 14.38e6, 20.24e6},
        {5.0e6, 10.04e6, 14.41e6, 20.24e6}
    };
    
    std::vector< std::vector<Evaluation> > p_2 = {
        {5.07e6, 10.4e6, 19.14e6},
        {5.04e6, 10.29e6, 19.02e6}
    };

    // Tolerance for Koschel et al. (2006) data
    const Scalar tol_koschel = 14e-2;

    // Test against reference data in Koschel et al. (2006)
    s = 1.0;
    for (int i = 0; i < 2; ++i) {
        // Convert to mass fraction
        salinity = 1 / ( 1 + 1 / (s * MmNaCl));

        // First table
        T = 323.1;
        for (int j = 0; j < 4; ++j) {
            // Get pressure 
            p = p_1[i][j];

            // Calculate solubility as mole fraction
            BinaryCoeffBrineCO2::calculateMoleFractions(T, p, salinity, -1, xlCO2, xgH2O, activityModel, extrapolate);

            // Compare to experimental data
            // BOOST_CHECK_CLOSE(xlCO2.value(), xSol_Koschel_1[i][j], tol);
            BOOST_CHECK_MESSAGE(close_at_tolerance(xlCO2.value(), xSol_Koschel_1[i][j], tol_koschel),
                "relative difference between solubility {"<<xlCO2.value()<<"} and Koschel et al. (2006) {"<<
                xSol_Koschel_1[i][j]<<"} exceeds tolerance {"<<tol_koschel<<"} at (T, p, S) = ("<<T.value()<<", "<<
                p.value()<<", "<<s.value()<<")");
        }

        // Second table 
        T = 373.1;
        for (int j = 0; j < 3; ++j) {
            // Get pressure 
            p = p_2[i][j];

            // Calculate solubility as mole fraction
            BinaryCoeffBrineCO2::calculateMoleFractions(T, p, salinity, -1, xlCO2, xgH2O, activityModel, extrapolate);

            // Compare to experimental data
            // BOOST_CHECK_CLOSE(xlCO2.value(), xSol_Koschel_2[i][j], tol);
            BOOST_CHECK_MESSAGE(close_at_tolerance(xlCO2.value(), xSol_Koschel_2[i][j], tol_koschel),
                "relative difference between solubility {"<<xlCO2.value()<<"} and Koschel et al. (2006) {"<<
                xSol_Koschel_2[i][j]<<"} exceeds tolerance {"<<tol_koschel<<"} at (T, p, S) = ("<<T.value()<<", "<<
                p.value()<<", "<<s.value()<<")");
        }
        // Increment salinity (to 3 mol/kg)
        s += 2.0;
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BrineDensityWithCO2, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    // Molar mass of NaCl [kg/mol]
    const Scalar MmNaCl = 58.44e-3;

    // Activity model for salt
    // 1 = Rumpf et al. (1994) as given in Spycher & Pruess (2005)
    // 2 = Duan-Sun model as modified in Spycher & Pruess (2009)
    // 3 = Duan-Sun model as given in Spycher & Pruess (2005)
    const int activityModel = 3;

    // Tolerance for Yan et al. (2011) data
    const Scalar tol_yan = 5e-3;

    // Yan et al, Int. J. Greenh. Gas Control, 5, 2011; Table 4
    static constexpr Scalar rho_Yan_1[3][6][3] = {{
        {0.99722e3, 0.96370e3, 0.92928e3},
        {1.00268e3, 0.96741e3, 0.93367e3},
        {1.00528e3, 0.97062e3, 0.93760e3},
        {1.00688e3, 0.97425e3, 0.94108e3},
        {1.01293e3, 0.97962e3, 0.94700e3},
        {1.01744e3, 0.98506e3, 0.95282e3}
    },
    {
        {1.03116e3, 1.00026e3, 0.96883e3},
        {1.03491e3, 1.00321e3, 0.97169e3},
        {1.03968e3, 1.00667e3, 0.97483e3},
        {1.04173e3, 1.00961e3, 0.97778e3},
        {1.04602e3, 1.01448e3, 0.98301e3},
        {1.05024e3, 1.01980e3, 0.98817e3}
    },
    {
        {1.15824e3, 1.12727e3, 1.09559e3},
        {1.16090e3, 1.12902e3, 1.10183e3},
        {1.16290e3, 1.13066e3, 1.10349e3},
        {1.16468e3, 1.13214e3, 1.10499e3},
        {1.16810e3, 1.13566e3, 1.10882e3},
        {1.17118e3, 1.13893e3, 1.11254e3}
    }};

    // Temperature, pressure and salinity for Yan et al (2011) data; Table 4
    std::vector<Evaluation> T = {323.2, 373.2, 413.2};  // K
    std::vector<Evaluation> p = {5e6, 10e6, 15e6, 20e6, 30e6, 40e6};  // Pa
    std::vector<Scalar> s = {0.0, 1.0, 5.0};

    // Test against Yan et al. (2011) data
    Evaluation rs_sat;
    Evaluation rho;
    std::vector<Scalar> salinity = {0.0};
    for (std::size_t iS = 0; iS < s.size(); ++iS) {
        for (std::size_t iP = 0; iP < p.size(); ++iP) {
            for (std::size_t iT = 0; iT < T.size(); ++iT) {
                // Calculate salinity in mass fraction
                salinity[0] = 1 / ( 1 + 1 / (s[iS] * MmNaCl));

                // Instantiate BrineCo2Pvt class
                Opm::BrineCo2Pvt<Scalar> brineCo2Pvt(salinity, activityModel);

                // Calculate saturated Rs (dissolved CO2 in brine)
                rs_sat = brineCo2Pvt.rsSat(/*regionIdx=*/0, T[iT], p[iP], Evaluation(salinity[0]));

                // Calculate density of brine with dissolved CO2
                rho = brineCo2Pvt.density(/*regionIdx=*/0, T[iT], p[iP], rs_sat, Evaluation(salinity[0]));

                // Compare with data
                BOOST_CHECK_MESSAGE(close_at_tolerance(rho.value(), rho_Yan_1[iS][iP][iT], tol_yan),
                                    "relative difference between density {"<<rho.value()<<"} and Yan et al. (2011) {"<<
                                    rho_Yan_1[iS][iP][iT]<<"} exceeds tolerance {"<<tol_yan<<"} at (T, p, S) = ("<<
                                    T[iT].value()<<", "<<p[iP].value()<<", "<<s[iS]<<")");

            }
        }
    }
}