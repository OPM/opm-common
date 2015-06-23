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

#define BOOST_TEST_MODULE ParsePLYSHLOG
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

using namespace Opm;



BOOST_AUTO_TEST_CASE( PARSE_PLYSHLOG_OK) {
    ParserPtr parser(new Parser());
    boost::filesystem::path deckFile("testdata/integration_tests/POLYMER/plyshlog.data");
    DeckPtr deck =  parser->parseFile(deckFile.string());
    DeckKeywordConstPtr kw = deck->getKeyword("PLYSHLOG");
    DeckRecordConstPtr rec1 = kw->getRecord(0); // reference conditions

    const auto itemRefPolyConc = rec1->getItem("REF_POLYMER_CONCENTRATION");
    const auto itemRefSali = rec1->getItem("REF_SALINITY");
    const auto itemRefTemp = rec1->getItem("REF_TEMPERATURE");

    BOOST_CHECK_EQUAL( true, itemRefPolyConc->hasValue(0) );
    BOOST_CHECK_EQUAL( true, itemRefSali->hasValue(0) );
    BOOST_CHECK_EQUAL( false, itemRefTemp->hasValue(0) );

    BOOST_CHECK_EQUAL( 1.0, itemRefPolyConc->getRawDouble(0) );
    BOOST_CHECK_EQUAL( 3.0, itemRefSali->getRawDouble(0) );

    DeckRecordConstPtr rec2 = kw->getRecord(1);
    DeckItemPtr itemData = rec2->getItem(0);

    BOOST_CHECK_EQUAL( 1.e-7 , itemData->getRawDouble(0) );
    BOOST_CHECK_EQUAL( 1.0 , itemData->getRawDouble(1) );
    BOOST_CHECK_EQUAL( 1.0 , itemData->getRawDouble(2) );
    BOOST_CHECK_EQUAL( 1.2 , itemData->getRawDouble(3) );
    BOOST_CHECK_EQUAL( 1.e3 , itemData->getRawDouble(4) );
    BOOST_CHECK_EQUAL( 2.4 , itemData->getRawDouble(5) );
}
