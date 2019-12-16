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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>


using namespace Opm;


const std::string& deckStr =  "START\n"
                              " 21 MAY 1981 /\n"
                              "\n"
                              "SCHEDULE\n"
                              "TSTEP\n"
                              " 1 2 3 4 5 /\n"
                              "\n"
                              "TUNING\n"
                              "2 300 0.3 0.30 6 0.6 0.2 2.25 2E20 /\n"
                              "0.2 0.002 2E-7 0.0002 11 0.02 2.0E-6 0.002 0.002 0.035 66 0.02 2/\n"
                              "13 2 26 2 9 9 4.0E6 4.0E6 4.0E6 1/\n"
                              "DATES\n"
                              " 1 JAN 1982 /\n"
                              " 1 JAN 1982 13:55:44 /\n"
                              " 3 JAN 1982 14:56:45.123 /\n"
                              "/\n"
                              "TSTEP\n"
                              " 9 10 /\n"
                              "\n"
                              "TUNING\n"
                              "2 300 0.3 0.30 6 0.6 0.2 2.25 2E20 10.0/\n"
                              "/\n"
                              "/\n";



static Deck createDeck(const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input);
}



BOOST_AUTO_TEST_CASE(TuningTest) {

  auto deck = createDeck(deckStr);
  EclipseGrid grid(10,10,10);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  FieldPropsManager fp(deck, grid, table);
  Runspec runspec (deck);
  Schedule schedule( deck, grid , fp, eclipseProperties, runspec);
  auto tuning = schedule.getTuning();
  auto event = schedule.getEvents();

  const double diff = 1.0e-14;

  /*** TIMESTEP 4***/
  /********* Record 1 ***********/
  size_t timestep = 4;
  BOOST_CHECK(!event.hasEvent(ScheduleEvents::TUNING_CHANGE, timestep));

  double TSINIT_default = tuning.get<double>("TSINIT", timestep);
  BOOST_CHECK_CLOSE(TSINIT_default, 1 * Metric::Time, diff);

  double TSMAXZ_default = tuning.get<double>("TSMAXZ", timestep);
  BOOST_CHECK_CLOSE(TSMAXZ_default, 365 * Metric::Time, diff);

  double TSMINZ_default = tuning.get<double>("TSMINZ", timestep);
  BOOST_CHECK_CLOSE(TSMINZ_default, 0.1 * Metric::Time, diff);

  double TSMCHP_default = tuning.get<double>("TSMCHP", timestep);
  BOOST_CHECK_CLOSE(TSMCHP_default, 0.15 * Metric::Time, diff);

  double TSFMAX_default = tuning.get<double>("TSFMAX", timestep);
  BOOST_CHECK_CLOSE(TSFMAX_default, 3.0, diff);

  double TSFMIN_default = tuning.get<double>("TSFMIN", timestep);
  BOOST_CHECK_CLOSE(TSFMIN_default, 0.3, diff);

  double TSFCNV_default = tuning.get<double>("TSFCNV", timestep);
  BOOST_CHECK_CLOSE(TSFCNV_default, 0.1, diff);

  double TFDIFF_default = tuning.get<double>("TFDIFF", timestep);
  BOOST_CHECK_CLOSE(TFDIFF_default, 1.25, diff);

  double THRUPT_default = tuning.get<double>("THRUPT", timestep);
  BOOST_CHECK_CLOSE(THRUPT_default, 1E20, diff);

  bool TMAXWC_has_value = tuning.has("TMAXWC", timestep);
  double TMAXWC_default = tuning.get<double>("TMAXWC", timestep);
  BOOST_CHECK_EQUAL(false, TMAXWC_has_value);
  BOOST_CHECK_CLOSE(TMAXWC_default, 0.0 * Metric::Time, diff);


  /********* Record 2 ************/
  double TRGTTE_default = tuning.get<double>("TRGTTE", timestep);
  BOOST_CHECK_CLOSE(TRGTTE_default, 0.1, diff);

  double TRGCNV_default = tuning.get<double>("TRGCNV", timestep);
  BOOST_CHECK_CLOSE(TRGCNV_default, 0.001, diff);

  double TRGMBE_default = tuning.get<double>("TRGMBE", timestep);
  BOOST_CHECK_CLOSE(TRGMBE_default, 1.0E-7, diff);

  double TRGLCV_default = tuning.get<double>("TRGLCV", timestep);
  BOOST_CHECK_CLOSE(TRGLCV_default, 0.0001, diff);

  double XXXTTE_default = tuning.get<double>("XXXTTE", timestep);
  BOOST_CHECK_CLOSE(XXXTTE_default, 10.0, diff);

  double XXXCNV_default = tuning.get<double>("XXXCNV", timestep);
  BOOST_CHECK_CLOSE(XXXCNV_default, 0.01, diff);

  double XXXMBE_default = tuning.get<double>("XXXMBE", timestep);
  BOOST_CHECK_CLOSE(XXXMBE_default, 1.0E-6, diff);

  double XXXLCV_default = tuning.get<double>("XXXLCV", timestep);
  BOOST_CHECK_CLOSE(XXXLCV_default, 0.001, diff);

  double XXXWFL_default = tuning.get<double>("XXXWFL", timestep);
  BOOST_CHECK_CLOSE(XXXWFL_default, 0.001, diff);

  double TRGFIP_default = tuning.get<double>("TRGFIP", timestep);
  BOOST_CHECK_CLOSE(TRGFIP_default, 0.025, diff);

  bool TRGSFT_has_value = tuning.has("TRGSFT", timestep);
  double TRGSFT_default = tuning.get<double>("TRGSFT", timestep);
  BOOST_CHECK_EQUAL(false, TRGSFT_has_value);
  BOOST_CHECK_CLOSE(TRGSFT_default, 0.0, diff);

  double THIONX_default = tuning.get<double>("THIONX", timestep);
  BOOST_CHECK_CLOSE(THIONX_default, 0.01, diff);

  int TRWGHT_default = tuning.get<int>("TRWGHT", timestep);
  BOOST_CHECK_EQUAL(TRWGHT_default, 1);


  /********* Record 3 ************/
  int NEWTMX_default = tuning.get<int>("NEWTMX", timestep);
  BOOST_CHECK_EQUAL(NEWTMX_default, 12);

  int NEWTMN_default = tuning.get<int>("NEWTMN", timestep);
  BOOST_CHECK_EQUAL(NEWTMN_default, 1);

  int LITMAX_default = tuning.get<int>("LITMAX", timestep);
  BOOST_CHECK_EQUAL(LITMAX_default, 25);

  int LITMIN_default = tuning.get<int>("LITMIN", timestep);
  BOOST_CHECK_EQUAL(LITMIN_default, 1);

  int MXWSIT_default = tuning.get<int>("MXWSIT", timestep);
  BOOST_CHECK_EQUAL(MXWSIT_default, 8);

  int MXWPIT_default = tuning.get<int>("MXWPIT", timestep);
  BOOST_CHECK_EQUAL(MXWPIT_default, 8);

  double DDPLIM_default = tuning.get<double>("DDPLIM", timestep);
  BOOST_CHECK_CLOSE(DDPLIM_default, 1.0E6 * Metric::Pressure, diff);

  double DDSLIM_default = tuning.get<double>("DDSLIM", timestep);
  BOOST_CHECK_CLOSE(DDSLIM_default, 1.0E6, diff);

  double TRGDPR_default = tuning.get<double>("TRGDPR", timestep);
  BOOST_CHECK_CLOSE(TRGDPR_default, 1.0E6 * Metric::Pressure, diff);

  bool XXXDPR_has_value = tuning.has("XXXDPR", timestep);
  double XXXDPR_default = tuning.get<double>("XXXDPR", timestep);
  BOOST_CHECK_EQUAL(false, XXXDPR_has_value);
  BOOST_CHECK_CLOSE(XXXDPR_default, 0.0, diff);




  /*** TIMESTEP 5***/
  /********* Record 1 ***********/
  timestep = 5;
  BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE, timestep));
  double TSINIT = tuning.get<double>("TSINIT", timestep);
  BOOST_CHECK_CLOSE(TSINIT, 2 * Metric::Time, diff);

  double TSMAXZ = tuning.get<double>("TSMAXZ", timestep);
  BOOST_CHECK_CLOSE(TSMAXZ, 300 * Metric::Time, diff);

  double TSMINZ = tuning.get<double>("TSMINZ", timestep);
  BOOST_CHECK_CLOSE(TSMINZ, 0.3 * Metric::Time, diff);

  double TSMCHP = tuning.get<double>("TSMCHP", timestep);
  BOOST_CHECK_CLOSE(TSMCHP, 0.30 * Metric::Time, diff);

  double TSFMAX = tuning.get<double>("TSFMAX", timestep);
  BOOST_CHECK_CLOSE(TSFMAX, 6.0, 1.0);

  double TSFMIN = tuning.get<double>("TSFMIN", timestep);
  BOOST_CHECK_CLOSE(TSFMIN, 0.6, 1.0);

  double TSFCNV = tuning.get<double>("TSFCNV", timestep);
  BOOST_CHECK_CLOSE(TSFCNV, 0.2, diff);

  double TFDIFF = tuning.get<double>("TFDIFF", timestep);
  BOOST_CHECK_CLOSE(TFDIFF, 2.25, diff);

  double THRUPT = tuning.get<double>("THRUPT", timestep);
  BOOST_CHECK_CLOSE(THRUPT, 2E20, diff);

  TMAXWC_has_value = tuning.has("TMAXWC", timestep);
  TMAXWC_default = tuning.get<double>("TMAXWC", timestep);
  BOOST_CHECK_EQUAL(false, TMAXWC_has_value);
  BOOST_CHECK_CLOSE(TMAXWC_default, 0.0 * Metric::Time, diff);

  /********* Record 2 ***********/
  double TRGTTE = tuning.get<double>("TRGTTE", timestep);
  BOOST_CHECK_CLOSE(TRGTTE, 0.2, diff);

  double TRGCNV = tuning.get<double>("TRGCNV", timestep);
  BOOST_CHECK_CLOSE(TRGCNV, 0.002, diff);

  double TRGMBE = tuning.get<double>("TRGMBE", timestep);
  BOOST_CHECK_CLOSE(TRGMBE, 2.0E-7, diff);

  double TRGLCV = tuning.get<double>("TRGLCV", timestep);
  BOOST_CHECK_CLOSE(TRGLCV, 0.0002, diff);

  double XXXTTE = tuning.get<double>("XXXTTE", timestep);
  BOOST_CHECK_CLOSE(XXXTTE, 11.0, diff);

  double XXXCNV = tuning.get<double>("XXXCNV", timestep);
  BOOST_CHECK_CLOSE(XXXCNV, 0.02, diff);

  double XXXMBE = tuning.get<double>("XXXMBE", timestep);
  BOOST_CHECK_CLOSE(XXXMBE, 2.0E-6, diff);

  double XXXLCV = tuning.get<double>("XXXLCV", timestep);
  BOOST_CHECK_CLOSE(XXXLCV, 0.002, diff);

  double XXXWFL = tuning.get<double>("XXXWFL", timestep);
  BOOST_CHECK_CLOSE(XXXWFL, 0.002, diff);

  double TRGFIP = tuning.get<double>("TRGFIP", timestep);
  BOOST_CHECK_CLOSE(TRGFIP, 0.035, diff);

  TRGSFT_has_value = tuning.has("TRGSFT", timestep);
  double TRGSFT = tuning.get<double>("TRGSFT", timestep);
  BOOST_CHECK_EQUAL(true, TRGSFT_has_value);
  BOOST_CHECK_CLOSE(TRGSFT, 66.0, diff);

  double THIONX = tuning.get<double>("THIONX", timestep);
  BOOST_CHECK_CLOSE(THIONX, 0.02, diff);

  int TRWGHT = tuning.get<int>("TRWGHT", timestep);
  BOOST_CHECK_EQUAL(TRWGHT, 2);

  /********* Record 3 ***********/
  int NEWTMX = tuning.get<int>("NEWTMX", timestep);
  BOOST_CHECK_EQUAL(NEWTMX, 13);

  int NEWTMN = tuning.get<int>("NEWTMN", timestep);
  BOOST_CHECK_EQUAL(NEWTMN, 2);

  int LITMAX = tuning.get<int>("LITMAX", timestep);
  BOOST_CHECK_EQUAL(LITMAX, 26);

  int LITMIN = tuning.get<int>("LITMIN", timestep);
  BOOST_CHECK_EQUAL(LITMIN, 2);

  int MXWSIT = tuning.get<int>("MXWSIT", timestep);
  BOOST_CHECK_EQUAL(MXWSIT, 9);

  int MXWPIT = tuning.get<int>("MXWPIT", timestep);
  BOOST_CHECK_EQUAL(MXWPIT, 9);

  double DDPLIM = tuning.get<double>("DDPLIM", timestep);
  BOOST_CHECK_CLOSE(DDPLIM, 4.0E6 * Metric::Pressure, diff);

  double DDSLIM = tuning.get<double>("DDSLIM", timestep);
  BOOST_CHECK_CLOSE(DDSLIM, 4.0E6, diff);

  double TRGDPR = tuning.get<double>("TRGDPR", timestep);
  BOOST_CHECK_CLOSE(TRGDPR, 4.0E6 * Metric::Pressure, diff);

  XXXDPR_has_value = tuning.has("XXXDPR", timestep);
  double XXXDPR = tuning.get<double>("XXXDPR", timestep);
  BOOST_CHECK_EQUAL(true, XXXDPR_has_value);
  BOOST_CHECK_CLOSE(XXXDPR, 1.0 * Metric::Pressure, diff);


  /*** TIMESTEP 7 ***/
  BOOST_CHECK(!event.hasEvent(ScheduleEvents::TUNING_CHANGE, 7));

  /*** TIMESTEP 10 ***/
  /********* Record 1 ***********/
  timestep = 10;
  BOOST_CHECK(event.hasEvent(ScheduleEvents::TUNING_CHANGE, timestep));
  TMAXWC_has_value = tuning.has("TMAXWC", timestep);
  TMAXWC_default = tuning.get<double>("TMAXWC", timestep);
  BOOST_CHECK_EQUAL(true, TMAXWC_has_value);
  BOOST_CHECK_CLOSE(TMAXWC_default, 10.0 * Metric::Time, diff);



}


