/*
Copyright 2017 TNO.

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

#define BOOST_TEST_MODULE AquanconTest

#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Aquancon.hpp>

using namespace Opm;

inline Deck createAQUANCONDeck() {
    const char *deckData =
        "DIMENS\n"
        "3 3 3 /\n"
        "\n"
        "GRID\n"
        "\n"
        "ACTNUM\n"
        " 0 8*1 0 8*1 0 8*1 /\n"
        "DXV\n"
        "1 1 1 /\n"
        "\n"
        "DYV\n"
        "1 1 1 /\n"
        "\n"
        "DZV\n"
        "1 1 1 /\n"
        "\n"
        "TOPS\n"
        "9*100 /\n"
        "\n"
        "SOLUTION\n"
        "\n"
        "AQUANCON\n"
        "   1      1  1  1    1   1  1  J-  1.0 1.0 NO /\n"
        "/ \n";

    Parser parser;
    return parser.parseString(deckData, ParseContext());
}

inline std::vector<Aquancon::AquanconOutput> init_aquancon(){
    auto deck = createAQUANCONDeck();
    EclipseState eclState( deck );
    Aquancon aqucon( eclState.getInputGrid(), deck);
    std::vector<Aquancon::AquanconOutput> aquifers = aqucon.getAquOutput();

    return aquifers;
}

BOOST_AUTO_TEST_CASE(AquanconTest){
    std::vector< Aquancon::AquanconOutput > aquifers = init_aquancon();
    for (const auto& it : aquifers){
        for (int i = 0; i < it.global_index.size(); ++i){
            BOOST_CHECK_EQUAL(it.aquiferID , 1);
            BOOST_CHECK_EQUAL(it.global_index.at(i) , 0);
            BOOST_CHECK_EQUAL(it.reservoir_face_dir.at(i) , 8);
        }
    }    
}    