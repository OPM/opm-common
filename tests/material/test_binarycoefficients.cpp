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
#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
#include <boost/mpl/list.hpp>
#endif

#include <opm/common/utility/SaltArray.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include <opm/material/binarycoefficients/Brine_CO2.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/CO2Tables.hpp>
#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/Co2GasPvt.hpp>

template<class Scalar>
bool close_at_tolerance(Scalar n1, Scalar n2, Scalar tolerance)
{
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 64
    auto comp = boost::test_tools::close_at_tolerance<Scalar>(boost::test_tools::percent_tolerance_t<Scalar>(tolerance*100.0));
#else
    auto comp = boost::math::fpc::close_at_tolerance<Scalar>(tolerance);
#endif
    return comp(n1, n2);
}

#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
using Types = boost::mpl::list<float,double>;
#else
using Types = std::tuple<float,double>;
#endif

BOOST_AUTO_TEST_CASE_TEMPLATE(Brine_CO2, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    using H2O = Opm::SimpleHuDuanH2O<Scalar>;
    using CO2 = Opm::CO2<Scalar>;

    using BinaryCoeffBrineCO2 = Opm::BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    // Extrapolate density?
    const bool extrapolate = true;

    // Activity model for salt
    // 1 = Rumpf et al. (1994) as given in Spycher & Pruess (2005)
    // 2 = Duan-Sun model as modified in Spycher & Pruess (2009)
    // 3 = Duan-Sun model as given in Spycher & Pruess (2005)
    std::vector<int> activityModel = {1, 2, 3};

    // Init pressure, temperature, and salinity variables
    std::vector<Evaluation> T = {303.15, 333.15, 363.15, 393.15};  // K
    std::vector<Evaluation> p = {1e5, 5e5, 10e5, 50e5, 100e5, 200e5, 300e5, 400e5, 500e5, 600e5};  // Pa
    std::vector<Opm::SaltArray<Evaluation, Opm::SaltMolality> > s(4);
    s[0][Opm::SaltIndex::NA] = 0.0;
    s[0][Opm::SaltIndex::CL] = 0.0;
    s[1][Opm::SaltIndex::NA] = 1.0;
    s[1][Opm::SaltIndex::CL] = 1.0;
    s[2][Opm::SaltIndex::NA] = 2.0;
    s[2][Opm::SaltIndex::CL] = 2.0;
    s[3][Opm::SaltIndex::NA] = 4.0;
    s[3][Opm::SaltIndex::CL] = 4.0;

    // Literature values from Duan & Sun (2003), An improved model calculating CO 2 solubility in pure water and aqueous
    // NaCl solutions from 273 to 533 K and from 0 to 2000 bar, Chemical Geology 193
    static constexpr Scalar m_co2_data[4][4][10] = {
    {
        {0.0286, 0.1442, 0.2809, 1.0811, 1.3611, 1.4889, 1.5989, 1.7005, 1.7965, 1.8883},
        {0.0137, 0.0803, 0.1602, 0.6695, 1.0275, 1.2344, 1.3495, 1.4478, 1.5368, 1.6194},
        {0.0036, 0.0511, 0.1086, 0.4952, 0.8219, 1.1308, 1.2802, 1.3954, 1.4954, 1.5857},
        {-999., 0.0298, 0.0781, 0.4157, 0.7314, 1.1100, 1.3184, 1.4700, 1.5972, 1.7102}
    },
    {
        {0.0238, 0.1185, 0.2294, 0.8729, 1.0958, 1.1990, 1.2910, 1.3781, 1.4620, 1.5438},
        {0.0116, 0.0674, 0.1335, 0.5502, 0.8405, 1.0072, 1.1012, -999., 1.2577, 1.3282},
        {0.0031, 0.0433, 0.0914, 0.4103, 0.6767, 0.9259, 1.0456, 1.1383, 1.2191, 1.2925},
        {-999., 0.0253, 0.0660, 0.3447, 0.6015, 0.9052, 1.0696, 1.1881, 1.2869, 1.3742}
    },
    {
        {0.0200, 0.0984, 0.1895, 0.7135, 0.8939, 0.9801, 1.0600, 1.1377, 1.2143, 1.2905},
        {0.0100, 0.0572, 0.1126, 0.4583, 0.6978, 0.8359, 0.9160, 0.9873, 1.0542, 1.1182},
        {0.0027, 0.0372, 0.0780, 0.3451, 0.5663, 0.7729, 0.8731, 0.9518, 1.0216, 1.0859},
        {-999., 0.0218, 0.0565, 0.2905, 0.5038, 0.7543, 0.8898, 0.9878, 1.0702, 1.1436},
    },
    {
        {0.0147, 0.0703, 0.1339, 0.4945, 0.6189, 0.6849, 0.7515, 0.8200, 0.8907, 0.9639},
        {0.0077, 0.0428, 0.0833, 0.3314, 0.5028, 0.6060, 0.6717, 0.7340, 0.7955, 0.8571},
        {0.0021, 0.0287, 0.0593, 0.2554, 0.4169, 0.5705, 0.6503, 0.7170, 0.7793, 0.8395},
        {-999., 0.0171, 0.0435, 0.2169, 0.3733, 0.5590, 0.6636, 0.7434, 0.8140, 0.8798},
    }
    };

    // Tolerance per activity model
    std::vector<Scalar> tol = {2.5e-1, 1.75e-1, 2e-1};
    Opm::CO2Tables params;

    // Test against Duan&Sun (2005) data
    Evaluation xgH2O;
    Evaluation xlCO2;
    Evaluation mCO2;
    Opm::SaltArray<Evaluation, Opm::SaltMassFraction> salinity;
    for (std::size_t iS = 0; iS < s.size(); ++iS) {
        for (std::size_t iT = 0; iT < T.size(); ++iT) {
            for (std::size_t iP = 0; iP < p.size(); ++iP) {
                for (std::size_t iA = 0; iA < activityModel.size(); ++iA) {
                    // Data that are negative do either not exist or wrong
                    if (m_co2_data[iS][iT][iP] < 0.0) {
                        continue;
                    }

                    // Calculate salinity in mass fraction
                    if (iS > 0) {
                        salinity = s[iS].template convert_to<Opm::SaltMassFraction>();
                    }

                    // Calculate solubility as mole fraction
                    BinaryCoeffBrineCO2::calculateMoleFractions(params,
                                                                T[iT],
                                                                p[iP],
                                                                salinity,
                                                                -1,
                                                                xlCO2,
                                                                xgH2O,
                                                                activityModel[iA],
                                                                extrapolate);

                    // Convert to molality
                    mCO2 = xlCO2 * (s[iS].sum() + BinaryCoeffBrineCO2::invMmH2O()) / (1.0 - xlCO2);

                    BOOST_CHECK_MESSAGE(
                        close_at_tolerance(mCO2.value(), m_co2_data[iS][iT][iP], tol[iA]),
                        "relative difference between solubility {"
                        << mCO2.value() << "} and Duan & Sun (2005) {" << m_co2_data[iS][iT][iP]
                        << "} exceeds tolerance {" << tol[iA] << "} at (T, p, S) = ("
                        << T[iT].value() << ", " << p[iP].value() << ", "
                        << s[iS][Opm::SaltIndex::NA].value() << ") for salt activity model = "
                        << activityModel[iA]);
                }
            }
        }
    }

}

