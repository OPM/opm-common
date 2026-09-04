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
 * \brief This is the unit test for the black oil PVT classes
 *
 * This test requires the presence of opm-parser.
 */
#include "config.h"

#include <boost/mpl/list.hpp>

#define BOOST_TEST_MODULE EclBlackOilPvt
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidsystems/blackoilpvt/LiveOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DeadOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WetGasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DryGasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityWaterPvt.hpp>

#include <opm/material/fluidsystems/blackoilpvt/GasPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/blackoilpvt/OilPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <tuple>

// values of strings based on the first SPE1 test case of opm-data.  note that in the
// real world it does not make much sense to specify a fluid phase using more than a
// single keyword, but for a unit test, this saves a lot of boiler-plate code.
static constexpr const char* deckString1 =
    "RUNSPEC\n"
    "\n"
    "DIMENS\n"
    "   10 10 3 /\n"
    "\n"
    "TABDIMS\n"
    " * 2 /\n"
    "\n"
    "OIL\n"
    "GAS\n"
    "WATER\n"
    "\n"
    "DISGAS\n"
    "\n"
    "METRIC\n"
    "\n"
    "GRID\n"
    "\n"
    "DX\n"
    "   	300*1000 /\n"
    "DY\n"
    "	300*1000 /\n"
    "DZ\n"
    "	100*20 100*30 100*50 /\n"
    "\n"
    "TOPS\n"
    "	100*1234 /\n"
    "\n"
    "PORO\n"
    "  300*0.15 /\n"
    "PROPS\n"
    "\n"
    "DENSITY\n"
    "      859.5  1033.0    0.854  /\n"
    "      860.04 1033.0    0.853  /\n"
    "\n"
    "PVTW\n"
    " 	1.0  1.1 1e-6 1.1 2.0e-9 /\n"
    " 	2.0  1.2 1e-7 1.2 3.0e-9 /\n"
    "\n"
    "PVCDO\n"
    " 	1.0  1.1 1e-6 1.1 2.0e-9 /\n"
    " 	2.0  1.2 1e-7 1.2 3.0e-9 /\n"
    "\n"
    "PVDG\n"
    "1.0	1.0	10.0\n"
    "2.0	*	*\n"
    "3.0	1e-10	30.0 /\n"
    "\n"
    "4.0	1.0	40.0\n"
    "5.0	*	*\n"
    "6.0	1e-10	60.0 /\n"
    "\n"
    "PVTG\n"
    "\n"
    "-- PVT region 2 --\n"
    "-- PRESSURE       RV        BG     VISCOSITY\n"
    "     1.00     1.1e-3       1.1      0.01\n"
    "              1.0e-3       1.15     0.005 /\n"
    "\n"
    "    500.00    0.9e-3       1.2     0.02\n"
    "              0.8e-3       1.25    0.015 /\n"
    "/\n"
    "\n"
    "-- PVT region 2 --\n"
    "-- PRESSURE       RV        BG     VISCOSITY\n"
    "     2.00     2.1e-3       2.1      0.02\n"
    "              2.0e-3       2.15     0.015 /\n"
    "\n"
    "    502.00    1.2e-3       2.2     2.02\n"
    "              1.1e-3       2.25    2.015 /\n"
    "/\n"
    "\n";

static Opm::Deck makeRSCONSTDeck(const std::string& rsconstRecord,
                                 const std::string& extraRunspec = "")
{
    return Opm::Parser{}.parseString(
        "RUNSPEC\n"
        "DIMENS\n"
        "  1 1 1 /\n"
        "TABDIMS\n"
        "  1 /\n"
        "OIL\n"
        "WATER\n"
        + extraRunspec +
        "METRIC\n"
        "GRID\n"
        "DX\n"
        "  100 /\n"
        "DY\n"
        "  100 /\n"
        "DZ\n"
        "  10 /\n"
        "TOPS\n"
        "  1000 /\n"
        "PORO\n"
        "  0.2 /\n"
        "PERMX\n"
        "  100 /\n"
        "PERMY\n"
        "  100 /\n"
        "PERMZ\n"
        "  20 /\n"
        "PROPS\n"
        "DENSITY\n"
        "  800 1000 1 /\n"
        "PVDO\n"
        "  100 1.1 2\n"
        "  200 1.0 2 /\n"
        "RSCONST\n"
        "  " + rsconstRecord + " /\n"
        "SCHEDULE\n"
        "END\n");
}

