/*
  Copyright 2018 Statoil ASA.

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

#define BOOST_TEST_MODULE UDQTests
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQExpression.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(KEYWORDS) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

)";
    Parser parser;

    auto deck = parser.parseString(input);
    auto runspec = Runspec(deck);
    auto udq_params = runspec.udqParams();

    BOOST_CHECK(udq_params.reseedRNG());
    BOOST_CHECK_EQUAL(0.25, udq_params.cmpEpsilon());
}


BOOST_AUTO_TEST_CASE(UDQ_KEWYORDS) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
  ASSIGN WUBHP 0.0 /
  UNITS  WUBHP 'BARSA' /
  DEFINE FUOPR  AVEG(WOPR) + 1/
/
)";

    Parser parser;

    auto deck = parser.parseString(input);
    auto udq_params = UDQParams(deck);
    auto udq = UDQ(deck);
    BOOST_CHECK_EQUAL(0.25, udq_params.cmpEpsilon());
    BOOST_CHECK_EQUAL(3, udq.expressions().size());
}

BOOST_AUTO_TEST_CASE(UDQ_KEYWORD) {
    // Invalid action
    BOOST_REQUIRE_THROW( UDQExpression("INVALID_ACTION", "WUBHP" , {"DATA1" ,"1"}), std::invalid_argument);

    // Invalid keyword
    BOOST_REQUIRE_THROW( UDQExpression("ASSIGN", "INVALID_KEYWORD", {}), std::invalid_argument);

    BOOST_CHECK_NO_THROW(UDQExpression("ASSIGN" ,"WUBHP", {"1"}));
}


BOOST_AUTO_TEST_CASE(UDQ_DATA) {
    const std::string input = R"(
RUNSPEC

UDQDIMS
   10* 'Y'/

UDQPARAM
  3* 0.25 /

SCHEDULE

UDQ
ASSIGN CUMW1 P12 10 12 1 (4.0 + 6*(4 - 2)) /
DEFINE WUMW1 WBHP 'P*1*' UMAX WBHP 'P*4*' /
/


)";
    Parser parser;

    auto deck = parser.parseString(input);
    auto udq_params = UDQParams(deck);
    auto udq = UDQ(deck);
    const auto& records = udq.expressions();
    const auto& rec0 = records[0];
    const auto& rec1 = records[1];
    const std::vector<std::string> exp0 = {"P12", "10", "12", "1", "(", "4.0", "+", "6", "*", "(", "4", "-", "2", ")", ")"};
    const std::vector<std::string> exp1 = {"WBHP", "P*1*", "UMAX", "WBHP" , "P*4*"};
    BOOST_CHECK_EQUAL_COLLECTIONS(rec0.tokens().begin(), rec0.tokens().end(), exp0.begin(), exp0.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(rec1.tokens().begin(), rec1.tokens().end(), exp1.begin(), exp1.end());
    BOOST_CHECK_EQUAL(0.25, udq_params.cmpEpsilon());
}


