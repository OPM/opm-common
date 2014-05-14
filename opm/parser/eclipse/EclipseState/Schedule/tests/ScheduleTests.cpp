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

#define BOOST_TEST_MODULE ScheduleTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

DeckPtr createDeck() {
    Opm::Parser parser;
    std::string input =
        "START\n"
        "8 MAR 1998 /\n"
        "\n"
        "SCHEDULE\n"
        "\n";

    return parser.parseString(input);
}

DeckPtr createDeckWithWells() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "WELSPECS\n"
            "     \'W_1\'        \'OP\'   30   37  3.33       \'OIL\'  7* /   \n"
            "/ \n"
            "DATES             -- 1\n"
            " 10  \'JUN\'  2007 / \n"
            "/\n"
            "DATES             -- 2,3\n"
            "  10  JLY 2007 / \n"
            "   10  AUG 2007 / \n"
            "/\n"
            "WELSPECS\n"
            "     \'WX2\'        \'OP\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'W_3\'        \'OP\'   20   51  3.92       \'OIL\'  7* /  \n"
            "/\n";

    return parser.parseString(input);
}



BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingSCHEDULE_Throws) {
    DeckPtr deck(new Deck());
    BOOST_CHECK_THROW(Schedule schedule(deck) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingReturnsDefaults) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr keyword(new DeckKeyword("SCHEDULE"));
    deck->addKeyword( keyword );
    Schedule schedule(deck);
    BOOST_CHECK_EQUAL( schedule.getStartTime() , boost::posix_time::ptime(boost::gregorian::date( 1983  , boost::gregorian::Jan , 1)));
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithStart) {
    DeckPtr deck = createDeck();
    Schedule schedule(deck);
    BOOST_CHECK_EQUAL( schedule.getStartTime() , boost::posix_time::ptime(boost::gregorian::date( 1998  , boost::gregorian::Mar , 8)));
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithSCHEDULENoThrow) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr keyword(new DeckKeyword("SCHEDULE"));
    deck->addKeyword( keyword );
    
    BOOST_CHECK_NO_THROW(Schedule schedule(deck));
}


BOOST_AUTO_TEST_CASE(EmptyScheduleHasNoWells) {
    DeckPtr deck = createDeck();
    Schedule schedule(deck); 
    BOOST_CHECK_EQUAL( 0U , schedule.numWells() );
    BOOST_CHECK_EQUAL( false , schedule.hasWell("WELL1") );
    BOOST_CHECK_THROW( schedule.getWell("WELL2") , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(CreateSchedule_DeckWithoutGRUPTREE_HasRootGroupTreeNodeForTimeStepZero) {
    DeckPtr deck = createDeck();
    Schedule schedule(deck); 
    BOOST_CHECK_EQUAL("FIELD", schedule.getGroupTree(0)->getNode("FIELD")->name());
}


BOOST_AUTO_TEST_CASE(CreateSchedule_DeckWithGRUPTREE_HasRootGroupTreeNodeForTimeStepZero) {
    DeckPtr deck = createDeck();
    DeckKeywordPtr gruptreeKeyword(new DeckKeyword("GRUPTREE"));
    
    DeckRecordPtr recordChildOfField(new DeckRecord());
    DeckStringItemPtr itemChild1(new DeckStringItem("CHILD_GROUP"));
    itemChild1->push_back("BARNET");
    DeckStringItemPtr itemParent1(new DeckStringItem("PARENT_GROUP"));
    itemParent1->push_back("FAREN");
    
    recordChildOfField->addItem(itemChild1);
    recordChildOfField->addItem(itemParent1);
    gruptreeKeyword->addRecord(recordChildOfField);
    deck->addKeyword(gruptreeKeyword);
    Schedule schedule(deck); 
    GroupTreeNodePtr fieldNode = schedule.getGroupTree(0)->getNode("FIELD");
    BOOST_CHECK_EQUAL("FIELD", fieldNode->name());
    GroupTreeNodePtr FAREN = fieldNode->getChildGroup("FAREN");
    BOOST_CHECK(FAREN->hasChildGroup("BARNET"));
}




BOOST_AUTO_TEST_CASE(EmptyScheduleHasFIELDGroup) {
    DeckPtr deck = createDeck();
    Schedule schedule(deck); 
    BOOST_CHECK_EQUAL( 1U , schedule.numGroups() );
    BOOST_CHECK_EQUAL( true , schedule.hasGroup("FIELD") );
    BOOST_CHECK_EQUAL( false , schedule.hasGroup("GROUP") );
    BOOST_CHECK_THROW( schedule.getGroup("GROUP") , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(WellsIterator_Empty_EmptyVectorReturned) {
    DeckPtr deck = createDeck();
    Schedule schedule(deck);

    std::vector<WellConstPtr> wells_alltimesteps = schedule.getWells();
    BOOST_CHECK_EQUAL(0U, wells_alltimesteps.size());
    std::vector<WellConstPtr> wells_t0 = schedule.getWells(0);
    BOOST_CHECK_EQUAL(0U, wells_t0.size());

    BOOST_CHECK_THROW(schedule.getWells(1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(WellsIterator_HasWells_WellsReturned) {
    DeckPtr deck = createDeckWithWells();
    Schedule schedule(deck);

    std::vector<WellConstPtr> wells_alltimesteps = schedule.getWells();
    BOOST_CHECK_EQUAL(3U, wells_alltimesteps.size());
    std::vector<WellConstPtr> wells_t0 = schedule.getWells(0);
    BOOST_CHECK_EQUAL(1U, wells_t0.size());
    std::vector<WellConstPtr> wells_t3 = schedule.getWells(3);
    BOOST_CHECK_EQUAL(3U, wells_t3.size());
}


BOOST_AUTO_TEST_CASE(WellsIteratorWithRegex_HasWells_WellsReturned) {
    DeckPtr deck = createDeckWithWells();
    Schedule schedule(deck);
    std::string wellNamePattern;
    std::vector<WellPtr> wells;

    wellNamePattern = "*";
    wells = schedule.getWells(wellNamePattern);
    BOOST_CHECK_EQUAL(3U, wells.size());

    wellNamePattern = "W_*";
    wells = schedule.getWells(wellNamePattern);
    BOOST_CHECK_EQUAL(2U, wells.size());

    wellNamePattern = "W_3";
    wells = schedule.getWells(wellNamePattern);
    BOOST_CHECK_EQUAL(1U, wells.size());
}

