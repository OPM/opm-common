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

#define BOOST_TEST_MODULE EclipseGridTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>


BOOST_AUTO_TEST_CASE(CreateMissingDIMENS_throws) {
    Opm::DeckPtr deck(new Opm::Deck());
    Opm::DeckKeywordPtr test0(new Opm::DeckKeyword("RUNSPEC"));
    Opm::DeckKeywordPtr test1(new Opm::DeckKeyword("GRID"));
    Opm::DeckKeywordPtr test2(new Opm::DeckKeyword("EDIT"));
    deck->addKeyword(test0);
    deck->addKeyword(test1);
    deck->addKeyword(test2);

    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}



static Opm::DeckPtr createDeckHeaders() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}



BOOST_AUTO_TEST_CASE(HasGridKeywords) {
    Opm::DeckPtr deck = createDeckHeaders();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}

static Opm::DeckPtr createCPDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "COORD\n"
        "1000*0.25 /\n"
        "ZCORN\n"
        "1000*0.25 /\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


static Opm::DeckPtr createCARTDeck() {
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
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


static Opm::DeckPtr createCARTDeckDEPTHZ() {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


static Opm::DeckPtr createCARTInvalidDeck() {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}

BOOST_AUTO_TEST_CASE(DEPTHZ_EQUAL_TOPS) {
    Opm::DeckPtr deck1 = createCARTDeck();
    Opm::DeckPtr deck2 = createCARTDeckDEPTHZ();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck1) );
    std::shared_ptr<Opm::GRIDSection> gridSection1(new Opm::GRIDSection(deck1) );
    std::shared_ptr<Opm::GRIDSection> gridSection2(new Opm::GRIDSection(deck2) );
    
    std::shared_ptr<Opm::EclipseGrid> grid1(new Opm::EclipseGrid( runspecSection , gridSection1 ));
    std::shared_ptr<Opm::EclipseGrid> grid2(new Opm::EclipseGrid( runspecSection , gridSection2 ));

    BOOST_CHECK( grid1->equal( *(grid2.get()) ));

    {
        BOOST_CHECK_THROW( grid1->getCellVolume(1000) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellVolume(10,0,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellVolume(0,10,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellVolume(0,0,10) , std::invalid_argument);
        
        for (size_t g=0; g < 1000; g++)
            BOOST_CHECK_CLOSE( grid1->getCellVolume(g) , 0.25*0.25*0.25 , 0.001);
        
        
        for (size_t k= 0; k < 10; k++)
            for (size_t j= 0; j < 10; j++)  
                for (size_t i= 0; i < 10; i++)
                    BOOST_CHECK_CLOSE( grid1->getCellVolume(i,j,k) , 0.25*0.25*0.25 , 0.001 );
    }
    {
        BOOST_CHECK_THROW( grid1->getCellCenter(1000) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellCenter(10,0,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellCenter(0,10,0) , std::invalid_argument);
        BOOST_CHECK_THROW( grid1->getCellCenter(0,0,10) , std::invalid_argument);

        for (size_t k= 0; k < 10; k++)
            for (size_t j= 0; j < 10; j++)  
                for (size_t i= 0; i < 10; i++) {
                    auto pos = grid1->getCellCenter(i,j,k);
                    
                    BOOST_CHECK_CLOSE( std::get<0>(pos) , i*0.25 + 0.125, 0.001);
                    BOOST_CHECK_CLOSE( std::get<1>(pos) , j*0.25 + 0.125, 0.001);
                    BOOST_CHECK_CLOSE( std::get<2>(pos) , k*0.25 + 0.125 + 0.25, 0.001);
                    
                }
    }
}



BOOST_AUTO_TEST_CASE(HasCPKeywords) {
    Opm::DeckPtr deck = createCPDeck();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK(  Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}


BOOST_AUTO_TEST_CASE(HasCartKeywords) {
    Opm::DeckPtr deck = createCARTDeck();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK(  Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}


BOOST_AUTO_TEST_CASE(HasCartKeywordsDEPTHZ) {
    Opm::DeckPtr deck = createCARTDeckDEPTHZ();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK(  Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}


BOOST_AUTO_TEST_CASE(HasINVALIDCartKeywords) {
    Opm::DeckPtr deck = createCARTInvalidDeck();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}





BOOST_AUTO_TEST_CASE(CreateMissingGRID_throws) {
    Opm::DeckPtr deck = createDeckHeaders();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


static Opm::DeckPtr createInvalidDXYZCARTDeck() {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRID) {
    Opm::DeckPtr deck = createInvalidDXYZCARTDeck();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


static Opm::DeckPtr createInvalidDXYZCARTDeckDEPTHZ() {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRIDDEPTHZ) {
    Opm::DeckPtr deck = createInvalidDXYZCARTDeckDEPTHZ();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


static Opm::DeckPtr createOnlyTopDZCartGrid() {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


static Opm::DeckPtr createInvalidDEPTHZDeck1 () {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


BOOST_AUTO_TEST_CASE(CreateCartesianGRIDInvalidDEPTHZ1) {
    Opm::DeckPtr deck = createInvalidDEPTHZDeck1();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


static Opm::DeckPtr createInvalidDEPTHZDeck2 () {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}

BOOST_AUTO_TEST_CASE(CreateCartesianGRIDInvalidDEPTHZ2) {
    Opm::DeckPtr deck = createInvalidDEPTHZDeck2();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(CreateCartesianGRIDOnlyTopLayerDZ) {
    Opm::DeckPtr deck = createOnlyTopDZCartGrid();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    std::shared_ptr<Opm::EclipseGrid> grid(new Opm::EclipseGrid( runspecSection , gridSection ));

    BOOST_CHECK_EQUAL( 10 , grid->getNX( ));
    BOOST_CHECK_EQUAL(  5 , grid->getNY( ));
    BOOST_CHECK_EQUAL( 20 , grid->getNZ( ));
    BOOST_CHECK_EQUAL( 1000 , grid->getNumActive());
}



BOOST_AUTO_TEST_CASE(AllActiveExportActnum) {
    Opm::DeckPtr deck = createOnlyTopDZCartGrid();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    std::shared_ptr<Opm::EclipseGrid> grid(new Opm::EclipseGrid( runspecSection , gridSection ));

    std::vector<int> actnum;

    actnum.push_back(100);

    grid->exportACTNUM( actnum );
    BOOST_CHECK_EQUAL( 0U , actnum.size());
}


BOOST_AUTO_TEST_CASE(CornerPointSizeMismatchCOORD) {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck = parser->parseString(deckData) ;
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    Opm::DeckKeywordConstPtr zcorn = gridSection->getKeyword("ZCORN");
    BOOST_CHECK_EQUAL( 8000U , zcorn->getDataSize( ));

    BOOST_CHECK_THROW(Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CornerPointSizeMismatchZCORN) {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck = parser->parseString(deckData) ;
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    Opm::DeckKeywordConstPtr zcorn = gridSection->getKeyword("ZCORN");
    BOOST_CHECK_THROW(Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CornerPointSizeMismatchACTNUM) {
    const char *deckData =
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
        "  999*1 / \n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck = parser->parseString(deckData) ;
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(ResetACTNUM) {
    const char *deckData =
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
 
    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckConstPtr deck = parser->parseString(deckData) ;
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    
    Opm::EclipseGrid grid(runspecSection , gridSection );
    BOOST_CHECK_EQUAL( 1000U , grid.getNumActive());
    std::vector<int> actnum(1000);
    actnum[0] = 1;
    grid.resetACTNUM( actnum.data() );
    BOOST_CHECK_EQUAL( 1U , grid.getNumActive() );
    
    grid.resetACTNUM( NULL );
    BOOST_CHECK_EQUAL( 1000U , grid.getNumActive() );
}
