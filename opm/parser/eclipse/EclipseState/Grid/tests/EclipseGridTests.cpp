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



Opm::DeckPtr createDeck1() {
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
    Opm::DeckPtr deck = createDeck1();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}

Opm::DeckPtr createCPDeck() {
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


Opm::DeckPtr createCARTDeck() {
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
        "TOPS\n"
        "1000*0.25 /\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


Opm::DeckPtr createCARTInvalidDeck() {
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


BOOST_AUTO_TEST_CASE(HasINVALIDCartKeywords) {
    Opm::DeckPtr deck = createCARTInvalidDeck();
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK( !Opm::EclipseGrid::hasCornerPointKeywords( gridSection ));
    BOOST_CHECK( !Opm::EclipseGrid::hasCartesianKeywords( gridSection ));
}



BOOST_AUTO_TEST_CASE(CreateMissingGRID_throws) {
    Opm::DeckPtr deck = createDeck1();
    std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
    std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
    BOOST_CHECK_THROW(new Opm::EclipseGrid( runspecSection , gridSection ) , std::invalid_argument);
}


Opm::DeckPtr createInvalidDXYZCARTDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "999*0.25 /\n"
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

Opm::DeckPtr createOnlyTopDZCartGrid() {
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
