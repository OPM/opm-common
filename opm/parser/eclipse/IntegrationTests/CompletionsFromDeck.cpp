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

#define BOOST_TEST_MODULE CompletionIntegrationTests

#include <map>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>


using namespace Opm;


BOOST_AUTO_TEST_CASE( CreateCompletionsFromRecord ) {

    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_COMPDAT1");
    DeckPtr deck =  parser->parseFile(scheduleFile.string());
    DeckKeywordConstPtr COMPDAT1 = deck->getKeyword("COMPDAT" , 0);
    DeckRecordConstPtr line0 = COMPDAT1->getRecord(0);
    DeckRecordConstPtr line1 = COMPDAT1->getRecord(1);
    
    std::pair< std::string , std::vector<CompletionConstPtr> > completionsList = Completion::completionsFromCOMPDATRecord( line0 );
    BOOST_CHECK_EQUAL( "W_1" , completionsList.first );
    BOOST_CHECK_EQUAL( 3U , completionsList.second.size() );

    {
        CompletionConstPtr completion0 = completionsList.second[0];
        CompletionConstPtr completion2 = completionsList.second[2];

        BOOST_CHECK_EQUAL( 30 , completion0->getI() );
        BOOST_CHECK_EQUAL( 37 , completion0->getJ() );
        BOOST_CHECK_EQUAL( 1  , completion0->getK() );
        BOOST_CHECK_EQUAL( OPEN  , completion0->getState() );
        BOOST_CHECK_EQUAL( 32.948  , completion0->getCF() );

        BOOST_CHECK_EQUAL( 30 , completion2->getI() );
        BOOST_CHECK_EQUAL( 37 , completion2->getJ() );
        BOOST_CHECK_EQUAL( 3  , completion2->getK() );
        BOOST_CHECK_EQUAL( OPEN  , completion2->getState() );
        BOOST_CHECK_EQUAL( 32.948  , completion2->getCF() );
    }
        
    // We do not accept a defaulted Connection Factor
    BOOST_CHECK_THROW(Completion::completionsFromCOMPDATRecord( line1 ) , std::invalid_argument );
}



BOOST_AUTO_TEST_CASE( CreateCompletionsFromKeyword ) {

    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/SCHEDULE/SCHEDULE_COMPDAT1");
    DeckPtr deck =  parser->parseFile(scheduleFile.string());
    DeckKeywordConstPtr COMPDAT1 = deck->getKeyword("COMPDAT" , 1);
    
    std::map< std::string , std::vector<CompletionConstPtr> > completions = Completion::completionsFromCOMPDATKeyword( COMPDAT1 );
    BOOST_CHECK_EQUAL( 3U , completions.size() );
    
    
    BOOST_CHECK( completions.find("W_1") != completions.end() );
    BOOST_CHECK( completions.find("W_2") != completions.end() );
    BOOST_CHECK( completions.find("W_3") != completions.end() );
    
    BOOST_CHECK_EQUAL( 17U , completions.find("W_1")->second.size() );
    BOOST_CHECK_EQUAL(  5U , completions.find("W_2")->second.size() );
    BOOST_CHECK_EQUAL(  5U , completions.find("W_3")->second.size() );

    std::vector<CompletionConstPtr> W_3Completions = completions.find("W_3")->second;
    
    CompletionConstPtr completion0 = W_3Completions[0];
    CompletionConstPtr completion4 = W_3Completions[4];

    BOOST_CHECK_EQUAL( 31     , completion0->getI() );
    BOOST_CHECK_EQUAL( 18     , completion0->getJ() );
    BOOST_CHECK_EQUAL( 1      , completion0->getK() );
    BOOST_CHECK_EQUAL( OPEN   , completion0->getState() );
    BOOST_CHECK_EQUAL( 27.412 , completion0->getCF() );

    BOOST_CHECK_EQUAL( 31     , completion4->getI() );
    BOOST_CHECK_EQUAL( 17     , completion4->getJ() );
    BOOST_CHECK_EQUAL( 4      , completion4->getK() );
    BOOST_CHECK_EQUAL( OPEN   , completion4->getState() );
    BOOST_CHECK_EQUAL( 4.728  , completion4->getCF() );

    
    
}


