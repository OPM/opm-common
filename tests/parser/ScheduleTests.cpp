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

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

using namespace Opm;

static Deck createDeck() {
    Opm::Parser parser;
    std::string input =
        "START\n"
        "8 MAR 1998 /\n"
        "\n"
        "SCHEDULE\n"
        "\n";

    return parser.parseString(input, ParseContext());
}

static Deck createDeckWithWells() {
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

    return parser.parseString(input, ParseContext());
}

static Deck createDeckForTestingCrossFlow() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "WELSPECS\n"
            "     \'DEFAULT\'    \'OP\'   30   37  3.33       \'OIL\'  7*/   \n"
            "     \'ALLOW\'      \'OP\'   30   37  3.33       \'OIL\'  3*  YES / \n"
            "     \'BAN\'        \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "/\n"

            "COMPDAT\n"
            " \'BAN\'  1  1   1   1 \'OPEN\' 1*    1.168   0.311   107.872 1*  1*  \'Z\'  21.925 / \n"
            "/\n"

            "WCONHIST\n"
            "     'BAN'      'OPEN'      'RESV'      0.000      0.000      0.000  5* / \n"
            "/\n"

            "DATES             -- 1\n"
            " 10  JUN 2007 / \n"
            "/\n"

            "WCONHIST\n"
            "     'BAN'      'OPEN'      'RESV'      1.000      0.000      0.000  5* / \n"
            "/\n"

            "DATES             -- 2\n"
            " 10  JUL 2007 / \n"
            "/\n"

            "WCONPROD\n"
            "     'BAN'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* / \n"
            "/\n"


            "DATES             -- 3\n"
            " 10  AUG 2007 / \n"
            "/\n"

            "WCONINJH\n"
            "     'BAN'      'WATER'      1*      0 / \n"
            "/\n"

            "DATES             -- 4\n"
            " 10  SEP 2007 / \n"
            "/\n"

            "WELOPEN\n"
            " 'BAN' OPEN / \n"
            "/\n"

            "DATES             -- 4\n"
            " 10  NOV 2007 / \n"
            "/\n"

            "WCONINJH\n"
            "     'BAN'      'WATER'      1*      1.0 / \n"
            "/\n";


    return parser.parseString(input, ParseContext());
}

static Deck createDeckWithWellsOrdered() {
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

    return parser.parseString(input, ParseContext());
}

static Deck createDeckWithWellsOrderedGRUPTREE() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "GRUPTREE\n"
            "  PG1 PLATFORM /\n"
            "  PG2 PLATFORM /\n"
            "  CG1  PG1 /\n"
            "  CG2  PG2 /\n"
            "/\n"
            "WELSPECS\n"
            "     \'DW_0\'        \'CG1\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'CW_1\'        \'CG1\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'BW_2\'        \'CG2\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'AW_3\'        \'CG2\'   20   51  3.92       \'OIL\'  7* /   \n"
            "/\n";

    return parser.parseString(input, ParseContext());
}

static Deck createDeckWithWellsAndCompletionData() {
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
      "COMPDAT\n" // with defaulted I and J
      " 'OP_1'  0  *   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
      "/\n";

    return parser.parseString(input, ParseContext());
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingReturnsDefaults) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    BOOST_CHECK_EQUAL( schedule.getStartTime() , TimeMap::mkdate(1983, 1 , 1));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrdered) {
    auto deck = createDeckWithWellsOrdered();
    EclipseGrid grid(100,100,100);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    auto wells = schedule.getWells();

    BOOST_CHECK_EQUAL( "CW_1" , wells[0]->name());
    BOOST_CHECK_EQUAL( "BW_2" , wells[1]->name());
    BOOST_CHECK_EQUAL( "AW_3" , wells[2]->name());
}

bool has_well( const std::vector<const Well*> wells, const std::string& well_name);

bool has_well( const std::vector<const Well*> wells, const std::string& well_name) {
    for (const auto& well : wells )
        if (well->name( ) == well_name)
            return true;

    return false;
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrderedGRUPTREE) {
    auto deck = createDeckWithWellsOrderedGRUPTREE();
    EclipseGrid grid(100,100,100);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());

    BOOST_CHECK_THROW( schedule.getWells( "NO_SUCH_GROUP" , 1 ), std::invalid_argument);

    {
        auto field_wells = schedule.getWells("FIELD" , 0);
        BOOST_CHECK_EQUAL( field_wells.size() , 4U);

        BOOST_CHECK( has_well( field_wells, "DW_0" ));
        BOOST_CHECK( has_well( field_wells, "CW_1" ));
        BOOST_CHECK( has_well( field_wells, "BW_2" ));
        BOOST_CHECK( has_well( field_wells, "AW_3" ));
    }

    {
        auto platform_wells = schedule.getWells("PLATFORM" , 0);
        BOOST_CHECK_EQUAL( platform_wells.size() , 4U);

        BOOST_CHECK( has_well( platform_wells, "DW_0" ));
        BOOST_CHECK( has_well( platform_wells, "CW_1" ));
        BOOST_CHECK( has_well( platform_wells, "BW_2" ));
        BOOST_CHECK( has_well( platform_wells, "AW_3" ));
    }

    {
        auto child_wells1 = schedule.getWells("CG1" , 0);
        BOOST_CHECK_EQUAL( child_wells1.size() , 2U);

        BOOST_CHECK( has_well( child_wells1, "DW_0" ));
        BOOST_CHECK( has_well( child_wells1, "CW_1" ));
    }

    {
        auto parent_wells2 = schedule.getWells("PG2" , 0);
        BOOST_CHECK_EQUAL( parent_wells2.size() , 2U);

        BOOST_CHECK( has_well( parent_wells2, "BW_2" ));
        BOOST_CHECK( has_well( parent_wells2, "AW_3" ));
    }
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithStart) {
    auto deck = createDeck();
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    BOOST_CHECK_EQUAL( schedule.getStartTime() , TimeMap::mkdate(1998, 3  , 8 ));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithSCHEDULENoThrow) {
    Deck deck;
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );

    BOOST_CHECK_NO_THROW( Schedule( deck, grid , eclipseProperties, Phases(true, true, true), ParseContext() ));
}

BOOST_AUTO_TEST_CASE(EmptyScheduleHasNoWells) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    BOOST_CHECK_EQUAL( 0U , schedule.numWells() );
    BOOST_CHECK_EQUAL( false , schedule.hasWell("WELL1") );
    BOOST_CHECK_THROW( schedule.getWell("WELL2") , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(CreateSchedule_DeckWithoutGRUPTREE_HasRootGroupTreeNodeForTimeStepZero) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    BOOST_CHECK(schedule.getGroupTree(0).exists("FIELD"));
}

static Deck deckWithGRUPTREE() {
    auto deck = createDeck();
    DeckKeyword gruptreeKeyword("GRUPTREE");

    DeckRecord recordChildOfField;
    DeckItem itemChild1( "CHILD_GROUP", std::string() );
    itemChild1.push_back(std::string("BARNET"));
    DeckItem itemParent1( "PARENT_GROUP", std::string() );
    itemParent1.push_back(std::string("FAREN"));

    recordChildOfField.addItem( std::move( itemChild1 ) );
    recordChildOfField.addItem( std::move( itemParent1 ) );
    gruptreeKeyword.addRecord( std::move( recordChildOfField ) );
    deck.addKeyword( std::move( gruptreeKeyword ) );

    return deck;
}

BOOST_AUTO_TEST_CASE(CreateSchedule_DeckWithGRUPTREE_HasRootGroupTreeNodeForTimeStepZero) {
    EclipseGrid grid(10,10,10);
    auto deck = deckWithGRUPTREE();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    BOOST_CHECK( schedule.getGroupTree( 0 ).exists( "FIELD" ) );
    BOOST_CHECK( schedule.getGroupTree( 0 ).exists( "FAREN" ) );
    BOOST_CHECK_EQUAL( "FAREN", schedule.getGroupTree( 0 ).parent( "BARNET" ) );
}

BOOST_AUTO_TEST_CASE(GetGroups) {
    auto deck = deckWithGRUPTREE();
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck , grid , eclipseProperties, Phases(true, true, true) , ParseContext());

    auto groups = schedule.getGroups();

    BOOST_CHECK_EQUAL( 3, groups.size() );

    std::vector< std::string > names;
    for( const auto group : groups ) names.push_back( group->name() );
    std::sort( names.begin(), names.end() );

    BOOST_CHECK_EQUAL( "BARNET", names[ 0 ] );
    BOOST_CHECK_EQUAL( "FAREN",  names[ 1 ] );
    BOOST_CHECK_EQUAL( "FIELD",  names[ 2 ] );
}