// The mutual solubility must stay finite (value and derivatives) for the non-physical low and
// negative pressures that a Newton iterate may transiently visit. The CO2 density table starts at
// 1 bar, and linear extrapolation below it gives a negative density, hence NaN in the fugacity
// coefficients, unless CO2::gasDensity and calculateMoleFractions guard against it.
BOOST_AUTO_TEST_CASE_TEMPLATE(Brine_CO2_LowPressure, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    using H2O = Opm::SimpleHuDuanH2O<Scalar>;
    using CO2 = Opm::CO2<Scalar>;
    using BinaryCoeffBrineCO2 = Opm::BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    const std::vector<int> activityModel = {1, 2, 3};
    // Includes T > 372.15 K where activity model 2 uses the high-temperature fixed-point iteration
    const std::vector<Scalar> T = {273.15, 288.15, 303.15, 333.15, 363.15, 373.15, 400.0, 450.0, 500.0};  // K
    const std::vector<Scalar> molality = {0.0, 1.0};  // NaCl molality [mol/kg]
    const std::vector<Scalar> p = {-1e5, -1e3, -1.0, 0.0, 1.0, 1e2, 1e3, 2e3, 3e3, 5e3, 1e4, 5e4, 1e5};  // Pa

    Opm::CO2Tables params;
    for (const int am : activityModel) {
        for (const Scalar Ti : T) {
            for (const Scalar si : molality) {
                Opm::SaltArray<Evaluation, Opm::SaltMolality> saltMolality;
                saltMolality[Opm::SaltIndex::NA] = si;
                saltMolality[Opm::SaltIndex::CL] = si;
                const auto salinity = saltMolality.template convert_to<Opm::SaltMassFraction>();
                for (const Scalar pi : p) {
                    // pressure as the primary variable to also check the derivatives
                    const Evaluation temperature = Evaluation(Ti);
                    const Evaluation pressure = Evaluation::createVariable(pi, 0);
                    Evaluation xlCO2;
                    Evaluation xgH2O;
                    BinaryCoeffBrineCO2::calculateMoleFractions(params, temperature, pressure, salinity, -1,
                                                                xlCO2, xgH2O, am, /*extrapolate=*/true);

                    BOOST_CHECK_MESSAGE(std::isfinite(xlCO2.value()) && std::isfinite(xgH2O.value()),
                                        "non-finite solubility (xlCO2 = " << xlCO2.value() << ", xgH2O = "
                                        << xgH2O.value() << ") at (T, p, S) = (" << Ti << ", " << pi << ", "
                                        << si << ") for salt activity model = " << am);
                    BOOST_CHECK_MESSAGE(std::isfinite(xlCO2.derivative(0)) && std::isfinite(xgH2O.derivative(0)),
                                        "non-finite derivative wrt pressure at (T, p, S) = (" << Ti << ", " << pi
                                        << ", " << si << ") for salt activity model = " << am);
                }
            }
        }
    }
}

