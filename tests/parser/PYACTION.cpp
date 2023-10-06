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
#include <memory>

#define BOOST_TEST_MODULE PY_ACTION_TESTER
#include <boost/test/unit_test.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(ParsePYACTION) {
    Parser parser;
    auto python = std::make_shared<Python>();
    auto deck = parser.parseFile("PYACTION.DATA");

    auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::count_from_string(record0.getItem(1).get<std::string>(0));
    auto run_when = Action::PyAction::when_from_string(record0.getItem(2).get<std::string>(0));
    // TODO|VK Should really do more checking than just the path... - should also be in constructor? 
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction(python, "ACT1", run_count, run_when, ok_module);
    BOOST_CHECK_EQUAL(pyaction.name(), "ACT1");
    
    // Second PYACTION keword, with non-default item record0 item 3
    auto keyword2 = deck.get<ParserKeywords::PYACTION>()[1];
    const auto& record20 = keyword2.getRecord(0);
    const auto& record21 = keyword2.getRecord(1);
    auto run_count2 = Action::PyAction::count_from_string(record20.getItem(1).get<std::string>(0));
    auto run_when2 = Action::PyAction::when_from_string(record20.getItem(2).get<std::string>(0));
    const std::string& ok_module2 = deck.makeDeckPath(record21.getItem(0).get<std::string>(0));
    Action::PyAction pyaction2(python, "ACT1B", run_count2, run_when2, ok_module2);
    BOOST_CHECK_EQUAL(pyaction2.name(), "ACT1B");
    BOOST_CHECK_EQUAL(pyaction2.when(), "PRE_REPORT");
    
}


#ifdef EMBEDDED_PYTHON
BOOST_AUTO_TEST_CASE(ParsePYACTION_Module_Run_Missing) {
    Parser parser;
    auto python = std::make_shared<Python>();
    auto deck = parser.parseFile("PYACTION.DATA");
    auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::count_from_string(record0.getItem(1).get<std::string>(0));
    auto run_when = Action::PyAction::when_from_string(record0.getItem(2).get<std::string>(0));    
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction(python, "ACT1", run_count, run_when, ok_module);

    const std::string& broken_module = deck.makeDeckPath("action_missing_run.py");
    BOOST_CHECK_THROW(Action::PyAction(python , "ACT2", run_count, run_when, broken_module), std::runtime_error);

}

BOOST_AUTO_TEST_CASE(ParsePYACTION_Module_Syntax_Error) {
    Parser parser;
    auto python = std::make_shared<Python>();
    auto deck = parser.parseFile("PYACTION.DATA");
    auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::count_from_string(record0.getItem(1).get<std::string>(0));
    auto run_when = Action::PyAction::when_from_string(record0.getItem(2).get<std::string>(0));    
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction(python, "ACT1", run_count, run_when, ok_module);

    const std::string& broken_module2 = deck.makeDeckPath("action_syntax_error.py");
    BOOST_CHECK_THROW(Action::PyAction(python , "ACT2", run_count, run_when, broken_module2), std::exception);

}

BOOST_AUTO_TEST_CASE(ParsePYACTION_ModuleMissing) {
    Parser parser;
    auto python = std::make_shared<Python>();
    auto deck = parser.parseFile("PYACTION.DATA");
    auto keyword = deck.get<ParserKeywords::PYACTION>().front();
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::count_from_string(record0.getItem(1).get<std::string>(0));
    auto run_when = Action::PyAction::when_from_string(record0.getItem(2).get<std::string>(0));    
    const std::string& ok_module = deck.makeDeckPath(record1.getItem(0).get<std::string>(0));
    Action::PyAction pyaction(python, "ACT1", run_count, run_when, ok_module);

    const std::string& missing_module = deck.makeDeckPath("no_such_module.py");
    BOOST_CHECK_THROW(Action::PyAction(python , "ACT2", run_count, run_when, missing_module), std::invalid_argument);
}

#endif

