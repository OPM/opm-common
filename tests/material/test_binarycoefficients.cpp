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
#include <opm/material/binarycoefficients/Brine_H2.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/H2.hpp>

#include <iostream>

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
    bool extrapolate = true;

    // Activity model for salt
    // 1 = Rumpf et al. (1994) as given in Spycher & Pruess (2005)
    // 2 = Duan-Sun model as modified in Spycher & Pruess (2009)
    const int activityModel = 1;

    // Init. water gas mole fraction (which we don't care about here)
    Evaluation xgH2O;

    // Init pressure, temperature, and salinity variables
    Evaluation T;
    Evaluation p;
    Evaluation s;

    // Boost rel. diff. tolerance (in percent)
    Scalar tol_zhao = 0.5e-2;

    // Reference data Zhao et al., Geochimica et Cosmochimica Acta 149, 2015
    // Experimental data:
    // static constexpr Scalar xSol_Zhao[3][7] = {
    //     {1.245, 1.016, 0.859, 0.734, 0.623, 0.551, 0.498},
    //     {1.020, 0.836, 0.706, 0.618, 0.527, 0.469, 0.427},
    //     {1.001, 0.800, 0.647, 0.566, 0.498, 0.440, 0.390}
    // };
    // SP2010 data
    static constexpr Scalar xSol_Zhao[3][7] = {
        {1.233, 1.012, 0.845, 0.716, 0.618, 0.541, 0.481},
        {1.001, 0.819, 0.686, 0.588, 0.515, 0.463, 0.425},
        {1.028, 0.824, 0.679, 0.577, 0.503, 0.450, 0.414}
    };

    // Temperature, pressure and salinity for Zhao et al (2015) data; Table 3
    T = 323.15;  // K
    int numT = 3;
    p = 150e5;  // Pa
    int numS = 7;

    // Test against Zhao et al (2015) data
    for (int iT = 0; iT < numT; ++iT) {
        // Salinity in mol/kg
        s = 0.0;

        for (int iS = 0; iS < numS; ++iS) {
            // convert to mass fraction
            Evaluation salinity = 1 / ( 1 + 1 / (s * MmNaCl));

            // Init. mole fraction CO2 in water
            Evaluation xlCO2;

            // Calculate solubility as mole fraction
            BinaryCoeffBrineCO2::calculateMoleFractions(T, p, salinity, -1, xlCO2, xgH2O, activityModel, extrapolate);

            // Convert to molality CO2
            Evaluation mCO2 = xlCO2 * (s + 55.508) / (1 - xlCO2);

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
    Scalar tol_koschel = 14e-2;

    // Test against reference data in Koschel et al. (2006)
    s = 1.0;
    for (int i = 0; i < 2; ++i) {
        // Convert to mass fraction
        Evaluation salinity = 1 / ( 1 + 1 / (s * MmNaCl));

        // First table
        T = 323.1;
        for (int j = 0; j < 4; ++j) {
            // Get pressure 
            p = p_1[i][j];

            // Init. mole fraction CO2 in water
            Evaluation xlCO2;

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

            // Init. mole fraction CO2 in water
            Evaluation xlCO2;

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

// BOOST_AUTO_TEST_CASE_TEMPLATE(Brine_H2, Scalar, Types)
// {
//     using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

//     using H2O = Opm::SimpleHuDuanH2O<Scalar>;
//     using H2 = Opm::H2<Scalar>;

//     using BinaryCoeffBrineH2 = Opm::BinaryCoeff::Brine_H2<Scalar, H2O, H2>;

//     // Init pressure, temperature, and salinity variables
//     Evaluation T;
//     Evaluation p;
//     Scalar s;

//     // Extrapolate density?
//     bool extrapolate = true;

//     // Boost rel. diff. tolerance (in percent)
//     double tol = 1;

//     // Reference experimental data from Chabab et al., Int. J. Hydrogen Energy 45, 2020;  Table 4
//     static constexpr Scalar xSol_Chabab[4][3] = {
//         {0.000461, 0.001544, 0.000857},
//         {0.000972, 0.000827, 0.000659},
//         {0.000400, 0.000456, 0.001333},
//         {0.000127, 0.000440, 0.000838}
//     };
//     std::vector< std::vector<Evaluation> > p_Chabab = {
//         {37.108e5, 121.706e5, 60.213e5},
//         {100.354e5, 80.673e5, 60.987e5},
//         {66.733e5, 66.536e5, 196.178e5},
//         {28.623e5, 100.979e5, 193.702e5}
//     };
//     std::vector< std::vector<Evaluation> > T_Chabab = {
//         {323.18, 323.19, 372.73},
//         {323.21, 347.91, 372.76},
//         {323.18, 372.74, 372.75},
//         {323.19, 323.19, 323.19}
//     };
//     Scalar s_Chabab[] = {0.0, 1.0, 3.0, 5.0};

//     // Test against Chabab et al. (2020) data
//     for (int i = 0; i < 4; ++i) {
//         // Pick out salinity
//         s = s_Chabab[i];
//         for (int j = 0; j < 3; ++j) {
//             // Temperature and pressure
//             p = p_Chabab[i][j];
//             T = T_Chabab[i][j];

//             // Init. mole fraction H2 in brine
//             Evaluation xlH2;

//             // Calculate molefration in brine
//             BinaryCoeffBrineH2::calculateMoleFractions(T, p, s, xlH2, extrapolate);

//             // Compare to reference
//             BOOST_CHECK_CLOSE(xlH2.value(), xSol_Chabab[i][j], tol);
//         }
//     }

//     // Reference data from Li et al., Int. J. Hydrogen Energy 43, 2018;  Tables A1-A4
//     static constexpr Scalar xSol_Li[4][4] = {
//         {0.038198, 0.070252, 0.142645, 0.227373},
//         {0.030609, 0.058746, 0.122717, 0.201243},
//         {0.024528, 0.049124, 0.105573, 0.178116},
//         {0.015751, 0.034349, 0.078136, 0.13953}
//     };
//     std::vector<Evaluation> p_Li = {5e6, 10e6, 20e6, 30e6};
//     std::vector<Evaluation> T_Li = {298.15, 328.15, 348.15, 368.15};
//     Scalar s_Li[] = {0.0, 1.0, 2.0, 4.0};

//     // Test against Li et al. (2018) data
//     for (int i = 0; i < 4; ++i) {
//         // Pick out salinity
//         s = s_Li[i];
//         for (int j = 0; j < 4; ++j) {
//             // Temperature and pressure
//             p = p_Li[j];
//             T = T_Li[j];
            
//             // Init. mole fraction H2 in brine
//             Evaluation xlH2;

//             // Calculate molefration in brine
//             BinaryCoeffBrineH2::calculateMoleFractions(T, p, s, xlH2, extrapolate);

//             // Convert to molality
//             Evaluation mH2 =  xlH2 * 55.508 / (1 - xlH2);

//             // Compare to reference
//             BOOST_CHECK_CLOSE(mH2.value(), xSol_Li[i][j], tol);
//         }
//     }
// }