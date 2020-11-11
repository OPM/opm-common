/*
  Copyright 2020 Statoil ASA.

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


#define BOOST_TEST_MODULE PAvgTests

#include <exception>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvg.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(DEFAULT_PAVG) {
    PAvg pavg;
    BOOST_CHECK_EQUAL(pavg.inner_weight(), 0.50);
    BOOST_CHECK_EQUAL(pavg.conn_weight(), 1.00);
    BOOST_CHECK_EQUAL(pavg.use_porv(), false);
    BOOST_CHECK(pavg.depth_correction() == PAvg::DepthCorrection::WELL);
    BOOST_CHECK(pavg.open_connections());
}


void invalid_deck(const std::string& deck_string, const std::string& kw) {
    Parser parser;
    auto deck = parser.parseString(deck_string);
    BOOST_CHECK_THROW( PAvg(deck.getKeyword(kw).getRecord(0)), std::exception );
}

void valid_deck(const std::string& deck_string, const std::string& kw) {
    Parser parser;
    auto deck = parser.parseString(deck_string);
    BOOST_CHECK_NO_THROW( PAvg(deck.getKeyword(kw).getRecord(0)));
}


BOOST_AUTO_TEST_CASE(PAVG_FROM_DECK) {
    std::string invalid_deck1 = R"(
WPAVE
   2*  Well /

WWPAVE
   W 2*  Well /
/
)";

    std::string invalid_deck2 = R"(
WPAVE
   2*  WELL all /

WWPAVE
   W 2*  WELL all /
/
)";

    std::string valid_input = R"(
WPAVE
   0.25 0.50  WELL ALL /

WWPAVE
   W 2*  WELL ALL /
/
)";
    invalid_deck(invalid_deck1, "WPAVE");
    invalid_deck(invalid_deck1, "WWPAVE");

    invalid_deck(invalid_deck2, "WPAVE");
    invalid_deck(invalid_deck2, "WWPAVE");

    valid_deck(valid_input, "WPAVE");
    valid_deck(valid_input, "WWPAVE");

    Parser parser;
    PAvg pavg( parser.parseString(valid_input).getKeyword("WPAVE").getRecord(0) );
    BOOST_CHECK_EQUAL( pavg.inner_weight(), 0.25);
    BOOST_CHECK_EQUAL( pavg.conn_weight(), 0.5);
    BOOST_CHECK( pavg.use_porv() );
}