template <class Evaluation, class OilPvt, class GasPvt, class WaterPvt>
void ensurePvtApi(const OilPvt& oilPvt, const GasPvt& gasPvt, const WaterPvt& waterPvt)
{
    // we don't want to run this, we just want to make sure that it compiles
    while (0) {
        Evaluation temperature = 273.15 + 20.0;
        Evaluation pressure = 1e5;
        Evaluation saltconcentration = 0.0;
        Evaluation Rs = 0.0;
        Evaluation Rsw = 0.0;
        Evaluation Rv = 0.0;
        Evaluation Rvw = 0.0;
        Evaluation So = 0.5;
        Evaluation maxSo = 1.0;
        Evaluation depth = 0.0;

        /////
        // water PVT API
        /////
        std::ignore = waterPvt.viscosity(/*regionIdx=*/0,
                                         temperature,
                                         pressure,
                                         depth,
                                         Rsw,
                                         saltconcentration);
        std::ignore = waterPvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                            temperature,
                                                            pressure,
                                                            depth,
                                                            Rsw,
                                                            saltconcentration);

        /////
        // oil PVT API
        /////
        std::ignore = oilPvt.viscosity(/*regionIdx=*/0,
                                       temperature,
                                       pressure,
                                       Rs);
        std::ignore = oilPvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                          temperature,
                                                          pressure,
                                                          Rs);
        std::ignore = oilPvt.saturatedViscosity(/*regionIdx=*/0,
                                                temperature,
                                                pressure);
        std::ignore = oilPvt.saturatedInverseFormationVolumeFactor(/*regionIdx=*/0,
                                                                   temperature,
                                                                   pressure);
        std::ignore = oilPvt.saturationPressure(/*regionIdx=*/0,
                                                temperature,
                                                Rs);
        std::ignore = oilPvt.saturatedGasDissolutionFactor(/*regionIdx=*/0,
                                                           temperature,
                                                           pressure);
        std::ignore = oilPvt.saturatedGasDissolutionFactor(/*regionIdx=*/0,
                                                           temperature,
                                                           pressure,
                                                           So,
                                                           maxSo);

        /////
        // gas PVT API
        /////
        std::ignore = gasPvt.viscosity(/*regionIdx=*/0,
                                       temperature,
                                       pressure,
                                       Rv,
                                       Rvw);
        std::ignore = gasPvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                          temperature,
                                                          pressure,
                                                          Rv,
                                                          Rvw);
        std::ignore = gasPvt.saturatedViscosity(/*regionIdx=*/0,
                                                temperature,
                                                pressure);
        std::ignore = gasPvt.saturatedInverseFormationVolumeFactor(/*regionIdx=*/0,
                                                                   temperature,
                                                                   pressure);
        std::ignore = gasPvt.saturationPressure(/*regionIdx=*/0,
                                                temperature,
                                                Rv);
        std::ignore = gasPvt.saturatedOilVaporizationFactor(/*regionIdx=*/0,
                                                            temperature,
                                                            pressure);
        std::ignore = gasPvt.saturatedOilVaporizationFactor(/*regionIdx=*/0,
                                                            temperature,
                                                            pressure,
                                                            So,
                                                            maxSo);
    }
}

struct Fixture {
    Fixture()
        : deck(Opm::Parser().parseString(deckString1))
        , eclState(deck)
        , schedule(deck, eclState, std::make_shared<Opm::Python>())
    {}

    Opm::Deck deck;
    Opm::EclipseState eclState;
    Opm::Schedule schedule;
};

BOOST_FIXTURE_TEST_SUITE(Generic, Fixture)

using Types = boost::mpl::list<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(ApiConformance, Scalar, Types)
{
    Opm::GasPvtMultiplexer<Scalar> gasPvt;
    Opm::OilPvtMultiplexer<Scalar> oilPvt;
    Opm::WaterPvtMultiplexer<Scalar> waterPvt;

    gasPvt.initFromState(eclState, schedule);
    oilPvt.initFromState(eclState, schedule);
    waterPvt.initFromState(eclState, schedule);

    using FooEval = Opm::DenseAd::Evaluation<Scalar, 1>;
    ensurePvtApi<Scalar>(oilPvt, gasPvt, waterPvt);
    ensurePvtApi<FooEval>(oilPvt, gasPvt, waterPvt);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ConstantCompressibilityWater, Scalar, Types)
{
    constexpr Scalar tolerance = std::numeric_limits<Scalar>::epsilon()*1e3;

    Opm::ConstantCompressibilityWaterPvt<Scalar> constCompWaterPvt;
    constCompWaterPvt.initFromState(eclState, schedule);

    // make sure that the values at the reference points are the ones specified in the
    // deck.
    Scalar refTmp, tmp;

    refTmp = 1.1e-3; // the deck value is given in cP, while the SI units use Pa s...
    tmp = constCompWaterPvt.viscosity(/*regionIdx=*/0,
                                      /*temperature=*/273.15 + 20.0,
                                      /*pressure=*/1e5,
                                      /*depth=*/0.0,
                                      /*disgas_in_water*/0.0,
                                      /*saltconcentration=*/0.0);
    BOOST_CHECK_MESSAGE(std::abs(tmp - refTmp) <= tolerance,
                        "The reference water viscosity at region 0 is supposed to be " <<
                        refTmp << ". (is " << tmp << ")");

    refTmp = 1.2e-3;
    tmp = constCompWaterPvt.viscosity(/*regionIdx=*/1,
                                      /*temperature=*/273.15 + 20.0,
                                      /*pressure=*/2e5,
                                      /*depth=*/0.0,
                                      /*disgas_in_water*/0.0,
                                      /*saltconcentration=*/0.0);
    BOOST_CHECK_MESSAGE(std::abs(tmp - refTmp) <= tolerance,
                        "The reference water viscosity at region 1 is supposed to be " <<
                        refTmp << ". (is " << tmp << ")");
}

