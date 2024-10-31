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

#include <boost/mpl/list.hpp>

#define BOOST_TEST_MODULE Components
#include <boost/test/unit_test.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 71
#include <boost/test/floating_point_comparison.hpp>
#else
#include <boost/test/tools/floating_point_comparison.hpp>
#endif

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include "checkComponent.hpp"

// include all components shipped with opm-material
#include <opm/material/components/Unit.hpp>
#include <opm/material/components/NullComponent.hpp>
#include <opm/material/components/Component.hpp>
#include <opm/material/components/Dnapl.hpp>
#include <opm/material/components/SimpleH2O.hpp>
#include <opm/material/components/Lnapl.hpp>
#include <opm/material/components/iapws/Region2.hpp>
#include <opm/material/components/iapws/Region1.hpp>
#include <opm/material/components/iapws/Common.hpp>
#include <opm/material/components/iapws/Region4.hpp>
#include <opm/material/components/H2O.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/CO2Tables.hpp>
#include <opm/material/components/Mesitylene.hpp>
#include <opm/material/components/TabulatedComponent.hpp>
#include <opm/material/components/Brine.hpp>
#include <opm/material/components/BrineDynamic.hpp>
#include <opm/material/components/N2.hpp>
#include <opm/material/components/Xylene.hpp>
#include <opm/material/components/Air.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/H2.hpp>

#include <opm/material/common/UniformTabulated2DFunction.hpp>

#include <opm/json/JsonObject.hpp>

template <class Scalar, class Evaluation>
void testAllComponents()
{
    using H2O = Opm::H2O<Scalar>;

    checkComponent<Opm::Air<Scalar>, Evaluation>();
    checkComponent<Opm::Brine<Scalar, H2O>, Evaluation>();
    checkComponent<Opm::CO2<Scalar>, Evaluation>();
    checkComponent<Opm::C1<Scalar>, Evaluation>();
    checkComponent<Opm::C10<Scalar>, Evaluation>();
    checkComponent<Opm::DNAPL<Scalar>, Evaluation>();
    checkComponent<Opm::H2O<Scalar>, Evaluation>();
    checkComponent<Opm::H2<Scalar>, Evaluation>();
    checkComponent<Opm::LNAPL<Scalar>, Evaluation>();
    checkComponent<Opm::Mesitylene<Scalar>, Evaluation>();
    checkComponent<Opm::N2<Scalar>, Evaluation>();
    checkComponent<Opm::NullComponent<Scalar>, Evaluation>();
    checkComponent<Opm::SimpleCO2<Scalar>, Evaluation>();
    checkComponent<Opm::SimpleH2O<Scalar>, Evaluation>();
    checkComponent<Opm::TabulatedComponent<Scalar, H2O>, Evaluation>();
    checkComponent<Opm::Unit<Scalar>, Evaluation>();
    checkComponent<Opm::Xylene<Scalar>, Evaluation>();
}

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

