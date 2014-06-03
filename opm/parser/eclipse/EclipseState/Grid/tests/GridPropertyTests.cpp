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

#define BOOST_TEST_MODULE EclipseGridTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>

// forward declarations
Opm::DeckKeywordConstPtr createSATNUMKeyword();
Opm::DeckKeywordConstPtr createTABDIMSKeyword();

BOOST_AUTO_TEST_CASE(Empty) {
    typedef typename Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 77, "1");
    Opm::GridProperty<int> gridProperty( 5 , 5 , 4 , keywordInfo);
    const std::vector<int>& data = gridProperty.getData();
    BOOST_CHECK_EQUAL( 100U , data.size());
    BOOST_CHECK_EQUAL( 100U , gridProperty.size());
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

Opm::DeckKeywordConstPtr createSATNUMKeyword( ) {
    const char *deckData =
    "SATNUM \n"
    "  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData);
    return deck->getKeyword("SATNUM");
}

Opm::DeckKeywordConstPtr createTABDIMSKeyword( ) {
    const char *deckData =
    "TABDIMS\n"
    "  0 1 2 3 4 5 / \n"
    "\n";

    Opm::ParserPtr parser(new Opm::Parser());
    Opm::DeckPtr deck = parser->parseString(deckData);
    return deck->getKeyword("TABDIMS");
}



BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_notData_Throws) {
    Opm::DeckKeywordConstPtr tabdimsKw = createTABDIMSKeyword(); 
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("TABDIMS" , 100, "1");
    Opm::GridProperty<int> gridProperty( 6 ,1,1 , keywordInfo);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( tabdimsKw ) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(SetFromDeckKeyword_wrong_size_throws) {
    Opm::DeckKeywordConstPtr satnumKw = createSATNUMKeyword(); 
    typedef Opm::GridProperty<int>::SupportedKeywordInfo SupportedKeywordInfo;
    SupportedKeywordInfo keywordInfo("SATNUM" , 66, "1");
    Opm::GridProperty<int> gridProperty( 15 ,1,1, keywordInfo);
    BOOST_CHECK_THROW( gridProperty.loadFromDeckKeyword( satnumKw ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(SetFromDeckKeyword) {
    Opm::DeckKeywordConstPtr satnumKw = createSATNUMKeyword(); 
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

