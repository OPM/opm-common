/*
  Copyright 2014 Statoil ASA.

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
#include <memory>

#define BOOST_TEST_MODULE EclipseGridTests

#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>


#include <ert/ecl/EclKW.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>

static const Opm::DeckKeyword createSATNUMKeyword( ) {
    const char *deckData =
    "SATNUM \n"
    "  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData, Opm::ParseMode());
    return deck->getKeyword("SATNUM");
}

static const Opm::DeckKeyword createTABDIMSKeyword( ) {
    const char *deckData =
    "TABDIMS\n"
    "  0 1 2 3 4 5 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData, Opm::ParseMode());
    return deck->getKeyword("TABDIMS");
}

BOOST_AUTO_TEST_CASE(Empty) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 77, "1");
    Opm::GridProperty<int> gridProperty( 5 , 5 , 4 , keywordInfo);
    const std::vector<int>& data = gridProperty.getData();
    BOOST_CHECK_EQUAL( 100U , data.size());
    BOOST_CHECK_EQUAL( 100U , gridProperty.getCartesianSize());
    BOOST_CHECK_EQUAL( 5U , gridProperty.getNX());
    BOOST_CHECK_EQUAL( 5U , gridProperty.getNY());
    BOOST_CHECK_EQUAL( 4U , gridProperty.getNZ());
    for (size_t k=0; k < 4; k++) {
        for (size_t j=0; j < 5; j++) {
            for (size_t i=0; i < 5; i++) {
                size_t g = i + j*5 + k*25;
                BOOST_CHECK_EQUAL( 77 , data[g] );
                BOOST_CHECK_EQUAL( 77 , gridProperty.iget( g ));
                BOOST_CHECK_EQUAL( 77 , gridProperty.iget( i,j,k ));
            }
        }
    }
}


BOOST_AUTO_TEST_CASE(HasNAN) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    typedef Opm::GridProperty<double>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("PORO" , nan , "1");
    Opm::GridProperty<double> poro( 2 , 2 , 1 , keywordInfo);

    BOOST_CHECK( poro.containsNaN() );
    poro.iset(0,0.15);
    poro.iset(1,0.15);
    poro.iset(2,0.15);
    BOOST_CHECK( poro.containsNaN() );
    poro.iset(3,0.15);
    BOOST_CHECK( !poro.containsNaN() );
}

BOOST_AUTO_TEST_CASE(EmptyDefault) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 0, "1");
    Opm::GridProperty<int> gridProperty( /*nx=*/10,
                                         /*ny=*/10,
                                         /*nz=*/1 ,
                                         keywordInfo);
    const std::vector<int>& data = gridProperty.getData();
    BOOST_CHECK_EQUAL( 100U , data.size());
    for (size_t i=0; i < data.size(); i++)
        BOOST_CHECK_EQUAL( 0 , data[i] );
}

BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_notData_Throws) {
    const auto& tabdimsKw = createTABDIMSKeyword();
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("TABDIMS" , 100, "1");
    Opm::GridProperty<int> gridProperty( 6 ,1,1 , keywordInfo);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( tabdimsKw ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_wrong_size_throws) {
    const auto& satnumKw = createSATNUMKeyword();
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 66, "1");
    Opm::GridProperty<int> gridProperty( 15 ,1,1, keywordInfo);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( satnumKw ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(SetFromDeckKeyword) {
    const auto& satnumKw = createSATNUMKeyword();
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 99, "1");
    Opm::GridProperty<int> gridProperty( 4 , 4 , 2 , keywordInfo);
    gridProperty.loadFromDeckKeyword( satnumKw );
    const std::vector<int>& data = gridProperty.getData();
    for (size_t k=0; k < 2; k++) {
        for (size_t j=0; j < 4; j++) {
            for (size_t i=0; i < 4; i++) {
                size_t g = i + j*4 + k*16;

                BOOST_CHECK_EQUAL( g , data[g] );
                BOOST_CHECK_EQUAL( g , gridProperty.iget(g) );
                BOOST_CHECK_EQUAL( g , gridProperty.iget(i,j,k) );

            }
        }
    }
}


BOOST_AUTO_TEST_CASE(copy) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo1("P1" , 0, "1");
    SupportedKeywordInfo keywordInfo2("P2" , 9, "1");
    Opm::GridProperty<int> prop1( 4 , 4 , 2 , keywordInfo1);
    Opm::GridProperty<int> prop2( 4 , 4 , 2 , keywordInfo2);

    Opm::Box global(4,4,2);
    std::shared_ptr<Opm::Box> layer0 = std::make_shared<Opm::Box>(global , 0,3,0,3,0,0);

    prop2.copyFrom(prop1 , layer0);

    for (size_t j=0; j < 4; j++) {
        for (size_t i=0; i < 4; i++) {

            BOOST_CHECK_EQUAL( prop2.iget(i,j,0) , 0 );
            BOOST_CHECK_EQUAL( prop2.iget(i,j,1) , 9 );
        }
    }
}