BOOST_AUTO_TEST_CASE(EmptyScheduleHasFIELDGroup) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck , grid , eclipseProperties, Phases(true, true, true) , ParseContext());

    BOOST_CHECK_EQUAL( 1U , schedule.numGroups() );
    BOOST_CHECK_EQUAL( true , schedule.hasGroup("FIELD") );
    BOOST_CHECK_EQUAL( false , schedule.hasGroup("GROUP") );
    BOOST_CHECK_THROW( schedule.getGroup("GROUP") , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(WellsIterator_Empty_EmptyVectorReturned) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck , grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    size_t timeStep = 0;
    const auto wells_alltimesteps = schedule.getWells();
    BOOST_CHECK_EQUAL(0U, wells_alltimesteps.size());
    const auto wells_t0 = schedule.getWells(timeStep);
    BOOST_CHECK_EQUAL(0U, wells_t0.size());

    BOOST_CHECK_THROW(schedule.getWells(1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(WellsIterator_HasWells_WellsReturned) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWells();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck , grid , eclipseProperties, Phases(true, true, true) , ParseContext( ));
    size_t timeStep = 0;

    const auto wells_alltimesteps = schedule.getWells();
    BOOST_CHECK_EQUAL(3U, wells_alltimesteps.size());
    const auto wells_t0 = schedule.getWells(timeStep);
    BOOST_CHECK_EQUAL(1U, wells_t0.size());
    const auto wells_t3 = schedule.getWells(3);
    BOOST_CHECK_EQUAL(3U, wells_t3.size());
}

BOOST_AUTO_TEST_CASE(WellsIteratorWithRegex_HasWells_WellsReturned) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWells();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    std::string wellNamePattern;

    wellNamePattern = "*";
    auto wells = schedule.getWellsMatching(wellNamePattern);
    BOOST_CHECK_EQUAL(3U, wells.size());

    wellNamePattern = "W_*";
    wells = schedule.getWellsMatching(wellNamePattern);
    BOOST_CHECK_EQUAL(2U, wells.size());

    wellNamePattern = "W_3";
    wells = schedule.getWellsMatching(wellNamePattern);
    BOOST_CHECK_EQUAL(1U, wells.size());
}

BOOST_AUTO_TEST_CASE(ReturnNumWellsTimestep) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWells();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext());

    BOOST_CHECK_EQUAL(schedule.numWells(0), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(1), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(2), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(3), 3);
}

BOOST_AUTO_TEST_CASE(ReturnMaxNumCompletionsForWellsInTimestep) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWellsAndCompletionData();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext() );

    BOOST_CHECK_EQUAL(schedule.getMaxNumCompletionsForWells(1), 7);
    BOOST_CHECK_EQUAL(schedule.getMaxNumCompletionsForWells(3), 9);
}

BOOST_AUTO_TEST_CASE(TestCrossFlowHandling) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckForTestingCrossFlow();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , ParseContext() );

    auto well_ban = schedule.getWell("BAN");
    BOOST_CHECK_EQUAL(well_ban->getAllowCrossFlow(), false);


    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well_ban->getStatus(0));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well_ban->getStatus(1));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well_ban->getStatus(2));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well_ban->getStatus(3));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well_ban->getStatus(4)); // not allow to open
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well_ban->getStatus(5));


    {
        auto well_allow = schedule.getWell("ALLOW");
        auto well_default = schedule.getWell("DEFAULT");

        BOOST_CHECK_EQUAL(well_default->getAllowCrossFlow(), true);
        BOOST_CHECK_EQUAL(well_allow->getAllowCrossFlow(), true);
    }
}

static Deck createDeckWithWellsAndCompletionDataWithWELOPEN() {
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
                    /*
                     * Completions for OP_2:
                     * 1 - 8 8 1
                     * 2 - 8 8 2
                     * 3 - 8 8 3
                     * 4 - 8 7 3
                     * 5 - 8 7 4
                     * 6 - 8 7 5
                     * 7 - 8 7 6
                     */
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

    return parser.parseString(input, ParseContext());
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsAndCompletionDataWithWELOPEN) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWellsAndCompletionDataWithWELOPEN();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck ,grid , eclipseProperties, Phases(true, true, true) , ParseContext());
    auto* well = schedule.getWell("OP_1");
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus( 0 ));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus( 3 ));

    well = schedule.getWell("OP_2");
    const auto& cs = well->getCompletions( 3 );

    constexpr auto shut = WellCompletion::StateEnum::SHUT;
    constexpr auto open = WellCompletion::StateEnum::OPEN;

    BOOST_CHECK_EQUAL( 7U, cs.size() );
    BOOST_CHECK_EQUAL(shut, cs.getFromIJK( 7, 6, 2 ).getState());
    BOOST_CHECK_EQUAL(shut, cs.getFromIJK( 7, 6, 3 ).getState());
    BOOST_CHECK_EQUAL(shut, cs.getFromIJK( 7, 6, 4 ).getState());
    BOOST_CHECK_EQUAL(open, cs.getFromIJK( 7, 7, 2 ).getState());

    const auto& cs2 = well->getCompletions( 4 );
    BOOST_CHECK_EQUAL(open, cs2.getFromIJK( 7, 6, 2 ).getState());
    BOOST_CHECK_EQUAL(open, cs2.getFromIJK( 7, 6, 3 ).getState());
    BOOST_CHECK_EQUAL(open, cs2.getFromIJK( 7, 6, 4 ).getState());
    BOOST_CHECK_EQUAL(open, cs2.getFromIJK( 7, 7, 2 ).getState());

    well = schedule.getWell("OP_3");
    const auto& cs3 = well->getCompletions( 3 );

    BOOST_CHECK_EQUAL(shut, cs3.get( 0 ).getState());

    const auto& cs4 = well->getCompletions( 4 );

    BOOST_CHECK_EQUAL(open, cs4.get( 0 ).getState());

    well = schedule.getWell("OP_1");
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus( 3 ));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus( 4 ));
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus( 5 ));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWELOPEN_TryToOpenWellWithShutCompletionsDoNotOpenWell) {
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

  EclipseGrid grid(10,10,10);
  ParseContext parseContext;
  auto deck = parser.parseString(input, parseContext);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  Schedule schedule(deck ,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
  auto* well = schedule.getWell("OP_1");
  size_t currentStep = 3;
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(currentStep));
  currentStep = 4;
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(currentStep));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWELOPEN_CombineShutCompletionsAndAddNewCompletionsDoNotShutWell) {
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
                  "COMPDAT\n"
                  " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                  "/\n"
                  "DATES             -- 4\n"
                  " 10  NOV 2008 / \n"
                  "/\n"
                  "WELOPEN\n"
                  " 'OP_1' SHUT 0 0 0 0 0 / \n "
                  "/\n"
                  "DATES             -- 5\n"
                  " 11  NOV 2008 / \n"
                  "/\n"
                  "COMPDAT\n"
                  " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                  "/\n"
                  "DATES             -- 6\n"
                  " 12  NOV 2008 / \n"
                  "/\n";

  EclipseGrid grid(10,10,10);
  ParseContext parseContext;
  auto deck = parser.parseString(input, parseContext);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
  auto* well = schedule.getWell("OP_1");
  // timestep 3. Close all completions with WELOPEN and immediately open new completions with COMPDAT.
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(3));
  BOOST_CHECK( !well->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 3 ));
  // timestep 4. Close all completions with WELOPEN. The well will be shut since no completions
  // are open.
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(4));
  BOOST_CHECK( well->hasEvent( ScheduleEvents::WELL_STATUS_CHANGE , 4 ));
  // timestep 5. Open new completions. But keep the well shut,
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well->getStatus(5));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWRFT) {
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
                    "    'OP_2'       'OP'   4   4 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_2'  4  4   4  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n"
                    "DATES             -- 2\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WRFT \n"
                    "/ \n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN / \n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 10  NOV 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_2' OPEN / \n"
                    "/\n"
                    "DATES             -- 4\n"
                    " 30  NOV 2008 / \n"
                    "/\n";


    EclipseGrid grid(10,10,10);
    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);

    {
        auto* well = schedule.getWell("OP_1");
        BOOST_CHECK_EQUAL(well->getRFTActive(2),true);
        BOOST_CHECK_EQUAL(2 , well->firstRFTOutput( ));
    }

    {
        auto* well = schedule.getWell("OP_2");
        BOOST_CHECK_EQUAL(well->getRFTActive(3),true);
        BOOST_CHECK_EQUAL(3 , well->firstRFTOutput( ));
    }
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithWRFTPLT) {
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
                    "WELOPEN\n"
                    " 'OP_1' SHUT / \n"
                    "/\n"
                    "DATES             -- 2\n"
                    " 10  OKT 2006 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' SHUT / \n"
                    "/\n"
                    "WRFTPLT \n"
                    " 'OP_1' FOPN / \n"
                    "/ \n"
                    "DATES             -- 3\n"
                    " 10  OKT 2007 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN 0 0 0 0 0 / \n"
                    "/\n"
                    "DATES             -- 4\n"
                    " 10  OKT 2008 / \n"
                    "/\n"
                    "WELOPEN\n"
                    " 'OP_1' OPEN / \n"
                    "/\n"
                    "DATES             -- 5\n"
                    " 10  NOV 2008 / \n"
                    "/\n";
    ParseContext parseContext;
    EclipseGrid grid(10,10,10);
    auto deck = parser.parseString(input, parseContext);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    auto* well = schedule.getWell("OP_1");

    size_t currentStep = 3;
    BOOST_CHECK_EQUAL(well->getRFTActive(currentStep),false);
    currentStep = 4;
    BOOST_CHECK_EQUAL(well->getRFTActive(currentStep),true);
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well->getStatus(currentStep));
    currentStep = 5;
    BOOST_CHECK_EQUAL(well->getRFTActive(currentStep),false);
}

