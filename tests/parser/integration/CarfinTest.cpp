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

}

BOOST_AUTO_TEST_CASE( CONSTRUCTOR_AND_UPDATE ) {
    auto deck = makeDeck( prefix() + "CARFIN/CARFINTEST1" );
    EclipseGrid grid(deck);
    const auto& carfin_keyword1 = deck["CARFIN"][0];
    const auto& carfin_keyword2 = deck["CARFIN"][1];
    Carfin lgr(grid,
            [&grid](const std::size_t global_index)
            {
                return grid.cellActive(global_index);
            },
            [&grid](const std::size_t global_index)
            {
                return grid.activeIndex(global_index);
            });
    lgr.update(carfin_keyword1.getRecord(0));
    BOOST_CHECK_EQUAL(lgr.size(), 324);

    lgr.reset();
    BOOST_CHECK_EQUAL(lgr.size(), 1000);

    lgr.update(carfin_keyword2.getRecord(0));
    BOOST_CHECK_EQUAL(lgr.size(), 576);
    BOOST_CHECK_EQUAL(lgr.NAME(), "LGR2");

}
