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
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE MULTREGTScannerTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserLog.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>



BOOST_AUTO_TEST_CASE(TestRegionName) {
    BOOST_CHECK_EQUAL( "FLUXNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "F"));
    BOOST_CHECK_EQUAL( "MULTNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "M"));
    BOOST_CHECK_EQUAL( "OPERNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "O"));

    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("o") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("X") , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(TestNNCBehaviourEnum) {
    BOOST_CHECK_EQUAL( Opm::MULTREGT::ALL      , Opm::MULTREGT::NNCBehaviourFromString( "ALL"));
    BOOST_CHECK_EQUAL( Opm::MULTREGT::NNC      , Opm::MULTREGT::NNCBehaviourFromString( "NNC"));
    BOOST_CHECK_EQUAL( Opm::MULTREGT::NONNC    , Opm::MULTREGT::NNCBehaviourFromString( "NONNC"));
    BOOST_CHECK_EQUAL( Opm::MULTREGT::NOAQUNNC , Opm::MULTREGT::NNCBehaviourFromString( "NOAQUNNC"));


    BOOST_CHECK_THROW(  Opm::MULTREGT::NNCBehaviourFromString( "Invalid") , std::invalid_argument);
}



static Opm::DeckPtr createInvalidMULTREGTDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 3 3 2 /\n"
        "GRID\n"
        "DX\n"
        "18*0.25 /\n"
        "DYV\n"
        "3*0.25 /\n"
        "DZ\n"
        "18*0.25 /\n"
        "TOPS\n"
        "9*0.25 /\n"
        "FLUXNUM\n"
        "1 1 2\n"
        "1 1 2\n"
        "1 1 2\n"
        "3 4 5\n"
        "3 4 5\n"
        "3 4 5\n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50   G   ALL    M / -- Invalid direction\n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50   X   ALL    G / -- Invalid region \n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50   X   ALL    M / -- Region not in deck \n"
        "/\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}




BOOST_AUTO_TEST_CASE(InvalidInput) {
    typedef Opm::GridProperties<int>::SupportedKeywordInfo SupportedKeywordInfo;
    std::vector<SupportedKeywordInfo> supportedKeywords = { SupportedKeywordInfo("FLUXNUM" , 1 , "1") , 
                                                            SupportedKeywordInfo("OPERNUM" , 1 , "1") ,
                                                            SupportedKeywordInfo("MULTNUM" , 1 , "1") };

    Opm::MULTREGTScanner scanner;
    Opm::DeckPtr deck = createInvalidMULTREGTDeck();
    Opm::EclipseGrid grid(deck);
    std::shared_ptr<Opm::GridProperties<int> > gridProperties = std::make_shared<Opm::GridProperties<int> >( grid.getNX() , grid.getNY() , grid.getNZ() , supportedKeywords);
    Opm::DeckKeywordConstPtr multregtKeyword0 = deck->getKeyword("MULTREGT",0);
    Opm::DeckKeywordConstPtr multregtKeyword1 = deck->getKeyword("MULTREGT",1);
    Opm::DeckKeywordConstPtr multregtKeyword2 = deck->getKeyword("MULTREGT",2);


    // Invalid direction
    BOOST_CHECK_THROW( scanner.addKeyword( multregtKeyword0 ) , std::invalid_argument);

    // Not supported region 
    BOOST_CHECK_THROW( scanner.addKeyword( multregtKeyword1 ) , std::invalid_argument);

    // The keyword is ok; but it refers to a region which is not in the deck.
    scanner.addKeyword( multregtKeyword2 );
    BOOST_CHECK_THROW( scanner.scanRegions( gridProperties ) , std::logic_error);
}


static Opm::DeckPtr createNotSupportedMULTREGTDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 3 3 2 /\n"
        "GRID\n"
        "DX\n"
        "18*0.25 /\n"
        "DYV\n"
        "3*0.25 /\n"
        "DZ\n"
        "18*0.25 /\n"
        "TOPS\n"
        "9*0.25 /\n"
        "FLUXNUM\n"
        "1 1 2\n"
        "1 1 2\n"
        "1 1 2\n"
        "3 4 5\n"
        "3 4 5\n"
        "3 4 5\n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50   X   NNC    M / -- Not yet support NNC behaviour \n"
        "/\n"
        "MULTREGT\n"  
        "*  2   0.50   X   ALL    M / -- Defaulted from region value \n"
        "/\n"
        "MULTREGT\n"  
        "2  *   0.50   X   ALL    M / -- Defaulted to region value \n"
        "/\n"
        "MULTREGT\n"  
        "2  2   0.50   X   ALL    M / -- Region values equal \n"
        "/\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


BOOST_AUTO_TEST_CASE(NotSupported) {
    Opm::DeckPtr deck = createNotSupportedMULTREGTDeck();
    Opm::DeckKeywordConstPtr multregtKeyword0 = deck->getKeyword("MULTREGT",0);
    Opm::DeckKeywordConstPtr multregtKeyword1 = deck->getKeyword("MULTREGT",1);
    Opm::DeckKeywordConstPtr multregtKeyword2 = deck->getKeyword("MULTREGT",2);
    Opm::DeckKeywordConstPtr multregtKeyword3 = deck->getKeyword("MULTREGT",3);
    Opm::MULTREGTScanner scanner;

    // Not supported NNC behaviour
    BOOST_CHECK_THROW( Opm::MULTREGTScanner::assertKeywordSupported(multregtKeyword0) , std::invalid_argument);
    BOOST_CHECK_THROW( scanner.addKeyword(multregtKeyword0) , std::invalid_argument);

    // Defaulted from value - not supported
    BOOST_CHECK_THROW( Opm::MULTREGTScanner::assertKeywordSupported(multregtKeyword1) , std::invalid_argument);
    BOOST_CHECK_THROW( scanner.addKeyword(multregtKeyword1) , std::invalid_argument);

    // Defaulted to value - not supported
    BOOST_CHECK_THROW( Opm::MULTREGTScanner::assertKeywordSupported(multregtKeyword2) , std::invalid_argument);
    BOOST_CHECK_THROW( scanner.addKeyword(multregtKeyword2) , std::invalid_argument);

    // srcValue == targetValue - not supported
    BOOST_CHECK_THROW( Opm::MULTREGTScanner::assertKeywordSupported(multregtKeyword3) , std::invalid_argument);
    BOOST_CHECK_THROW( scanner.addKeyword(multregtKeyword3) , std::invalid_argument);
}


static Opm::DeckPtr createSimpleMULTREGTDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "2 2 2 /\n"
        "GRID\n"
        "DX\n"
        "8*0.25 /\n"
        "DYV\n"
        "2*0.25 /\n"
        "DZ\n"
        "8*0.25 /\n"
        "TOPS\n"
        "4*0.25 /\n"
        "FLUXNUM\n"
        "1 2\n"
        "1 2\n"
        "3 4\n"
        "3 4\n"
        "/\n"
        "MULTNUM\n"
        "1 2\n"
        "1 2\n"
        "3 4\n"
        "3 4\n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50   X   ALL    M / \n"
        "/\n"
        "MULTREGT\n"  
        "2  1   1.50   X   ALL    M / \n"
        "/\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}


BOOST_AUTO_TEST_CASE(SimpleMULTREGT) {
    typedef Opm::GridProperties<int>::SupportedKeywordInfo SupportedKeywordInfo;
    std::vector<SupportedKeywordInfo> supportedKeywords = { SupportedKeywordInfo("FLUXNUM" , 1 , "1") , 
                                                            SupportedKeywordInfo("OPERNUM" , 1 , "1") ,
                                                            SupportedKeywordInfo("MULTNUM" , 1 , "1") };

    Opm::DeckPtr deck = createSimpleMULTREGTDeck();
    Opm::EclipseGrid grid(deck);
    std::shared_ptr<const Opm::Box> inputBox = std::make_shared<const Opm::Box>( grid.getNX() , grid.getNY() , grid.getNZ() );

    std::shared_ptr<Opm::GridProperties<int> > gridProperties = std::make_shared<Opm::GridProperties<int> >( grid.getNX() , grid.getNY() , grid.getNZ() , supportedKeywords);
    std::shared_ptr<Opm::GridProperty<int> > fluxNum = gridProperties->getKeyword("FLUXNUM");
    std::shared_ptr<Opm::GridProperty<int> > multNum = gridProperties->getKeyword("MULTNUM");
    Opm::DeckKeywordConstPtr fluxnumKeyword = deck->getKeyword("FLUXNUM",0);
    Opm::DeckKeywordConstPtr multnumKeyword = deck->getKeyword("MULTNUM",0);

    Opm::DeckKeywordConstPtr multregtKeyword0 = deck->getKeyword("MULTREGT",0);
    Opm::DeckKeywordConstPtr multregtKeyword1 = deck->getKeyword("MULTREGT",1);
    
    multNum->loadFromDeckKeyword( inputBox , multnumKeyword );
    fluxNum->loadFromDeckKeyword( inputBox , fluxnumKeyword );
    
    {
        Opm::MULTREGTScanner scanner;
        scanner.addKeyword(multregtKeyword0);
    
        auto cells = scanner.scanRegions(gridProperties);
        
        BOOST_CHECK_EQUAL( 2 , cells.size() );
        auto cell0 = cells[0];
        auto cell1 = cells[1];

        BOOST_CHECK_EQUAL( 0 , std::get<0>(cell0));
        BOOST_CHECK_EQUAL( Opm::FaceDir::XPlus , std::get<1>(cell0));
        BOOST_CHECK_EQUAL( 0.50 , std::get<2>(cell0));
        
        BOOST_CHECK_EQUAL( 2 , std::get<0>(cell1));
        BOOST_CHECK_EQUAL( Opm::FaceDir::XPlus , std::get<1>(cell1));
        BOOST_CHECK_EQUAL( 0.50 , std::get<2>(cell1));
    }

    {
        Opm::MULTREGTScanner scanner;
        scanner.addKeyword(multregtKeyword1);
        
        auto cells = scanner.scanRegions(gridProperties);
        
        BOOST_CHECK_EQUAL( 2 , cells.size() );
        
        auto cell0 = cells[0];
        auto cell1 = cells[1];

        
        BOOST_CHECK_EQUAL( 1 , std::get<0>(cell0));
        BOOST_CHECK_EQUAL( Opm::FaceDir::XMinus , std::get<1>(cell0));
        BOOST_CHECK_EQUAL( 1.50 , std::get<2>(cell0));

        BOOST_CHECK_EQUAL( 3 , std::get<0>(cell1));
        BOOST_CHECK_EQUAL( Opm::FaceDir::XMinus , std::get<1>(cell1));
        BOOST_CHECK_EQUAL( 1.50 , std::get<2>(cell1));
    }

}


static Opm::DeckPtr createCopyMULTNUMDeck() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        "2 2 2 /\n"
        "GRID\n"
        "DX\n"
        "8*0.25 /\n"
        "DYV\n"
        "2*0.25 /\n"
        "DZ\n"
        "8*0.25 /\n"
        "TOPS\n"
        "4*0.25 /\n"
        "FLUXNUM\n"
        "1 2\n"
        "1 2\n"
        "3 4\n"
        "3 4\n"
        "/\n"
        "COPY\n"
        " FLUXNUM  MULTNUM /\n"
        "/\n"
        "MULTREGT\n"  
        "1  2   0.50/ \n"
        "/\n"
        "EDIT\n"
        "\n";
 
    Opm::ParserPtr parser(new Opm::Parser());
    return parser->parseString(deckData) ;
}



BOOST_AUTO_TEST_CASE(MULTREGT_COPY_MULTNUM) {
    Opm::DeckPtr deck = createCopyMULTNUMDeck();
    Opm::ParserLogPtr parserLog(new Opm::ParserLog());
    BOOST_CHECK_NO_THROW( Opm::EclipseState( deck , parserLog ));     
}