BOOST_AUTO_TEST_CASE(SCALE) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo1("P1" , 1, "1");
    SupportedKeywordInfo keywordInfo2("P2" , 9, "1");

    Opm::GridProperty<int> prop1( 4 , 4 , 2 , keywordInfo1);
    Opm::GridProperty<int> prop2( 4 , 4 , 2 , keywordInfo2);

    std::shared_ptr<Opm::Box> global = std::make_shared<Opm::Box>(4,4,2);
    std::shared_ptr<Opm::Box> layer0 = std::make_shared<Opm::Box>(*global , 0,3,0,3,0,0);

    prop2.copyFrom(prop1 , layer0);
    prop2.scale( 2 , global );
    prop2.scale( 2 , layer0 );

    for (size_t j=0; j < 4; j++) {
        for (size_t i=0; i < 4; i++) {

            BOOST_CHECK_EQUAL( prop2.iget(i,j,0) , 4 );
            BOOST_CHECK_EQUAL( prop2.iget(i,j,1) , 18 );
        }
    }
}


BOOST_AUTO_TEST_CASE(SET) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("P1" , 1, "1");
    Opm::GridProperty<int> prop( 4 , 4 , 2 , keywordInfo);

    std::shared_ptr<Opm::Box> global = std::make_shared<Opm::Box>(4,4,2);
    std::shared_ptr<Opm::Box> layer0 = std::make_shared<Opm::Box>(*global , 0,3,0,3,0,0);

    prop.setScalar( 2 , global );
    prop.setScalar( 4 , layer0 );

    for (size_t j=0; j < 4; j++) {
        for (size_t i=0; i < 4; i++) {

            BOOST_CHECK_EQUAL( prop.iget(i,j,0) , 4 );
            BOOST_CHECK_EQUAL( prop.iget(i,j,1) , 2 );
        }
    }
}


BOOST_AUTO_TEST_CASE(ADD) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo1("P1" , 1, "1");
    SupportedKeywordInfo keywordInfo2("P2" , 9, "1");
    Opm::GridProperty<int> prop1( 4 , 4 , 2 , keywordInfo1);
    Opm::GridProperty<int> prop2( 4 , 4 , 2 , keywordInfo2);

    std::shared_ptr<Opm::Box> global = std::make_shared<Opm::Box>(4,4,2);
    std::shared_ptr<Opm::Box> layer0 = std::make_shared<Opm::Box>(*global , 0,3,0,3,0,0);

    prop2.copyFrom(prop1 , layer0);
    prop2.add( 2 , global );
    prop2.add( 2 , layer0 );

    for (size_t j=0; j < 4; j++) {
        for (size_t i=0; i < 4; i++) {

            BOOST_CHECK_EQUAL( prop2.iget(i,j,0) , 5 );
            BOOST_CHECK_EQUAL( prop2.iget(i,j,1) , 11 );
        }
    }
}

