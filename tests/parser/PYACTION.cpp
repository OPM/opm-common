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
#include <opm/parser/eclipse/EclipseState/Schedule/Action/PyAction.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(ParsePYACTION) {
    const std::string input_code = R"(from math import sin
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

    const std::string deck_string1 = R"(
SCHEDULE

PYACTION Her comes an ignored comment
)" + input_code + "<<<";

    const std::string deck_string2 = R"(
SCHEDULE

PYACTION
)" + input_code;


    const std::string deck_string3 = R"(
SCHEDULE

PYACTION -- Comment
)" + input_code + "<<<" + "\nGRID";


    Parser parser;
    {
        auto deck = parser.parseString(deck_string1);
        const auto& parsed_code = deck.getKeyword("PYACTION").getRecord(0).getItem("code").get<std::string>(0);
        BOOST_CHECK_EQUAL(parsed_code, input_code);
    }
    {
        auto deck = parser.parseString(deck_string2);
        const auto& parsed_code = deck.getKeyword("PYACTION").getRecord(0).getItem("code").get<std::string>(0);
        BOOST_CHECK_EQUAL(parsed_code, input_code);
    }
    {
        auto deck = parser.parseString(deck_string3);
        const auto& parsed_code = deck.getKeyword("PYACTION").getRecord(0).getItem("code").get<std::string>(0);
        BOOST_CHECK_EQUAL(parsed_code, input_code);
        BOOST_CHECK( deck.hasKeyword("GRID"));
    }

    PyAction pyact(input_code);
}
