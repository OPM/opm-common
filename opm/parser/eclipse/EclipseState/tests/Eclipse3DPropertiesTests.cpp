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

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Units/Units.hpp>

static Opm::Deck createDeck() {
    const char *deckData = "RUNSPEC\n"
            "\n"
            "DIMENS\n"
            " 10 10 10 /\n"
            "GRID\n"
            "DX\n"
            "1000*0.25 /\n"
            "DY\n"
            "1000*0.25 /\n"
            "DZ\n"
            "1000*0.25 /\n"
            "TOPS\n"
            "100*0.25 /\n"
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
            "swat\n"
            "1000*1 /\n"
            "SATNUM\n"
            "1000*2 /\n"
            "ROCKNUM\n"
            "200*1 200*2 200*3 400*4 /\n"
            "\n";

    Opm::Parser parser;
    return parser.parseString(deckData, Opm::ParseContext() );
}



static Opm::Deck createValidIntDeck() {
    const char *deckData = "RUNSPEC\n"
            "GRIDOPTS\n"
            "  'YES'  2 /\n"
            "\n"
            "DIMENS\n"
            " 5 5 1 /\n"
            "GRID\n"
            "DX\n"
            "25*0.25 /\n"
            "DY\n"
            "25*0.25 /\n"
            "DZ\n"
            "25*0.25 /\n"
            "TOPS\n"
            "25*0.25 /\n"
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
            "  satnum 11 1    M / \n"
            "  SATNUM 20 2      / \n"
            "/\n"
            "EDIT\n"
            "\n";

    Opm::Parser parser;
    return parser.parseString(deckData, Opm::ParseContext() );
}

static Opm::Deck createValidPERMXDeck() {
    const char *deckData = "RUNSPEC\n"
            "GRIDOPTS\n"
            "  'YES'  2 /\n"
            "\n"
            "DIMENS\n"
            " 5 5 1 /\n"
            "GRID\n"
            "DX\n"
            "25*0.25 /\n"
            "DY\n"
            "25*0.25 /\n"
            "DZ\n"
            "25*0.25 /\n"
            "TOPS\n"
            "25*0.25 /\n"
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
            "'PermX   '   1 1     / \n"
            "PErmX   3 2     / \n"
            "/\n"
            "EDIT\n"
            "\n";

    Opm::Parser parser;
    return parser.parseString(deckData, Opm::ParseContext() );
}


/// Setup fixture
struct Setup
{
    Opm::ParseContext parseContext;
    Opm::Deck deck;
    Opm::TableManager tablemanager;
    Opm::EclipseGrid grid;
    Opm::Eclipse3DProperties props;

    explicit Setup(Opm::Deck&& deckArg) :
            deck(std::move( deckArg ) ),
            tablemanager(deck),
            grid(deck),
            props(deck, tablemanager, grid)
    {
    }
};


BOOST_AUTO_TEST_CASE(HasDeckProperty) {
    Setup s(createDeck());
    BOOST_CHECK(s.props.hasDeckIntGridProperty("SATNUM"));
}

BOOST_AUTO_TEST_CASE(SupportsProperty) {
    Setup s(createDeck());
    std::vector<std::string> keywordList = {
        // int props
        "ACTNUM", "SATNUM", "IMBNUM", "PVTNUM", "EQLNUM", "ENDNUM", "FLUXNUM", "MULTNUM", "FIPNUM", "MISCNUM", "OPERNUM", "ROCKNUM",
        // double props
        "TEMPI", "MULTPV", "PERMX", "permy", "PERMZ", "SWATINIT", "THCONR", "NTG"
    };

    for (auto keyword : keywordList)
        BOOST_CHECK(s.props.supportsGridProperty(keyword));

}

BOOST_AUTO_TEST_CASE(DefaultRegionFluxnum) {
    Setup s(createDeck());
    BOOST_CHECK_EQUAL(s.props.getDefaultRegionKeyword(), "FLUXNUM");
}

BOOST_AUTO_TEST_CASE(UnsupportedKeywordsThrows) {
    Setup s(createDeck());
    BOOST_CHECK_THROW(s.props.hasDeckIntGridProperty("NONO"), std::logic_error);
    BOOST_CHECK_THROW(s.props.hasDeckDoubleGridProperty("NONO"), std::logic_error);

    BOOST_CHECK_THROW(s.props.getIntGridProperty("NONO"), std::logic_error);
    BOOST_CHECK_THROW(s.props.getDoubleGridProperty("NONO"), std::logic_error);

    BOOST_CHECK_NO_THROW(s.props.hasDeckIntGridProperty("FluxNUM"));
    BOOST_CHECK_NO_THROW(s.props.supportsGridProperty("NONO"));
}

