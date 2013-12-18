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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Units/ConversionFactors.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(ParseDENSITY) {
    ParserPtr parser(new Parser());
    boost::filesystem::path file("testdata/integration_tests/DENSITY/DENSITY1");
    DeckPtr deck = parser->parse(file.string());
    DeckKeywordPtr densityKw = deck->getKeyword("DENSITY" , 0);

    
    BOOST_CHECK_EQUAL( 2U , densityKw->size());
    DeckRecordConstPtr rec1 = densityKw->getRecord(0);
    DeckRecordConstPtr rec2 = densityKw->getRecord(1);
    
    {
        DeckItemPtr oilDensity = rec1->getItem("OIL");
        DeckItemPtr waterDensity = rec1->getItem("WATER");
        DeckItemPtr gasDensity = rec1->getItem("GAS");
        
        BOOST_CHECK_CLOSE(  500 * Field::Density , oilDensity->getSIDouble(0) , 0.001);
        BOOST_CHECK_CLOSE( 1000 * Field::Density , waterDensity->getSIDouble(0) , 0.001);
        BOOST_CHECK_CLOSE(    1 * Field::Density , gasDensity->getSIDouble(0) , 0.001);
    }
    
}
