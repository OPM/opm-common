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

#define BOOST_TEST_MODULE ParseMULTREGT
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
using namespace Opm;




BOOST_AUTO_TEST_CASE( parse_MULTREGT_OK ) {
    ParserPtr parser(new Parser());
    DeckPtr deck =  parser->parseFile("testdata/integration_tests/MULTREGT/MULTREGT");
    DeckKeywordConstPtr multregtKeyword = deck->getKeyword("MULTREGT" , 0);
    BOOST_CHECK_NO_THROW( MULTREGTScanner::assertKeywordSupported( multregtKeyword ) );
}



BOOST_AUTO_TEST_CASE( MULTREGT_ECLIPSE_STATE ) {
    ParserPtr parser(new Parser());
    DeckPtr deck =  parser->parseFile("testdata/integration_tests/MULTREGT/MULTREGT.DATA");
    EclipseState state(deck);
    auto transMult = state.getTransMult();

    BOOST_CHECK_EQUAL( 0.10 , transMult->getMultiplier( 0 , 0 , 0 , FaceDir::XPlus ));
    BOOST_CHECK_EQUAL( 0.10 , transMult->getMultiplier( 0 , 1 , 0 , FaceDir::XPlus ));
    BOOST_CHECK_EQUAL( 0.20 , transMult->getMultiplier( 1 , 0 , 0 , FaceDir::XMinus ));
    BOOST_CHECK_EQUAL( 0.20 , transMult->getMultiplier( 1 , 1 , 0 , FaceDir::XMinus ));
    BOOST_CHECK_EQUAL( 1.50 , transMult->getMultiplier( 0 , 0 , 0 , FaceDir::ZPlus ));
    BOOST_CHECK_EQUAL( 1.50 , transMult->getMultiplier( 0 , 1 , 0 , FaceDir::ZPlus ));
    BOOST_CHECK_EQUAL( 1.00 , transMult->getMultiplier( 1 , 0 , 0 , FaceDir::ZPlus ));
    BOOST_CHECK_EQUAL( 1.00 , transMult->getMultiplier( 1 , 1 , 0 , FaceDir::ZPlus ));
    BOOST_CHECK_EQUAL( 0.60 , transMult->getMultiplier( 1 , 0 , 1 , FaceDir::ZMinus ));
    BOOST_CHECK_EQUAL( 0.60 , transMult->getMultiplier( 1 , 1 , 1 , FaceDir::ZMinus ));
}
