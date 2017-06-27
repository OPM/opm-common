/*
  Copyright 2013 Statoil ASA.

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
#include <cstdio>

#define BOOST_TEST_MODULE EclipseGridTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>


BOOST_AUTO_TEST_CASE(CreateMissingDIMENS_throws) {
    Opm::Deck deck;
    deck.addKeyword( Opm::DeckKeyword( "RUNSPEC" ) );
    deck.addKeyword( Opm::DeckKeyword( "GRID" ) );
    deck.addKeyword( Opm::DeckKeyword( "EDIT" ) );

    BOOST_CHECK_THROW(Opm::EclipseGrid{ deck } , std::invalid_argument);
}

static Opm::Deck createDeckHeaders() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

static Opm::Deck createDeckDIMENS() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 13 17 19/\n"
        "GRID\n"
        "EDIT\n"
        "\n";
    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

static Opm::Deck createDeckSPECGRID() {
    const char* deckData =
        "GRID\n"
        "SPECGRID \n"
        "  13 17 19 / \n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "EDIT\n"
        "\n";
    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

static Opm::Deck createDeckMissingDIMS() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "GRID\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}

BOOST_AUTO_TEST_CASE(MissingDimsThrows) {
    Opm::Deck deck = createDeckMissingDIMS();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(HasGridKeywords) {
    Opm::Deck deck = createDeckHeaders();
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( deck ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( deck ));
}


BOOST_AUTO_TEST_CASE(CreateGridNoCells) {
    Opm::Deck deck = createDeckHeaders();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);

    const Opm::GridDims grid( deck);
    BOOST_CHECK_EQUAL( 10 , grid.getNX());
    BOOST_CHECK_EQUAL( 10 , grid.getNY());
    BOOST_CHECK_EQUAL( 10 , grid.getNZ());
    BOOST_CHECK_EQUAL( 1000 , grid.getCartesianSize());
}

BOOST_AUTO_TEST_CASE(CheckGridIndex) {
    Opm::EclipseGrid grid(17, 19, 41); // prime time

    auto v_start = grid.getIJK(0);
    BOOST_CHECK_EQUAL(v_start[0], 0);
    BOOST_CHECK_EQUAL(v_start[1], 0);
    BOOST_CHECK_EQUAL(v_start[2], 0);

    auto v_end = grid.getIJK(17*19*41 - 1);
    BOOST_CHECK_EQUAL(v_end[0], 16);
    BOOST_CHECK_EQUAL(v_end[1], 18);
    BOOST_CHECK_EQUAL(v_end[2], 40);

    auto v167 = grid.getIJK(167);
    BOOST_CHECK_EQUAL(v167[0], 14);
    BOOST_CHECK_EQUAL(v167[1], 9);
    BOOST_CHECK_EQUAL(v167[2], 0);
    BOOST_CHECK_EQUAL(grid.getGlobalIndex(14, 9, 0), 167);

    auto v5723 = grid.getIJK(5723);
    BOOST_CHECK_EQUAL(v5723[0], 11);
    BOOST_CHECK_EQUAL(v5723[1], 13);
    BOOST_CHECK_EQUAL(v5723[2], 17);
    BOOST_CHECK_EQUAL(grid.getGlobalIndex(11, 13, 17), 5723);

    BOOST_CHECK_EQUAL(17 * 19 * 41, grid.getCartesianSize());
}

static Opm::Deck createCPDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createPinchedCPDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "PINCH \n"
        "  0.2 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createMinpvDefaultCPDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "MINPV \n"
        "  / \n"
        "MINPVFIL \n"
        "  / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createMinpvCPDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "MINPV \n"
        "  10 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}



static Opm::Deck createMinpvFilCPDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "MINPVFIL \n"
        "  20 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createCARTDeck() {
    const char* deckData =
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
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createCARTDeckDEPTHZ() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DXV\n"
        "10*0.25 /\n"
        "DYV\n"
        "10*0.25 /\n"
        "DZV\n"
        "10*0.25 /\n"
        "DEPTHZ\n"
        "121*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createCARTInvalidDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "1000*0.25 /\n"
        "DYV\n"
        "1000*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}

BOOST_AUTO_TEST_CASE(CREATE_SIMPLE) {
    Opm::EclipseGrid grid(10,20,30);

    BOOST_CHECK_EQUAL( grid.getNX() , 10 );
    BOOST_CHECK_EQUAL( grid.getNY() , 20 );
    BOOST_CHECK_EQUAL( grid.getNZ() , 30 );
    BOOST_CHECK_EQUAL( grid.getCartesianSize() , 6000 );
}

BOOST_AUTO_TEST_CASE(DEPTHZ_EQUAL_TOPS) {
    Opm::Deck deck1 = createCARTDeck();
    Opm::Deck deck2 = createCARTDeckDEPTHZ();

    Opm::EclipseGrid grid1( deck1 );
    Opm::EclipseGrid grid2( deck2 );

    BOOST_CHECK( grid1.equal( grid2 ) );

    {
        BOOST_CHECK_THROW( grid1.getCellVolume(1000) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellVolume(10,0,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellVolume(0,10,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellVolume(0,0,10) , std::invalid_argument);

        for (size_t g=0; g < 1000; g++)
            BOOST_CHECK_CLOSE( grid1.getCellVolume(g) , 0.25*0.25*0.25 , 0.001);


        for (size_t k= 0; k < 10; k++)
            for (size_t j= 0; j < 10; j++)
                for (size_t i= 0; i < 10; i++)
                    BOOST_CHECK_CLOSE( grid1.getCellVolume(i,j,k) , 0.25*0.25*0.25 , 0.001 );
    }
    {
        BOOST_CHECK_THROW( grid1.getCellCenter(1000) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellCenter(10,0,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellCenter(0,10,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1.getCellCenter(0,0,10) , std::invalid_argument);

        for (size_t k= 0; k < 10; k++)
            for (size_t j= 0; j < 10; j++)
                for (size_t i= 0; i < 10; i++) {
                    auto pos = grid1.getCellCenter(i,j,k);

                    BOOST_CHECK_CLOSE( std::get<0>(pos) , i*0.25 + 0.125, 0.001);
                    BOOST_CHECK_CLOSE( std::get<1>(pos) , j*0.25 + 0.125, 0.001);
                    BOOST_CHECK_CLOSE( std::get<2>(pos) , k*0.25 + 0.125 + 0.25, 0.001);

                }
    }
}



BOOST_AUTO_TEST_CASE(HasCPKeywords) {
    Opm::Deck deck = createCPDeck();
    BOOST_CHECK(  Opm::EclipseGrid::hasCornerPointKeywords( deck ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( deck ));
}


BOOST_AUTO_TEST_CASE(HasCartKeywords) {
    Opm::Deck deck = createCARTDeck();
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( deck ));
    BOOST_CHECK(  Opm::EclipseGrid::hasCartesianKeywords( deck ));
}


BOOST_AUTO_TEST_CASE(HasCartKeywordsDEPTHZ) {
    Opm::Deck deck = createCARTDeckDEPTHZ();
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( deck ));
    BOOST_CHECK(  Opm::EclipseGrid::hasCartesianKeywords( deck ));
}


BOOST_AUTO_TEST_CASE(HasINVALIDCartKeywords) {
    Opm::Deck deck = createCARTInvalidDeck();
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( deck ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( deck ));
}





BOOST_AUTO_TEST_CASE(CreateMissingGRID_throws) {
    auto deck= createDeckHeaders();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


static Opm::Deck createInvalidDXYZCARTDeck() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "99*0.25 /\n"
        "DY\n"
        "1000*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "TOPS\n"
        "1000*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRID) {
    auto deck = createInvalidDXYZCARTDeck();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


static Opm::Deck createInvalidDXYZCARTDeckDEPTHZ() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "100*0.25 /\n"
        "DY\n"
        "1000*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "DEPTHZ\n"
        "101*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRIDDEPTHZ) {
    auto deck = createInvalidDXYZCARTDeckDEPTHZ();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


static Opm::Deck createOnlyTopDZCartGrid() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 5 20 /\n"
        "GRID\n"
        "DX\n"
        "1000*0.25 /\n"
        "DY\n"
        "1000*0.25 /\n"
        "DZ\n"
        "101*0.25 /\n"
        "TOPS\n"
        "110*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


static Opm::Deck createInvalidDEPTHZDeck1 () {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 5 20 /\n"
        "GRID\n"
        "DXV\n"
        "1000*0.25 /\n"
        "DYV\n"
        "5*0.25 /\n"
        "DZV\n"
        "20*0.25 /\n"
        "DEPTHZ\n"
        "66*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}


BOOST_AUTO_TEST_CASE(CreateCartesianGRIDInvalidDEPTHZ1) {
    auto deck = createInvalidDEPTHZDeck1();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


static Opm::Deck createInvalidDEPTHZDeck2 () {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 5 20 /\n"
        "GRID\n"
        "DXV\n"
        "10*0.25 /\n"
        "DYV\n"
        "5*0.25 /\n"
        "DZV\n"
        "20*0.25 /\n"
        "DEPTHZ\n"
        "67*0.25 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext()) ;
}

BOOST_AUTO_TEST_CASE(CreateCartesianGRIDInvalidDEPTHZ2) {
    auto deck = createInvalidDEPTHZDeck2();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRIDOnlyTopLayerDZ) {
    Opm::Deck deck = createOnlyTopDZCartGrid();
    Opm::EclipseGrid grid( deck );

    BOOST_CHECK_EQUAL( 10 , grid.getNX( ));
    BOOST_CHECK_EQUAL(  5 , grid.getNY( ));
    BOOST_CHECK_EQUAL( 20 , grid.getNZ( ));
    BOOST_CHECK_EQUAL( 1000 , grid.getNumActive());
}



BOOST_AUTO_TEST_CASE(AllActiveExportActnum) {
    Opm::Deck deck = createOnlyTopDZCartGrid();
    Opm::EclipseGrid grid( deck );

    std::vector<int> actnum( 1, 100 );

    grid.exportACTNUM( actnum );
    BOOST_CHECK_EQUAL( 0U , actnum.size());
}


BOOST_AUTO_TEST_CASE(CornerPointSizeMismatchCOORD) {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  725*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    auto deck = parser.parseString( deckData, Opm::ParseContext()) ;
    const auto& zcorn = deck.getKeyword("ZCORN");
    BOOST_CHECK_EQUAL( 8000U , zcorn.getDataSize( ));

    BOOST_CHECK_THROW(Opm::EclipseGrid{ deck }, std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CornerPointSizeMismatchZCORN) {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8001*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    auto deck = parser.parseString( deckData, Opm::ParseContext()) ;
    BOOST_CHECK_THROW(Opm::EclipseGrid{ deck }, std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(ResetACTNUM) {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    auto deck = parser.parseString( deckData, Opm::ParseContext()) ;

    Opm::EclipseGrid grid( deck);
    BOOST_CHECK_EQUAL( 1000U , grid.getNumActive());
    std::vector<int> actnum(1000);
    actnum[0] = 1;
    actnum[2] = 1;
    actnum[4] = 1;
    actnum[6] = 1;
    grid.resetACTNUM( actnum.data() );
    BOOST_CHECK_EQUAL( 4U , grid.getNumActive() );
    {
        std::vector<int> full(grid.getCartesianSize());
        std::iota(full.begin(), full.end(), 0);

        auto compressed = grid.compressedVector( full );
        BOOST_CHECK_EQUAL( compressed.size() , 4U );
        BOOST_CHECK_EQUAL( compressed[0] , 0 );
        BOOST_CHECK_EQUAL( compressed[1] , 2 );
        BOOST_CHECK_EQUAL( compressed[2] , 4 );
        BOOST_CHECK_EQUAL( compressed[3] , 6 );
    }
    {
        const auto& activeMap = grid.getActiveMap( );
        BOOST_CHECK_EQUAL( 4U , activeMap.size() );
        BOOST_CHECK_EQUAL( 0 , activeMap[0] );
        BOOST_CHECK_EQUAL( 2 , activeMap[1] );
        BOOST_CHECK_EQUAL( 4 , activeMap[2] );
        BOOST_CHECK_EQUAL( 6 , activeMap[3] );
    }

    grid.resetACTNUM( NULL );
    BOOST_CHECK_EQUAL( 1000U , grid.getNumActive() );

    {
        const auto&  activeMap = grid.getActiveMap( );
        BOOST_CHECK_EQUAL( 1000U , activeMap.size() );
        BOOST_CHECK_EQUAL( 0 , activeMap[0] );
        BOOST_CHECK_EQUAL( 1 , activeMap[1] );
        BOOST_CHECK_EQUAL( 2 , activeMap[2] );
        BOOST_CHECK_EQUAL( 999 , activeMap[999] );
    }
}


BOOST_AUTO_TEST_CASE(ACTNUM_BEST_EFFORT) {
    const char* deckData1 =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  100*1 /\n"
        "EDIT\n"
        "\n";

    const char* deckData2 =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  100*1 800*0 100*1 /\n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    auto deck1 = parser.parseString( deckData1, Opm::ParseContext()) ;
    auto deck2 = parser.parseString( deckData2, Opm::ParseContext()) ;

    Opm::EclipseGrid grid1(deck1);
    // Actnum vector is too short - ignored
    BOOST_CHECK_EQUAL( 1000U , grid1.getNumActive());

    Opm::EclipseGrid grid2(deck2);
    BOOST_CHECK_EQUAL( 200U , grid2.getNumActive());
}



BOOST_AUTO_TEST_CASE(LoadFromBinary) {
    BOOST_CHECK_THROW(Opm::EclipseGrid( "No/does/not/exist" ) , std::invalid_argument);
}






BOOST_AUTO_TEST_CASE(ConstructorNORUNSPEC) {
    const char* deckData =
        "GRID\n"
        "SPECGRID \n"
        "  10 10 10 / \n"
        "COORD\n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "EDIT\n"
        "\n";

    Opm::Parser parser;
    auto deck1 = parser.parseString( deckData, Opm::ParseContext()) ;
    auto deck2 = createCPDeck();

    Opm::EclipseGrid grid1(deck1);
    Opm::EclipseGrid grid2(deck2);

    BOOST_CHECK(grid1.equal( grid2 ));
}



BOOST_AUTO_TEST_CASE(ConstructorNoSections) {
    const char* deckData =
        "DIMENS \n"
        "  10 10 10 / \n"
        "COORD \n"
        "  726*1 / \n"
        "ZCORN \n"
        "  8000*1 / \n"
        "ACTNUM \n"
        "  1000*1 / \n"
        "\n";

    Opm::Parser parser;
    auto deck1 = parser.parseString( deckData, Opm::ParseContext()) ;
    auto deck2 = createCPDeck();

    Opm::EclipseGrid grid1(deck1);
    Opm::EclipseGrid grid2(deck2);

    BOOST_CHECK(grid1.equal( grid2 ));
}



BOOST_AUTO_TEST_CASE(ConstructorNORUNSPEC_PINCH) {
    auto deck1 = createCPDeck();
    auto deck2 = createPinchedCPDeck();

    Opm::EclipseGrid grid1(deck1);
    Opm::EclipseGrid grid2(deck2);

    BOOST_CHECK(!grid1.equal( grid2 ));
    BOOST_CHECK(!grid1.isPinchActive());
    BOOST_CHECK_THROW(grid1.getPinchThresholdThickness(), std::logic_error);
    BOOST_CHECK(grid2.isPinchActive());
    BOOST_CHECK_EQUAL(grid2.getPinchThresholdThickness(), 0.2);
}




BOOST_AUTO_TEST_CASE(ConstructorMINPV) {
    auto deck1 = createCPDeck();
    auto deck2 = createMinpvDefaultCPDeck();
    auto deck3 = createMinpvCPDeck();
    auto deck4 = createMinpvFilCPDeck();

    Opm::EclipseGrid grid1(deck1);
    BOOST_CHECK_THROW(Opm::EclipseGrid grid2(deck2), std::invalid_argument);
    Opm::EclipseGrid grid3(deck3);
    Opm::EclipseGrid grid4(deck4);

    BOOST_CHECK(!grid1.equal( grid3 ));
    BOOST_CHECK_EQUAL(grid1.getMinpvMode(), Opm::MinpvMode::ModeEnum::Inactive);
    BOOST_CHECK_EQUAL(grid3.getMinpvMode(), Opm::MinpvMode::ModeEnum::EclSTD);
    BOOST_CHECK_EQUAL(grid3.getMinpvValue(), 10.0);
    BOOST_CHECK_EQUAL(grid4.getMinpvMode(), Opm::MinpvMode::ModeEnum::OpmFIL);
    BOOST_CHECK_EQUAL(grid4.getMinpvValue(), 20.0);
}


static Opm::Deck createActnumDeck() {
    const char* deckData = "RUNSPEC\n"
            "\n"
            "DIMENS \n"
            "  2 2 2 / \n"
            "GRID\n"
            "DXV\n"
            "  2*0.25 /\n"
            "DYV\n"
            "  2*0.25 /\n"
            "DZV\n"
            "  2*0.25 /\n"
            "DEPTHZ\n"
            "  9*0.25 /\n"
            "EQUALS\n"
            " ACTNUM 0 1 1 1 1 1 1 /\n"
            "/ \n"
            "FLUXNUM\n"
            "8*0 /\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}


/// creates a deck where the top-layer has ACTNUM = 0 and two partially
/// overlapping 2*2*2 boxes in the center, one [5,7]^3 and one [6,8]^3
/// have ACTNUM = 0
static Opm::Deck createActnumBoxDeck() {
    const char* deckData = "RUNSPEC\n"
            "\n"
            "DIMENS \n"
            "  10 10 10 / \n"
            "GRID\n"
            "DXV\n"
            "  10*0.25 /\n"
            "DYV\n"
            "  10*0.25 /\n"
            "DZV\n"
            "  10*0.25 /\n"
            "DEPTHZ\n"
            "  121*0.25 /\n"
            "EQUALS\n"
            " ACTNUM 0 1 10 1 10 1 1 /\n" // disable top layer
            "/ \n"
            // start box
            "BOX\n"
            "  5 7 5 7 5 7 /\n"
            "ACTNUM \n"
            "    0 0 0 0 0 0 0 0 0\n"
            "    0 0 0 0 0 0 0 0 0\n"
            "    0 0 0 0 0 0 0 0 0\n"
            "/\n"
            "BOX\n" // don't need ENDBOX
            "  6 8 6 8 6 8 /\n"
            "ACTNUM \n"
            "    27*0\n"
            "/\n"
            "ENDBOX\n"
            // end   box
            "FLUXNUM\n"
            "1000*0 /\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

BOOST_AUTO_TEST_CASE(GridBoxActnum) {
    auto deck = createActnumBoxDeck();
    Opm::EclipseState es( deck, Opm::ParseContext());
    auto ep = es.get3DProperties();
    const auto& grid = es.getInputGrid();

    BOOST_CHECK_NO_THROW(ep.getIntGridProperty("ACTNUM"));

    size_t active = 10 * 10 * 10     // 1000
                    - (10 * 10 * 1)  // - top layer
                    - ( 3 *  3 * 3)  // - [5,7]^3 box
                    - ( 3 *  3 * 3)  // - [6,8]^3 box
                    + ( 2 *  2 * 2); // + inclusion/exclusion

    BOOST_CHECK_NO_THROW(grid.getNumActive());
    BOOST_CHECK_EQUAL(grid.getNumActive(), active);

    BOOST_CHECK_EQUAL(es.getInputGrid().getNumActive(), active);

    {
        size_t active_index = 0;
        // NB: The implementation of this test actually assumes that
        //     the loops are running with z as the outer and x as the
        //     inner direction.
        for (size_t z = 0; z < grid.getNZ(); z++) {
            for (size_t y = 0; y < grid.getNY(); y++) {
                for (size_t x = 0; x < grid.getNX(); x++) {
                    if (z == 0)
                        BOOST_CHECK(!grid.cellActive(x, y, z));
                    else if (x >= 4 && x <= 6 && y >= 4 && y <= 6 && z >= 4 && z <= 6)
                        BOOST_CHECK(!grid.cellActive(x, y, z));
                    else if (x >= 5 && x <= 7 && y >= 5 && y <= 7 && z >= 5 && z <= 7)
                        BOOST_CHECK(!grid.cellActive(x, y, z));
                    else {
                        size_t g = grid.getGlobalIndex( x,y,z );

                        BOOST_CHECK(grid.cellActive(x, y, z));
                        BOOST_CHECK_EQUAL( grid.activeIndex(x,y,z) , active_index );
                        BOOST_CHECK_EQUAL( grid.activeIndex(g) , active_index );

                        active_index++;
                    }
                }
            }
        }

        BOOST_CHECK_THROW( grid.activeIndex(0,0,0) , std::invalid_argument );
    }
}

BOOST_AUTO_TEST_CASE(GridActnumVia3D) {
    auto deck = createActnumDeck();

    Opm::EclipseState es( deck, Opm::ParseContext());
    auto ep = es.get3DProperties();
    const auto& grid = es.getInputGrid();
    Opm::EclipseGrid grid2( grid );

    BOOST_CHECK_NO_THROW(ep.getIntGridProperty("ACTNUM"));
    BOOST_CHECK_NO_THROW(grid.getNumActive());
    BOOST_CHECK_EQUAL(grid.getNumActive(), 2 * 2 * 2 - 1);

    BOOST_CHECK_NO_THROW(grid2.getNumActive());
    BOOST_CHECK_EQUAL(grid2.getNumActive(), 2 * 2 * 2 - 1);
}

BOOST_AUTO_TEST_CASE(GridActnumViaState) {
    auto deck = createActnumDeck();

    BOOST_CHECK_NO_THROW(Opm::EclipseState( deck, Opm::ParseContext()));
    Opm::EclipseState es( deck, Opm::ParseContext());
    BOOST_CHECK_EQUAL(es.getInputGrid().getNumActive(), 2 * 2 * 2 - 1);
}


BOOST_AUTO_TEST_CASE(GridDimsSPECGRID) {
    auto deck =  createDeckSPECGRID();
    auto gd = Opm::GridDims( deck );
    BOOST_CHECK_EQUAL(gd.getNX(), 13);
    BOOST_CHECK_EQUAL(gd.getNY(), 17);
    BOOST_CHECK_EQUAL(gd.getNZ(), 19);
}

BOOST_AUTO_TEST_CASE(GridDimsDIMENS) {
    auto deck =  createDeckDIMENS();
    auto gd = Opm::GridDims( deck );
    BOOST_CHECK_EQUAL(gd.getNX(), 13);
    BOOST_CHECK_EQUAL(gd.getNY(), 17);
    BOOST_CHECK_EQUAL(gd.getNZ(), 19);
}


BOOST_AUTO_TEST_CASE(ProcessedCopy) {
    Opm::EclipseGrid gd(10,10,10);
    std::vector<double> zcorn;
    std::vector<int> actnum;

    gd.exportZCORN( zcorn );
    gd.exportACTNUM( actnum  );

    {
        Opm::EclipseGrid gd2(gd , zcorn , actnum );
        BOOST_CHECK( gd.equal( gd2 ));
    }

    zcorn[0] -= 1;
    {
        Opm::EclipseGrid gd2(gd , zcorn , actnum );
        BOOST_CHECK( !gd.equal( gd2 ));
    }

    {
        Opm::EclipseGrid gd2(gd , actnum );
        BOOST_CHECK( gd.equal( gd2 ));
    }

    actnum.assign( gd.getCartesianSize() , 1);
    actnum[0] = 0;
    {
        Opm::EclipseGrid gd2(gd , actnum );
        BOOST_CHECK( !gd.equal( gd2 ));
        BOOST_CHECK( !gd2.cellActive( 0 ));
    }
}


BOOST_AUTO_TEST_CASE(ZcornMapper) {
    int nx = 3;
    int ny = 4;
    int nz = 5;
    Opm::EclipseGrid grid(nx,ny,nz);
    Opm::ZcornMapper zmp = grid.zcornMapper( );
    const ecl_grid_type * ert_grid = grid.c_ptr();


    BOOST_CHECK_THROW(zmp.index(nx,1,1,0) , std::invalid_argument);
    BOOST_CHECK_THROW(zmp.index(0,ny,1,0) , std::invalid_argument);
    BOOST_CHECK_THROW(zmp.index(0,1,nz,0) , std::invalid_argument);
    BOOST_CHECK_THROW(zmp.index(0,1,2,8) , std::invalid_argument);

    for (int k=0; k < nz; k++)
        for (int j=0; j < ny; j++)
            for (int i=0; i < nx; i++)
                for (int c=0; c < 8; c++) {
                    size_t g = i + j*nx + k*nx*ny;
                    BOOST_CHECK_EQUAL( zmp.index(g , c) , zmp.index( i,j,k,c));
                    BOOST_CHECK_EQUAL( zmp.index(i,j,k,c) , ecl_grid_zcorn_index( ert_grid, i , j , k, c));
                }

    std::vector<double> zcorn;
    auto points_adjusted = grid.exportZCORN( zcorn );
    BOOST_CHECK_EQUAL( points_adjusted , 0 );
    BOOST_CHECK( zmp.validZCORN( zcorn ));

    /* Manually destroy it - cell internal */
    zcorn[ zmp.index(0,0,0,4) ] = zcorn[ zmp.index(0,0,0,0) ] - 0.1;
    BOOST_CHECK( !zmp.validZCORN( zcorn ));
    points_adjusted = zmp.fixupZCORN( zcorn );
    BOOST_CHECK_EQUAL( points_adjusted , 1 );
    BOOST_CHECK( zmp.validZCORN( zcorn ));

    /* Manually destroy it - cell 2 cell */
    zcorn[ zmp.index(0,0,0,4) ] = zcorn[ zmp.index(0,0,1,0) ] + 0.1;
    BOOST_CHECK( !zmp.validZCORN( zcorn ));
    points_adjusted = zmp.fixupZCORN( zcorn );
    BOOST_CHECK_EQUAL( points_adjusted , 1 );
    BOOST_CHECK( zmp.validZCORN( zcorn ));

    /* Manually destroy it - cell 2 cell and cell internal*/
    zcorn[ zmp.index(0,0,0,4) ] = zcorn[ zmp.index(0,0,1,0) ] + 0.1;
    zcorn[ zmp.index(0,0,0,0) ] = zcorn[ zmp.index(0,0,0,4) ] + 0.1;
    BOOST_CHECK( !zmp.validZCORN( zcorn ));
    points_adjusted = zmp.fixupZCORN( zcorn );
    BOOST_CHECK_EQUAL( points_adjusted , 2 );
    BOOST_CHECK( zmp.validZCORN( zcorn ));
}