BOOST_AUTO_TEST_CASE(RSCONST_RequiresDeadOilMode)
{
    const auto deck2 = makeRSCONSTDeck("0.37 101.5", "GAS\nDISGAS\n");
    const Opm::EclipseState eclState2(deck2);
    const Opm::Schedule schedule2(deck2, eclState2, std::make_shared<Opm::Python>());

    Opm::OilPvtMultiplexer<double> oilPvt;
    BOOST_CHECK_THROW(oilPvt.initFromState(eclState2, schedule2), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RSCONST_UsesConstantRsAcrossPressure)
{
    const auto deck2 = makeRSCONSTDeck("0.37 101.5");
    const Opm::EclipseState eclState2(deck2);
    const Opm::Schedule schedule2(deck2, eclState2, std::make_shared<Opm::Python>());

    Opm::OilPvtMultiplexer<double> oilPvt;
    oilPvt.initFromState(eclState2, schedule2);

    // Bubble point is 101.5 barsa = 101.5e5 Pa, so use pressures above this
    const auto rsAtBubble = oilPvt.saturatedGasDissolutionFactor(0, 300.0, 101.5e5);
    const auto rsHighP = oilPvt.saturatedGasDissolutionFactor(0, 300.0, 5.0e7);

    BOOST_CHECK_CLOSE(rsAtBubble, 0.37, 1e-12);
    BOOST_CHECK_CLOSE(rsHighP, 0.37, 1e-12);
}

BOOST_AUTO_TEST_CASE(RSCONST_BubblePointMappedToSaturationPressure)
{
    const auto deck2 = makeRSCONSTDeck("0.37 101.5");
    const Opm::EclipseState eclState2(deck2);
    const Opm::Schedule schedule2(deck2, eclState2, std::make_shared<Opm::Python>());

    Opm::OilPvtMultiplexer<double> oilPvt;
    oilPvt.initFromState(eclState2, schedule2);

    const auto pb1 = oilPvt.saturationPressure(0, 300.0, 0.1);
    const auto pb2 = oilPvt.saturationPressure(0, 300.0, 0.6);

    BOOST_CHECK_CLOSE(pb1, 101.5 * Opm::unit::barsa, 1e-8);
    BOOST_CHECK_CLOSE(pb2, 101.5 * Opm::unit::barsa, 1e-8);
}

BOOST_AUTO_TEST_CASE(RSCONST_ThrowsBelowBubblePoint)
{
    const auto deck2 = makeRSCONSTDeck("0.37 101.5");
    const Opm::EclipseState eclState2(deck2);
    const Opm::Schedule schedule2(deck2, eclState2, std::make_shared<Opm::Python>());

    Opm::OilPvtMultiplexer<double> oilPvt;
    oilPvt.initFromState(eclState2, schedule2);

    // Bubble point from RSCONST: 101.5 barsa = 101.5e5 Pa = 10.15 MPa
    const double bubblePointPa = 101.5e5;

    // Test 1: pressure BELOW bubble point should throw NumericalProblem
    const double pressureBelowPb = 50e5;  // 50 barsa < 101.5 barsa
    BOOST_CHECK_THROW(
        oilPvt.saturatedGasDissolutionFactor(0, 300.0, pressureBelowPb),
        Opm::NumericalProblem
    );

    // Test 2: pressure AT bubble point should NOT throw
    const double pressureAtPb = bubblePointPa;
    BOOST_CHECK_NO_THROW(
        oilPvt.saturatedGasDissolutionFactor(0, 300.0, pressureAtPb)
    );

    // Test 3: pressure ABOVE bubble point should NOT throw
    const double pressureAbovePb = 200e5;  // 200 barsa > 101.5 barsa
    BOOST_CHECK_NO_THROW(
        oilPvt.saturatedGasDissolutionFactor(0, 300.0, pressureAbovePb)
    );
}

BOOST_AUTO_TEST_SUITE_END()