BOOST_AUTO_TEST_CASE(createDeckWithWeltArg) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
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
            " 20  JAN 2010 / \n"
            "/\n"
            "WELTARG\n"
            " OP_1     ORAT        1300 /\n"
            " OP_1     WRAT        1400 /\n"
            " OP_1     GRAT        1500.52 /\n"
            " OP_1     LRAT        1600.58 /\n"
            " OP_1     RESV        1801.05 /\n"
            " OP_1     BHP         1900 /\n"
            " OP_1     THP         2000 /\n"
            " OP_1     VFP         2100.09 /\n"
            " OP_1     GUID        2300.14 /\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck,  grid , eclipseProperties, Phases(true, true, true) , parseContext);
    auto* well = schedule.getWell("OP_1");

    size_t currentStep = 1;
    WellProductionProperties wpp = well->getProductionProperties(currentStep);
    BOOST_CHECK_EQUAL(wpp.WaterRate,0);

    Opm::UnitSystem unitSystem = deck.getActiveUnitSystem();
    double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();
    double siFactorG = unitSystem.parse("GasSurfaceVolume/Time").getSIScaling();
    double siFactorP = unitSystem.parse("Pressure").getSIScaling();

    currentStep = 2;
    wpp = well->getProductionProperties(currentStep);
    BOOST_CHECK_EQUAL(wpp.OilRate, 1300 * siFactorL);
    BOOST_CHECK_EQUAL(wpp.WaterRate, 1400 * siFactorL);
    BOOST_CHECK_EQUAL(wpp.GasRate, 1500.52 * siFactorG);
    BOOST_CHECK_EQUAL(wpp.LiquidRate, 1600.58 * siFactorL);
    BOOST_CHECK_EQUAL(wpp.ResVRate, 1801.05 * siFactorL);
    BOOST_CHECK_EQUAL(wpp.BHPLimit, 1900 * siFactorP);
    BOOST_CHECK_EQUAL(wpp.THPLimit, 2000 * siFactorP);
    BOOST_CHECK_EQUAL(wpp.VFPTableNumber, 2100);
    BOOST_CHECK_EQUAL(well->getGuideRate(2), 2300.14);
}

BOOST_AUTO_TEST_CASE(createDeckWithWeltArgException) {
    Opm::Parser parser;
    std::string input =
            "SCHEDULE\n"
            "WELTARG\n"
            " OP_1     GRAT        1500.52 /\n"
            " OP_1     LRAT        /\n"
            " OP_1     RESV        1801.05 /\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);

    BOOST_CHECK_THROW(Schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(createDeckWithWeltArgException2) {
    Opm::Parser parser;
    std::string input =
            "SCHEDULE\n"
            "WELTARG\n"
            " OP_1     LRAT        /\n"
            " OP_1     RESV        1801.05 /\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_THROW(Schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(createDeckWithWPIMULT) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
                    "19 JUN 2007 / \n"
                    "SCHEDULE\n"
                    "DATES             -- 1\n"
                    " 10  OKT 2008 / \n"
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
                    " 20  JAN 2010 / \n"
                    "/\n"
                    "WELTARG\n"
                    " OP_1     ORAT        1300 /\n"
                    " OP_1     WRAT        1400 /\n"
                    " OP_1     GRAT        1500.52 /\n"
                    " OP_1     LRAT        1600.58 /\n"
                    " OP_1     RESV        1801.05 /\n"
                    " OP_1     BHP         1900 /\n"
                    " OP_1     THP         2000 /\n"
                    " OP_1     VFP         2100.09 /\n"
                    " OP_1     GUID        2300.14 /\n"
                    "/\n"
                    "WPIMULT\n"
                    "OP_1  1.30 /\n"
                    "/\n"
                    "DATES             -- 3\n"
                    " 20  JAN 2011 / \n"
                    "/\n"
                    "WPIMULT\n"
                    "OP_1  1.30 /\n"
                    "/\n"
                    "DATES             -- 4\n"
                    " 20  JAN 2012 / \n"
                    "/\n"
                    "COMPDAT\n"
                    " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                    " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                    "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    auto* well = schedule.getWell("OP_1");

    const auto& cs2 = well->getCompletions( 2 );
    for(size_t i = 0; i < cs2.size(); i++) {
        BOOST_CHECK_EQUAL(cs2.get( i ).getWellPi(), 1.3);
    }

    const auto& cs3 = well->getCompletions( 3 );
    for(size_t i = 0; i < cs3.size(); i++ ) {
        BOOST_CHECK_EQUAL(cs3.get( i ).getWellPi(), (1.3*1.3));
    }

    const auto& cs4 = well->getCompletions( 4 );
    for(size_t i = 0; i < cs4.size(); i++ ) {
        BOOST_CHECK_EQUAL(cs4.get( i ).getWellPi(), 1.0);
    }
}

BOOST_AUTO_TEST_CASE(createDeckWithDRSDT) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "DRSDT\n"
            "0.0003\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    size_t currentStep = 1;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const auto& ovap = schedule.getOilVaporizationProperties(currentStep);

    BOOST_CHECK_EQUAL(true,   ovap.getOption());
    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::DRSDT);
}


BOOST_AUTO_TEST_CASE(createDeckWithDRSDTthenDRVDT) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "DRSDT\n"
            "0.0003\n"
            "/\n"
            "DATES             -- 1\n"
            " 10  OKT 2009 / \n"
            "/\n"
            "DRVDT\n"
            "0.100\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    size_t currentStep = 2;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const OilVaporizationProperties& ovap = schedule.getOilVaporizationProperties(currentStep);
    double value =  ovap.getMaxDRVDT();
    BOOST_CHECK_EQUAL(1.1574074074074074e-06, value);
    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::DRVDT);
}

BOOST_AUTO_TEST_CASE(createDeckWithVAPPARS) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "VAPPARS\n"
            "2 0.100\n"
            "/\n";

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    size_t currentStep = 1;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const OilVaporizationProperties& ovap = schedule.getOilVaporizationProperties(currentStep);
    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::VAPPARS);
    double vap1 =  ovap.getVap1();
    BOOST_CHECK_EQUAL(2, vap1);
    double vap2 =  ovap.getVap2();
    BOOST_CHECK_EQUAL(0.100, vap2);

}


BOOST_AUTO_TEST_CASE(createDeckWithOutOilVaporizationProperties) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n";


    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);

    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), false);


}

BOOST_AUTO_TEST_CASE(changeBhpLimitInHistoryModeWithWeltarg) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "WELSPECS\n"
            "    'P'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "/\n"
            "COMPDAT\n"
            " 'P'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            " 'P'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
            " 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            "/\n"
            "WCONHIST\n"
            " 'P' 'OPEN' 'RESV' 6*  500 / \n"
            "/\n"
            "WCONINJH\n"
            " 'I' 'WATER' 1* 100 250 / \n"
            "/\n"
            "WELTARG\n"
            "   'P' 'BHP' 50 / \n"
            "   'I' 'BHP' 600 / \n"
            "/\n"
            "DATES             -- 2\n"
            " 15  OKT 2008 / \n"
            "/\n"
            "WCONHIST\n"
            "   'P' 'OPEN' 'RESV' 6*  500/\n/\n"
            "WCONINJH\n"
            " 'I' 'WATER' 1* 100 250 / \n"
            "/\n"
            "DATES             -- 3\n"
            " 18  OKT 2008 / \n"
            "/\n"
            "WCONHIST\n"
            "   'I' 'OPEN' 'RESV' 6*  /\n/\n"
            "DATES             -- 3\n"
            " 20  OKT 2008 / \n"
            "/\n"
            "WCONINJH\n"
            " 'I' 'WATER' 1* 100 250 / \n"
            "/\n"
            ;

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    auto* well_p = schedule.getWell("P");

    BOOST_CHECK_EQUAL(well_p->getProductionProperties(0).BHPLimit, 0); //start
    BOOST_CHECK_EQUAL(well_p->getProductionProperties(1).BHPLimit, 50 * 1e5); // 1
    // The BHP limit should not be effected by WCONHIST
    BOOST_CHECK_EQUAL(well_p->getProductionProperties(2).BHPLimit, 50 * 1e5); // 2

    auto* well_i = schedule.getWell("I");

    BOOST_CHECK_EQUAL(well_i->getInjectionProperties(0).BHPLimit, 0); //start
    BOOST_CHECK_EQUAL(well_i->getInjectionProperties(1).BHPLimit, 600 * 1e5); // 1
    BOOST_CHECK_EQUAL(well_i->getInjectionProperties(2).BHPLimit, 600 * 1e5); // 2

    // Check that the BHP limit is reset when changing between injector and producer.
    BOOST_CHECK_EQUAL(well_i->getInjectionProperties(3).BHPLimit, 0); // 3
    BOOST_CHECK_EQUAL(well_i->getInjectionProperties(4).BHPLimit, 0); // 4

    BOOST_CHECK_EQUAL( true  , well_i->getInjectionProperties(2).hasInjectionControl(Opm::WellInjector::BHP) );
    BOOST_CHECK_EQUAL( false , well_i->getInjectionProperties(3).hasInjectionControl(Opm::WellInjector::BHP) );
    BOOST_CHECK_EQUAL( false , well_i->getInjectionProperties(4).hasInjectionControl(Opm::WellInjector::BHP) );
}

