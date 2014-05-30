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
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>


using namespace Opm;


BOOST_AUTO_TEST_CASE( PARSE_BOX_OK ) {
    ParserPtr parser(new Parser( ));
    boost::filesystem::path boxFile("testdata/integration_tests/BOX/BOXTEST1");
    DeckPtr deck =  parser->parseFile(boxFile.string() , false);
    EclipseState state(deck);

    std::shared_ptr<GridProperty<int> > satnum = state.getIntProperty("SATNUM");
    {
        size_t i,j,k;
        std::shared_ptr<const EclipseGrid> grid = state.getEclipseGrid();
        for (k = 0; k < grid->getNZ(); k++) {
            for (j = 0; j < grid->getNY(); j++) {
                for (i = 0; i < grid->getNX(); i++) {

                    size_t g = i + j*grid->getNX() + k * grid->getNX() * grid->getNY();
                    if (i <= 1 && j <= 1 && k <= 1)
                        BOOST_CHECK_EQUAL(satnum->iget(g) , 10);
                    else
                        BOOST_CHECK_EQUAL(satnum->iget(g) , 2);
                    
                }
            }
        }
    }
}
