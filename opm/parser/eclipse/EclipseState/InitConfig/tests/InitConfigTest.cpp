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



#define BOOST_TEST_MODULE InitConfigTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>


using namespace Opm;

const std::string& deckStr =
                            "RUNSPEC\n"
                            "DIMENS\n"
                            " 10 10 10 /\n"
                            "SOLUTION\n"
                            "RESTART\n"
                            "BASE 5\n"
                            "/\n"
                            "GRID\n"
                            "START             -- 0 \n"
                            "19 JUN 2007 / \n"
                            "SCHEDULE\n"
                            "SKIPREST \n"
                            "/\n";

const std::string& deckStr2 =
                            "RUNSPEC\n"
                            "DIMENS\n"
                            " 10 10 10 /\n"
                            "SOLUTION\n"
                            "/\n"
                            "GRID\n"
                            "START             -- 0 \n"
                            "19 JUN 2007 / \n"
                            "SCHEDULE\n"
                            "/\n";

const std::string& deckStr3 =
                            "RUNSPEC\n"
                            "DIMENS\n"
                            " 10 10 10 /\n"
                            "SOLUTION\n"
                            "RESTART\n"
                            "BASE 5 SAVE UNFORMATTED\n"
                            "/\n"
                            "GRID\n"
                            "START             -- 0 \n"
                            "19 JUN 2007 / \n"
                            "SCHEDULE\n"
                            "SKIPREST \n"
                            "/\n";

const std::string& deckStr4 =
                            "RUNSPEC\n"
                            "DIMENS\n"
                            " 10 10 10 /\n"
                            "SOLUTION\n"
                            "RESTART\n"
                            "BASE 5\n"
                            "/\n"
                            "GRID\n"
                            "START             -- 0 \n"
                            "19 JUN 2007 / \n"
                            "SCHEDULE\n"
                            "/\n";

static DeckPtr createDeck(const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input);
}

BOOST_AUTO_TEST_CASE(InitConfigTest) {

    DeckPtr deck = createDeck(deckStr);
    InitConfigPtr initConfigPtr;
    BOOST_CHECK_NO_THROW(initConfigPtr = std::make_shared<InitConfig>(deck));
    BOOST_CHECK_EQUAL(initConfigPtr->getRestartInitiated(), true);
    BOOST_CHECK_EQUAL(initConfigPtr->getRestartStep(), 5);
    BOOST_CHECK_EQUAL(initConfigPtr->getRestartRootName(), "BASE");

    DeckPtr deck2 = createDeck(deckStr2);
    InitConfigPtr initConfigPtr2;
    BOOST_CHECK_NO_THROW(initConfigPtr2 = std::make_shared<InitConfig>(deck2));
    BOOST_CHECK_EQUAL(initConfigPtr2->getRestartInitiated(), false);
    BOOST_CHECK_EQUAL(initConfigPtr2->getRestartStep(), 0);
    BOOST_CHECK_EQUAL(initConfigPtr2->getRestartRootName(), "");

    DeckPtr deck3 = createDeck(deckStr3);
    BOOST_CHECK_THROW(std::make_shared<InitConfig>(deck3), std::runtime_error);

    DeckPtr deck4 = createDeck(deckStr4);
    BOOST_CHECK_THROW(std::make_shared<InitConfig>(deck4), std::runtime_error);
}