BOOST_AUTO_TEST_CASE(changeModeWithWHISTCTL) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "WELSPECS\n"
            "    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "/\n"
            "COMPDAT\n"
            " 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            " 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
            " 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            " 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
            " 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            "/\n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            "DATES             -- 2\n"
            " 15  OKT 2008 / \n"
            "/\n"
            "WHISTCTL\n"
            " RESV / \n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            "DATES             -- 3\n"
            " 18  OKT 2008 / \n"
            "/\n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            "DATES             -- 4\n"
            " 20  OKT 2008 / \n"
            "/\n"
            "WHISTCTL\n"
            " LRAT / \n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            "DATES             -- 5\n"
            " 25  OKT 2008 / \n"
            "/\n"
            "WHISTCTL\n"
            " NONE / \n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            ;

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule(deck, grid , eclipseProperties, Phases(true, true, true) , parseContext);
    auto* well_p1 = schedule.getWell("P1");
    auto* well_p2 = schedule.getWell("P2");

    //Start
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(0).controlMode, Opm::WellProducer::CMODE_UNDEFINED);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(0).controlMode, Opm::WellProducer::CMODE_UNDEFINED);

    //10  OKT 2008
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(1).controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(1).controlMode, Opm::WellProducer::ORAT);

    //15  OKT 2008
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(2).controlMode, Opm::WellProducer::RESV);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(2).controlMode, Opm::WellProducer::RESV);

    //18  OKT 2008
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(3).controlMode, Opm::WellProducer::RESV);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(3).controlMode, Opm::WellProducer::RESV);

    // 20 OKT 2008
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(4).controlMode, Opm::WellProducer::LRAT);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(4).controlMode, Opm::WellProducer::LRAT);

    // 25 OKT 2008
    BOOST_CHECK_EQUAL(well_p1->getProductionProperties(5).controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK_EQUAL(well_p2->getProductionProperties(5).controlMode, Opm::WellProducer::ORAT);
}

BOOST_AUTO_TEST_CASE(unsupportedOptionWHISTCTL) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "WELSPECS\n"
            "    'P1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "    'P2'       'OP'   5   5 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "    'I'       'OP'   1   1 1*     'WATER' 1*      1*  1*   1*  1*   1*  1*  / \n"
            "/\n"
            "COMPDAT\n"
            " 'P1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            " 'P1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
            " 'P2'  5  5   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            " 'P2'  5  5   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
            " 'I'  1  1   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
            "/\n"
            "WCONHIST\n"
            " 'P1' 'OPEN' 'ORAT' 5*/ \n"
            " 'P2' 'OPEN' 'ORAT' 5*/ \n"
            "/\n"
            "DATES             -- 2\n"
            " 15  OKT 2008 / \n"
            "/\n"
            "WHISTCTL\n"
            " * YES / \n"
            ;

    ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    BOOST_CHECK_THROW(Schedule(deck, grid, eclipseProperties, Phases(true, true, true) , parseContext), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(move_HEAD_I_location) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
            DATES             -- 2
                15  OKT 2008 /
            /

            WELSPECS
                'W1' 'G1'  4 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString(input, ctx);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true ) , ctx);

    const auto& well = *schedule.getWell( "W1" );
    BOOST_CHECK_EQUAL( 3, well.getHeadI() );
    BOOST_CHECK_EQUAL( 2, well.getHeadI( 1 ) );
}

BOOST_AUTO_TEST_CASE(change_ref_depth) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
            DATES             -- 2
                15  OKT 2008 /
            /

            WELSPECS
                'W1' 'G1'  3 3 12.0 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString(input, ctx);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& well = *schedule.getWell( "W1" );
    BOOST_CHECK_EQUAL( 12.0, well.getRefDepth() );
    BOOST_CHECK_CLOSE( 2873.94, well.getRefDepth( 1 ), 1e-5 );
}

BOOST_AUTO_TEST_CASE( COMPDAT_sets_automatic_complnum ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        SCHEDULE
        DATES             -- 1
            10  OKT 2008 /
        /
        WELSPECS
            'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
        /

        COMPDAT
            'W1' 0 0 1 1 'SHUT' 1*    / -- regular completion (1)
            'W1' 0 0 2 2 'SHUT' 1*    / -- regular completion (2)
            'W1' 0 0 3 4 'SHUT' 1*    / -- two completions in one record (3, 4)
        /

        DATES             -- 2
            11  OKT 2008 /
        /

        COMPDAT
            'W1' 0 0 1 1 'SHUT' 1*    / -- respecify, essentially ignore (1)
        /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString(input, ctx);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& cs1 = schedule.getWell( "W1" )->getCompletions( 1 );
    BOOST_CHECK_EQUAL( 1, cs1.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs1.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs1.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs1.get( 3 ).complnum() );

    const auto& cs2 = schedule.getWell( "W1" )->getCompletions( 2 );
    BOOST_CHECK_EQUAL( 1, cs2.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs2.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs2.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs2.get( 3 ).complnum() );
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_wells ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        SCHEDULE
        DATES             -- 1
            10  OKT 2008 /
        /
        WELSPECS
            'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
        /

        COMPDAT
            'W1' 0 0 1 1 'SHUT' 1*    / -- regular completion (1)
            'W1' 0 0 2 2 'SHUT' 1*    / -- regular completion (2)
            'W1' 0 0 3 4 'SHUT' 1*    / -- two completions in one record (3, 4)
            'W3' 0 0 1 3 'SHUT' 1*    / -- doesn't exist, ignored
            'W2' 0 0 3 3 'SHUT' 1*    / -- regular completion (1)
            'W2' 0 0 1 3 'SHUT' 1*    / -- two completions (one exist already) (2, 3)
        /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString( input, ctx );
    EclipseGrid grid( 10, 10, 10 );
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& w1cs = schedule.getWell( "W1" )->getCompletions();
    BOOST_CHECK_EQUAL( 1, w1cs.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, w1cs.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, w1cs.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, w1cs.get( 3 ).complnum() );

    const auto& w2cs = schedule.getWell( "W2" )->getCompletions();
    BOOST_CHECK_EQUAL( 1, w2cs.getFromIJK( 4, 4, 2 ).complnum() );
    BOOST_CHECK_EQUAL( 2, w2cs.getFromIJK( 4, 4, 0 ).complnum() );
    BOOST_CHECK_EQUAL( 3, w2cs.getFromIJK( 4, 4, 1 ).complnum() );
    BOOST_CHECK_THROW( w2cs.get( 3 ).complnum(), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_records_same_completion ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        SCHEDULE
        DATES             -- 1
            10  OKT 2008 /
        /
        WELSPECS
            'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
        /

        COMPDAT
            'W1' 0 0 1 2 'SHUT' 1*    / -- multiple completion (1, 2)
            'W1' 0 0 2 2 'SHUT' 1*    / -- updated completion (2)
            'W1' 0 0 3 3 'SHUT' 1*    / -- regular completion (3)
        /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString(input, ctx);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& cs = schedule.getWell( "W1" )->getCompletions();
    BOOST_CHECK_EQUAL( 3U, cs.size() );
    BOOST_CHECK_EQUAL( 1, cs.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs.get( 2 ).complnum() );
}

BOOST_AUTO_TEST_CASE( complump_less_than_1 ) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE

            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            COMPDAT
                'W1' 0 0 1 2 'SHUT' 1*    /
            /

            COMPLUMP
                'W1' 0 0 0 0 0 /
            /
    )";

    ParseContext ctx;
    auto deck = Parser().parseString( input, ctx );
    EclipseGrid grid( 10, 10, 10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Phases p( true, true, true );
    BOOST_CHECK_THROW( Schedule( deck , grid, eclipseProperties, p , ctx), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE( complump ) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE

            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            COMPDAT
                'W1' 0 0 1 2 'SHUT' 1*    /
                'W1' 0 0 2 3 'SHUT' 1*    /
                'W1' 0 0 4 6 'SHUT' 1*    /
                'W2' 0 0 3 4 'SHUT' 1*    /
                'W2' 0 0 1 4 'SHUT' 1*    /
            /

            COMPLUMP
                -- name I J K1 K2 C
                -- where C is the completion number of this lump
                'W1' 0 0 1 3 1 /
            /

            DATES             -- 1
             10  OKT 2008 /
            /

            WELOPEN
                'W1' 'OPEN' 0 0 0 1 1 /
            /
    )";

    constexpr auto open = WellCompletion::StateEnum::OPEN;
    constexpr auto shut = WellCompletion::StateEnum::SHUT;

    ParseContext ctx;
    auto deck = Parser().parseString(input, ctx);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& well = *schedule.getWell( "W1" );
    const auto& sc0  = well.getCompletions( 0 );

    /* complnum should be modified by COMPLNUM */
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 0 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 1 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, sc0.getFromIJK( 2, 2, 3 ).complnum() );

    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 0 ).getState() );
    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 1 ).getState() );
    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 2 ).getState() );

    const auto& sc1  = well.getCompletions( 1 );
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 0 ).getState() );
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 1 ).getState() );
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 2 ).getState() );
    BOOST_CHECK_EQUAL( shut, sc1.getFromIJK( 2, 2, 3 ).getState() );
}

