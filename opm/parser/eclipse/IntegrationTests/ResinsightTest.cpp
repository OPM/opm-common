/*
  Copyright 2013 Statoil ASA.

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

#define BOOST_TEST_MODULE BoxTest
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE( test_parse ) {
    Parser parser(false);
    ParseMode parseMode;
    parseMode.unknownKeyword = InputError::IGNORE;
    parseMode.randomText = InputError::IGNORE;
    parseMode.randomSlash = InputError::IGNORE;

    parser.addKeyword<ParserKeywords::SPECGRID>();
    parser.addKeyword<ParserKeywords::FAULTS>();

    auto deck = parser.parseFile("testdata/integration_tests/Resinsight/DECK1.DATA" , parseMode);

    BOOST_CHECK( deck->hasKeyword<ParserKeywords::SPECGRID>() );
    BOOST_CHECK( deck->hasKeyword<ParserKeywords::FAULTS>() );
}


BOOST_AUTO_TEST_CASE( test_state ) {
    Parser parser(false);
    ParseMode parseMode;
    parseMode.unknownKeyword = InputError::IGNORE;
    parseMode.randomText = InputError::IGNORE;
    parseMode.randomSlash = InputError::IGNORE;

    parser.addKeyword<ParserKeywords::SPECGRID>();
    parser.addKeyword<ParserKeywords::FAULTS>();
    auto deck = parser.parseFile("testdata/integration_tests/Resinsight/DECK1.DATA" , parseMode);
    auto grid = std::make_shared<EclipseGrid>( deck );
    auto faults = std::make_shared<FaultCollection>( deck , grid );
}
