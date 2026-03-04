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

#include "config.h"

#define BOOST_TEST_MODULE PY_ACTION_TESTER

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>

#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Opm;

BOOST_AUTO_TEST_CASE(ParsePYACTION) {
    auto python = std::make_shared<Python>();

    const auto deck = Parser{python}.parseFile("PYACTION.DATA");

    const auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::from_string(record0.getItem(1).get<std::string>(0));
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));

    Action::PyAction pyaction(python, "ACT1", run_count, ok_module);
    BOOST_CHECK_EQUAL(pyaction.name(), "ACT1");
}

#ifdef EMBEDDED_PYTHON

BOOST_AUTO_TEST_CASE(ParsePYACTION_Module_Syntax_Error) {
    auto python = std::make_shared<Python>();

    const auto deck = Parser{python}.parseFile("PYACTION.DATA");
    const auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::from_string(record0.getItem(1).get<std::string>(0));

    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction1(python, "ACT1", run_count, ok_module);

    const std::string& broken_module2 = deck.makeDeckPath("action_syntax_error.py");
    Action::PyAction pyaction2(python, "ACT2", run_count, broken_module2);

    EclipseState state;
    Schedule schedule;
    SummaryState summary_state;
    const std::function<void(const std::string&, const std::vector<std::string>&)> actionx_callback;

    BOOST_CHECK_THROW(pyaction2.run(state, schedule, 0, summary_state, actionx_callback), std::exception);
}

BOOST_AUTO_TEST_CASE(ParsePYACTION_ModuleMissing) {
    auto python = std::make_shared<Python>();

    auto deck = Parser{python}.parseFile("PYACTION.DATA");
    auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::from_string(record0.getItem(1).get<std::string>(0));
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction(python, "ACT1", run_count, ok_module);

    const std::string& missing_module = deck.makeDeckPath("no_such_module.py");
    BOOST_CHECK_THROW(Action::PyAction(python , "ACT2", run_count, missing_module), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(PYACTION_Log) {
    std::ostringstream log_stream; // Custom stream for capturing log messages
    std::shared_ptr<Opm::StreamLog> stream_log = std::make_shared<Opm::StreamLog>(log_stream, Opm::Log::DefaultMessageTypes);

    OpmLog::addBackend( "STREAM" , stream_log);

    auto python = std::make_shared<Python>();

    auto deck = Parser{python}.parseFile("PYACTION.DATA");
    const std::string& logger_module = deck.makeDeckPath("logger.py");

    Action::PyAction pyaction(python , "ACT3", Action::PyAction::RunCount::unlimited, logger_module);

    const std::function<void(const std::string&, const std::vector<std::string>&)> actionx_callback;
    EclipseState state;
    Schedule schedule;
    SummaryState summary_state;

    pyaction.run(state, schedule, 0, summary_state, actionx_callback);
    std::string log_output = log_stream.str();

    BOOST_CHECK(log_output.find("Info from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Warning from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Error from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Problem from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Bug from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Debug from logger.py!") != std::string::npos);
    BOOST_CHECK(log_output.find("Note from logger.py!") != std::string::npos);
}

#endif // EMBEDDED_PYTHON
