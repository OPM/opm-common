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

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>

using namespace Opm;

inline std::string prefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE( CreateCompletionsFromKeyword ) {

    Parser parser;
    const auto scheduleFile = prefix() + "SCHEDULE/SCHEDULE_COMPDAT1";
    auto deck =  parser.parseFile(scheduleFile, ParseContext());
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    const Schedule schedule( ParseContext(), grid, eclipseProperties, deck, Phases(true, true, true) );
    const auto& COMPDAT1 = deck.getKeyword("COMPDAT" , 1);

    const auto wells = schedule.getWells( 0 );
    auto completions = Completion::fromCOMPDAT( grid, eclipseProperties, COMPDAT1, wells );
    BOOST_CHECK_EQUAL( 3U , completions.size() );

    BOOST_CHECK( completions.find("W_1") != completions.end() );
    BOOST_CHECK( completions.find("W_2") != completions.end() );
    BOOST_CHECK( completions.find("W_3") != completions.end() );

    BOOST_CHECK_EQUAL( 17U , completions.find("W_1")->second.size() );
    BOOST_CHECK_EQUAL(  5U , completions.find("W_2")->second.size() );
    BOOST_CHECK_EQUAL(  5U , completions.find("W_3")->second.size() );

    std::vector<Completion> W_3Completions = completions.find("W_3")->second;

    const auto& completion0 = W_3Completions[0];
    const auto& completion4 = W_3Completions[4];

    BOOST_CHECK_EQUAL( 2     , completion0.getI() );
    BOOST_CHECK_EQUAL( 7     , completion0.getJ() );
    BOOST_CHECK_EQUAL( 0     , completion0.getK() );
    BOOST_CHECK_EQUAL( WellCompletion::OPEN   , completion0.getState() );
    BOOST_CHECK_EQUAL( 3.1726851851851847e-12 , completion0.getConnectionTransmissibilityFactor() );
    BOOST_CHECK_EQUAL( WellCompletion::DirectionEnum::Y, completion0.getDirection() );

    BOOST_CHECK_EQUAL( 2     , completion4.getI() );
    BOOST_CHECK_EQUAL( 6     , completion4.getJ() );
    BOOST_CHECK_EQUAL( 3     , completion4.getK() );
    BOOST_CHECK_EQUAL( WellCompletion::OPEN   , completion4.getState() );
    BOOST_CHECK_EQUAL( 5.4722222222222212e-13 , completion4.getConnectionTransmissibilityFactor() );
    BOOST_CHECK_EQUAL( WellCompletion::DirectionEnum::Y, completion4.getDirection() );


    // Check that wells with all completions shut is also itself shut
    const Well* well1 = schedule.getWell("W_1");
    BOOST_CHECK (!well1->getCompletions(0).allCompletionsShut());
    BOOST_CHECK_EQUAL (well1->getStatus(0) , WellCommon::StatusEnum::OPEN);

    const Well* well2 = schedule.getWell("W_2");
    BOOST_CHECK (well2->getCompletions(0).allCompletionsShut());
    BOOST_CHECK_EQUAL (well2->getStatus(0) , WellCommon::StatusEnum::SHUT);

    // Check saturation table number for connection
    std::vector<Completion> W_1Completions = completions.find("W_1")->second;
    const auto& W1_completion0 = W_1Completions[0];
    const auto& W1_completion3 = W_1Completions[3];
    const auto& W1_completion4 = W_1Completions[4];

    BOOST_CHECK_EQUAL( 1 , W1_completion0.getSatTableId());
    BOOST_CHECK_EQUAL( 2 , W1_completion3.getSatTableId());
    BOOST_CHECK_EQUAL( 3 , W1_completion4.getSatTableId());

}
