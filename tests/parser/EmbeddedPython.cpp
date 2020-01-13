/*
  Copyright 2019 Equinor ASA.

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

#include <stdexcept>
#include <iostream>
#include <memory>

#define BOOST_TEST_MODULE EMBEDDED_PYTHON

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

using namespace Opm;

#ifndef EMBEDDED_PYTHON

BOOST_AUTO_TEST_CASE(INSTANTIATE) {
    Python python;
    BOOST_CHECK(!python);
    BOOST_CHECK_THROW(python.exec("print('Hello world')"), std::logic_error);
}

#else

BOOST_AUTO_TEST_CASE(INSTANTIATE) {
    Python python;
    BOOST_CHECK(python);
    BOOST_CHECK_NO_THROW(python.exec("print('Hello world')"));

    Parser parser;
    Deck deck;
    std::string python_code = R"(
print('Parser: {}'.format(context.parser))
print('Deck: {}'.format(context.deck))
kw = context.DeckKeyword( context.parser['FIELD'] )
context.deck.add(kw)
)";
    BOOST_CHECK_NO_THROW( python.exec(python_code, parser, deck));
    BOOST_CHECK( deck.hasKeyword("FIELD") );
}

BOOST_AUTO_TEST_CASE(PYINPUT_BASIC) {

    Parser parser;
    std::string input = R"(
        START             -- 0
        31 AUG 1993 /
        RUNSPEC
        PYINPUT
        kw = context.DeckKeyword( context.parser['FIELD'] )
        context.deck.add(kw)
        PYEND
        DIMENS
        2 2 1 /
        PYINPUT
        import numpy as np
        dx = np.array([0.25, 0.25, 0.25, 0.25])
        active_unit_system = context.deck.active_unit_system()
        default_unit_system = context.deck.default_unit_system()
        kw = context.DeckKeyword( context.parser['DX'], dx, active_unit_system, default_unit_system )
        context.deck.add(kw)
        PYEND
        DY
        4*0.25 /
        )";

    Deck deck = parser.parseString(input);
    BOOST_CHECK( deck.hasKeyword("START") );
    BOOST_CHECK( deck.hasKeyword("FIELD") );
    BOOST_CHECK( deck.hasKeyword("DIMENS") );
    BOOST_CHECK( deck.hasKeyword("DX") );
    auto DX = deck.getKeyword("DX");
    std::vector<double> dx_data = DX.getSIDoubleData();
    BOOST_CHECK_EQUAL( dx_data.size(), 4 );
    BOOST_CHECK_EQUAL( dx_data[2], 0.25 * 0.3048 );
    BOOST_CHECK( deck.hasKeyword("DY") );

}

BOOST_AUTO_TEST_CASE(PYACTION) {
const std::string deck_string = R"(
RUNSPEC

DIMENS
   10 10 3 /


GRID

DX
   	300*1000 /
DY
	300*1000 /
DZ
	100*20 100*30 100*50 /

TOPS
	100*8325 /

PORO
   	300*0.3 /

PERMX
   300*1 /

PERMY
   300*1 /

PERMZ
   300*1 /

SCHEDULE

PYACTION
import sys
sys.stdout.write("Running PYACTION\n")
if "FOPR" in context.sim:
    sys.stdout.write("Have FOPR: {}\n".format( context.sim["FOPR"] ))
else:
    sys.stdout.write("Missing FOPR\n")

grid = context.state.grid()
sys.stdout.write("Grid dimensions: ({},{},{})\n".format(grid.nx, grid.ny, grid.nz))

prod_well = context.schedule.get_well("PROD1", context.report_step)
sys.stdout.write("Well status: {}\n".format(prod_well.status()))
if not "list" in context.storage:
    context.storage["list"] = []
context.storage["list"].append(context.report_step)

if context.sim.well_var("PROD1", "WWCT") > 0.80:
    context.schedule.shut_well("PROD1", context.report_step)
    context.schedule.open_well("PROD2", context.report_step)
    context.sim.update("RUN_COUNT", 1)
print(context.storage["list"])
PYEND


WELSPECS
	'PROD1'	'G1'	10	10	8400	'OIL' /
	'PROD2'	'G1'	5	  5	  8400	'OIL' /
	'INJ'	'G1'	1	1	8335	'GAS' /
/

COMPDAT
	'PROD1'	10	10	3	3	'OPEN'	1*	1*	0.5 /
	'PROD2'	5	  5 	3	3	'SHUT'	1*	1*	0.5 /
	'INJ'	1	1	1	1	'OPEN'	1*	1*	0.5 /
/


WCONPROD
	'PROD1' 'OPEN' 'ORAT' 20000 4* 1000 /
/

WCONINJE
	'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 9014 /
/

TSTEP
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31 /

END
)";
    Parser parser;
    auto deck = parser.parseString(deck_string);
    auto ecl_state = EclipseState(deck);
    auto schedule = Schedule(deck, ecl_state);

    Python python;
    SummaryState st(std::chrono::system_clock::now());
    PyAction py_action(deck.getKeyword("PYACTION").getRecord(0).getItem("code").get<std::string>(0));
    st.update_well_var("PROD1", "WWCT", 0);
    python.exec(py_action, ecl_state, schedule, 10, st);

    st.update("FOPR", 0);
    python.exec(py_action, ecl_state, schedule, 10, st);

    st.update("FOPR", 100);
    st.update_well_var("PROD1", "WWCT", 0.90);
    python.exec(py_action, ecl_state, schedule, 10, st);
    const auto& well1 = schedule.getWell("PROD1", 10);
    const auto& well2 = schedule.getWell("PROD2", 10);
    BOOST_CHECK( well1.getStatus() == Well::Status::SHUT );
    BOOST_CHECK( well2.getStatus() == Well::Status::OPEN );
    BOOST_CHECK( st.has("RUN_COUNT") );
}



#endif

