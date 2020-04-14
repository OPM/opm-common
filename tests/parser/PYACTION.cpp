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
#include <iostream>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/PyAction.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(ParsePYACTION) {
    Parser parser;
    auto deck = parser.parseFile("PYACTION.DATA");

    auto keyword = deck.getKeyword<ParserKeywords::PYACTION>(0);
    const auto& record0 = keyword.getRecord(0);
    const auto& record1 = keyword.getRecord(1);

    auto run_count = Action::PyAction::from_string(record0.getItem(1).get<std::string>(0));
    std::string code = Action::PyAction::load(deck.getInputPath(), record1.getItem(0).get<std::string>(0));

    std::string literal_code =R"(from math import sin
import random
print("sin(0) = {}".format(sin(0)))
#---
if random.random() > 0.25:
    print("Large outcome")
else:
    print("Small result")
A = 100
B = A / 10
C = B * 20
)";

    Action::PyAction pyaction("ACT1", run_count, code);
    BOOST_CHECK_EQUAL(pyaction.name(), "ACT1");
    BOOST_CHECK_EQUAL(pyaction.code(), literal_code);
    BOOST_CHECK(pyaction.run_count() == Action::PyAction::RunCount::single);
}