BOOST_AUTO_TEST_CASE( COMPLUMP_specific_coordinates ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        SCHEDULE

        WELSPECS
            'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
        /

        COMPDAT                         -- completion number
            'W1' 1 1 1 1 'SHUT' 1*    / -- 1
            'W1' 1 1 2 2 'SHUT' 1*    / -- 2
            'W1' 0 0 1 2 'SHUT' 1*    / -- 3, 4
            'W1' 0 0 2 3 'SHUT' 1*    / -- 5
            'W1' 2 2 1 1 'SHUT' 1*    / -- 6
            'W1' 2 2 4 6 'SHUT' 1*    / -- 7, 8, 9
        /

        DATES             -- 1
            10  OKT 2008 /
        /

        WELOPEN -- open completion 1, rest are still shut
            'W1' OPEN 1 1 0 1 1 /
        /

        DATES             -- 2
            15  OKT 2008 /
        /

        COMPLUMP
            -- name I J K1 K2 C
            -- where C is the completion number of this lump
            'W1' 0 0 2 3 2 / -- all with k = [2 <= k <= 3] -> {2, 4, 5}
            'W1' 2 2 1 5 7 / -- fix'd i,j, k = [1 <= k <= 5] -> {6, 7, 8}
        /

        WELOPEN
            'W1' OPEN 0 0 0 2 2 / -- open the new 2 {2, 4, 5}
            'W1' OPEN 0 0 0 5 7 / -- open 5..7 {5, 6, 7, 8}
        /
    )";

    constexpr auto open = WellCompletion::StateEnum::OPEN;
    constexpr auto shut = WellCompletion::StateEnum::SHUT;

    ParseContext ctx;
    auto deck = Parser().parseString( input, ctx );
    EclipseGrid grid( 10, 10, 10 );
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Schedule schedule( deck, grid, eclipseProperties, Phases( true, true, true )  ,ctx);

    const auto& well = *schedule.getWell( "W1" );
    const auto& cs1 = well.getCompletions( 1 );
    const auto& cs2 = well.getCompletions( 2 );

    BOOST_CHECK_EQUAL( 9U, cs1.size() );
    BOOST_CHECK_EQUAL( open, cs1.getFromIJK( 0, 0, 0 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 0, 0, 1 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 0 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 1 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 2 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 0 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 3 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 4 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 5 ).getState() );

    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 0, 0, 0 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 0, 0, 1 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs2.getFromIJK( 2, 2, 0 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 2, 2, 1 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 2, 2, 2 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 0 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 3 ).getState() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 4 ).getState() );
    BOOST_CHECK_EQUAL( shut, cs2.getFromIJK( 1, 1, 5 ).getState() );
}

BOOST_AUTO_TEST_CASE(TestCompletionStateEnum2String) {
    BOOST_CHECK_EQUAL( "AUTO" , WellCompletion::StateEnum2String(WellCompletion::AUTO));
    BOOST_CHECK_EQUAL( "OPEN" , WellCompletion::StateEnum2String(WellCompletion::OPEN));
    BOOST_CHECK_EQUAL( "SHUT" , WellCompletion::StateEnum2String(WellCompletion::SHUT));
}


BOOST_AUTO_TEST_CASE(TestCompletionStateEnumFromString) {
    BOOST_CHECK_THROW( WellCompletion::StateEnumFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( WellCompletion::AUTO , WellCompletion::StateEnumFromString("AUTO"));
    BOOST_CHECK_EQUAL( WellCompletion::SHUT , WellCompletion::StateEnumFromString("SHUT"));
    BOOST_CHECK_EQUAL( WellCompletion::SHUT , WellCompletion::StateEnumFromString("STOP"));
    BOOST_CHECK_EQUAL( WellCompletion::OPEN , WellCompletion::StateEnumFromString("OPEN"));
}



BOOST_AUTO_TEST_CASE(TestCompletionStateEnumLoop) {
    BOOST_CHECK_EQUAL( WellCompletion::AUTO , WellCompletion::StateEnumFromString( WellCompletion::StateEnum2String( WellCompletion::AUTO ) ));
    BOOST_CHECK_EQUAL( WellCompletion::SHUT , WellCompletion::StateEnumFromString( WellCompletion::StateEnum2String( WellCompletion::SHUT ) ));
    BOOST_CHECK_EQUAL( WellCompletion::OPEN , WellCompletion::StateEnumFromString( WellCompletion::StateEnum2String( WellCompletion::OPEN ) ));

    BOOST_CHECK_EQUAL( "AUTO" , WellCompletion::StateEnum2String(WellCompletion::StateEnumFromString(  "AUTO" ) ));
    BOOST_CHECK_EQUAL( "OPEN" , WellCompletion::StateEnum2String(WellCompletion::StateEnumFromString(  "OPEN" ) ));
    BOOST_CHECK_EQUAL( "SHUT" , WellCompletion::StateEnum2String(WellCompletion::StateEnumFromString(  "SHUT" ) ));
}


/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestCompletionDirectionEnum2String)
{
    using namespace WellCompletion;

    BOOST_CHECK_EQUAL("X", DirectionEnum2String(DirectionEnum::X));
    BOOST_CHECK_EQUAL("Y", DirectionEnum2String(DirectionEnum::Y));
    BOOST_CHECK_EQUAL("Z", DirectionEnum2String(DirectionEnum::Z));
}

BOOST_AUTO_TEST_CASE(TestCompletionDirectionEnumFromString)
{
    using namespace WellCompletion;

    BOOST_CHECK_THROW(DirectionEnumFromString("XXX"), std::invalid_argument);

    BOOST_CHECK_EQUAL(DirectionEnum::X, DirectionEnumFromString("X"));
    BOOST_CHECK_EQUAL(DirectionEnum::Y, DirectionEnumFromString("Y"));
    BOOST_CHECK_EQUAL(DirectionEnum::Z, DirectionEnumFromString("Z"));
}

BOOST_AUTO_TEST_CASE(TestCompletionDirectionEnumLoop)
{
    using namespace WellCompletion;

    BOOST_CHECK_EQUAL(DirectionEnum::X, DirectionEnumFromString(DirectionEnum2String(DirectionEnum::X)));
    BOOST_CHECK_EQUAL(DirectionEnum::Y, DirectionEnumFromString(DirectionEnum2String(DirectionEnum::Y)));
    BOOST_CHECK_EQUAL(DirectionEnum::Z, DirectionEnumFromString(DirectionEnum2String(DirectionEnum::Z)));

    BOOST_CHECK_EQUAL("X", DirectionEnum2String(DirectionEnumFromString("X")));
    BOOST_CHECK_EQUAL("Y", DirectionEnum2String(DirectionEnumFromString("Y")));
    BOOST_CHECK_EQUAL("Z", DirectionEnum2String(DirectionEnumFromString("Z")));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE" , GroupInjection::ControlEnum2String(GroupInjection::NONE));
    BOOST_CHECK_EQUAL( "RATE" , GroupInjection::ControlEnum2String(GroupInjection::RATE));
    BOOST_CHECK_EQUAL( "RESV" , GroupInjection::ControlEnum2String(GroupInjection::RESV));
    BOOST_CHECK_EQUAL( "REIN" , GroupInjection::ControlEnum2String(GroupInjection::REIN));
    BOOST_CHECK_EQUAL( "VREP" , GroupInjection::ControlEnum2String(GroupInjection::VREP));
    BOOST_CHECK_EQUAL( "FLD"  , GroupInjection::ControlEnum2String(GroupInjection::FLD));
}


BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnumFromString) {
    BOOST_CHECK_THROW( GroupInjection::ControlEnumFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( GroupInjection::NONE , GroupInjection::ControlEnumFromString("NONE"));
    BOOST_CHECK_EQUAL( GroupInjection::RATE , GroupInjection::ControlEnumFromString("RATE"));
    BOOST_CHECK_EQUAL( GroupInjection::RESV , GroupInjection::ControlEnumFromString("RESV"));
    BOOST_CHECK_EQUAL( GroupInjection::REIN , GroupInjection::ControlEnumFromString("REIN"));
    BOOST_CHECK_EQUAL( GroupInjection::VREP , GroupInjection::ControlEnumFromString("VREP"));
    BOOST_CHECK_EQUAL( GroupInjection::FLD  , GroupInjection::ControlEnumFromString("FLD"));
}



BOOST_AUTO_TEST_CASE(TestGroupInjectionControlEnumLoop) {
    BOOST_CHECK_EQUAL( GroupInjection::NONE , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::NONE ) ));
    BOOST_CHECK_EQUAL( GroupInjection::RATE , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::RATE ) ));
    BOOST_CHECK_EQUAL( GroupInjection::RESV , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::RESV ) ));
    BOOST_CHECK_EQUAL( GroupInjection::REIN , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::REIN ) ));
    BOOST_CHECK_EQUAL( GroupInjection::VREP , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::VREP ) ));
    BOOST_CHECK_EQUAL( GroupInjection::FLD  , GroupInjection::ControlEnumFromString( GroupInjection::ControlEnum2String( GroupInjection::FLD ) ));

    BOOST_CHECK_EQUAL( "NONE" , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL( "RATE" , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "RATE" ) ));
    BOOST_CHECK_EQUAL( "RESV" , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "RESV" ) ));
    BOOST_CHECK_EQUAL( "REIN" , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "REIN" ) ));
    BOOST_CHECK_EQUAL( "VREP" , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "VREP" ) ));
    BOOST_CHECK_EQUAL( "FLD"  , GroupInjection::ControlEnum2String(GroupInjection::ControlEnumFromString( "FLD"  ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE" , GroupProduction::ControlEnum2String(GroupProduction::NONE));
    BOOST_CHECK_EQUAL( "ORAT" , GroupProduction::ControlEnum2String(GroupProduction::ORAT));
    BOOST_CHECK_EQUAL( "WRAT" , GroupProduction::ControlEnum2String(GroupProduction::WRAT));
    BOOST_CHECK_EQUAL( "GRAT" , GroupProduction::ControlEnum2String(GroupProduction::GRAT));
    BOOST_CHECK_EQUAL( "LRAT" , GroupProduction::ControlEnum2String(GroupProduction::LRAT));
    BOOST_CHECK_EQUAL( "CRAT" , GroupProduction::ControlEnum2String(GroupProduction::CRAT));
    BOOST_CHECK_EQUAL( "RESV" , GroupProduction::ControlEnum2String(GroupProduction::RESV));
    BOOST_CHECK_EQUAL( "PRBL" , GroupProduction::ControlEnum2String(GroupProduction::PRBL));
}


BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnumFromString) {
    BOOST_CHECK_THROW( GroupProduction::ControlEnumFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL(GroupProduction::NONE  , GroupProduction::ControlEnumFromString("NONE"));
    BOOST_CHECK_EQUAL(GroupProduction::ORAT  , GroupProduction::ControlEnumFromString("ORAT"));
    BOOST_CHECK_EQUAL(GroupProduction::WRAT  , GroupProduction::ControlEnumFromString("WRAT"));
    BOOST_CHECK_EQUAL(GroupProduction::GRAT  , GroupProduction::ControlEnumFromString("GRAT"));
    BOOST_CHECK_EQUAL(GroupProduction::LRAT  , GroupProduction::ControlEnumFromString("LRAT"));
    BOOST_CHECK_EQUAL(GroupProduction::CRAT  , GroupProduction::ControlEnumFromString("CRAT"));
    BOOST_CHECK_EQUAL(GroupProduction::RESV  , GroupProduction::ControlEnumFromString("RESV"));
    BOOST_CHECK_EQUAL(GroupProduction::PRBL  , GroupProduction::ControlEnumFromString("PRBL"));
}



BOOST_AUTO_TEST_CASE(TestGroupProductionControlEnumLoop) {
    BOOST_CHECK_EQUAL( GroupProduction::NONE, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::NONE ) ));
    BOOST_CHECK_EQUAL( GroupProduction::ORAT, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::ORAT ) ));
    BOOST_CHECK_EQUAL( GroupProduction::WRAT, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::WRAT ) ));
    BOOST_CHECK_EQUAL( GroupProduction::GRAT, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::GRAT ) ));
    BOOST_CHECK_EQUAL( GroupProduction::LRAT, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::LRAT ) ));
    BOOST_CHECK_EQUAL( GroupProduction::CRAT, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::CRAT ) ));
    BOOST_CHECK_EQUAL( GroupProduction::RESV, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::RESV ) ));
    BOOST_CHECK_EQUAL( GroupProduction::PRBL, GroupProduction::ControlEnumFromString( GroupProduction::ControlEnum2String( GroupProduction::PRBL ) ));

    BOOST_CHECK_EQUAL( "NONE" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL( "ORAT" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "ORAT" ) ));
    BOOST_CHECK_EQUAL( "WRAT" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "WRAT" ) ));
    BOOST_CHECK_EQUAL( "GRAT" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "GRAT" ) ));
    BOOST_CHECK_EQUAL( "LRAT" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "LRAT" ) ));
    BOOST_CHECK_EQUAL( "CRAT" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "CRAT" ) ));
    BOOST_CHECK_EQUAL( "RESV" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "RESV" ) ));
    BOOST_CHECK_EQUAL( "PRBL" , GroupProduction::ControlEnum2String(GroupProduction::ControlEnumFromString( "PRBL" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitControlEnum2String) {
    BOOST_CHECK_EQUAL( "NONE"     , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::NONE));
    BOOST_CHECK_EQUAL( "CON"      , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::CON));
    BOOST_CHECK_EQUAL( "+CON"     , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::CON_PLUS));
    BOOST_CHECK_EQUAL( "WELL"     , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::WELL));
    BOOST_CHECK_EQUAL( "PLUG"     , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::PLUG));
    BOOST_CHECK_EQUAL( "RATE"     , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::RATE));
}


BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitActionEnumFromString) {
    BOOST_CHECK_THROW( GroupProductionExceedLimit::ActionEnumFromString("XXX") , std::invalid_argument );

    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::NONE     , GroupProductionExceedLimit::ActionEnumFromString("NONE"));
    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::CON      , GroupProductionExceedLimit::ActionEnumFromString("CON" ));
    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::CON_PLUS , GroupProductionExceedLimit::ActionEnumFromString("+CON"));
    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::WELL     , GroupProductionExceedLimit::ActionEnumFromString("WELL"));
    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::PLUG     , GroupProductionExceedLimit::ActionEnumFromString("PLUG"));
    BOOST_CHECK_EQUAL(GroupProductionExceedLimit::RATE     , GroupProductionExceedLimit::ActionEnumFromString("RATE"));
}