BOOST_AUTO_TEST_CASE(MoveTest) {
    int nx = 3;
    int ny = 4;
    int nz = 5;
    Opm::EclipseGrid grid1(nx,ny,nz);
    Opm::EclipseGrid grid2( std::move( grid1 )); // grid2 should be move constructed from grid1

    BOOST_CHECK( !grid1.c_ptr() );               // We peek at some internal details ...
    BOOST_CHECK( !grid1.circle( ));
}




static Opm::Deck radial_missing_INRAD() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "RADIAL\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}



static Opm::Deck radial_keywords_OK() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 6 10 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "6*60 /\n"
        "DZV\n"
        "10*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

static Opm::Deck radial_keywords_OK_CIRCLE() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 6 10 /\n"
        "RADIAL\n"
        "GRID\n"
        "CIRCLE\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "6*60 /\n"
        "DZV\n"
        "10*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}



BOOST_AUTO_TEST_CASE(RadialTest) {
    Opm::Deck deck = radial_missing_INRAD();
    BOOST_CHECK_THROW( Opm::EclipseGrid{ deck }, std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(RadialKeywordsOK) {
    Opm::Deck deck = radial_keywords_OK();
    Opm::EclipseGrid grid( deck );
    BOOST_CHECK(!grid.circle());
}

BOOST_AUTO_TEST_CASE(RadialKeywordsOK_CIRCLE) {
    Opm::Deck deck = radial_keywords_OK_CIRCLE();
    Opm::EclipseGrid grid( deck );
    BOOST_CHECK(grid.circle());
}



static Opm::Deck radial_keywords_DRV_size_mismatch() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "10 6 12 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "9*1 /\n"
        "DTHETAV\n"
        "6*60 /\n"
        "DZV\n"
        "12*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}


static Opm::Deck radial_keywords_DZV_size_mismatch() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "10 6 12 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "6*60 /\n"
        "DZV\n"
        "11*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

static Opm::Deck radial_keywords_DTHETAV_size_mismatch() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "10 6 12 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "5*60 /\n"
        "DZV\n"
        "12*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}

/*
  This is stricter than the ECLIPSE implementation; we assume that
  *only* the top layer is explicitly given.
*/

static Opm::Deck radial_keywords_TOPS_size_mismatch() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "10 6 12 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "6*60 /\n"
        "DZV\n"
        "12*0.25 /\n"
        "TOPS\n"
        "65*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}


static Opm::Deck radial_keywords_ANGLE_OVERFLOW() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "10 6 12 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "10*1 /\n"
        "DTHETAV\n"
        "6*70 /\n"
        "DZV\n"
        "12*0.25 /\n"
        "TOPS\n"
        "60*0.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}




BOOST_AUTO_TEST_CASE(RadialKeywords_SIZE_ERROR) {
    BOOST_CHECK_THROW( Opm::EclipseGrid{ radial_keywords_DRV_size_mismatch() } , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::EclipseGrid{ radial_keywords_DZV_size_mismatch() } , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::EclipseGrid{ radial_keywords_TOPS_size_mismatch() } , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::EclipseGrid{ radial_keywords_DTHETAV_size_mismatch() } , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::EclipseGrid{ radial_keywords_ANGLE_OVERFLOW() } , std::invalid_argument);
}



static Opm::Deck radial_details() {
    const char* deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "1 5 2 /\n"
        "RADIAL\n"
        "GRID\n"
        "INRAD\n"
        "1 /\n"
        "DRV\n"
        "1 /\n"
        "DTHETAV\n"
        "3*90 60 30/\n"
        "DZV\n"
        "2*1 /\n"
        "TOPS\n"
        "5*1.0 /\n"
        "\n";

    Opm::Parser parser;
    return parser.parseString( deckData, Opm::ParseContext());
}



BOOST_AUTO_TEST_CASE(RadialDetails) {
    Opm::Deck deck = radial_details();
    Opm::EclipseGrid grid( deck );

    BOOST_CHECK_CLOSE( grid.getCellVolume( 0 , 0 , 0 ) , 0.5*(2*2 - 1)*1, 0.0001);
    BOOST_CHECK_CLOSE( grid.getCellVolume( 0 , 3 , 0 ) , sqrt(3.0)*0.25*( 4 - 1 ) , 0.0001);
    auto pos0 = grid.getCellCenter(0,0,0);
    auto pos2 = grid.getCellCenter(0,2,0);


    BOOST_CHECK_CLOSE( std::get<0>(pos0) , 0.75 , 0.0001);
    BOOST_CHECK_CLOSE( std::get<1>(pos0) , 0.75 , 0.0001);
    BOOST_CHECK_CLOSE( std::get<2>(pos0) , 1.50 , 0.0001);

    BOOST_CHECK_CLOSE( std::get<0>(pos2) , -0.75 , 0.0001);
    BOOST_CHECK_CLOSE( std::get<1>(pos2) , -0.75 , 0.0001);
    BOOST_CHECK_CLOSE( std::get<2>(pos2) , 1.50 , 0.0001);

    {
        const auto& p0 = grid.getCornerPos( 0,0,0 , 0 );
        const auto& p6 = grid.getCornerPos( 0,0,0 , 6 );

        BOOST_CHECK_CLOSE( p0[0]*p0[0] + p0[1]*p0[1] , 1.0, 0.0001);
        BOOST_CHECK_CLOSE( p6[0]*p6[0] + p6[1]*p6[1] , 1.0, 0.0001);

        BOOST_CHECK_THROW( grid.getCornerPos( 0,0,0 , 8 ) , std::invalid_argument);
    }
}


BOOST_AUTO_TEST_CASE(CoordMapper) {
    size_t nx = 10;
    size_t ny = 7;
    Opm::CoordMapper cmp = Opm::CoordMapper( nx , ny );
    BOOST_CHECK_THROW( cmp.index(12,6,0,0), std::invalid_argument );
    BOOST_CHECK_THROW( cmp.index(10,8,0,0), std::invalid_argument );
    BOOST_CHECK_THROW( cmp.index(10,7,5,0), std::invalid_argument );
    BOOST_CHECK_THROW( cmp.index(10,5,1,2), std::invalid_argument );

    BOOST_CHECK_EQUAL( cmp.index(10,7,2,1) + 1 , cmp.size( ));
}
