/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BOOST_TEST_MODULE TuningTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Tuning.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

using namespace Opm;

const std::string& deckStr =  R"(
START
 21 MAY 1981 /

SCHEDULE
NEXTSTEP
  5 YES /

TSTEP
 1 2 3 4 5 /

TUNING
2 300 0.3 0.30 6 0.6 0.2 2.25 2E20 /
0.2 0.002 2E-7 0.0002 11 0.02 2.0E-6 0.002 0.002 0.035 66 0.02 2/
13 2 26 2 9 9 4.0E6 4.0E6 4.0E6 1 3/
DATES
 1 JAN 1982 /  -- 6
 1 JAN 1982 13:55:44 /  --7
 3 JAN 1982 14:56:45.123 /   -- 8
/

NEXTSTEP
 10 /

TSTEP
 9 10 /

TUNING
2 300 0.3 0.30 6 0.6 0.2 2.25 2E20 10.0/
/
/
WSEGITER
/
)";


static Deck createDeck(const std::string& input) {
    return Parser{}.parseString(input);
}



BOOST_AUTO_TEST_CASE(TuningTest)
{
   const auto deck = createDeck(deckStr);
  EclipseGrid grid(10,10,10);
  TableManager table ( deck );
  FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
  Runspec runspec (deck);
  const Schedule schedule {
      deck, grid, fp, NumericalAquifers{},
      runspec, std::make_shared<Python>()
  };

  const double diff = 1.0e-14;

  /*** TIMESTEP 4***/
  /********* Record 1 ***********/

  {
      const std::size_t timestep = 4;
      const auto& event = schedule[timestep].events();
      // Because NEXTSTEP is persistent a tuning event is triggered at each report step
      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));

      const auto& tuning = schedule[timestep].tuning();
      std::optional<double> TSINIT_default = tuning.TSINIT;
      BOOST_CHECK(TSINIT_default == std::nullopt);

      BOOST_CHECK_CLOSE(schedule[timestep].max_next_tstep(), 5*Metric::Time, diff);

      double TSMAXZ_default = tuning.TSMAXZ;
      BOOST_CHECK_CLOSE(TSMAXZ_default, 365 * Metric::Time, diff);

      double TSMINZ_default = tuning.TSMINZ;
      BOOST_CHECK_CLOSE(TSMINZ_default, 0.1 * Metric::Time, diff);

      double TSMCHP_default = tuning.TSMCHP;
      BOOST_CHECK_CLOSE(TSMCHP_default, 0.15 * Metric::Time, diff);

      double TSFMAX_default = tuning.TSFMAX;
      BOOST_CHECK_CLOSE(TSFMAX_default, 3.0, diff);

      double TSFMIN_default = tuning.TSFMIN;
      BOOST_CHECK_CLOSE(TSFMIN_default, 0.3, diff);

      double TSFCNV_default = tuning.TSFCNV;
      BOOST_CHECK_CLOSE(TSFCNV_default, 0.1, diff);

      double TFDIFF_default = tuning.TFDIFF;
      BOOST_CHECK_CLOSE(TFDIFF_default, 1.25, diff);

      double THRUPT_default = tuning.THRUPT;
      BOOST_CHECK_CLOSE(THRUPT_default, 1E20, diff);

      bool TMAXWC_has_value = tuning.TMAXWC_has_value;
      double TMAXWC_default = tuning.TMAXWC;
      BOOST_CHECK_EQUAL(false, TMAXWC_has_value);
      BOOST_CHECK_CLOSE(TMAXWC_default, 0.0 * Metric::Time, diff);


      /********* Record 2 ************/
      double TRGTTE_default = tuning.TRGTTE;
      BOOST_CHECK_CLOSE(TRGTTE_default, 0.1, diff);

      double TRGCNV_default = tuning.TRGCNV;
      BOOST_CHECK_CLOSE(TRGCNV_default, 0.001, diff);

      double TRGMBE_default = tuning.TRGMBE;
      BOOST_CHECK_CLOSE(TRGMBE_default, 1e-7, diff);

      double TRGLCV_default = tuning.TRGLCV;
      BOOST_CHECK_CLOSE(TRGLCV_default, 0.0001, diff);

      double XXXTTE_default = tuning.XXXTTE;
      BOOST_CHECK_CLOSE(XXXTTE_default, 10.0, diff);

      double XXXCNV_default = tuning.XXXCNV;
      BOOST_CHECK_CLOSE(XXXCNV_default, 0.01, diff);

      double XXXMBE_default = tuning.XXXMBE;
      BOOST_CHECK_CLOSE(XXXMBE_default, 1.0E-6, diff);

      double XXXLCV_default = tuning.XXXLCV;
      BOOST_CHECK_CLOSE(XXXLCV_default, 0.001, diff);

      double XXXWFL_default = tuning.XXXWFL;
      BOOST_CHECK_CLOSE(XXXWFL_default, 0.001, diff);

      double TRGFIP_default = tuning.TRGFIP;
      BOOST_CHECK_CLOSE(TRGFIP_default, 0.025, diff);

      bool TRGSFT_has_value = tuning.TRGSFT_has_value;
      double TRGSFT_default = tuning.TRGSFT;
      BOOST_CHECK_EQUAL(false, TRGSFT_has_value);
      BOOST_CHECK_CLOSE(TRGSFT_default, 0.0, diff);

      double THIONX_default = tuning.THIONX;
      BOOST_CHECK_CLOSE(THIONX_default, 0.01, diff);

      int TRWGHT_default = tuning.TRWGHT;
      BOOST_CHECK_EQUAL(TRWGHT_default, 1);


      /********* Record 3 ************/
      int NEWTMX_default = tuning.NEWTMX;
      BOOST_CHECK_EQUAL(NEWTMX_default, 12);

      int NEWTMN_default = tuning.NEWTMN;
      BOOST_CHECK_EQUAL(NEWTMN_default, 1);

      int LITMAX_default = tuning.LITMAX;
      BOOST_CHECK_EQUAL(LITMAX_default, 25);

      int LITMIN_default = tuning.LITMIN;
      BOOST_CHECK_EQUAL(LITMIN_default, 1);

      int MXWSIT_default = tuning.MXWSIT;
      BOOST_CHECK_EQUAL(MXWSIT_default, 8);

      int MXWPIT_default = tuning.MXWPIT;
      BOOST_CHECK_EQUAL(MXWPIT_default, 8);

      double DDPLIM_default = tuning.DDPLIM;
      BOOST_CHECK_CLOSE(DDPLIM_default, 1.0E6 * Metric::Pressure, diff);

      double DDSLIM_default = tuning.DDSLIM;
      BOOST_CHECK_CLOSE(DDSLIM_default, 1.0E6, diff);

      double TRGDPR_default = tuning.TRGDPR;
      BOOST_CHECK_CLOSE(TRGDPR_default, 1.0E6 * Metric::Pressure, diff);

      bool XXXDPR_has_value = tuning.XXXDPR_has_value;
      double XXXDPR_default = tuning.XXXDPR;
      BOOST_CHECK_EQUAL(false, XXXDPR_has_value);
      BOOST_CHECK_CLOSE(XXXDPR_default, 0.0, diff);

      int MNWRFP_default = tuning.MNWRFP;
      BOOST_CHECK_EQUAL(MNWRFP_default, 4);
  }



  /*** TIMESTEP 5***/
  /********* Record 1 ***********/
  {
      const std::size_t timeStep = 5;
      const auto& event = schedule[timeStep].events();
      const auto& tuning = schedule[timeStep].tuning();

      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));
                        std::optional<double> TSINIT = tuning.TSINIT;
      BOOST_CHECK_CLOSE(TSINIT.value(), 2 * Metric::Time, diff);

      BOOST_CHECK_CLOSE(schedule[timeStep].max_next_tstep(), 5*Metric::Time, diff);

      double TSMAXZ = tuning.TSMAXZ;
      BOOST_CHECK_CLOSE(TSMAXZ, 300 * Metric::Time, diff);

      double TSMINZ = tuning.TSMINZ;
      BOOST_CHECK_CLOSE(TSMINZ, 0.3 * Metric::Time, diff);

      double TSMCHP = tuning.TSMCHP;
      BOOST_CHECK_CLOSE(TSMCHP, 0.30 * Metric::Time, diff);

      double TSFMAX = tuning.TSFMAX;
      BOOST_CHECK_CLOSE(TSFMAX, 6.0, 1.0);

      double TSFMIN = tuning.TSFMIN;
      BOOST_CHECK_CLOSE(TSFMIN, 0.6, 1.0);

      double TSFCNV = tuning.TSFCNV;
      BOOST_CHECK_CLOSE(TSFCNV, 0.2, diff);

      double TFDIFF = tuning.TFDIFF;
      BOOST_CHECK_CLOSE(TFDIFF, 2.25, diff);

      double THRUPT = tuning.THRUPT;
      BOOST_CHECK_CLOSE(THRUPT, 2E20, diff);

      BOOST_CHECK_EQUAL(false, tuning.TMAXWC_has_value);
      BOOST_CHECK_CLOSE(tuning.TMAXWC, 0.0 * Metric::Time, diff);

      /********* Record 2 ***********/
      double TRGTTE = tuning.TRGTTE;
      BOOST_CHECK_CLOSE(TRGTTE, 0.2, diff);

      double TRGCNV = tuning.TRGCNV;
      BOOST_CHECK_CLOSE(TRGCNV, 0.002, diff);

      double TRGMBE = tuning.TRGMBE;
      BOOST_CHECK_CLOSE(TRGMBE, 2.0E-7, diff);

      double TRGLCV = tuning.TRGLCV;
      BOOST_CHECK_CLOSE(TRGLCV, 0.0002, diff);

      double XXXTTE = tuning.XXXTTE;
      BOOST_CHECK_CLOSE(XXXTTE, 11.0, diff);

      double XXXCNV = tuning.XXXCNV;
      BOOST_CHECK_CLOSE(XXXCNV, 0.02, diff);

      double XXXMBE = tuning.XXXMBE;
      BOOST_CHECK_CLOSE(XXXMBE, 2.0E-6, diff);

      double XXXLCV = tuning.XXXLCV;
      BOOST_CHECK_CLOSE(XXXLCV, 0.002, diff);

      double XXXWFL = tuning.XXXWFL;
      BOOST_CHECK_CLOSE(XXXWFL, 0.002, diff);

      double TRGFIP = tuning.TRGFIP;
      BOOST_CHECK_CLOSE(TRGFIP, 0.035, diff);

      BOOST_CHECK_EQUAL(true, tuning.TRGSFT_has_value);
      BOOST_CHECK_CLOSE(tuning.TRGSFT, 66.0, diff);

      double THIONX = tuning.THIONX;
      BOOST_CHECK_CLOSE(THIONX, 0.02, diff);

      int TRWGHT = tuning.TRWGHT;
      BOOST_CHECK_EQUAL(TRWGHT, 2);

      /********* Record 3 ***********/
      int NEWTMX = tuning.NEWTMX;
      BOOST_CHECK_EQUAL(NEWTMX, 13);

      int NEWTMN = tuning.NEWTMN;
      BOOST_CHECK_EQUAL(NEWTMN, 2);

      int LITMAX = tuning.LITMAX;
      BOOST_CHECK_EQUAL(LITMAX, 26);

      int LITMIN = tuning.LITMIN;
      BOOST_CHECK_EQUAL(LITMIN, 2);

      int MXWSIT = tuning.MXWSIT;
      BOOST_CHECK_EQUAL(MXWSIT, 9);

      int MXWPIT = tuning.MXWPIT;
      BOOST_CHECK_EQUAL(MXWPIT, 9);

      double DDPLIM = tuning.DDPLIM;
      BOOST_CHECK_CLOSE(DDPLIM, 4.0E6 * Metric::Pressure, diff);

      double DDSLIM = tuning.DDSLIM;
      BOOST_CHECK_CLOSE(DDSLIM, 4.0E6, diff);

      double TRGDPR = tuning.TRGDPR;
      BOOST_CHECK_CLOSE(TRGDPR, 4.0E6 * Metric::Pressure, diff);

      BOOST_CHECK_EQUAL(true, tuning.XXXDPR_has_value);
      BOOST_CHECK_CLOSE(tuning.XXXDPR, 1.0 * Metric::Pressure, diff);

      int MNWRFP = tuning.MNWRFP;
      BOOST_CHECK_EQUAL(MNWRFP, 3);
  }

  /*** TIMESTEP 7 ***/
  {
      const std::size_t timestep = 7;
      const auto& event = schedule[timestep].events();
                        // Because NEXTSTEP is persistent a tuning event is triggered at each report step
      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));
  }

  /*** TIMESTEP 8 ***/
  {
      const std::size_t timestep = 8;
      const auto& event = schedule[timestep].events();
      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));

      BOOST_CHECK_CLOSE(schedule[timestep].max_next_tstep(), 10*Metric::Time, diff);
  }

  /*** TIMESTEP 9 ***/
  {
      const std::size_t timestep = 9;
      const auto& event = schedule[timestep].events();
      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));

      /* Assume --enable-tuning=true, so the next time step spec in TUNING has an effect */
      BOOST_CHECK_CLOSE(schedule[timestep].max_next_tstep(true), -1.0, diff);
  }

  /*** TIMESTEP 10 ***/
  {
      /********* Record 1 ***********/
      const std::size_t timestep = 10;
      const auto& tuning = schedule[timestep].tuning();
      const auto& event = schedule[timestep].events();
      BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE));
      BOOST_CHECK_EQUAL(true, tuning.TMAXWC_has_value);
      BOOST_CHECK_CLOSE(tuning.TMAXWC, 10.0 * Metric::Time, diff);

      BOOST_CHECK_EQUAL(tuning.MXWSIT, ParserKeywords::WSEGITER::MAX_WELL_ITERATIONS::defaultValue);

      /********* Record 2 [and 3] (should be unchanged) ***********/
      double TRGTTE = tuning.TRGTTE;
      BOOST_CHECK_CLOSE(TRGTTE, 0.2, diff);

      int NEWTMX = tuning.NEWTMX;
      BOOST_CHECK_EQUAL(NEWTMX, 13);
  }
}