BOOST_AUTO_TEST_CASE(TestGroupProductionExceedLimitActionEnumLoop) {
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::NONE     , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::NONE     ) ));
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::CON      , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::CON      ) ));
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::CON_PLUS , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::CON_PLUS ) ));
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::WELL     , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::WELL     ) ));
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::PLUG     , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::PLUG     ) ));
    BOOST_CHECK_EQUAL( GroupProductionExceedLimit::RATE     , GroupProductionExceedLimit::ActionEnumFromString( GroupProductionExceedLimit::ActionEnum2String( GroupProductionExceedLimit::RATE     ) ));

    BOOST_CHECK_EQUAL("NONE" , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "NONE" ) ));
    BOOST_CHECK_EQUAL("CON"  , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "CON"  ) ));
    BOOST_CHECK_EQUAL("+CON" , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "+CON" ) ));
    BOOST_CHECK_EQUAL("WELL" , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "WELL" ) ));
    BOOST_CHECK_EQUAL("PLUG" , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "PLUG" ) ));
    BOOST_CHECK_EQUAL("RATE" , GroupProductionExceedLimit::ActionEnum2String(GroupProductionExceedLimit::ActionEnumFromString( "RATE" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(TestInjectorEnum2String) {
    BOOST_CHECK_EQUAL( "OIL"  ,  WellInjector::Type2String(WellInjector::OIL));
    BOOST_CHECK_EQUAL( "GAS"  ,  WellInjector::Type2String(WellInjector::GAS));
    BOOST_CHECK_EQUAL( "WATER" , WellInjector::Type2String(WellInjector::WATER));
    BOOST_CHECK_EQUAL( "MULTI" , WellInjector::Type2String(WellInjector::MULTI));
}


BOOST_AUTO_TEST_CASE(TestInjectorEnumFromString) {
    BOOST_CHECK_THROW( WellInjector::TypeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( WellInjector::OIL   , WellInjector::TypeFromString("OIL"));
    BOOST_CHECK_EQUAL( WellInjector::WATER , WellInjector::TypeFromString("WATER"));
    BOOST_CHECK_EQUAL( WellInjector::WATER , WellInjector::TypeFromString("WAT"));
    BOOST_CHECK_EQUAL( WellInjector::GAS   , WellInjector::TypeFromString("GAS"));
    BOOST_CHECK_EQUAL( WellInjector::MULTI , WellInjector::TypeFromString("MULTI"));
}



BOOST_AUTO_TEST_CASE(TestInjectorEnumLoop) {
    BOOST_CHECK_EQUAL( WellInjector::OIL     , WellInjector::TypeFromString( WellInjector::Type2String( WellInjector::OIL ) ));
    BOOST_CHECK_EQUAL( WellInjector::WATER   , WellInjector::TypeFromString( WellInjector::Type2String( WellInjector::WATER ) ));
    BOOST_CHECK_EQUAL( WellInjector::GAS     , WellInjector::TypeFromString( WellInjector::Type2String( WellInjector::GAS ) ));
    BOOST_CHECK_EQUAL( WellInjector::MULTI   , WellInjector::TypeFromString( WellInjector::Type2String( WellInjector::MULTI ) ));

    BOOST_CHECK_EQUAL( "MULTI"    , WellInjector::Type2String(WellInjector::TypeFromString(  "MULTI" ) ));
    BOOST_CHECK_EQUAL( "OIL"      , WellInjector::Type2String(WellInjector::TypeFromString(  "OIL" ) ));
    BOOST_CHECK_EQUAL( "GAS"      , WellInjector::Type2String(WellInjector::TypeFromString(  "GAS" ) ));
    BOOST_CHECK_EQUAL( "WATER"    , WellInjector::Type2String(WellInjector::TypeFromString(  "WATER" ) ));
}

/*****************************************************************/

BOOST_AUTO_TEST_CASE(InjectorCOntrolMopdeEnum2String) {
    BOOST_CHECK_EQUAL( "RATE"  ,  WellInjector::ControlMode2String(WellInjector::RATE));
    BOOST_CHECK_EQUAL( "RESV"  ,  WellInjector::ControlMode2String(WellInjector::RESV));
    BOOST_CHECK_EQUAL( "BHP" , WellInjector::ControlMode2String(WellInjector::BHP));
    BOOST_CHECK_EQUAL( "THP" , WellInjector::ControlMode2String(WellInjector::THP));
    BOOST_CHECK_EQUAL( "GRUP" , WellInjector::ControlMode2String(WellInjector::GRUP));
}


BOOST_AUTO_TEST_CASE(InjectorControlModeEnumFromString) {
    BOOST_CHECK_THROW( WellInjector::ControlModeFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( WellInjector::RATE   , WellInjector::ControlModeFromString("RATE"));
    BOOST_CHECK_EQUAL( WellInjector::BHP , WellInjector::ControlModeFromString("BHP"));
    BOOST_CHECK_EQUAL( WellInjector::RESV   , WellInjector::ControlModeFromString("RESV"));
    BOOST_CHECK_EQUAL( WellInjector::THP , WellInjector::ControlModeFromString("THP"));
    BOOST_CHECK_EQUAL( WellInjector::GRUP , WellInjector::ControlModeFromString("GRUP"));
}



BOOST_AUTO_TEST_CASE(InjectorControlModeEnumLoop) {
    BOOST_CHECK_EQUAL( WellInjector::RATE     , WellInjector::ControlModeFromString( WellInjector::ControlMode2String( WellInjector::RATE ) ));
    BOOST_CHECK_EQUAL( WellInjector::BHP   , WellInjector::ControlModeFromString( WellInjector::ControlMode2String( WellInjector::BHP ) ));
    BOOST_CHECK_EQUAL( WellInjector::RESV     , WellInjector::ControlModeFromString( WellInjector::ControlMode2String( WellInjector::RESV ) ));
    BOOST_CHECK_EQUAL( WellInjector::THP   , WellInjector::ControlModeFromString( WellInjector::ControlMode2String( WellInjector::THP ) ));
    BOOST_CHECK_EQUAL( WellInjector::GRUP   , WellInjector::ControlModeFromString( WellInjector::ControlMode2String( WellInjector::GRUP ) ));

    BOOST_CHECK_EQUAL( "THP"    , WellInjector::ControlMode2String(WellInjector::ControlModeFromString(  "THP" ) ));
    BOOST_CHECK_EQUAL( "RATE"      , WellInjector::ControlMode2String(WellInjector::ControlModeFromString(  "RATE" ) ));
    BOOST_CHECK_EQUAL( "RESV"      , WellInjector::ControlMode2String(WellInjector::ControlModeFromString(  "RESV" ) ));
    BOOST_CHECK_EQUAL( "BHP"    , WellInjector::ControlMode2String(WellInjector::ControlModeFromString(  "BHP" ) ));
    BOOST_CHECK_EQUAL( "GRUP"    , WellInjector::ControlMode2String(WellInjector::ControlModeFromString(  "GRUP" ) ));
}



/*****************************************************************/

BOOST_AUTO_TEST_CASE(InjectorStatusEnum2String) {
    BOOST_CHECK_EQUAL( "OPEN"  ,  WellCommon::Status2String(WellCommon::OPEN));
    BOOST_CHECK_EQUAL( "SHUT"  ,  WellCommon::Status2String(WellCommon::SHUT));
    BOOST_CHECK_EQUAL( "AUTO"   ,  WellCommon::Status2String(WellCommon::AUTO));
    BOOST_CHECK_EQUAL( "STOP"   ,  WellCommon::Status2String(WellCommon::STOP));
}


BOOST_AUTO_TEST_CASE(InjectorStatusEnumFromString) {
    BOOST_CHECK_THROW( WellCommon::StatusFromString("XXX") , std::invalid_argument );
    BOOST_CHECK_EQUAL( WellCommon::OPEN   , WellCommon::StatusFromString("OPEN"));
    BOOST_CHECK_EQUAL( WellCommon::AUTO , WellCommon::StatusFromString("AUTO"));
    BOOST_CHECK_EQUAL( WellCommon::SHUT   , WellCommon::StatusFromString("SHUT"));
    BOOST_CHECK_EQUAL( WellCommon::STOP , WellCommon::StatusFromString("STOP"));
}



BOOST_AUTO_TEST_CASE(InjectorStatusEnumLoop) {
    BOOST_CHECK_EQUAL( WellCommon::OPEN     , WellCommon::StatusFromString( WellCommon::Status2String( WellCommon::OPEN ) ));
    BOOST_CHECK_EQUAL( WellCommon::AUTO   , WellCommon::StatusFromString( WellCommon::Status2String( WellCommon::AUTO ) ));
    BOOST_CHECK_EQUAL( WellCommon::SHUT     , WellCommon::StatusFromString( WellCommon::Status2String( WellCommon::SHUT ) ));
    BOOST_CHECK_EQUAL( WellCommon::STOP   , WellCommon::StatusFromString( WellCommon::Status2String( WellCommon::STOP ) ));

    BOOST_CHECK_EQUAL( "STOP"    , WellCommon::Status2String(WellCommon::StatusFromString(  "STOP" ) ));
    BOOST_CHECK_EQUAL( "OPEN"      , WellCommon::Status2String(WellCommon::StatusFromString(  "OPEN" ) ));
    BOOST_CHECK_EQUAL( "SHUT"      , WellCommon::Status2String(WellCommon::StatusFromString(  "SHUT" ) ));
    BOOST_CHECK_EQUAL( "AUTO"    , WellCommon::Status2String(WellCommon::StatusFromString(  "AUTO" ) ));
}



/*****************************************************************/

BOOST_AUTO_TEST_CASE(ProducerCOntrolMopdeEnum2String) {
    BOOST_CHECK_EQUAL( "ORAT"  ,  WellProducer::ControlMode2String(WellProducer::ORAT));
    BOOST_CHECK_EQUAL( "WRAT"  ,  WellProducer::ControlMode2String(WellProducer::WRAT));
    BOOST_CHECK_EQUAL( "GRAT"  , WellProducer::ControlMode2String(WellProducer::GRAT));
    BOOST_CHECK_EQUAL( "LRAT"  , WellProducer::ControlMode2String(WellProducer::LRAT));
    BOOST_CHECK_EQUAL( "CRAT"  , WellProducer::ControlMode2String(WellProducer::CRAT));
    BOOST_CHECK_EQUAL( "RESV"  ,  WellProducer::ControlMode2String(WellProducer::RESV));
    BOOST_CHECK_EQUAL( "BHP"   , WellProducer::ControlMode2String(WellProducer::BHP));
    BOOST_CHECK_EQUAL( "THP"   , WellProducer::ControlMode2String(WellProducer::THP));
    BOOST_CHECK_EQUAL( "GRUP"  , WellProducer::ControlMode2String(WellProducer::GRUP));
}


BOOST_AUTO_TEST_CASE(ProducerControlModeEnumFromString) {
    BOOST_CHECK_THROW( WellProducer::ControlModeFromString("XRAT") , std::invalid_argument );
    BOOST_CHECK_EQUAL( WellProducer::ORAT   , WellProducer::ControlModeFromString("ORAT"));
    BOOST_CHECK_EQUAL( WellProducer::WRAT   , WellProducer::ControlModeFromString("WRAT"));
    BOOST_CHECK_EQUAL( WellProducer::GRAT   , WellProducer::ControlModeFromString("GRAT"));
    BOOST_CHECK_EQUAL( WellProducer::LRAT   , WellProducer::ControlModeFromString("LRAT"));
    BOOST_CHECK_EQUAL( WellProducer::CRAT   , WellProducer::ControlModeFromString("CRAT"));
    BOOST_CHECK_EQUAL( WellProducer::RESV   , WellProducer::ControlModeFromString("RESV"));
    BOOST_CHECK_EQUAL( WellProducer::BHP    , WellProducer::ControlModeFromString("BHP" ));
    BOOST_CHECK_EQUAL( WellProducer::THP    , WellProducer::ControlModeFromString("THP" ));
    BOOST_CHECK_EQUAL( WellProducer::GRUP   , WellProducer::ControlModeFromString("GRUP"));
}



BOOST_AUTO_TEST_CASE(ProducerControlModeEnumLoop) {
    BOOST_CHECK_EQUAL( WellProducer::ORAT     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::ORAT ) ));
    BOOST_CHECK_EQUAL( WellProducer::WRAT     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::WRAT ) ));
    BOOST_CHECK_EQUAL( WellProducer::GRAT     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::GRAT ) ));
    BOOST_CHECK_EQUAL( WellProducer::LRAT     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::LRAT ) ));
    BOOST_CHECK_EQUAL( WellProducer::CRAT     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::CRAT ) ));
    BOOST_CHECK_EQUAL( WellProducer::RESV     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::RESV ) ));
    BOOST_CHECK_EQUAL( WellProducer::BHP      , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::BHP  ) ));
    BOOST_CHECK_EQUAL( WellProducer::THP      , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::THP  ) ));
    BOOST_CHECK_EQUAL( WellProducer::GRUP     , WellProducer::ControlModeFromString( WellProducer::ControlMode2String( WellProducer::GRUP ) ));

    BOOST_CHECK_EQUAL( "ORAT"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "ORAT"  ) ));
    BOOST_CHECK_EQUAL( "WRAT"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "WRAT"  ) ));
    BOOST_CHECK_EQUAL( "GRAT"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "GRAT"  ) ));
    BOOST_CHECK_EQUAL( "LRAT"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "LRAT"  ) ));
    BOOST_CHECK_EQUAL( "CRAT"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "CRAT"  ) ));
    BOOST_CHECK_EQUAL( "RESV"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "RESV"  ) ));
    BOOST_CHECK_EQUAL( "BHP"       , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "BHP"   ) ));
    BOOST_CHECK_EQUAL( "THP"       , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "THP"   ) ));
    BOOST_CHECK_EQUAL( "GRUP"      , WellProducer::ControlMode2String(WellProducer::ControlModeFromString( "GRUP"  ) ));
}