// Below the CO2 density table minimum (1 bar) the density must stay positive, also for zero and
// negative pressures, since e.g. the internal energy divides by it.
BOOST_AUTO_TEST_CASE_TEMPLATE(Co2_LowPressureDensity, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using CO2 = Opm::CO2<Scalar>;

    Opm::CO2Tables params;
    for (const Scalar Ti : {273.15, 293.15, 333.15, 373.15}) {
        for (const Scalar pi : {-1e5, -1e3, -1.0, 0.0, 1.0, 1e3, 5e4, 1e5}) {
            const Evaluation temperature = Evaluation(Ti);
            const Evaluation pressure = Evaluation::createVariable(pi, 0);
            const Evaluation rho = CO2::gasDensity(params, temperature, pressure, /*extrapolate=*/true);
            const Evaluation u = CO2::gasInternalEnergy(params, temperature, pressure, /*extrapolate=*/true);
            BOOST_CHECK_MESSAGE(std::isfinite(rho.value()) && rho.value() > 0.0,
                                "non-positive density " << rho.value() << " at (T, p) = (" << Ti << ", " << pi << ")");
            BOOST_CHECK_MESSAGE(std::isfinite(u.value()) && std::isfinite(u.derivative(0)),
                                "non-finite internal energy " << u.value() << " at (T, p) = (" << Ti << ", " << pi
                                << ")");
        }
    }
}

