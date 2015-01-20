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

static DeckPtr createDeck() {
    Opm::Parser parser;
    std::string input =
        "START\n"
        "8 MAR 1998 /\n"
        "\n"
        "SCHEDULE\n"
        "\n";

    return parser.parseString(input);
}

static DeckPtr createDeckWithWells() {
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


static DeckPtr createDeckWithWellsOrdered() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "WELSPECS\n"
            "     \'CW_1\'        \'OP\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'BW_2\'        \'OP\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'AW_3\'        \'OP\'   20   51  3.92       \'OIL\'  7* /  \n"
            "/\n";

    return parser.parseString(input);
}


static DeckPtr createDeckWithWellsAndCompletionData() {
    Opm::Parser parser;
    std::string input =
      "START             -- 0 \n"
      "1 NOV 1979 / \n"
      "SCHEDULE\n"
      "DATES             -- 1\n"
      " 1 DES 1979/ \n"
      "/\n"
      "WELSPECS\n"
      "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
      "    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
      "    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
      "/\n"
      "COMPDAT\n"
      " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
      " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
      " 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 / \n"
      " 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 / \n"
      " 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 / \n"
      " 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 / \n"
      " 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 / \n"
      "/\n"
      "DATES             -- 2,3\n"
      " 10  JUL 2007 / \n"
      " 10  AUG 2007 / \n"
      "/\n"
      "COMPDAT\n"
      " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
      "/\n";


    return parser.parseString(input);
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingReturnsDefaults) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr keyword(new DeckKeyword("SCHEDULE"));
    deck->addKeyword( keyword );
    Schedule schedule(deck);
    BOOST_CHECK_EQUAL( schedule.getStartTime() , boost::posix_time::ptime(boost::gregorian::date( 1983  , boost::gregorian::Jan , 1)));
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrdered) {
    DeckPtr deck = createDeckWithWellsOrdered();
    Schedule schedule(deck);
    std::vector<WellConstPtr> wells = schedule.getWells();

    BOOST_CHECK_EQUAL( "CW_1" , wells[0]->name());
    BOOST_CHECK_EQUAL( "BW_2" , wells[1]->name());
    BOOST_CHECK_EQUAL( "AW_3" , wells[2]->name());
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

BOOST_AUTO_TEST_CASE(ReturnNumWellsTimestep) {
    DeckPtr deck = createDeckWithWells();
    Schedule schedule(deck);

    BOOST_CHECK_EQUAL(schedule.numWells(0), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(1), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(2), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(3), 3);
}

BOOST_AUTO_TEST_CASE(ReturnMaxNumCompletionsForWellsInTimestep) {
    DeckPtr deck = createDeckWithWellsAndCompletionData();
    Schedule schedule(deck);

    BOOST_CHECK_EQUAL(schedule.getMaxNumCompletionsForWells(1), 7);
    BOOST_CHECK_EQUAL(schedule.getMaxNumCompletionsForWells(3), 9);
}

static DeckPtr createDeckWithWellsAndCompletionDataWithWELOPEN() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "1 NOV 1979 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 1 DES 1979/ \n"
                    "/\n"
                    "WELSPECS\n"
                    "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "    'OP_2'       'OP'   8   8 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "    'OP_3'       'OP'   7   7 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_2'  8  8   1   3 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 / \n"
                    " 'OP_2'  8  7   3   3 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 / \n"
                    " 'OP_2'  8  7   3   6 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 / \n"
                    " 'OP_3'  7  7   1   1 'OPEN' 1*   27.412   0.311  2445.337 1*  1*  'Y'  18.521 / \n"
                    " 'OP_3'  7  7   2   2 'OPEN' 1*   55.195   0.311  4923.842 1*  1*  'Y'  18.524 / \n"
                    "/\n"
                    "DATES             -- 2,3\n"
                    " 10  JUL 2007 / \n"
                    " 10  AUG 2007 / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' SHUT / \n"
                    " '*'    OPEN 0 0 3 / \n"
                    " 'OP_2' SHUT 0 0 0 4 6 / \n "
                    " 'OP_3' SHUT 0 0 0 / \n"
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  JUL 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN / \n"
                    " 'OP_2' OPEN 0 0 0 4 6 / \n "
                    " 'OP_3' OPEN 0 0 0 / \n"
                    "/\n"
                    "DATES             -- 5\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' SHUT 0 0 0 0 0 / \n "
                    "/\n";

    return parser.parseString(input);
}




BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsAndCompletionDataWithWELOPEN) {
    DeckPtr deck = createDeckWithWellsAndCompletionDataWithWELOPEN();
    Schedule schedule(deck);
    WellPtr well;
    well = schedule.getWell("OP_1");
    size_t currentStep = 0;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(currentStep));
    currentStep = 3;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(currentStep));

    well = schedule.getWell("OP_2");
    CompletionSetConstPtr completionSet = well->getCompletions(currentStep);

    size_t index = 3;
    CompletionConstPtr completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::SHUT, completion->getState());
    index = 4;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::SHUT, completion->getState());
    index = 5;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::SHUT, completion->getState());
    index = 6;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());

    currentStep = 4;
    completionSet = well->getCompletions(currentStep);
    index = 3;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());
    index = 4;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());
    index = 5;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());
    index = 6;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());

    well = schedule.getWell("OP_3");
    currentStep = 3;
    completionSet = well->getCompletions(currentStep);

    index = 0;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::SHUT, completion->getState());

    currentStep = 4;
    completionSet = well->getCompletions(currentStep);

    index = 0;
    completion = completionSet->get(index);
    BOOST_CHECK_EQUAL(WellCompletion::StateEnum::OPEN, completion->getState());

    well = schedule.getWell("OP_1");
    currentStep = 0;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(currentStep));

    currentStep = 3;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(currentStep));

    currentStep = 4;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(currentStep));

    currentStep = 5;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(currentStep));


}



BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWELOPEN_OpenWellWithShutCompletionsThrowsExcpetion) {
  Opm::Parser parser;
  std::string input =
          "START             -- 0 \n"
                  "1 NOV 1979 / \n"
                  "SCHEDULE\n"
                  "DATES             -- 1\n"
                  " 1 DES 1979/ \n"
                  "/\n"
                  "WELSPECS\n"
                  "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                  "/\n"
                  "COMPDAT\n"
                  " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                  " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                  " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                  "/\n"
                  "DATES             -- 2\n"
                  " 10  JUL 2008 / \n"
                  "/\n"
                  "WELOPEN\n"
                  " 'OP_1' OPEN / \n"
                  "/\n"
                  "DATES             -- 3\n"
                  " 10  OKT 2008 / \n"
                  "/\n"
                  "WELOPEN\n"
                  " 'OP_1' SHUT 0 0 0 0 0 / \n "
                  "/\n"
                  "DATES             -- 4\n"
                  " 10  NOV 2008 / \n"
                  "/\n"
                  "WELOPEN\n"
                  " 'OP_1' OPEN / \n "
                  "/\n";


  DeckPtr deck = parser.parseString(input);
  BOOST_CHECK_THROW(Schedule schedule(deck), std::exception);
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithCOMPLUMPwithC1_ThrowsExcpetion) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "1 NOV 1979 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 1 DES 1979/ \n"
                    "/\n"
                    "WELSPECS\n"
                    "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN 0 0 0 1 0 / \n"
                    "/\n"
                    "COMPLUMP\n"
                    " 'OP_1' 0 0 0 0 0 / \n "
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  NOV 2008 / \n"
                    "/\n";


    DeckPtr deck = parser.parseString(input);
    BOOST_CHECK_THROW(Schedule schedule(deck), std::exception);
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithCOMPLUMPwithC1andC2_ThrowsExcpetion) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "1 NOV 1979 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 1 DES 1979/ \n"
                    "/\n"
                    "WELSPECS\n"
                    "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN 0 0 0 1 4 / \n"
                    "/\n"
                    "COMPLUMP\n"
                    " 'OP_1' 0 0 0 0 0 / \n "
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  NOV 2008 / \n"
                    "/\n";


    DeckPtr deck = parser.parseString(input);
    BOOST_CHECK_THROW(Schedule schedule(deck), std::exception);
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithCOMPLUMPwithC2_ThrowsExcpetion) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "1 NOV 1979 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 1 DES 1979/ \n"
                    "/\n"
                    "WELSPECS\n"
                    "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN 0 0 0 0 4 / \n"
                    "/\n"
                    "COMPLUMP\n"
                    " 'OP_1' 0 0 0 0 0 / \n "
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  NOV 2008 / \n"
                    "/\n";


    DeckPtr deck = parser.parseString(input);
    BOOST_CHECK_THROW(Schedule schedule(deck), std::exception);
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithCOMPLUMPwithDefaultValues_ThrowsExcpetion) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "1 NOV 1979 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 1 DES 1979/ \n"
                    "/\n"
                    "WELSPECS\n"
                    "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN 0 0 0 0 0 / \n"
                    "/\n"
                    "COMPLUMP\n"
                    " 'OP_1' 0 0 0 0 0 / \n "
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  NOV 2008 / \n"
                    "/\n";


    DeckPtr deck = parser.parseString(input);
    Schedule schedule(deck);
    WellPtr well;
    well = schedule.getWell("OP_1");
    size_t currentStep = 0;
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(currentStep));
}