/*******************************************************************/
/*****************************************************************/

BOOST_AUTO_TEST_CASE(GuideRatePhaseEnum2String) {
    BOOST_CHECK_EQUAL( "OIL"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::OIL));
    BOOST_CHECK_EQUAL( "WAT"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::WAT));
    BOOST_CHECK_EQUAL( "GAS"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::GAS));
    BOOST_CHECK_EQUAL( "LIQ"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::LIQ));
    BOOST_CHECK_EQUAL( "COMB"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::COMB));
    BOOST_CHECK_EQUAL( "WGA"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::WGA));
    BOOST_CHECK_EQUAL( "CVAL"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::CVAL));
    BOOST_CHECK_EQUAL( "RAT"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::RAT));
    BOOST_CHECK_EQUAL( "RES"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::RES));
    BOOST_CHECK_EQUAL( "UNDEFINED"  ,  GuideRate::GuideRatePhaseEnum2String(GuideRate::UNDEFINED));
}


BOOST_AUTO_TEST_CASE(GuideRatePhaseEnumFromString) {
    BOOST_CHECK_THROW( GuideRate::GuideRatePhaseEnumFromString("XRAT") , std::invalid_argument );
    BOOST_CHECK_EQUAL( GuideRate::OIL   , GuideRate::GuideRatePhaseEnumFromString("OIL"));
    BOOST_CHECK_EQUAL( GuideRate::WAT   , GuideRate::GuideRatePhaseEnumFromString("WAT"));
    BOOST_CHECK_EQUAL( GuideRate::GAS   , GuideRate::GuideRatePhaseEnumFromString("GAS"));
    BOOST_CHECK_EQUAL( GuideRate::LIQ   , GuideRate::GuideRatePhaseEnumFromString("LIQ"));
    BOOST_CHECK_EQUAL( GuideRate::COMB   , GuideRate::GuideRatePhaseEnumFromString("COMB"));
    BOOST_CHECK_EQUAL( GuideRate::WGA   , GuideRate::GuideRatePhaseEnumFromString("WGA"));
    BOOST_CHECK_EQUAL( GuideRate::CVAL   , GuideRate::GuideRatePhaseEnumFromString("CVAL"));
    BOOST_CHECK_EQUAL( GuideRate::RAT   , GuideRate::GuideRatePhaseEnumFromString("RAT"));
    BOOST_CHECK_EQUAL( GuideRate::RES   , GuideRate::GuideRatePhaseEnumFromString("RES"));
    BOOST_CHECK_EQUAL( GuideRate::UNDEFINED, GuideRate::GuideRatePhaseEnumFromString("UNDEFINED"));
}



BOOST_AUTO_TEST_CASE(GuideRatePhaseEnum2Loop) {
    BOOST_CHECK_EQUAL( GuideRate::OIL     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::OIL ) ));
    BOOST_CHECK_EQUAL( GuideRate::WAT     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::WAT ) ));
    BOOST_CHECK_EQUAL( GuideRate::GAS     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::GAS ) ));
    BOOST_CHECK_EQUAL( GuideRate::LIQ     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::LIQ ) ));
    BOOST_CHECK_EQUAL( GuideRate::COMB     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::COMB ) ));
    BOOST_CHECK_EQUAL( GuideRate::WGA     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::WGA ) ));
    BOOST_CHECK_EQUAL( GuideRate::CVAL     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::CVAL ) ));
    BOOST_CHECK_EQUAL( GuideRate::RAT     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::RAT ) ));
    BOOST_CHECK_EQUAL( GuideRate::RES     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::RES ) ));
    BOOST_CHECK_EQUAL( GuideRate::UNDEFINED     , GuideRate::GuideRatePhaseEnumFromString( GuideRate::GuideRatePhaseEnum2String( GuideRate::UNDEFINED ) ));

    BOOST_CHECK_EQUAL( "OIL"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "OIL"  ) ));
    BOOST_CHECK_EQUAL( "WAT"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "WAT"  ) ));
    BOOST_CHECK_EQUAL( "GAS"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "GAS"  ) ));
    BOOST_CHECK_EQUAL( "LIQ"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "LIQ"  ) ));
    BOOST_CHECK_EQUAL( "COMB"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "COMB"  ) ));
    BOOST_CHECK_EQUAL( "WGA"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "WGA"  ) ));
    BOOST_CHECK_EQUAL( "CVAL"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "CVAL"  ) ));
    BOOST_CHECK_EQUAL( "RAT"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "RAT"  ) ));
    BOOST_CHECK_EQUAL( "RES"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "RES"  ) ));
    BOOST_CHECK_EQUAL( "UNDEFINED"      , GuideRate::GuideRatePhaseEnum2String(GuideRate::GuideRatePhaseEnumFromString( "UNDEFINED"  ) ));

}
