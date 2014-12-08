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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;




BOOST_AUTO_TEST_CASE( parse_ACTION_OK ) {
    ParserPtr parser(new Parser( false ));
    boost::filesystem::path actionFile("testdata/integration_tests/ACTION/ACTION.txt");
    boost::filesystem::path actionFile2("testdata/integration_tests/ACTION/ACTION_EXCEPTION.txt");
    ParserKeywordConstPtr DIMENS = ParserKeyword::createFixedSized("DIMENS" , (size_t) 1 , IGNORE_WARNING );
    ParserKeywordConstPtr THROW = ParserKeyword::createFixedSized("THROW" , UNKNOWN , THROW_EXCEPTION );

    BOOST_REQUIRE( parser->loadKeywordFromFile( boost::filesystem::path( std::string(KEYWORD_DIRECTORY) + std::string("/W/WCONHIST") )) );
    parser->addParserKeyword( DIMENS );
    parser->addParserKeyword( THROW );

    BOOST_REQUIRE( parser->isRecognizedKeyword( "DIMENS" ));
    BOOST_REQUIRE( parser->isRecognizedKeyword( "WCONHIST" ));
    BOOST_REQUIRE( parser->isRecognizedKeyword( "THROW" ));

    BOOST_REQUIRE_THROW(  parser->parseFile( actionFile2.string() , false) , std::invalid_argument );

    ParserLogPtr parserLog(new ParserLog);
    DeckPtr deck =  parser->parseFile(actionFile.string() , false, parserLog);
    DeckKeywordConstPtr kw1 = deck->getKeyword("WCONHIST" , 0);
    BOOST_CHECK_EQUAL( 3U , kw1->size() );


    DeckRecordConstPtr rec1 = kw1->getRecord(0);
    BOOST_CHECK_EQUAL( 11U , rec1->size() );

    DeckRecordConstPtr rec3 = kw1->getRecord(2);
    BOOST_CHECK_EQUAL( 11U , rec3->size() );

    DeckItemConstPtr item1       = rec1->getItem("WELL");
    DeckItemConstPtr item1_index = rec1->getItem(0);

    BOOST_CHECK_EQUAL( item1  , item1_index );
    BOOST_CHECK_EQUAL( "OP_1" , item1->getString(0));


    item1 = rec3->getItem("WELL");
    BOOST_CHECK_EQUAL( "OP_3" , item1->getString(0));


    BOOST_CHECK_EQUAL( false , deck->hasKeyword( "DIMENS" ));
    BOOST_CHECK_EQUAL( 2U , parserLog->size() );
    {
        BOOST_CHECK_EQUAL( actionFile.string() ,  parserLog->getFileName(0));
        BOOST_CHECK_EQUAL( 2U , parserLog->getLineNumber(0));
    }
    {
        BOOST_CHECK_EQUAL( actionFile.string() ,  parserLog->getFileName(1));
        BOOST_CHECK_EQUAL( 6U , parserLog->getLineNumber(1));
    }

}