BOOST_AUTO_TEST_CASE(TuningInitTest) {

  auto deck = createDeck(deckStr);
  EclipseGrid grid(10,10,10);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  FieldPropsManager fp(deck, grid, table);
  Runspec runspec (deck);
  Schedule schedule(deck , grid , fp, eclipseProperties, runspec);
  auto tuning = schedule.getTuning();


  const double diff = 1.0e-14;

  /*** TIMESTEP 4***/
  /********* Record 1 ***********/
  size_t timestep = 0;
  double value = 100.00;
  tuning.setInitial("TSINIT", value, false);
  double TSINIT_default = tuning.get<double>("TSINIT", timestep);
  BOOST_CHECK_CLOSE(TSINIT_default, 100.00, diff);

  timestep = 10;
  bool TMAXWC_has_value = tuning.has("TMAXWC", timestep);
  double TMAXWC_default = tuning.get<double>("TMAXWC", timestep);
  BOOST_CHECK_EQUAL(true, TMAXWC_has_value);
  BOOST_CHECK_CLOSE(TMAXWC_default, 10.0 * Metric::Time, diff);


  }


BOOST_AUTO_TEST_CASE(TuningResetTest) {

  auto deck = createDeck(deckStr);
  EclipseGrid grid(10,10,10);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  FieldPropsManager fp(deck, grid, table);
  Runspec runspec (deck);
  Schedule schedule(deck, grid , fp, eclipseProperties, runspec);
  auto tuning = schedule.getTuning();


  const double diff = 1.0e-14;

  /*** TIMESTEP 4***/
  /********* Record 1 ***********/
  size_t timestep = 0;
  double value = 666.00;
  tuning.setInitial("TSINIT", value, true);
  double TSINIT_default = tuning.get<double>("TSINIT", timestep);
  BOOST_CHECK_CLOSE(TSINIT_default, 666.00, diff);

  timestep = 10;
  TSINIT_default = tuning.get<double>("TSINIT", timestep);
  BOOST_CHECK_CLOSE(TSINIT_default, 666.00, diff);

}
