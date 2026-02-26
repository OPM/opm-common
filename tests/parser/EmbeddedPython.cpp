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

#define BOOST_TEST_MODULE EMBEDDED_PYTHON

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQParams.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Opm;

#ifndef EMBEDDED_PYTHON

BOOST_AUTO_TEST_CASE(INSTANTIATE)
{
    Python python;
    BOOST_CHECK_MESSAGE(!python.enabled(), "Python must not be enabled in a default-constructed Python object unless we have Embedded Python support");
    BOOST_CHECK_MESSAGE(!python.exec("print('Hello world')"), "Default-constructed Python object must not run Python code unless we have Embedded Python support");
    BOOST_CHECK_MESSAGE(!Python::supported(), "Python must not be supported unless we have Embedded Python support");

    Python python_cond(Python::Enable::TRY);
    BOOST_CHECK(!python_cond.enabled());

    Python python_off(Python::Enable::OFF);
    BOOST_CHECK(!python_off.enabled());
}

#else // EMBEDDED_PYTHON

BOOST_AUTO_TEST_CASE(INSTANTIATE)
{
    auto python = std::make_shared<Python>();
    BOOST_CHECK_MESSAGE(Python::supported(), "Python interpreter must be available when we have Embedded Python support");
    BOOST_CHECK_MESSAGE(python->enabled(), "Python interpreter must be enabled in a default-constructed Python object when we have Embedded Python support");
    BOOST_CHECK_NO_THROW(python->exec("import sys"));

    Parser parser{python};
    Deck deck;
    const std::string python_code = R"(
print('Parser: {}'.format(context.parser))
print('Deck: {}'.format(context.deck))
kw = context.DeckKeyword( context.parser['FIELD'] )
context.deck.add(kw)
)";
    BOOST_CHECK_NO_THROW(python->exec(python_code, parser, deck));
    BOOST_CHECK(deck.hasKeyword("FIELD"));
}

BOOST_AUTO_TEST_CASE(PYINPUT_BASIC)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
START             -- 0
31 AUG 1993 /

PYINPUT
kw = context.DeckKeyword(context.parser['FIELD'])
context.deck.add(kw)
PYEND

DIMENS
2 2 1 /

PYINPUT
try:
    import numpy as np
    dx = np.array([0.25, 0.25, 0.25, 0.25])
    active_unit_system = context.deck.active_unit_system()
    default_unit_system = context.deck.default_unit_system()
    kw = context.DeckKeyword(context.parser['DX'], dx, active_unit_system, default_unit_system)
    context.deck.add(kw)
except ImportError:
    # NumPy might not be available on host. Nothing to do in this case.
    pass
PYEND

DY
4*0.25 /

END
)");

    BOOST_CHECK_MESSAGE(deck.hasKeyword("START"), "START keyword must be present");
    BOOST_CHECK_MESSAGE(deck.hasKeyword("FIELD"), "FIELD keyword must be present");
    BOOST_CHECK_MESSAGE(deck.hasKeyword("DIMENS"), "DIMENS keyword must be present");

    if (deck.hasKeyword("DX")) {
        auto DX = deck["DX"].back();
        std::vector<double> dx_data = DX.getSIDoubleData();
        BOOST_CHECK_EQUAL(dx_data.size(), 4);
        BOOST_CHECK_EQUAL(dx_data[2], 0.25 * 0.3048);
    }

    BOOST_CHECK_MESSAGE(deck.hasKeyword("DY"), "DY keyword must be present");
}

BOOST_AUTO_TEST_CASE(PYACTION)
{
    auto python = std::make_shared<Python>(Python::Enable::ON);

    const auto deck = Parser{python}.parseFile("EMBEDDED_PYTHON.DATA");

    auto ecl_state = EclipseState(deck);
    auto schedule = Schedule(deck, ecl_state, python);
    auto st = SummaryState {
        TimeService::now(), ecl_state.runspec().udqParams().undefinedValue()
    };

    const auto& pyaction_kw = deck.get<ParserKeywords::PYACTION>().front();
    const std::string& fname = pyaction_kw.getRecord(1).getItem(0).get<std::string>(0);

    Action::PyAction py_action(python, "WCLOSE", Action::PyAction::RunCount::unlimited, deck.makeDeckPath(fname));
    auto actionx_callback = [] (const std::string&, const std::vector<std::string>&) {};

    Action::State action_state;

    st.update_well_var("PROD1", "WWCT", 0);
    py_action.run(ecl_state, schedule, 10, st, actionx_callback);

    st.update("FOPR", 0);
    py_action.run(ecl_state, schedule, 10, st, actionx_callback);

    st.update("FOPR", 100);
    st.update_well_var("PROD1", "WWCT", 0.90);
    py_action.run(ecl_state, schedule, 10, st, actionx_callback);

    const auto& well1 = schedule.getWell("PROD1", 10);
    const auto& well2_1 = schedule.getWell("PROD2", 1);
    const auto& well2_10 = schedule.getWell("PROD2", 10);
    BOOST_CHECK( well1.getStatus() == Well::Status::SHUT );
    BOOST_CHECK( well2_1.getStatus() == Well::Status::SHUT );
    BOOST_CHECK( well2_10.getStatus() == Well::Status::OPEN );
    BOOST_CHECK( st.has("RUN_COUNT") );

    std::map<std::string, Action::PyAction> action_map;
    for (const auto * p : schedule[0].actions().pending_python(action_state))
        action_map.emplace( p->name(), *p );

    const auto& pyaction_unlimited  = action_map.at("UNLIMITED");
    const auto& pyaction_single     = action_map.at("SINGLE");
    const auto& pyaction_first_true = action_map.at("FIRST_TRUE");

    auto actions = schedule[0].actions();
    BOOST_CHECK( actions.pending_python(action_state).size() == 4);

    action_state.add_run( py_action, true);
    BOOST_CHECK( actions.pending_python(action_state).size() == 4);

    action_state.add_run( pyaction_unlimited, true);
    BOOST_CHECK( actions.pending_python(action_state).size() == 4);

    action_state.add_run( pyaction_single, false);
    BOOST_CHECK( actions.pending_python(action_state).size() == 3);

    action_state.add_run( pyaction_first_true, false);
    BOOST_CHECK( actions.pending_python(action_state).size() == 3);

    action_state.add_run( pyaction_first_true, true);
    BOOST_CHECK( actions.pending_python(action_state).size() == 2);
}

BOOST_AUTO_TEST_CASE(Python_Constructor)
{
    Python python_off(Python::Enable::OFF);
    BOOST_CHECK(!python_off.enabled());

    Python python_on(Python::Enable::ON);
    BOOST_CHECK(python_on.enabled());

    //.enabled() Can only have one Python interpreter active at any time
    BOOST_CHECK_THROW(Python python_throw(Python::Enable::ON), std::logic_error);
}

BOOST_AUTO_TEST_CASE(Python_Constructor2)
{
    Python python_cond1(Python::Enable::TRY);
    BOOST_CHECK(python_cond1.enabled());

    Python python_cond2(Python::Enable::TRY);
    BOOST_CHECK(!python_cond2.enabled());
}

#endif  // EMBEDDED_PYTHON