const std::string& deckWithTUNINGDP = R"(
START
    1 JAN 2026 /

SCHEDULE
TSTEP
    1 2 /

TUNINGDP
/

TSTEP
    3 4 /

TUNINGDP
    0.025 0.125 10.0 20.0 30.0 40.0 /

TSTEP
    5 6 /

TUNINGDP
    1* 1* 15.0 1* 1* 1* /

TSTEP
    7 8 /

TUNINGDP
    1* 1* 1* 25.0 1* 1* /

TSTEP
    9 10 /

TUNINGDP
    1* 1* 1* 1* 35.0 1* /

TSTEP
    11 12 /

TUNINGDP
    1* 1* 1* 1* 1* 45.0 /
)";

BOOST_AUTO_TEST_CASE(TuningDpTest)
{
    // Set up deck with schedule
    const auto deck = createDeck(deckWithTUNINGDP);
    EclipseGrid grid(10, 10, 10);
    TableManager table (deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    // TUNINGDP_CHANGE events checks
    {
        for (std::size_t timestep = 1; timestep < 13; ++timestep) {
            const auto& event = schedule[timestep].events();
            bool tuning_change = event.hasEvent(ScheduleEvents::TUNINGDP_CHANGE);
            bool expected_tuning_change = (timestep % 2 == 0);  // even time steps have tuning changes
            BOOST_CHECK_MESSAGE(tuning_change == expected_tuning_change,
                                "Unexpected TUNINGDP_CHANGE event = " << tuning_change << " for time step "
                                << timestep);
        }
    }

    // Test tolerance
    const double tol = 1e-6;

    // TIMESTEP 1:
    {
        // Check that values when TUNINGDP is not set
        // For TRGLCV and XXXLCV defaults are equal to TUNING defaults, while TRGDDP and TRGDDS are 0.0
        const std::size_t timestep = 1;
        const auto& tuning_dp = schedule[timestep].tuning_dp();
        const auto& tuning = schedule[timestep].tuning();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, tuning.TRGLCV, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, tuning.XXXLCV, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 0.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 0.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 0.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 0.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 2
    {
        // Check defaults when TUNINGDP is set but defaulted in the deck, which should be the defaults set in TUNINGDP
        // keyword
        const std::size_t timestep = 2;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.00001, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.001, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 1.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 0.01, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 0.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 0.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 4:
    {
        const std::size_t timestep = 4;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.025, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.125, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 10.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 20.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 30.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 40.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 6:
    {
        const std::size_t timestep = 6;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.025, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.125, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 15.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 20.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 30.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 40.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 8:
    {
        const std::size_t timestep = 8;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.025, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.125, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 15.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 25.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 30.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 40.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 10:
    {
        const std::size_t timestep = 10;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.025, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.125, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 15.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 25.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 35.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 40.0 * Metric::OilDissolutionFactor, tol);
    }

    // TIMESTEP 12:
    {
        const std::size_t timestep = 12;
        const auto& tuning_dp = schedule[timestep].tuning_dp();

        BOOST_CHECK_CLOSE(tuning_dp.TRGLCV, 0.025, tol);
        BOOST_CHECK_CLOSE(tuning_dp.XXXLCV, 0.125, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDP, 15.0 * Metric::Pressure, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDS, 25.0, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRS, 35.0 * Metric::GasDissolutionFactor, tol);
        BOOST_CHECK_CLOSE(tuning_dp.TRGDDRV, 45.0 * Metric::OilDissolutionFactor, tol);
    }
}

namespace {

Schedule makeSchedule(const std::string& input)
{
    const auto deck = createDeck(input);
    EclipseGrid grid(10, 10, 10);
    TableManager table (deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec (deck);
    return Schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(CvcritTest)
{
    const auto schedule = makeSchedule(R"(
START
    1 JAN 2026 /

SCHEDULE
TSTEP
    1 /

CVCRIT
/

TSTEP
    1 /

CVCRIT
    0.5 30 1e-8 50 1e-4 3 0.02 5.0 2 1* 1e-3 1e-8 0.3 4.0 20.0 7*
    30 0.02 60 70 5 3* 900 1e10 4* 200 50 150 /

TSTEP
    1 /

CVCRIT
    1* 25 /

TSTEP
    1 /

CVCRIT
    40* 0.1 1e-5 /

TSTEP
    1 /
)");

    for (std::size_t step = 0; step < schedule.size(); ++step) {
        BOOST_CHECK_MESSAGE(schedule[step].events().hasEvent(ScheduleEvents::CVCRIT_CHANGE)
                            == (step >= 1 && step <= 4),
                            "Unexpected CVCRIT_CHANGE event at report step " << step);
    }

    BOOST_CHECK(!schedule[0].cvcrit().has_value());

    const double tol = 1e-8;
    const double kgMole = 1000.0 * unit::mol;
    const double m3 = unit::cubic(unit::meter);

    // Defaulted items take the keyword's defaults, and the items whose
    // default depends on the solution method have no value.
    {
        const auto& cvcrit = schedule[1].cvcrit();
        BOOST_REQUIRE(cvcrit.has_value());
        BOOST_CHECK_CLOSE(cvcrit->pressure_change_tolerance, 0.1 * unit::atm, tol);
        BOOST_CHECK(!cvcrit->max_nonlinear_iterations.has_value());
        BOOST_CHECK_CLOSE(cvcrit->linear_residual_reduction, 1e-10, tol);
        BOOST_CHECK(!cvcrit->max_linear_iterations.has_value());
        BOOST_CHECK_CLOSE(cvcrit->fugacity_error_tolerance, 1e-3, tol);
        BOOST_CHECK(!cvcrit->min_linear_iterations.has_value());
        BOOST_CHECK(!cvcrit->specific_volume_change_tolerance.has_value());
        BOOST_CHECK_CLOSE(cvcrit->temperature_change_tolerance, 10.0, tol);
        BOOST_CHECK(!cvcrit->min_nonlinear_iterations.has_value());
        BOOST_CHECK_CLOSE(cvcrit->residual_max_norm_tolerance, 1e-4, tol);
        BOOST_CHECK_CLOSE(cvcrit->residual_sum_tolerance, 1e-20, tol);
        BOOST_CHECK(!cvcrit->max_molar_density_change.has_value());
        BOOST_CHECK(!cvcrit->max_temperature_change.has_value());
        BOOST_CHECK(!cvcrit->max_pressure_change.has_value());
        BOOST_CHECK_EQUAL(cvcrit->msw_max_iterations, 40);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_density_tolerance, 0.015 * kgMole / m3, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_rate_tolerance, 50.0 * kgMole / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_energy_density_tolerance, 50.0e3 / m3, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_velocity_tolerance, 10.0 * unit::meter / unit::second, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_residual_tolerance, 1000.0 * kgMole / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_energy_residual_tolerance, 1e23 / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_pressure_change, 250.0 * unit::atm, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_specific_volume_change, 100.0, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_temperature_change, 200.0, tol);
        BOOST_CHECK_CLOSE(cvcrit->additional_residual_max_norm_tolerance, 1e20, tol);
        BOOST_CHECK_CLOSE(cvcrit->additional_residual_sum_tolerance, 1e20, tol);
    }

    {
        const auto& cvcrit = schedule[2].cvcrit();
        BOOST_REQUIRE(cvcrit.has_value());
        BOOST_CHECK_CLOSE(cvcrit->pressure_change_tolerance, 0.5 * unit::barsa, tol);
        BOOST_CHECK_EQUAL(cvcrit->max_nonlinear_iterations.value_or(-1), 30);
        BOOST_CHECK_CLOSE(cvcrit->linear_residual_reduction, 1e-8, tol);
        BOOST_CHECK_EQUAL(cvcrit->max_linear_iterations.value_or(-1), 50);
        BOOST_CHECK_CLOSE(cvcrit->fugacity_error_tolerance, 1e-4, tol);
        BOOST_CHECK_EQUAL(cvcrit->min_linear_iterations.value_or(-1), 3);
        BOOST_CHECK_CLOSE(cvcrit->specific_volume_change_tolerance.value_or(-1.0), 0.02, tol);
        BOOST_CHECK_CLOSE(cvcrit->temperature_change_tolerance, 5.0, tol);
        BOOST_CHECK_EQUAL(cvcrit->min_nonlinear_iterations.value_or(-1), 2);
        BOOST_CHECK_CLOSE(cvcrit->residual_max_norm_tolerance, 1e-3, tol);
        BOOST_CHECK_CLOSE(cvcrit->residual_sum_tolerance, 1e-8, tol);
        BOOST_CHECK_CLOSE(cvcrit->max_molar_density_change.value_or(-1.0), 0.3, tol);
        BOOST_CHECK_CLOSE(cvcrit->max_temperature_change.value_or(-1.0), 4.0, tol);
        BOOST_CHECK_CLOSE(cvcrit->max_pressure_change.value_or(-1.0), 20.0 * unit::barsa, tol);
        BOOST_CHECK_EQUAL(cvcrit->msw_max_iterations, 30);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_density_tolerance, 0.02 * kgMole / m3, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_rate_tolerance, 60.0 * kgMole / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_energy_density_tolerance, 70.0e3 / m3, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_velocity_tolerance, 5.0 * unit::meter / unit::second, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_molar_residual_tolerance, 900.0 * kgMole / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_energy_residual_tolerance, 1e13 / unit::day, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_pressure_change, 200.0 * unit::barsa, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_specific_volume_change, 50.0, tol);
        BOOST_CHECK_CLOSE(cvcrit->msw_max_temperature_change, 150.0, tol);
        BOOST_CHECK_CLOSE(cvcrit->additional_residual_max_norm_tolerance, 1e20, tol);
        BOOST_CHECK_CLOSE(cvcrit->additional_residual_sum_tolerance, 1e20, tol);
    }

    // Items defaulted in a later CVCRIT take the keyword's defaults again,
    // not their values from the earlier CVCRIT.
    {
        auto expected = *schedule[1].cvcrit();
        expected.max_nonlinear_iterations = 25;
        BOOST_CHECK(schedule[3].cvcrit() == expected);

        expected = *schedule[1].cvcrit();
        expected.additional_residual_max_norm_tolerance = 0.1;
        expected.additional_residual_sum_tolerance = 1e-5;
        BOOST_CHECK(schedule[4].cvcrit() == expected);

        // No CVCRIT in the last step, so the state carries over unchanged
        BOOST_CHECK(schedule[5].cvcrit() == expected);
    }
}

BOOST_AUTO_TEST_CASE(CvcritFieldUnits)
{
    const auto schedule = makeSchedule(R"(
RUNSPEC
FIELD

START
    1 JAN 2026 /

SCHEDULE
TSTEP
    1 /

CVCRIT
/

TSTEP
    1 /

CVCRIT
    2.0 22* 0.5 /
)");

    const double tol = 1e-8;

    // The defaults do not depend on the unit system.
    BOOST_CHECK_CLOSE(schedule[1].cvcrit()->pressure_change_tolerance, 0.1 * unit::atm, tol);
    BOOST_CHECK_CLOSE(schedule[1].cvcrit()->msw_max_pressure_change, 250.0 * unit::atm, tol);

    BOOST_CHECK_CLOSE(schedule[2].cvcrit()->pressure_change_tolerance,
                      2.0 * Field::Pressure, tol);
    BOOST_CHECK_CLOSE(schedule[2].cvcrit()->msw_molar_density_tolerance,
                      0.5 * Field::Moles / Field::ReservoirVolume, tol);
}
