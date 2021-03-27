/*
  Copyright 2021 Equinor ASA.

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

#define BOOST_TEST_MODULE ImportTests
#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/ImportContainer.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <tests/WorkArea.cpp>
#include <iostream>
#include <fstream>

using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateImportContainer) {
    WorkArea work;
    auto unit_system = UnitSystem::newMETRIC();
    Parser parser;
    BOOST_CHECK_THROW(ImportContainer(parser, unit_system, "/no/such/file", false, 0), std::exception);
    {
        EclIO::EclOutput output {"FILE_NAME", false};
    }

    ImportContainer container1(parser, unit_system, "FILE_NAME", false, 0);
    Deck deck;

    for (auto kw : container1)
        deck.addKeyword(std::move(kw));

    BOOST_CHECK_EQUAL(deck.size(), 0);



    {
        EclIO::EclOutput output {"FILE_NAME", false};
        output.write<double>("PORO", {0, 1, 2, 3, 4});
        output.write<float>("PERMX", {10, 20, 30, 40});
        output.write<int>("FIPNUM", {100, 200, 300, 400});
        output.write<int>("UNKNOWN", {100, 200, 300, 400});
    }

    ImportContainer container2(parser, unit_system, "FILE_NAME", false, 0);
    for (auto kw : container2)
        deck.addKeyword(std::move(kw));

    BOOST_CHECK_EQUAL(deck.size(), 3);
}