// The saturated water vaporization factor Rvw = XgW / (1 - XgW) * rho_gRef / rho_wRef diverges when
// the gas is pure water vapor, i.e. when the pressure is at or below the vapor pressure of water
// (~2.3 kPa at 20 C). A Newton iterate may transiently visit such a pressure, and the infinite Rvw
// then gives non-finite densities in the simulator.
BOOST_AUTO_TEST_CASE_TEMPLATE(Co2Pvt_LowPressure, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    const std::vector<int> activityModel = {1, 2, 3};
    const std::vector<Scalar> T = {273.15, 293.15, 303.15, 333.15, 363.15, 373.15, 400.0, 450.0};  // K
    const std::vector<Scalar> p = {-1e3, 0.0, 1.0, 1e3, 2e3, 2225.0, 2339.0, 2500.0, 5e3, 1e4, 5e4, 1e5};  // Pa
    std::vector<Opm::SaltArray<Scalar, Opm::SaltMassFraction> > salinity(1);
    const Opm::SaltArray<Evaluation, Opm::SaltMassFraction> evalSalinity(salinity[0]);

    for (const int am : activityModel) {
        Opm::Co2GasPvt<Scalar> co2Pvt(salinity, /*enableMultiCompSalt=*/false, /*useH2ODensity=*/true, am);
        Opm::BrineCo2Pvt<Scalar> brineCo2Pvt(salinity, /*enableMultiCompSalt=*/false, /*useH2ODensity=*/true, am);
        for (const Scalar Ti : T) {
            for (const Scalar pi : p) {
                const Evaluation temperature = Evaluation(Ti);
                const Evaluation pressure = Evaluation::createVariable(pi, 0);

                const Evaluation rvw = co2Pvt.saturatedWaterVaporizationFactor(/*regionIdx=*/0, temperature, pressure);
                const Evaluation invB = co2Pvt.saturatedInverseFormationVolumeFactor(/*regionIdx=*/0, temperature,
                                                                                     pressure);
                const Evaluation rs = brineCo2Pvt.rsSat(/*regionIdx=*/0, temperature, pressure, evalSalinity);

                BOOST_CHECK_MESSAGE(std::isfinite(rvw.value()) && std::isfinite(invB.value())
                                    && std::isfinite(rs.value()),
                                    "non-finite saturated Rvw = " << rvw.value() << ", invB = " << invB.value()
                                    << ", Rs = " << rs.value() << " at (T, p) = (" << Ti << ", " << pi
                                    << ") for salt activity model = " << am);
                BOOST_CHECK_MESSAGE(std::isfinite(rvw.derivative(0)) && std::isfinite(invB.derivative(0))
                                    && std::isfinite(rs.derivative(0)),
                                    "non-finite derivative wrt pressure at (T, p) = (" << Ti << ", " << pi
                                    << ") for salt activity model = " << am);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BrineDensityWithCO2, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    // Activity model for salt
    // 1 = Rumpf et al. (1994) as given in Spycher & Pruess (2005)
    // 2 = Duan-Sun model as modified in Spycher & Pruess (2009)
    // 3 = Duan-Sun model as given in Spycher & Pruess (2005)
    const int activityModel = 3;

    // Tolerance for Yan et al. (2011) data
    const Scalar tol_yan = 5e-3;
    const Scalar tol_yan_multicomp_salt = 2e-2;

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
    std::vector<Evaluation> p = {5e6, 10e6, 15e6, 20e6, 30e6, 40e6}; // Pa
    std::vector<Opm::SaltArray<Scalar, Opm::SaltMolality> > s(3);
    s[0][Opm::SaltIndex::NA] = 0.0;
    s[0][Opm::SaltIndex::CL] = 0.0;
    s[1][Opm::SaltIndex::NA] = 1.0;
    s[1][Opm::SaltIndex::CL] = 1.0;
    s[2][Opm::SaltIndex::NA] = 5.0;
    s[2][Opm::SaltIndex::CL] = 5.0;

    // Test against Yan et al. (2011) data
    Evaluation rs_sat;
    Evaluation rho;
    Evaluation rs_sat_multicomp_salt;
    Evaluation rho_multicomp_salt;
    std::vector<Opm::SaltArray<Scalar, Opm::SaltMassFraction> > salinity(1);
    for (std::size_t iS = 0; iS < s.size(); ++iS) {
        for (std::size_t iP = 0; iP < p.size(); ++iP) {
            for (std::size_t iT = 0; iT < T.size(); ++iT) {
                // Calculate salinity in mass fraction for nonzero salinity
                if (iS > 0) {
                    salinity[0] = s[iS].template convert_to<Opm::SaltMassFraction>();
                }

                // Instantiate BrineCo2Pvt class
                Opm::BrineCo2Pvt<Scalar> brineCo2Pvt(salinity, false, true, activityModel);
                Opm::BrineCo2Pvt<Scalar> brineCo2PvtMultiCompSalt(salinity, true, true, activityModel);

                // Calculate saturated Rs (dissolved CO2 in brine)
                Opm::SaltArray<Evaluation, Opm::SaltMassFraction> evalSalinity(salinity[0]);
                rs_sat = brineCo2Pvt.rsSat(/*regionIdx=*/0, T[iT], p[iP], evalSalinity);
                rs_sat_multicomp_salt =
                    brineCo2PvtMultiCompSalt.rsSat(0, T[iT], p[iP], evalSalinity);

                // Calculate density of brine with dissolved CO2
                rho = brineCo2Pvt.density(/*regionIdx=*/0, T[iT], p[iP], rs_sat, evalSalinity);
                rho_multicomp_salt =
                    brineCo2PvtMultiCompSalt.density(0,
                                                     T[iT],
                                                     p[iP],
                                                     rs_sat_multicomp_salt,
                                                     evalSalinity);

                // Compare with data
                BOOST_CHECK_MESSAGE(
                    close_at_tolerance(rho.value(), rho_Yan_1[iS][iP][iT], tol_yan),
                    "relative difference between density {"<< rho.value()
                    << "} and Yan et al. (2011) {" << rho_Yan_1[iS][iP][iT]
                    << "} exceeds tolerance {" << tol_yan << "} at (T, p, S) = (" << T[iT].value()
                    << ", " << p[iP].value() << ", " << s[iS].sum() << ")");

                BOOST_CHECK_MESSAGE(
                    close_at_tolerance(rho_multicomp_salt.value(),
                                       rho_Yan_1[iS][iP][iT],
                                       tol_yan_multicomp_salt),
                    "relative difference between density with multicomponent salt {"
                    << rho_multicomp_salt.value()
                    << "} and Yan et al. (2011) {" << rho_Yan_1[iS][iP][iT]
                    << "} exceeds tolerance {" << tol_yan_multicomp_salt << "} at (T, p, S) = ("
                    << T[iT].value() << ", " << p[iP].value() << ", " << s[iS].sum() << ")");
            }
        }
    }
}