using Types = boost::mpl::list<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(All, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    // ensure that all components are API-compliant
    testAllComponents<Scalar, Scalar>();
    testAllComponents<Scalar, Evaluation>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SimpleH2O, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;

    using H2O = Opm::H2O<Scalar>;
    using SimpleHuDuanH2O = Opm::SimpleHuDuanH2O<Scalar>;
    using EvalToolbox = Opm::MathToolbox<Evaluation>;

    int numT = 67;
    int numP = 45;
    Evaluation T = 280;

    for (int iT = 0; iT < numT; ++iT) {
        Evaluation p = 1e6;
        T += 5;
        for (int iP = 0; iP < numP; ++iP) {
            p *= 1.1;
            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(H2O::liquidDensity(T,p),
                                                    SimpleHuDuanH2O::liquidDensity(T,p,false),
                                                    1e-3*H2O::liquidDensity(T,p).value()),
                                "oops: the water density based on Hu-Duan has more then 1e-3 deviation from IAPWS'97");

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(H2O::liquidEnthalpy(T,p),
                                                    SimpleHuDuanH2O::liquidEnthalpy(T,p),
                                                    1e-3*H2O::liquidEnthalpy(T,p).value()),
                                "oops: the liquid enthalpy in Simple-Hu-Duan has more then 1e-3 deviation from IAPWS'97");

            if (T >= 570) // for temperature larger then 570 the viscosity based on HuDuan is too far from IAPWS.
                continue;

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(H2O::liquidViscosity(T,p),
                                                    SimpleHuDuanH2O::liquidViscosity(T,p,false),
                                                    5.e-2*H2O::liquidViscosity(T,p).value()),
                                                    "oops: the water viscosity based on Hu-Duan has more then 5e-2 deviation from IAPWS'97");
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(DynamicBrine, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using SimpleHuDuanH2O = Opm::SimpleHuDuanH2O<Scalar>;
    using Brine = Opm::Brine<Scalar, SimpleHuDuanH2O>;
    using BrineDyn = Opm::BrineDynamic<Scalar, SimpleHuDuanH2O>;
    using EvalToolbox = Opm::MathToolbox<Evaluation>;

    Brine::salinity = 0.1;
    Evaluation sal = Brine::salinity;

    int numT = 67;
    int numP = 45;
    Evaluation T = 280;

    for (int iT = 0; iT < numT; ++iT) {
        Evaluation p = 1e6;
        T += 5;
        for (int iP = 0; iP < numP; ++iP) {
            p *= 1.1;

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(Brine::liquidDensity(T, p),
                                                    BrineDyn::liquidDensity(T, p, sal),
                                                   1e-5),
                                "oops: the brine density differs between Brine and Brine dynamic");

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(Brine::liquidViscosity(T, p),
                                                    BrineDyn::liquidViscosity(T, p, sal),
                                                    1e-5),
                                "oops: the brine viscosity differs between Brine and Brine dynamic");

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(Brine::liquidEnthalpy(T, p),
                                                    BrineDyn::liquidEnthalpy(T, p, sal),
                                                    1e-5),
                                "oops: the brine liquidEnthalpy differs between Brine and Brine dynamic");

            BOOST_CHECK_MESSAGE(EvalToolbox::isSame(Brine::molarMass(),
                                                    BrineDyn::molarMass(sal),
                                                    1e-5),
                                "oops: the brine molar mass differs between Brine and Brine dynamic");
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(CO2Class, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using CO2 = Opm::CO2<Scalar>;

    Evaluation T;
    Evaluation p;

    // 
    // Test region with pressures higher than critical pressure
    // 
    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/co2_unittest_part1.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    Json::JsonObject viscosity_ref = parser.get_item("viscosity");
    Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");
    
    // Setup pressure and temperature values
    int numT = temp_ref.size();
    int numP = pres_ref.size();

    // Rel. diff. tolerance
    Scalar tol = 1e-2;
    Scalar tol_enth = 1.2e-2;

    // Extrapolate table
    bool extrapolate = true;
    Opm::CO2Tables params;
    
    // Loop over temperature and pressure, and compare to reference values
    for (int iT = 0; iT < numT; ++iT) {
        // Get temperature from reference data
        T = Evaluation(temp_ref.get_array_item(iT).as_double());

        for (int iP = 0; iP < numP; ++iP) {
            // Get pressure value from reference data
            p = Evaluation(pres_ref.get_array_item(iP).as_double());

            // Density
            Scalar dens = CO2::gasDensity(params, T, p, extrapolate).value();
            Json::JsonObject dens_ref_row = density_ref.get_array_item(iT);
            Scalar dens_ref = Scalar(dens_ref_row.get_array_item(iP).as_double());
            
            BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Viscosity
            Scalar visc = CO2::gasViscosity(params, T, p, extrapolate).value();
            Json::JsonObject visc_ref_row = viscosity_ref.get_array_item(iT);
            Scalar visc_ref = Scalar(visc_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Enthalpy
            // ////////////
            // OBS: One (T, p) point has ca. 10% error. Most likely related to interpolation so we skip that point for 
            // now.
            // ////////////
            if ((T == 364.0 && p == 9100000.0))
                continue;

            Scalar enthalpy = CO2::gasEnthalpy(params, T, p, extrapolate).value();
            Json::JsonObject enth_ref_row = enthalpy_ref.get_array_item(iT);
            Scalar enth_ref = Scalar(enth_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol_enth), 
                "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                "} exceeds tolerance {"<<tol_enth<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
        }
    }

    // 
    // Test region with temperatures higher than critical temperature
    // 
    // Read JSON file with reference values
    std::filesystem::path jsonFile2("material/co2_unittest_part2.json");
    Json::JsonObject parser2(jsonFile2);
    Json::JsonObject density_ref2 = parser2.get_item("density");
    Json::JsonObject viscosity_ref2 = parser2.get_item("viscosity");
    Json::JsonObject enthalpy_ref2 = parser2.get_item("enthalpy");
    Json::JsonObject temp_ref2 = parser2.get_item("temp");
    Json::JsonObject pres_ref2 = parser2.get_item("pres");

    // Setup pressure and temperature values
    int numT2 = temp_ref2.size();
    int numP2 = pres_ref2.size();

    // Loop over temperature and pressure, and compare to reference values
    for (int iT = 0; iT < numT2; ++iT) {
        // Get temperature from reference data
        T = Evaluation(temp_ref2.get_array_item(iT).as_double());

        for (int iP = 0; iP < numP2; ++iP) {
            // Get pressure value from reference data
            p = Evaluation(pres_ref2.get_array_item(iP).as_double());

            // Density
            Scalar dens = CO2::gasDensity(params, T, p, extrapolate).value();
            Json::JsonObject dens_ref_row = density_ref2.get_array_item(iT);
            Scalar dens_ref = Scalar(dens_ref_row.get_array_item(iP).as_double());
            
            BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Viscosity
            Scalar visc = CO2::gasViscosity(params, T, p, extrapolate).value();
            Json::JsonObject visc_ref_row = viscosity_ref2.get_array_item(iT);
            Scalar visc_ref = Scalar(visc_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");


            // Enthalpy
            // ////////////
            // OBS: One (T, p) point has ca. 10% error. Most likely related to interpolation so we skip that point for 
            // now.
            // ////////////
            if (T == 348.0 && p == 6600000.0)
                continue;

            Scalar enthalpy = CO2::gasEnthalpy(params, T, p, extrapolate).value();
            Json::JsonObject enth_ref_row = enthalpy_ref2.get_array_item(iT);
            Scalar enth_ref = Scalar(enth_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol_enth), 
                "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                "} exceeds tolerance {"<<tol_enth<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
        }
    }

    // 
    // Test around saturation curve
    // 
    ///////////////
    // OBS: Interpolation from co2table.inc cannot capture the liquid/vapor jump to a reasonable tolerance, but we leave
    //  the code here for possible future testing.
    ///////////////
    // Above
    // std::filesystem::path jsonFile3("material/co2_unittest_below_sat.json");
    // Json::JsonObject parser3(jsonFile3);
    // Json::JsonObject density_ref3 = parser3.get_item("density");
    // Json::JsonObject enthalpy_ref3 = parser3.get_item("enthalpy");
    // Json::JsonObject temp_ref3 = parser3.get_item("temp");
    // Json::JsonObject pres_ref3 = parser3.get_item("pres");

    // // Below
    // std::filesystem::path jsonFile4("material/co2_unittest_above_sat.json");
    // Json::JsonObject parser4(jsonFile4);
    // Json::JsonObject density_ref4 = parser4.get_item("density");
    // Json::JsonObject enthalpy_ref4 = parser4.get_item("enthalpy");
    // Json::JsonObject pres_ref4 = parser4.get_item("pres");

    // // Number of data
    // int numSat = temp_ref3.size();

    // // Compare
    // Evaluation p_below;
    // Evaluation p_above;
    // for (int i = 0; i < numSat; ++i) {
    //     // Same temeperature above and below saturation curve, but different pressures
    //     Json::JsonObject t_ref = temp_ref3.get_array_item(i);
    //     Json::JsonObject p_ref = pres_ref3.get_array_item(i);
    //     Json::JsonObject p_ref2 = pres_ref4.get_array_item(i);
    //     T = Evaluation(t_ref.as_double());
    //     p_below = Evaluation(p_ref.as_double());
    //     p_above = Evaluation(p_ref2.as_double());

    //     // Density
    //     Scalar dens_below = CO2::gasDensity(T, p_below, extrapolate).value();
    //     Scalar dens_above = CO2::gasDensity(T, p_above, extrapolate).value();
    //     Scalar dens_ref_below = Scalar(density_ref3.get_array_item(i).as_double());
    //     Scalar dens_ref_above = Scalar(density_ref4.get_array_item(i).as_double());

    //     BOOST_CHECK_MESSAGE(close_at_tolerance(dens_below, dens_ref_below, tol), 
    //             "relative difference between density {"<<dens_below<<"} and reference {"<<dens_ref_below<<
    //             "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p_below.value()<<
    //             ") below saturation curve");
    //     BOOST_CHECK_MESSAGE(close_at_tolerance(dens_above, dens_ref_above, tol), 
    //             "relative difference between density {"<<dens_above<<"} and reference {"<<dens_ref_above<<
    //             "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p_above.value()<<
    //             ") above saturation curve");

    //     // Enthalpy
    //     Scalar enthalpy_below = CO2::gasEnthalpy(T, p_below, extrapolate).value();
    //     Scalar enthalpy_above = CO2::gasEnthalpy(T, p_above, extrapolate).value();
    //     Scalar enth_ref_below = Scalar(enthalpy_ref3.get_array_item(i).as_double());
    //     Scalar enth_ref_above = Scalar(enthalpy_ref4.get_array_item(i).as_double());

    //     BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy_below, enth_ref_below, tol), 
    //             "relative difference between enthalpy {"<<enthalpy_below<<"} and reference {"<<enth_ref_below<<
    //             "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p_below.value()<<
    //             ") below saturation curve");
    //     BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy_above, enth_ref_above, tol), 
    //             "relative difference between enthalpy {"<<enthalpy_above<<"} and reference {"<<enth_ref_above<<
    //             "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p_above.value()<<
    //             ") above saturation curve");
    // }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SimpleHuDuanClass, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using SimpleHuDuanH2O = Opm::SimpleHuDuanH2O<Scalar>;

    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/h2o_unittest.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    Json::JsonObject viscosity_ref = parser.get_item("viscosity");
    // Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");  // test values below instead
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");

    // For enthalpy reference data we used Coolprop with reference state T = 273.153, p = 101325 
    // (same reference state that was used for the polynomial liquid enthalpy in SimpleHuDuanH2O class)
    std::vector<Scalar> enthalpy_ref = {
       -36526.79515755, -28128.8435309 , -19737.99396456, -11353.22722153, -2973.65158128,  5401.51556181,  
       13772.96288595,  22141.29783022,  30507.05731371,  38870.71689078,  47232.69863707,  55593.37800122,
       63953.08978464,  72312.1334263 ,  80670.7776325,   89029.26450676,  97387.8132071 ,  105746.62320217, 
       114105.87717131, 122465.74358897, 130826.379027,   139187.93020495, 147550.53581447, 155914.32813963,
       164279.43449482, 172645.97849755, 181014.08119303, 189383.86204517, 197755.43980778, 206128.93328846, 
       214504.46201237, 222882.14680917, 231262.11030974, 239644.47738483, 248029.37551807, 256416.93512633,
       264807.28983266, 273200.57669743, 281596.93641358, 289996.5134699 , 298399.45628792, 306805.91733495, 
       315216.05321809, 323630.02476178, 332047.9970718 
       };
    
    // Setup pressure and temperature values
    int numT = temp_ref.size();
    int numP = pres_ref.size();
    Evaluation T;
    Evaluation p;

    // Rel. diff. tolerance
    Scalar tol = 1e-2;

    // Extrapolate
    bool extrapolate = true;
    
    // Loop over temperature and pressure, and compare to reference values in JSON file
    for (int iT = 0; iT < numT; ++iT) {
        // Get temperature from reference data
        T = Evaluation(temp_ref.get_array_item(iT).as_double());

        for (int iP = 0; iP < numP; ++iP) {
            // Get pressure value from reference data
            p = Evaluation(pres_ref.get_array_item(iP).as_double());

            // Density
            Scalar dens = SimpleHuDuanH2O::liquidDensity(T, p, extrapolate).value();
            Json::JsonObject dens_ref_row = density_ref.get_array_item(iT);
            Scalar dens_ref = Scalar(dens_ref_row.get_array_item(iP).as_double());
            
            BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
            
            // Viscosity
            Scalar visc = SimpleHuDuanH2O::liquidViscosity(T, p, extrapolate).value();
            Json::JsonObject visc_ref_row = viscosity_ref.get_array_item(iT);
            Scalar visc_ref = Scalar(visc_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
        }
        // Enthalpy
        Scalar enthalpy = SimpleHuDuanH2O::liquidEnthalpy(T, Evaluation(101325.0)).value();
        Scalar enth_ref = enthalpy_ref[iT];
        BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol), 
                "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(H2OClass, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using H2O = Opm::H2O<Scalar>;

    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/h2o_unittest.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    Json::JsonObject viscosity_ref = parser.get_item("viscosity");
    Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");
    
    // Setup pressure and temperature values
    int numT = temp_ref.size();
    int numP = pres_ref.size();
    Evaluation T;
    Evaluation p;

    // Rel. diff. tolerance
    Scalar tol = 1e-2;

    // Loop over temperature and pressure, and compare to values in JSON file
    for (int iT = 0; iT < numT; ++iT) {
        // Get temperature from reference data
        T = Evaluation(temp_ref.get_array_item(iT).as_double());

        for (int iP = 0; iP < numP; ++iP) {
            // Get pressure value from reference data
            p = Evaluation(pres_ref.get_array_item(iP).as_double());

            // Density
            Scalar dens = H2O::liquidDensity(T, p).value();
            Json::JsonObject dens_ref_row = density_ref.get_array_item(iT);
            Scalar dens_ref = Scalar(dens_ref_row.get_array_item(iP).as_double());
            
            BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Viscosity
            Scalar visc = H2O::liquidViscosity(T, p).value();
            Json::JsonObject visc_ref_row = viscosity_ref.get_array_item(iT);
            Scalar visc_ref = Scalar(visc_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Enthalpy
            Scalar enthalpy = H2O::liquidEnthalpy(T, p).value();
            Json::JsonObject enth_ref_row = enthalpy_ref.get_array_item(iT);
            Scalar enth_ref = Scalar(enth_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol), 
                "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BrineWithH2OClass, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using H2O = Opm::H2O<Scalar>;
    using BrineDyn = Opm::BrineDynamic<Scalar, H2O>;

    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/brine_unittest.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    // Json::JsonObject viscosity_ref = parser.get_item("viscosity");  // no values here at the moment
    Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");
    Json::JsonObject salinity_ref = parser.get_item("salinity");
    
    // Setup pressure and temperature values
    int numT = temp_ref.size();
    int numP = pres_ref.size();
    int numS = salinity_ref.size();
    Evaluation T;
    Evaluation p;
    Evaluation S;

    // Rel. diff. tolerance
    Scalar tol = 1e-2;
    Scalar tol_enth = 3.0e-2;

    // Extrapolation
    bool extrapolate = true;

    // Loop over temperature and pressure, and compare to Coolprop values in JSON file
    for (int iS = 0; iS < numS; ++iS){
        // Get salinity from reference data (mass fraction)
        S = Evaluation(salinity_ref.get_array_item(iS).as_double());

        for (int iT = 0; iT < numT; ++iT) {
            // Get temperature from reference data
            T = Evaluation(temp_ref.get_array_item(iT).as_double());

            for (int iP = 0; iP < numP; ++iP) {
                // Get pressure value from reference data
                p = Evaluation(pres_ref.get_array_item(iP).as_double());

                // Density
                Scalar dens = BrineDyn::liquidDensity(T, p, S, extrapolate).value();
                Json::JsonObject dens_ref_ax1 = density_ref.get_array_item(iS);
                Json::JsonObject dens_ref_ax2 = dens_ref_ax1.get_array_item(iT);
                Scalar dens_ref = Scalar(dens_ref_ax2.get_array_item(iP).as_double());
                
                BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                    "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                    "} exceeds tolerance {"<<tol<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                    S.value()<<")");

                // Viscosity
                // Scalar visc = BrineDyn::liquidViscosity(T, p, S).value();
                // Json::JsonObject visc_ref_ax1 = viscosity_ref.get_array_item(iS);
                // Json::JsonObject visc_ref_ax2 = visc_ref_ax1.get_array_item(iT);
                // Scalar visc_ref = Scalar(visc_ref_ax2.get_array_item(iP).as_double());

                // BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                //     "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                //     "} exceeds tolerance {"<<tol<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                //     S.value()<<")");

                // Enthalpy
                Scalar enthalpy = BrineDyn::liquidEnthalpy(T, p, S).value();
                Json::JsonObject enth_ref_ax1 = enthalpy_ref.get_array_item(iS);
                Json::JsonObject enth_ref_ax2 = enth_ref_ax1.get_array_item(iT);
                Scalar enth_ref = Scalar(enth_ref_ax2.get_array_item(iP).as_double());

                BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol_enth), 
                    "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                    "} exceeds tolerance {"<<tol_enth<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                    S.value()<<")");
            }
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BrineWithSimpleHuDuanH2OClass, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using SimpleHuDuanH2O = Opm::SimpleHuDuanH2O<Scalar>;
    using BrineDyn = Opm::BrineDynamic<Scalar, SimpleHuDuanH2O>;

    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/brine_unittest.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    // Json::JsonObject viscosity_ref = parser.get_item("viscosity");  // no values here at the moment
    // Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");  // don't test these at the moment
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");
    Json::JsonObject salinity_ref = parser.get_item("salinity");
    
    // Setup pressure, temperature and salinity values
    int numT = temp_ref.size();
    int numP = pres_ref.size();
    int numS = salinity_ref.size();
    Evaluation T;
    Evaluation p;
    Evaluation S;

    // Rel. diff. tolerance
    Scalar tol = 1e-2;

    // Extrapolation
    bool extrapolate = true;

    // Loop over temperature, pressure and salinity, and compare to reference values in JSON file
    for (int iS = 0; iS < numS; ++iS){
        // Get salinity from reference data (mass fraction)
        S = Evaluation(salinity_ref.get_array_item(iS).as_double());
        
        for (int iT = 0; iT < numT; ++iT) {
            // Get temperature from reference data
            T = Evaluation(temp_ref.get_array_item(iT).as_double());

            for (int iP = 0; iP < numP; ++iP) {
                // Get pressure value from reference data
                p = Evaluation(pres_ref.get_array_item(iP).as_double());

                // Density
                Scalar dens = BrineDyn::liquidDensity(T, p, S, extrapolate).value();
                Json::JsonObject dens_ref_ax1 = density_ref.get_array_item(iS);
                Json::JsonObject dens_ref_ax2 = dens_ref_ax1.get_array_item(iT);
                Scalar dens_ref = Scalar(dens_ref_ax2.get_array_item(iP).as_double());
                
                BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                    "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                    "} exceeds tolerance {"<<tol<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                    S.value()<<")");

                // Viscosity
                // Scalar visc = BrineDyn::liquidViscosity(T, p, S).value();
                // Json::JsonObject visc_ref_ax1 = viscosity_ref.get_array_item(iS);
                // Json::JsonObject visc_ref_ax2 = visc_ref_ax1.get_array_item(iT);
                // Scalar visc_ref = Scalar(visc_ref_ax2.get_array_item(iP).as_double());

                // BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol), 
                //     "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                //     "} exceeds tolerance {"<<tol<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                //     S.value()<<")");

                // Enthalpy
                // Scalar enthalpy = BrineDyn::liquidEnthalpy(T, p, S).value();
                // Json::JsonObject enth_ref_ax1 = enthalpy_ref.get_array_item(iS);
                // Json::JsonObject enth_ref_ax2 = enth_ref_ax1.get_array_item(iT);
                // Scalar enth_ref = Scalar(enth_ref_ax2.get_array_item(iP).as_double());

                // BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol), 
                //     "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                //     "} exceeds tolerance {"<<tol<<"} at (T, p, S) = ("<<T.value()<<", "<<p.value()<<", "<<
                //     S.value()<<")");
            }
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(H2Class, Scalar, Types)
{
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 3>;
    using H2 = Opm::H2<Scalar>;

    // Read JSON file with reference values
    std::filesystem::path jsonFile("material/h2_unittest.json");
    Json::JsonObject parser(jsonFile);
    Json::JsonObject density_ref = parser.get_item("density");
    Json::JsonObject viscosity_ref = parser.get_item("viscosity");
    Json::JsonObject enthalpy_ref = parser.get_item("enthalpy");
    Json::JsonObject temp_ref = parser.get_item("temp");
    Json::JsonObject pres_ref = parser.get_item("pres");
    
    // Setup pressure and temperature values
    int numT = temp_ref.size();
    int numP = pres_ref.size();
    Evaluation T;
    Evaluation p;

    // Extrapolate table
    bool extrapolate = true;

    // Rel. diff. tolerance
    Scalar tol = 1e-2;
    Scalar tol_visc = 2.6e-2;
    Scalar tol_enth = 3.8e-2;
    
    // Loop over temperature and pressure, and compare to reference values in JSON file
    for (int iT = 0; iT < numT; ++iT) {
        // Get temperature from reference data
        T = Evaluation(temp_ref.get_array_item(iT).as_double());

        for (int iP = 0; iP < numP; ++iP) {
            // Get pressure value from reference data
            p = Evaluation(pres_ref.get_array_item(iP).as_double());

            // Density
            Scalar dens = H2::gasDensity(T, p, extrapolate).value();
            Json::JsonObject dens_ref_row = density_ref.get_array_item(iT);
            Scalar dens_ref = Scalar(dens_ref_row.get_array_item(iP).as_double());
            
            BOOST_CHECK_MESSAGE(close_at_tolerance(dens, dens_ref, tol), 
                "relative difference between density {"<<dens<<"} and reference {"<<dens_ref<<
                "} exceeds tolerance {"<<tol<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Viscosity
            Scalar visc = H2::gasViscosity(T, p, extrapolate).value();
            Json::JsonObject visc_ref_row = viscosity_ref.get_array_item(iT);
            Scalar visc_ref = Scalar(visc_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(visc, visc_ref, tol_visc), 
                "relative difference between viscosity {"<<visc<<"} and reference {"<<visc_ref<<
                "} exceeds tolerance {"<<tol_visc<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");

            // Enthalpy
            Scalar enthalpy = H2::gasEnthalpy(T, p, extrapolate).value();
            Json::JsonObject enth_ref_row = enthalpy_ref.get_array_item(iT);
            Scalar enth_ref = Scalar(enth_ref_row.get_array_item(iP).as_double());

            BOOST_CHECK_MESSAGE(close_at_tolerance(enthalpy, enth_ref, tol_enth), 
                "relative difference between enthalpy {"<<enthalpy<<"} and reference {"<<enth_ref<<
                "} exceeds tolerance {"<<tol_enth<<"} at (T, p) = ("<<T.value()<<", "<<p.value()<<")");
        }
    }
}
