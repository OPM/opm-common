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

#define BOOST_TEST_MODULE AquifetpTests

#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifetp.hpp>

using namespace Opm;

inline Deck createAquifetpDeck() {
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
        "AQUFETP\n"
        "   1	70000.0	4.0e3	2.0e9	1.0e-5	500	1	0	0 /\n"
        "/ \n";

    Parser parser;
    return parser.parseString(deckData, ParseContext());
}

inline std::vector<Aquifetp::AQUFETP_data> init_aquifetp(){
    auto deck = createAquifetpDeck();
    Aquifetp aqufetp(deck);
    std::vector<Aquifetp::AQUFETP_data> aquifetp = aqufetp.getAquifers();

    return aquifetp;
}

BOOST_AUTO_TEST_CASE(AquifetpTest){
    std::vector< Aquifetp::AQUFETP_data > aquifetp = init_aquifetp();
    for (const auto& it : aquifetp){
        BOOST_CHECK_EQUAL(it.aquiferID , 1);
        BOOST_CHECK_EQUAL(it.V0 , 2.0E9);
        BOOST_CHECK_EQUAL(it.J, 500/86400e5);
        
    }    
}    
