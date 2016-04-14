/*
 Copyright 2016 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE Eclipse3DPropertiesTests

#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Units/ConversionFactors.hpp>

static Opm::DeckPtr createDeck() {
    const char *deckData = "RUNSPEC\n"
            "\n"
            "DIMENS\n"
            " 10 10 10 /\n"
            "GRID\n"
            "FAULTS \n"
            "  'F1'  1  1  1  4   1  4  'X' / \n"
            "  'F2'  5  5  1  4   1  4  'X-' / \n"
            "/\n"
            "MULTFLT \n"
            "  'F1' 0.50 / \n"
            "  'F2' 0.50 / \n"
            "/\n"
            "EDIT\n"
            "MULTFLT /\n"
            "  'F2' 0.25 / \n"
            "/\n"
            "OIL\n"
            "\n"
            "GAS\n"
            "\n"
            "PROPS\n"
            "REGIONS\n"
            "SWAT\n"
            "1000*1 /\n"
            "SATNUM\n"
            "1000*2 /\n"
            "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData, Opm::ParseContext());
}

static Opm::DeckPtr createValidIntDeck() {
    const char *deckData = "RUNSPEC\n"
            "GRIDOPTS\n"
            "  'YES'  2 /\n"
            "\n"
            "DIMENS\n"
            " 5 5 1 /\n"
            "GRID\n"
            "MULTNUM \n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "/\n"
            "SATNUM\n"
            " 25*1 \n"
            "/\n"
            "ADDREG\n"
            "  SATNUM 11 1    M / \n"
            "  SATNUM 20 2      / \n"
            "/\n"
            "EDIT\n"
            "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData, Opm::ParseContext());
}

static Opm::DeckPtr createValidPERMXDeck() {
    const char *deckData = "RUNSPEC\n"
            "GRIDOPTS\n"
            "  'YES'  2 /\n"
            "\n"
            "DIMENS\n"
            " 5 5 1 /\n"
            "GRID\n"
            "MULTNUM \n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "1  1  2  2 2\n"
            "/\n"
            "BOX\n"
            "  1 2  1 5 1 1 / \n"
            "PERMZ\n"
            "  10*1 /\n"
            "ENDBOX\n"
            "BOX\n"
            "  3 5  1 5 1 1 / \n"
            "PERMZ\n"
            "  15*2 /\n"
            "ENDBOX\n"
            "PERMX\n"
            "25*1 /\n"
            "ADDREG\n"
            "  PERMX 1 1     / \n"
            "  PERMX 3 2     / \n"
            "/\n"
            "EDIT\n"
            "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData, Opm::ParseContext());
}

static Opm::Eclipse3DProperties getProps(Opm::DeckPtr deck) {
    return Opm::Eclipse3DProperties(*deck, *new Opm::TableManager(*deck), *new Opm::EclipseGrid(deck));
}

BOOST_AUTO_TEST_CASE(HasDeckProperty) {
    auto ep = getProps(createDeck());
    BOOST_CHECK(ep.hasDeckIntGridProperty("SATNUM"));
}

BOOST_AUTO_TEST_CASE(SupportsProperty) {
    auto ep = getProps(createDeck());
    std::vector<std::string> keywordList = {
        // int props
        "SATNUM", "IMBNUM", "PVTNUM", "EQLNUM", "ENDNUM", "FLUXNUM", "MULTNUM", "FIPNUM", "MISCNUM", "OPERNUM",
        // double props
        "TEMPI", "MULTPV", "PERMX", "PERMY", "PERMZ", "SWATINIT", "THCONR", "NTG"
    };

    for (auto keyword : keywordList)
        BOOST_CHECK(ep.supportsGridProperty(keyword));

}

BOOST_AUTO_TEST_CASE(DefaultRegionFluxnum) {
    auto ep = getProps(createDeck());
    BOOST_CHECK_EQUAL(ep.getDefaultRegionKeyword(), "FLUXNUM");
}

BOOST_AUTO_TEST_CASE(UnsupportedKeywordsThrows) {
    auto ep = getProps(createDeck());
    BOOST_CHECK_THROW(ep.hasDeckIntGridProperty("NONO"), std::logic_error);
    BOOST_CHECK_THROW(ep.hasDeckDoubleGridProperty("NONO"), std::logic_error);

    BOOST_CHECK_THROW(ep.getIntGridProperty("NONO"), std::logic_error);
    BOOST_CHECK_THROW(ep.getDoubleGridProperty("NONO"), std::logic_error);

    BOOST_CHECK_NO_THROW(ep.hasDeckIntGridProperty("FLUXNUM"));
    BOOST_CHECK_NO_THROW(ep.supportsGridProperty("NONO"));
}

BOOST_AUTO_TEST_CASE(IntGridProperty) {
    auto ep = getProps(createDeck());
    int cnt = 0;
    for (auto x : ep.getIntGridProperty("SATNUM").getData()) {
        BOOST_CHECK_EQUAL(x, 2);
        cnt++;
    }
    BOOST_CHECK_EQUAL(cnt, 1000);
}

BOOST_AUTO_TEST_CASE(AddregIntSetCorrectly) {
    Opm::DeckPtr deck = createValidIntDeck();
    const auto& ep = getProps(deck);

    const auto& property = ep.getIntGridProperty("SATNUM");
    for (size_t j = 0; j < 5; j++)
        for (size_t i = 0; i < 5; i++) {
            if (i < 2)
                BOOST_CHECK_EQUAL(12, property.iget(i, j, 0));
            else
                BOOST_CHECK_EQUAL(21, property.iget(i, j, 0));
        }

}

BOOST_AUTO_TEST_CASE(PermxUnitAppliedCorrectly) {
    Opm::DeckPtr deck = createValidPERMXDeck();
    const auto& props = getProps(deck);
    const auto& permx = props.getDoubleGridProperty("PERMX");

    for (size_t j = 0; j < 5; j++)
        for (size_t i = 0; i < 5; i++) {
            if (i < 2)
                BOOST_CHECK_CLOSE(2 * Opm::Metric::Permeability, permx.iget(i, j, 0), 0.0001);
            else
                BOOST_CHECK_CLOSE(4 * Opm::Metric::Permeability, permx.iget(i, j, 0), 0.0001);
        }
}