BOOST_AUTO_TEST_CASE(GridPropertyInitialization) {
    const char *deckString =
        "RUNSPEC\n"
        "\n"
        "OIL\n"
        "GAS\n"
        "WATER\n"
        "TABDIMS\n"
        "3 /\n"
        "\n"
        "METRIC\n"
        "\n"
        "DIMENS\n"
        "3 3 3 /\n"
        "\n"
        "GRID\n"
        "\n"
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
        "\n"
        "SWOF\n"
        // table 1
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.1    0        1.0      2.0\n"
        "  0.15   0        0.9      1.0\n"
        "  0.2    0.01     0.5      0.5\n"
        "  0.93   0.91     0.0      0.0\n"
        "/\n"
        // table 2
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.00   0        1.0      2.0\n"
        "  0.05   0.01     1.0      2.0\n"
        "  0.10   0.02     0.9      1.0\n"
        "  0.15   0.03     0.5      0.5\n"
        "  0.852  1.00     0.0      0.0\n"
        "/\n"
        // table 3
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.00   0.00     0.9      2.0\n"
        "  0.05   0.02     0.8      1.0\n"
        "  0.10   0.03     0.5      0.5\n"
        "  0.801  1.00     0.0      0.0\n"
        "/\n"
        "\n"
        "SGOF\n"
        // table 1
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.00   0.00     0.9      2.0\n"
        "  0.05   0.02     0.8      1.0\n"
        "  0.10   0.03     0.5      0.5\n"
        "  0.80   1.00     0.0      0.0\n"
        "/\n"
        // table 2
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.05   0.00     1.0      2\n"
        "  0.10   0.02     0.9      1\n"
        "  0.15   0.03     0.5      0.5\n"
        "  0.85   1.00     0.0      0\n"
        "/\n"
        // table 3
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.1    0        1.0      2\n"
        "  0.15   0        0.9      1\n"
        "  0.2    0.01     0.5      0.5\n"
        "  0.9    0.91     0.0      0\n"
        "/\n"
        "\n"
        "SWU\n"
        "* /\n"
        "\n"
        "ISGU\n"
        "* /\n"
        "\n"
        "SGCR\n"
        "* /\n"
        "\n"
        "ISGCR\n"
        "* /\n"
        "\n"
        "REGIONS\n"
        "\n"
        "SATNUM\n"
        "9*1 9*2 9*3 /\n"
        "\n"
        "IMBNUM\n"
        "9*3 9*2 9*1 /\n"
        "\n"
        "SOLUTION\n"
        "\n"
        "SCHEDULE\n";

    Opm::ParseMode parseMode;
    Opm::ParserPtr parser(new Opm::Parser);

    auto deck = parser->parseString(deckString, parseMode);

    auto eclipseState = std::make_shared<Opm::EclipseState>(deck , parseMode);

    // make sure that EclipseState throws if it is bugged about an _unsupported_ keyword
    BOOST_CHECK_THROW(eclipseState->hasDeckIntGridProperty("ISWU"), std::logic_error);
    BOOST_CHECK_THROW(eclipseState->hasDeckDoubleGridProperty("FLUXNUM"), std::logic_error);

    // make sure that EclipseState does not throw if it is asked for a supported grid
    // property that is not contained  in the deck
    BOOST_CHECK(!eclipseState->hasDeckDoubleGridProperty("ISWU"));
    BOOST_CHECK(!eclipseState->hasDeckIntGridProperty("FLUXNUM"));

    BOOST_CHECK(eclipseState->hasDeckIntGridProperty("SATNUM"));
    BOOST_CHECK(eclipseState->hasDeckIntGridProperty("IMBNUM"));

    BOOST_CHECK(eclipseState->hasDeckDoubleGridProperty("SWU"));
    BOOST_CHECK(eclipseState->hasDeckDoubleGridProperty("ISGU"));
    BOOST_CHECK(eclipseState->hasDeckDoubleGridProperty("SGCR"));
    BOOST_CHECK(eclipseState->hasDeckDoubleGridProperty("ISGCR"));

    const auto& swuPropData = eclipseState->getDoubleGridProperty("SWU")->getData();
    BOOST_CHECK_EQUAL(swuPropData[0 * 3*3], 0.93);
    BOOST_CHECK_EQUAL(swuPropData[1 * 3*3], 0.852);
    BOOST_CHECK_EQUAL(swuPropData[2 * 3*3], 0.801);

    const auto& sguPropData = eclipseState->getDoubleGridProperty("ISGU")->getData();
    BOOST_CHECK_EQUAL(sguPropData[0 * 3*3], 0.9);
    BOOST_CHECK_EQUAL(sguPropData[1 * 3*3], 0.85);
    BOOST_CHECK_EQUAL(sguPropData[2 * 3*3], 0.80);
}


std::vector< double >& TestPostProcessorMul(std::vector< double >& values, const Opm::Deck&, const Opm::EclipseState& ) {
    for( size_t g = 0; g < values.size(); g++ )
        values[g] *= 2.0;

    return values;
}



static Opm::DeckPtr createDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "1000*0.25 /\n"
        "DYV\n"
        "10*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "TOPS\n"
        "100*0.25 /\n"
        "MULTPV \n"
        "1000*0.10 / \n"
        "PORO \n"
        "1000*0.10 / \n"
        "EDIT\n"
        "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData, Opm::ParseMode()) ;
}

