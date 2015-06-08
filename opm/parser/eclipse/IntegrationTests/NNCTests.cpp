/*
  Copyright 2015 IRIS
  
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

#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif
#define NVERBOSE // to suppress our messages when throwing

#define BOOST_TEST_MODULE NNCTests
#include <boost/test/unit_test.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(noNNC)
{
    const std::string filename = "testdata/integration_tests/NNC/noNNC.DATA";
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck(parser->parseFile(filename));
    Opm::EclipseStateConstPtr eclipseState(new EclipseState(deck));
    auto eclGrid = eclipseState->getEclipseGrid();
    Opm::NNC nnc(deck, eclGrid);
    BOOST_CHECK(!nnc.hasNNC());
}

BOOST_AUTO_TEST_CASE(readDeck)
{
    const std::string filename = "testdata/integration_tests/NNC/NNC.DATA";
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck(parser->parseFile(filename));
    Opm::EclipseStateConstPtr eclipseState(new EclipseState(deck));
    auto eclGrid = eclipseState->getEclipseGrid();
    Opm::NNC nnc(deck, eclGrid);
    BOOST_CHECK(nnc.hasNNC());
    const std::vector<int>& NNC1 = nnc.nnc1();
    const std::vector<int>& NNC2 = nnc.nnc2();
    const std::vector<double>& trans = nnc.trans();

    // test the NNCs in nnc.DATA
    BOOST_CHECK_EQUAL(nnc.numNNC(), 4);
    BOOST_CHECK_EQUAL(NNC1[0], 0);
    BOOST_CHECK_EQUAL(NNC2[0], 1);
    BOOST_CHECK_EQUAL(trans[0], 0.5);
    BOOST_CHECK_EQUAL(NNC1[1], 0);
    BOOST_CHECK_EQUAL(NNC2[1], 10);
    BOOST_CHECK_EQUAL(trans[1], 1.0);

}
BOOST_AUTO_TEST_CASE(addNNC)
{
    const std::string filename = "testdata/integration_tests/NNC/NNC.DATA";
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck(parser->parseFile(filename));
    Opm::EclipseStateConstPtr eclipseState(new EclipseState(deck));
    auto eclGrid = eclipseState->getEclipseGrid();
    Opm::NNC nnc(deck, eclGrid);
    const std::vector<int>& NNC1 = nnc.nnc1();
    const std::vector<int>& NNC2 = nnc.nnc2();
    const std::vector<double>& trans = nnc.trans();

    // test add NNC
    nnc.addNNC(2,2,2.0);
    BOOST_CHECK_EQUAL(nnc.numNNC(), 5);
    BOOST_CHECK_EQUAL(NNC1[4], 2);
    BOOST_CHECK_EQUAL(NNC2[4], 2);
    BOOST_CHECK_EQUAL(trans[4], 2.0);
}


