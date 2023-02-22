/*
  Copyright 2023 Equinor ASA.

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

#define BOOST_TEST_MODULE CarfinTest

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>

#include <filesystem>

using namespace Opm;

namespace {

std::string prefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

Deck makeDeck(const std::string& fileName) {
    Parser parser;
    std::filesystem::path boxFile(fileName);
    return parser.parseFile(boxFile.string());
}

EclipseState makeState(const std::string& fileName) {
    return EclipseState( makeDeck(fileName) );
}

}

BOOST_AUTO_TEST_CASE( PARSE_CARFIN_OK ) {
    EclipseState state = makeState( prefix() + "CARFIN/CARFINTEST1" );
    {
        size_t i, j, k;
        const EclipseGrid& grid = state.getInputGrid();
        for (k = 0; k < grid.getNZ(); k++) {
            for (j = 0; j < grid.getNY(); j++) {
                for (i = 0; i < grid.getNX(); i++) {

                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( CONSTRUCTOR_AND_UPDATE ) {
    auto deck = makeDeck( prefix() + "CARFIN/CARFINTEST1" );
    EclipseGrid grid(deck);
    const auto& carfin_keyword = deck["CARFIN"][0];
    Carfin lgr(grid,
            [&grid](const std::size_t global_index)
            {
                return grid.cellActive(global_index);
            },
            [&grid](const std::size_t global_index)
            {
                return grid.activeIndex(global_index);
            });
    lgr.update(carfin_keyword.getRecord(0));
    BOOST_CHECK_EQUAL(lgr.size(), 324);

    lgr.reset();
    BOOST_CHECK_EQUAL(lgr.size(), 1000);
}