BOOST_AUTO_TEST_CASE(IntGridProperty) {
    Setup s(createDeck());
    int cnt = 0;
    for (auto x : s.props.getIntGridProperty("SaTNuM").getData()) {
        BOOST_CHECK_EQUAL(x, 2);
        cnt++;
    }
    BOOST_CHECK_EQUAL(cnt, 1000);
}

BOOST_AUTO_TEST_CASE(AddregIntSetCorrectly) {
    Setup s(createValidIntDeck());
    const auto& property = s.props.getIntGridProperty("SATNUM");
    for (size_t j = 0; j < 5; j++)
        for (size_t i = 0; i < 5; i++) {
            if (i < 2)
                BOOST_CHECK_EQUAL(12, property.iget(i, j, 0));
            else
                BOOST_CHECK_EQUAL(21, property.iget(i, j, 0));
        }

}

BOOST_AUTO_TEST_CASE(RocknumTest) {
    Setup s(createDeck());
    const auto& rocknum = s.props.getIntGridProperty("ROCKNUM");
    for (size_t i = 0; i < 10; i++) {
        for (size_t j = 0; j < 10; j++) {
            for (size_t k = 0; k < 10; k++) {
                if (k < 2)
                    BOOST_CHECK_EQUAL(1, rocknum.iget(i, j, k));
                else if (k < 4)
                    BOOST_CHECK_EQUAL(2, rocknum.iget(i, j, k));
                else if (k < 6)
                    BOOST_CHECK_EQUAL(3, rocknum.iget(i, j, k));
                else
                    BOOST_CHECK_EQUAL(4, rocknum.iget(i, j, k));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PermxUnitAppliedCorrectly) {
    Setup s( createValidPERMXDeck() );
    const auto& permx = s.props.getDoubleGridProperty("PermX");

    for (size_t j = 0; j < 5; j++)
        for (size_t i = 0; i < 5; i++) {
            if (i < 2)
                BOOST_CHECK_CLOSE(2 * Opm::Metric::Permeability, permx.iget(i, j, 0), 0.0001);
            else
                BOOST_CHECK_CLOSE(4 * Opm::Metric::Permeability, permx.iget(i, j, 0), 0.0001);
        }
}

BOOST_AUTO_TEST_CASE(DoubleIterator) {
    Setup s( createValidPERMXDeck() );
    const auto& doubleProperties = s.props.getDoubleProperties();
    std::vector<std::string> kw_list;
    for (const auto& prop : doubleProperties )
        kw_list.push_back( prop.getKeywordName() );

    BOOST_CHECK_EQUAL( 2 , kw_list.size() );
    BOOST_CHECK( std::find( kw_list.begin() , kw_list.end() , "PERMX") != kw_list.end());
    BOOST_CHECK( std::find( kw_list.begin() , kw_list.end() , "PERMZ") != kw_list.end());
}


BOOST_AUTO_TEST_CASE(IntIterator) {
    Setup s( createValidPERMXDeck() );
    const auto& intProperties = s.props.getIntProperties();
    std::vector<std::string> kw_list;
    for (const auto& prop : intProperties )
        kw_list.push_back( prop.getKeywordName() );

    BOOST_CHECK_EQUAL( 1 , kw_list.size() );
    BOOST_CHECK_EQUAL( kw_list[0] , "MULTNUM" );
}


BOOST_AUTO_TEST_CASE(getRegions) {
    const char* input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "RUNSPEC\n"
            "\n"
            "DIMENS\n"
            " 2 2 1 /\n"
            "GRID\n"
            "DX\n"
            "4*0.25 /\n"
            "DY\n"
            "4*0.25 /\n"
            "DZ\n"
            "4*0.25 /\n"
            "TOPS\n"
            "4*0.25 /\n"
            "REGIONS\n"
            "OPERNUM\n"
            "3 3 1 3 /\n"
            "FIPPGDX\n"
            "2 1 1 2 /\n"
            "FIPREG\n"
            "3 2 3 2 /\n"
            "FIPNUM\n"
            "1 1 2 3 /\n";

    Setup s( Opm::Parser().parseString(input, Opm::ParseContext() ) );

    std::vector< int > ref = { 1, 2, 3 };
    const auto& regions = s.props.getRegions( "FIPNUM" );

    BOOST_CHECK_EQUAL_COLLECTIONS( ref.begin(), ref.end(),
                                   regions.begin(), regions.end() );

    BOOST_CHECK( s.props.getRegions( "EQLNUM" ).empty() );
    BOOST_CHECK( ! s.props.getRegions( "FIPPGDX" ).empty() );

    const auto& fipreg = s.props.getRegions( "FIPREG" );
    BOOST_CHECK_EQUAL( 2, fipreg.at(0) );
    BOOST_CHECK_EQUAL( 3, fipreg.at(1) );

    const auto& opernum = s.props.getRegions( "OPERNUM" );
    BOOST_CHECK_EQUAL( 1, opernum.at(0) );
    BOOST_CHECK_EQUAL( 3, opernum.at(1) );
}
