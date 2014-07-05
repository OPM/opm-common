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

#define BOOST_TEST_MODULE EclipseStateTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

using namespace Opm;

static DeckPtr createDeck() {
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
        "1000*0.25 /\n"
        "FAULTS \n"
        "  'F1'  1  1  1  4   1  4  'X' / \n"
        "  'F2'  5  5  1  4   1  4  'X-' / \n"
        "/\n"
        "MULTFLT \n"
        "  'F1' 0.50 / \n"
        "  'F2' 0.50 / \n"
        "/\n"
        "EDIT\n"
        "MULTFLT /\n"
        "  'F2' 0.25 / \n"
        "/\n"
        "OIL\n"
        "\n"
        "GAS\n"
        "\n"
        "TITLE\n"
        "The title\n"
        "\n"
        "START\n"
        "8 MAR 1998 /\n"
        "\n"
        "REGIONS\n"
        "FLUXNUM\n"
        "1000*1 /\n"
        "SATNUM\n"
        "1000*2 /\n"
        "SCHEDULE\n"
        "\n";

    ParserPtr parser(new Parser());
    return parser->parseString(deckData) ;
}



BOOST_AUTO_TEST_CASE(CreatSchedule) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);
    ScheduleConstPtr schedule = state.getSchedule();
    EclipseGridConstPtr eclipseGrid = state.getEclipseGrid();

    BOOST_CHECK_EQUAL( schedule->getStartTime() , boost::posix_time::ptime(boost::gregorian::date(1998 , 3 , 8 )));
}



BOOST_AUTO_TEST_CASE(PhasesCorrect) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);

    BOOST_CHECK(  state.hasPhase( Phase::PhaseEnum::OIL ));
    BOOST_CHECK(  state.hasPhase( Phase::PhaseEnum::GAS ));
    BOOST_CHECK( !state.hasPhase( Phase::PhaseEnum::WATER ));
}


BOOST_AUTO_TEST_CASE(TitleCorrect) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);

    BOOST_CHECK_EQUAL( state.getTitle(), "The title");
}


BOOST_AUTO_TEST_CASE(IntProperties) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);

    BOOST_CHECK_EQUAL( false , state.supportsGridProperty("NONO"));
    BOOST_CHECK_EQUAL( true  , state.supportsGridProperty("SATNUM"));
    BOOST_CHECK_EQUAL( true  , state.hasIntGridProperty("SATNUM"));
}



BOOST_AUTO_TEST_CASE(PropertiesNotSupportedThrows) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);
    DeckKeywordConstPtr fluxNUM = deck->getKeyword("FLUXNUM");
    BOOST_CHECK_EQUAL( false , state.supportsGridProperty("FLUXNUM"));
    BOOST_CHECK_THROW( state.loadGridPropertyFromDeckKeyword( std::make_shared<const Box>(10,10,10) , fluxNUM ) , std::invalid_argument)
}


BOOST_AUTO_TEST_CASE(GetProperty) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);

    std::shared_ptr<GridProperty<int> > satNUM = state.getIntGridProperty( "SATNUM" );

    BOOST_CHECK_EQUAL(1000U , satNUM->size() );
    for (size_t i=0; i < satNUM->size(); i++) 
        BOOST_CHECK_EQUAL( 2 , satNUM->iget(i) );
    
    BOOST_CHECK_THROW( satNUM->iget(100000) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(GetTransMult) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);
    std::shared_ptr<const TransMult> transMult = state.getTransMult();
    
    
    BOOST_CHECK_EQUAL( 1.0 , transMult->getMultiplier(1,0,0,FaceDir::XPlus));
    BOOST_CHECK_THROW(transMult->getMultiplier(1000 , FaceDir::XPlus) , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(GetFaults) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);
    std::shared_ptr<const FaultCollection> faults = state.getFaults();

    BOOST_CHECK( faults->hasFault("F1") );
    BOOST_CHECK( faults->hasFault("F2") );

    std::shared_ptr<Fault> F1 = faults->getFault("F1");
    std::shared_ptr<Fault> F2 = faults->getFault("F2");

    BOOST_CHECK_EQUAL( 0.50 , F1->getTransMult());
    BOOST_CHECK_EQUAL( 0.25 , F2->getTransMult());

    std::shared_ptr<const TransMult> transMult = state.getTransMult();
    BOOST_CHECK_EQUAL( transMult->getMultiplier(0 , 0 , 0 , FaceDir::XPlus) , 0.50 );
    BOOST_CHECK_EQUAL( transMult->getMultiplier(4 , 3 , 0 , FaceDir::XMinus) , 0.25 );
    BOOST_CHECK_EQUAL( transMult->getMultiplier(4 , 3 , 0 , FaceDir::ZPlus) , 1.00 );
}