BOOST_AUTO_TEST_CASE(GridPropertyPostProcessors) {
    typedef Opm::GridPropertySupportedKeywordInfo<double> SupportedKeywordInfo;
    SupportedKeywordInfo kwInfo1("MULTPV" , 1.0 , "1");
    Opm::GridPropertyFunction< double > gfunc( &TestPostProcessorMul, nullptr, nullptr );
    SupportedKeywordInfo kwInfo2("PORO", 1.0, gfunc, "1");
    std::vector<SupportedKeywordInfo > supportedKeywords = { kwInfo1, kwInfo2 };
    Opm::DeckPtr deck = createDeck();
    std::shared_ptr<Opm::EclipseGrid> grid = std::make_shared<Opm::EclipseGrid>(deck);
    Opm::GridProperties<double> properties(grid, std::move( supportedKeywords ) );

    {
        auto poro = properties.getKeyword("PORO");
        auto multpv = properties.getKeyword("MULTPV");

        poro->loadFromDeckKeyword( deck->getKeyword("PORO" , 0));
        multpv->loadFromDeckKeyword( deck->getKeyword("MULTPV" , 0));

        poro->runPostProcessor();
        multpv->runPostProcessor();

        for (size_t g = 0; g < 1000; g++) {
            BOOST_CHECK_EQUAL( multpv->iget(g) , 0.10 );
            BOOST_CHECK_EQUAL( poro->iget(g)  , 0.20 );
        }

        poro->runPostProcessor();
        multpv->runPostProcessor();

        for (size_t g = 0; g < 1000; g++) {
            BOOST_CHECK_EQUAL( multpv->iget(g) , 0.10 );
            BOOST_CHECK_EQUAL( poro->iget(g)  , 0.20 );
        }

    }
}



BOOST_AUTO_TEST_CASE(multiply) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("P" , 10 , "1");
    Opm::GridProperty<int> p1( 5 , 5 , 4 , keywordInfo);
    Opm::GridProperty<int> p2( 5 , 5 , 5 , keywordInfo);
    Opm::GridProperty<int> p3( 5 , 5 , 4 , keywordInfo);

    BOOST_CHECK_THROW( p1.multiplyWith(p2) , std::invalid_argument );
    p1.multiplyWith(p3);

    for (size_t g = 0; g < p1.getCartesianSize(); g++)
        BOOST_CHECK_EQUAL( 100 , p1.iget(g));

}



BOOST_AUTO_TEST_CASE(mask_test) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo1("P" , 10 , "1");
    SupportedKeywordInfo keywordInfo2("P" , 20 , "1");
    Opm::GridProperty<int> p1( 5 , 5 , 4 , keywordInfo1);
    Opm::GridProperty<int> p2( 5 , 5 , 4 , keywordInfo2);

    std::vector<bool> mask;

    p1.initMask(10 , mask);
    p2.maskedSet( 10 , mask);

    for (size_t g = 0; g < p1.getCartesianSize(); g++)
        BOOST_CHECK_EQUAL( p1.iget(g) , p2.iget(g));
}



BOOST_AUTO_TEST_CASE(kw_test) {
    Opm::GridProperty<int>::SupportedKeywordInfo keywordInfo1("P" , 10 , "1");
    Opm::GridProperty<double>::SupportedKeywordInfo keywordInfo2("P" , 20 , "1");
    Opm::GridProperty<int> p1( 5 , 5 , 4 , keywordInfo1);
    Opm::GridProperty<double> p2( 5 , 5 , 4 , keywordInfo2);


    ERT::EclKW<int> kw1 = p1.getEclKW();
    ERT::EclKW<double> kw2 = p2.getEclKW();

    for (size_t g = 0; g < kw1.size(); g++)
        BOOST_CHECK_EQUAL( p1.iget(g) , kw1[g]);

    for (size_t g = 0; g < kw2.size(); g++)
        BOOST_CHECK_EQUAL( p2.iget(g) , kw2[g]);
}


BOOST_AUTO_TEST_CASE(CheckLimits) {
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo1("P" , 1 , "1");
    Opm::GridProperty<int> p1( 5 , 5 , 4 , keywordInfo1);

    p1.checkLimits(0,2);
    BOOST_CHECK_THROW( p1.checkLimits(-2,0) , std::invalid_argument);
}
