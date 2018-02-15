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

#define BOOST_TEST_MODULE AquiferCTTest

#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/AquiferCT.hpp>

using namespace Opm;

inline Deck createAquiferCTDeck() {
    const char *deckData =
        "DIMENS\n"
        "3 3 3 /\n"
        "\n"
        "AQUDIMS\n"
        "1* 1* 2 100 1 1000 /\n"
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
        "PROPS\n"
        "AQUTAB\n"
        " 0.01 0.112 \n" 
        " 0.05 0.229 /\n" 
        "SOLUTION\n"
        "\n"
        "AQUCT\n"
        "   1 2000.0 1000 100 .3 3.0e-5 330 10 360.0 1 2 /\n"
        "/ \n";

    Parser parser;
    return parser.parseString(deckData, ParseContext());
}

inline std::vector<AquiferCT::AQUCT_data> init_aquiferct(){
    auto deck = createAquiferCTDeck();
    EclipseState eclState( deck );
    AquiferCT aquct( eclState, deck);
    std::vector<AquiferCT::AQUCT_data> aquiferct = aquct.getAquifers();

    return aquiferct;
}

BOOST_AUTO_TEST_CASE(AquiferCTTest){
    std::vector< AquiferCT::AQUCT_data > aquiferct = init_aquiferct();
    for (const auto& it : aquiferct){
        BOOST_CHECK_EQUAL(it.aquiferID , 1);
        BOOST_CHECK_EQUAL(it.phi_aq , 0.3);
        BOOST_CHECK_EQUAL(it.inftableID , 2);
        
    }    
}    