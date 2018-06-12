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

#define BOOST_TEST_MODULE ConnectionIntegrationTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ConnectionSet.hpp>

using namespace Opm;

inline std::string prefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE( CreateConnectionsFromKeyword ) {

    Parser parser;
    const auto scheduleFile = prefix() + "SCHEDULE/SCHEDULE_COMPDAT1";
    auto deck =  parser.parseFile(scheduleFile, ParseContext());
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    const Schedule schedule( deck, grid, eclipseProperties, Phases(true, true, true) , ParseContext());
    const auto& COMPDAT1 = deck.getKeyword("COMPDAT" , 1);

    const auto wells = schedule.getWells( 0 );
    auto connections = Connection::fromCOMPDAT( grid, eclipseProperties, COMPDAT1, wells, ParseContext(), schedule );
    BOOST_CHECK_EQUAL( 3U , connections.size() );

    BOOST_CHECK( connections.find("W_1") != connections.end() );
    BOOST_CHECK( connections.find("W_2") != connections.end() );
    BOOST_CHECK( connections.find("W_3") != connections.end() );

    BOOST_CHECK_EQUAL( 17U , connections.find("W_1")->second.size() );
    BOOST_CHECK_EQUAL(  5U , connections.find("W_2")->second.size() );
    BOOST_CHECK_EQUAL(  5U , connections.find("W_3")->second.size() );

    std::vector<Connection> W_3Connections = connections.find("W_3")->second;

    const auto& connection0 = W_3Connections[0];
    const auto& connection4 = W_3Connections[4];

    BOOST_CHECK_EQUAL( 2     , connection0.getI() );
    BOOST_CHECK_EQUAL( 7     , connection0.getJ() );
    BOOST_CHECK_EQUAL( 0     , connection0.getK() );
    BOOST_CHECK_EQUAL( WellCompletion::OPEN   , connection0.getState() );
    BOOST_CHECK_EQUAL( 3.1726851851851847e-12 , connection0.getConnectionTransmissibilityFactor() );
    BOOST_CHECK_EQUAL( WellCompletion::DirectionEnum::Y, connection0.getDirection() );

    BOOST_CHECK_EQUAL( 2     , connection4.getI() );
    BOOST_CHECK_EQUAL( 6     , connection4.getJ() );
    BOOST_CHECK_EQUAL( 3     , connection4.getK() );
    BOOST_CHECK_EQUAL( WellCompletion::OPEN   , connection4.getState() );
    BOOST_CHECK_EQUAL( 5.4722222222222212e-13 , connection4.getConnectionTransmissibilityFactor() );
    BOOST_CHECK_EQUAL( WellCompletion::DirectionEnum::Y, connection4.getDirection() );


    // Check that wells with all connections shut is also itself shut
    const Well* well1 = schedule.getWell("W_1");
    BOOST_CHECK (!well1->getConnections(0).allConnectionsShut());
    BOOST_CHECK_EQUAL (well1->getStatus(0) , WellCommon::StatusEnum::OPEN);

    const Well* well2 = schedule.getWell("W_2");
    BOOST_CHECK (well2->getConnections(0).allConnectionsShut());
    BOOST_CHECK_EQUAL (well2->getStatus(0) , WellCommon::StatusEnum::SHUT);

    // Check saturation table number for connection
    std::vector<Connection> W_1Connections = connections.find("W_1")->second;
    const auto& W1_connection0 = W_1Connections[0];
    const auto& W1_connection3 = W_1Connections[3];
    const auto& W1_connection4 = W_1Connections[4];

    BOOST_CHECK_EQUAL( 1 , W1_connection0.getSatTableId());
    BOOST_CHECK_EQUAL( 2 , W1_connection3.getSatTableId());
    BOOST_CHECK_EQUAL( 3 , W1_connection4.getSatTableId());

}
