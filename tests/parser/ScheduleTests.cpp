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

#include <ert/util/util.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RFTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include "src/opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp"
#include "src/opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp"

using namespace Opm;

static Deck createDeck() {
    Opm::Parser parser;
    std::string input =
        "START\n"
        "8 MAR 1998 /\n"
        "\n"
        "SCHEDULE\n"
        "\n";

    return parser.parseString(input);
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

    return parser.parseString(input);
}

static Deck createDeckWTEST() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "WELSPECS\n"
            "     \'DEFAULT\'    \'OP\'   30   37  3.33       \'OIL\'  7*/   \n"
            "     \'ALLOW\'      \'OP\'   30   37  3.33       \'OIL\'  3*  YES / \n"
            "     \'BAN\'        \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "     \'W1\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "     \'W2\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "     \'W3\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "/\n"

            "COMPDAT\n"
            " \'BAN\'  1  1   1   1 \'OPEN\' 1*    1.168   0.311   107.872 1*  1*  \'Z\'  21.925 / \n"
            "/\n"

            "WCONHIST\n"
            "     'BAN'      'OPEN'      'RESV'      0.000      0.000      0.000  5* / \n"
            "/\n"

            "WTEST\n"
            "   'ALLOW'   1   'PE' / \n"
            "/\n"

            "DATES             -- 1\n"
            " 10  JUN 2007 / \n"
            "/\n"

            "WTEST\n"
            "   'ALLOW'  1  '' / \n"
            "   'BAN'    1  'DGC' / \n"
            "/\n"

            "WCONHIST\n"
            "     'BAN'      'OPEN'      'RESV'      1.000      0.000      0.000  5* / \n"
            "/\n"

            "DATES             -- 2\n"
            " 10  JUL 2007 / \n"
            "/\n"

            "WELSPECS\n"
            "     \'I1\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "     \'I2\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "     \'I3\'         \'OP\'   20   51  3.92       \'OIL\'  3*  NO /  \n"
            "/\n"

            "WLIST\n"
            "  \'*ILIST\'  \'NEW\'  I1 /\n"
            "  \'*ILIST\'  \'ADD\'  I2 /\n"
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


    return parser.parseString(input);
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


    return parser.parseString(input);
}

static Deck createDeckWithWellsOrdered() {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "10 MAI 2007 / \n"
            "SCHEDULE\n"
            "WELSPECS\n"
            "     \'CW_1\'        \'CG\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'BW_2\'        \'BG\'   30   37  3.33       \'OIL\'  7* /   \n"
            "     \'AW_3\'        \'AG\'   20   51  3.92       \'OIL\'  7* /  \n"
            "/\n";

    return parser.parseString(input);
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

    return parser.parseString(input);
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

    return parser.parseString(input);
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckMissingReturnsDefaults) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec );
    BOOST_CHECK_EQUAL( schedule.getStartTime() , TimeMap::mkdate(1983, 1 , 1));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrdered) {
    auto deck = createDeckWithWellsOrdered();
    EclipseGrid grid(100,100,100);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    auto well_names = schedule.wellNames();

    BOOST_CHECK_EQUAL( "CW_1" , well_names[0]);
    BOOST_CHECK_EQUAL( "BW_2" , well_names[1]);
    BOOST_CHECK_EQUAL( "AW_3" , well_names[2]);

    auto groups = schedule.getGroups();
    // groups[0] is 'FIELD'
    BOOST_CHECK_EQUAL( "CG", groups[1]->name());
    BOOST_CHECK_EQUAL( "BG", groups[2]->name());
    BOOST_CHECK_EQUAL( "AG", groups[3]->name());
}


bool has_well( const std::vector<Well2>& wells, const std::string& well_name) {
    for (const auto& well : wells )
        if (well.name( ) == well_name)
            return true;
    return false;
}


BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsOrderedGRUPTREE) {
    auto deck = createDeckWithWellsOrderedGRUPTREE();
    EclipseGrid grid(100,100,100);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    BOOST_CHECK_THROW( schedule.getChildWells2( "NO_SUCH_GROUP" , 1 , GroupWellQueryMode::Recursive), std::invalid_argument);
    {
        auto field_wells = schedule.getChildWells2("FIELD" , 0, GroupWellQueryMode::Recursive);
        BOOST_CHECK_EQUAL( field_wells.size() , 4U);

        BOOST_CHECK( has_well( field_wells, "DW_0" ));
        BOOST_CHECK( has_well( field_wells, "CW_1" ));
        BOOST_CHECK( has_well( field_wells, "BW_2" ));
        BOOST_CHECK( has_well( field_wells, "AW_3" ));
    }

    {
        auto platform_wells = schedule.getChildWells2("PLATFORM" , 0, GroupWellQueryMode::Recursive);
        BOOST_CHECK_EQUAL( platform_wells.size() , 4U);

        BOOST_CHECK( has_well( platform_wells, "DW_0" ));
        BOOST_CHECK( has_well( platform_wells, "CW_1" ));
        BOOST_CHECK( has_well( platform_wells, "BW_2" ));
        BOOST_CHECK( has_well( platform_wells, "AW_3" ));
    }

    {
        auto child_wells1 = schedule.getChildWells2("CG1" , 0, GroupWellQueryMode::Recursive);
        BOOST_CHECK_EQUAL( child_wells1.size() , 2U);

        BOOST_CHECK( has_well( child_wells1, "DW_0" ));
        BOOST_CHECK( has_well( child_wells1, "CW_1" ));
    }

    {
        auto parent_wells2 = schedule.getChildWells2("PG2" , 0, GroupWellQueryMode::Recursive);
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
    Runspec runspec (deck);

    Schedule schedule(deck, grid , eclipseProperties, runspec);
    BOOST_CHECK_EQUAL( schedule.getStartTime() , TimeMap::mkdate(1998, 3  , 8 ));
}

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWithSCHEDULENoThrow) {
    Deck deck;
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );
    Runspec runspec (deck);

    BOOST_CHECK_NO_THROW( Schedule( deck, grid , eclipseProperties, runspec));
}

BOOST_AUTO_TEST_CASE(EmptyScheduleHasNoWells) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    BOOST_CHECK_EQUAL( 0U , schedule.numWells() );
    BOOST_CHECK_EQUAL( false , schedule.hasWell("WELL1") );
    BOOST_CHECK_THROW( schedule.getWell2("WELL2", 0) , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(CreateSchedule_DeckWithoutGRUPTREE_HasRootGroupTreeNodeForTimeStepZero) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeck();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
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
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    BOOST_CHECK( schedule.getGroupTree( 0 ).exists( "FIELD" ) );
    BOOST_CHECK( schedule.getGroupTree( 0 ).exists( "FAREN" ) );
    BOOST_CHECK_EQUAL( "FAREN", schedule.getGroupTree( 0 ).parent( "BARNET" ) );
}

BOOST_AUTO_TEST_CASE(GetGroups) {
    auto deck = deckWithGRUPTREE();
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck , grid , eclipseProperties, runspec);

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
    Runspec runspec (deck);
    Schedule schedule(deck , grid , eclipseProperties, runspec);

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
    Runspec runspec (deck);
    Schedule schedule(deck , grid , eclipseProperties, runspec);

    const auto wells_alltimesteps = schedule.getWells2atEnd();
    BOOST_CHECK_EQUAL(0U, wells_alltimesteps.size());

    const auto wells_t0 = schedule.getWells2(0);
    BOOST_CHECK_EQUAL(0U, wells_t0.size());

    // The time argument is beyond the length of the vector
    BOOST_CHECK_THROW(schedule.getWells2(1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(WellsIterator_HasWells_WellsReturned) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWells();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck , grid , eclipseProperties, runspec);
    size_t timeStep = 0;

    const auto wells_alltimesteps = schedule.getWells2atEnd();
    BOOST_CHECK_EQUAL(3U, wells_alltimesteps.size());
    const auto wells_t0 = schedule.getWells2(timeStep);
    BOOST_CHECK_EQUAL(1U, wells_t0.size());
    const auto wells_t3 = schedule.getWells2(3);
    BOOST_CHECK_EQUAL(3U, wells_t3.size());
}



BOOST_AUTO_TEST_CASE(ReturnNumWellsTimestep) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckWithWells();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    BOOST_CHECK_EQUAL(schedule.numWells(0), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(1), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(2), 1);
    BOOST_CHECK_EQUAL(schedule.numWells(3), 3);
}

BOOST_AUTO_TEST_CASE(TestCrossFlowHandling) {
    EclipseGrid grid(10,10,10);
    auto deck = createDeckForTestingCrossFlow();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    BOOST_CHECK_EQUAL(schedule.getWell2("BAN", 0).getAllowCrossFlow(), false);
    BOOST_CHECK_EQUAL(schedule.getWell2("ALLOW", 0).getAllowCrossFlow(), true);
    BOOST_CHECK_EQUAL(schedule.getWell2("DEFAULT", 0).getAllowCrossFlow(), true);
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, schedule.getWell2("BAN", 0).getStatus());
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, schedule.getWell2("BAN", 1).getStatus());
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, schedule.getWell2("BAN", 2).getStatus());
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, schedule.getWell2("BAN", 3).getStatus());
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, schedule.getWell2("BAN", 4).getStatus()); // not allow to open
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, schedule.getWell2("BAN", 5).getStatus());
}

static Deck createDeckWithWellsAndConnectionDataWithWELOPEN() {
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

BOOST_AUTO_TEST_CASE(CreateScheduleDeckWellsAndConnectionDataWithWELOPEN) {
    EclipseGrid grid(10,10,10);
  auto deck = createDeckWithWellsAndConnectionDataWithWELOPEN();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck ,grid , eclipseProperties, runspec);
    {
        constexpr auto well_shut = WellCommon::StatusEnum::SHUT;
        constexpr auto well_open = WellCommon::StatusEnum::OPEN;

        BOOST_CHECK_EQUAL(well_shut, schedule.getWell2("OP_1", 3).getStatus(  ));
        BOOST_CHECK_EQUAL(well_open, schedule.getWell2("OP_1", 4).getStatus(  ));
        BOOST_CHECK_EQUAL(well_shut, schedule.getWell2("OP_1", 5).getStatus(  ));
    }
    {
        constexpr auto comp_shut = WellCompletion::StateEnum::SHUT;
        constexpr auto comp_open = WellCompletion::StateEnum::OPEN;
        {
            const auto& well = schedule.getWell2("OP_2", 3);
            const auto& cs = well.getConnections( );

            BOOST_CHECK_EQUAL( 7U, cs.size() );
            BOOST_CHECK_EQUAL(comp_shut, cs.getFromIJK( 7, 6, 2 ).state());
            BOOST_CHECK_EQUAL(comp_shut, cs.getFromIJK( 7, 6, 3 ).state());
            BOOST_CHECK_EQUAL(comp_shut, cs.getFromIJK( 7, 6, 4 ).state());
            BOOST_CHECK_EQUAL(comp_open, cs.getFromIJK( 7, 7, 2 ).state());
        }
        {
            const auto& well = schedule.getWell2("OP_2", 4);
            const auto& cs2 = well.getConnections( );
            BOOST_CHECK_EQUAL(comp_open, cs2.getFromIJK( 7, 6, 2 ).state());
            BOOST_CHECK_EQUAL(comp_open, cs2.getFromIJK( 7, 6, 3 ).state());
            BOOST_CHECK_EQUAL(comp_open, cs2.getFromIJK( 7, 6, 4 ).state());
            BOOST_CHECK_EQUAL(comp_open, cs2.getFromIJK( 7, 7, 2 ).state());
        }
        {
            const auto& well = schedule.getWell2("OP_3", 3);
            const auto& cs3 = well.getConnections(  );
            BOOST_CHECK_EQUAL(comp_shut, cs3.get( 0 ).state());
        }
        {
            const auto& well = schedule.getWell2("OP_3", 4);
            const auto& cs4 = well.getConnections(  );
            BOOST_CHECK_EQUAL(comp_open, cs4.get( 0 ).state());
        }
    }
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
    auto deck = parser.parseString(input);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck ,  grid , eclipseProperties, runspec);
    const auto& well2_3 = schedule.getWell2("OP_1",3);
    const auto& well2_4 = schedule.getWell2("OP_1",4);
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well2_3.getStatus());
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well2_4.getStatus());
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
  auto deck = parser.parseString(input);
  TableManager table ( deck );
  Eclipse3DProperties eclipseProperties ( deck , table, grid);
  Runspec runspec (deck);
  Schedule schedule(deck, grid , eclipseProperties, runspec);
  const auto& well_3 = schedule.getWell2("OP_1", 3);
  const auto& well_4 = schedule.getWell2("OP_1", 4);
  const auto& well_5 = schedule.getWell2("OP_1", 5);
  // timestep 3. Close all completions with WELOPEN and immediately open new completions with COMPDAT.
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well_3.getStatus());
  BOOST_CHECK( !schedule.hasWellEvent( "OP_1", ScheduleEvents::WELL_STATUS_CHANGE , 3 ));
  // timestep 4. Close all completions with WELOPEN. The well will be shut since no completions
  // are open.
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well_4.getStatus());
  BOOST_CHECK( schedule.hasWellEvent( "OP_1", ScheduleEvents::WELL_STATUS_CHANGE , 4 ));
  // timestep 5. Open new completions. But keep the well shut,
  BOOST_CHECK_EQUAL(WellCommon::StatusEnum::SHUT, well_5.getStatus());
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
    auto deck = parser.parseString(input);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    const auto& rft_config = schedule.rftConfig();

    BOOST_CHECK_EQUAL(2 , rft_config.firstRFTOutput());
    BOOST_CHECK_EQUAL(true, rft_config.rft("OP_1", 2));
    BOOST_CHECK_EQUAL(true, rft_config.rft("OP_2", 3));
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
    EclipseGrid grid(10,10,10);
    auto deck = parser.parseString(input);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    const auto& well = schedule.getWell2("OP_1", 4);
    BOOST_CHECK_EQUAL(WellCommon::StatusEnum::OPEN, well.getStatus());

    const auto& rft_config = schedule.rftConfig();
    BOOST_CHECK_EQUAL(rft_config.rft("OP_1", 3),false);
    BOOST_CHECK_EQUAL(rft_config.rft("OP_1", 4),true);
    BOOST_CHECK_EQUAL(rft_config.rft("OP_1", 5),false);
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck,  grid , eclipseProperties, runspec);
    Opm::UnitSystem unitSystem = deck.getActiveUnitSystem();
    double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();
    double siFactorG = unitSystem.parse("GasSurfaceVolume/Time").getSIScaling();
    double siFactorP = unitSystem.parse("Pressure").getSIScaling();

    const auto& well_1 = schedule.getWell2("OP_1", 1);
    const auto wpp_1 = well_1.getProductionProperties();
    BOOST_CHECK_EQUAL(wpp_1.WaterRate, 0);

    const auto& well_2 = schedule.getWell2("OP_1", 2);
    const auto wpp_2 = well_2.getProductionProperties();
    BOOST_CHECK_EQUAL(wpp_2.OilRate, 1300 * siFactorL);
    BOOST_CHECK_EQUAL(wpp_2.WaterRate, 1400 * siFactorL);
    BOOST_CHECK_EQUAL(wpp_2.GasRate, 1500.52 * siFactorG);
    BOOST_CHECK_EQUAL(wpp_2.LiquidRate, 1600.58 * siFactorL);
    BOOST_CHECK_EQUAL(wpp_2.ResVRate, 1801.05 * siFactorL);
    BOOST_CHECK_EQUAL(wpp_2.BHPLimit, 1900 * siFactorP);
    BOOST_CHECK_EQUAL(wpp_2.THPLimit, 2000 * siFactorP);
    BOOST_CHECK_EQUAL(wpp_2.VFPTableNumber, 2100);
    BOOST_CHECK_EQUAL(well_2.getGuideRate(), 2300.14);
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);

    BOOST_CHECK_THROW(Schedule(deck, grid , eclipseProperties, runspec),
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    BOOST_CHECK_THROW(Schedule(deck, grid , eclipseProperties, runspec), std::out_of_range);
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    const auto& cs2 = schedule.getWell2("OP_1", 2).getConnections();
    const auto& cs3 = schedule.getWell2("OP_1", 3).getConnections();
    const auto& cs4 = schedule.getWell2("OP_1", 4).getConnections();
    for(size_t i = 0; i < cs2.size(); i++)
        BOOST_CHECK_EQUAL(cs2.get( i ).wellPi(), 1.3);

    for(size_t i = 0; i < cs3.size(); i++ )
        BOOST_CHECK_EQUAL(cs3.get( i ).wellPi(), (1.3*1.3));

    for(size_t i = 0; i < cs4.size(); i++ )
        BOOST_CHECK_EQUAL(cs4.get( i ).wellPi(), 1.0);

    for(size_t i = 0; i < cs4.size(); i++ )
        BOOST_CHECK_EQUAL(cs4.get( i ).wellPi(), 1.0);

    BOOST_CHECK_THROW(schedule.simTime(10000), std::invalid_argument);
    auto sim_time1 = schedule.simTime(1);
    int day, month,year;
    util_set_date_values_utc(sim_time1, &day, &month, &year);
    BOOST_CHECK_EQUAL(day, 10);
    BOOST_CHECK_EQUAL(month, 10);
    BOOST_CHECK_EQUAL(year, 2008);

    sim_time1 = schedule.simTime(3);
    util_set_date_values_utc(sim_time1, &day, &month, &year);
    BOOST_CHECK_EQUAL(day, 20);
    BOOST_CHECK_EQUAL(month, 1);
    BOOST_CHECK_EQUAL(year, 2011);
}

BOOST_AUTO_TEST_CASE(createDeckModifyMultipleGCONPROD) {
        Opm::Parser parser;
        const std::string input = R"(
        START  -- 0
         10 'JAN' 2000 /
        RUNSPEC
        DIMENS
          10 10 10 /
        GRID
        DX
        1000*0.25 /
        DY
        1000*0.25 /
        DZ
        1000*0.25 /
        TOPS
        100*0.25 /
        SCHEDULE
        DATES             -- 1
         10  OKT 2008 /
        /
        WELSPECS
            'PROD1' 'G1'  1 1 10 'OIL' /
            'PROD2' 'G2'  2 2 10 'OIL' /
            'PROD3' 'H1'  3 3 10 'OIL' /
        /
        GCONPROD
        'G1' 'ORAT' 1000 /
        /
        DATES             -- 2
         10  NOV 2008 /
        /
        GCONPROD
        'G*' 'ORAT' 2000 /
        /
        )";

        auto deck = parser.parseString(input);
        EclipseGrid grid( deck );
        TableManager table ( deck );
        Eclipse3DProperties eclipseProperties ( deck , table, grid);
        Runspec runspec (deck);
        Opm::Schedule schedule(deck,  grid, eclipseProperties, runspec);

        Opm::UnitSystem unitSystem = deck.getActiveUnitSystem();
        double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();

        auto g_g1 = schedule.getGroup("G1");
        BOOST_CHECK_EQUAL(g_g1.getOilTargetRate(1), 1000 * siFactorL);
        BOOST_CHECK_EQUAL(g_g1.getOilTargetRate(2), 2000 * siFactorL);

        auto g_g2 = schedule.getGroup("G2");
        BOOST_CHECK_EQUAL(g_g2.getOilTargetRate(1), -999e100); // Invalid group rate - default
        BOOST_CHECK_EQUAL(g_g2.getOilTargetRate(2), 2000 * siFactorL);

        auto g_h1 = schedule.getGroup("H1");
        BOOST_CHECK_EQUAL(g_h1.getOilTargetRate(0), -999e100);
        BOOST_CHECK_EQUAL(g_h1.getOilTargetRate(1), -999e100);
        BOOST_CHECK_EQUAL(g_h1.getOilTargetRate(2), -999e100);
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    size_t currentStep = 1;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const auto& ovap = schedule.getOilVaporizationProperties(currentStep);

    BOOST_CHECK_EQUAL(true,   ovap.getOption(0));
    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());
}

BOOST_AUTO_TEST_CASE(createDeckWithDRSDTR) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "TABDIMS\n"
            " 1* 3 \n "
            "/\n"
            "SCHEDULE\n"
            "DATES             -- 1\n"
            " 10  OKT 2008 / \n"
            "/\n"
            "DRSDTR\n"
            "0 /\n"
            "1 /\n"
            "2 /\n";

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    size_t currentStep = 1;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const auto& ovap = schedule.getOilVaporizationProperties(currentStep);
    auto unitSystem =  UnitSystem::newMETRIC();
    for (int i = 0; i < 3; ++i) {
        double value = unitSystem.to_si( UnitSystem::measure::gas_surface_rate, i );
        BOOST_CHECK_EQUAL(value, ovap.getMaxDRSDT(i));
        BOOST_CHECK_EQUAL(true,   ovap.getOption(i));
    }

    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());
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
            "DATES             -- 2\n"
            " 10  OKT 2009 / \n"
            "/\n"
            "DRVDT\n"
            "0.100\n"
            "/\n"
            "DATES             -- 3\n"
            " 10  OKT 20010 / \n"
            "/\n"
            "VAPPARS\n"
            "2 0.100\n"
            "/\n";

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);

    const OilVaporizationProperties& ovap1 = schedule.getOilVaporizationProperties(1);
    BOOST_CHECK_EQUAL(ovap1.getType(), Opm::OilVaporizationEnum::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap1.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap1.drvdtActive());

    const OilVaporizationProperties& ovap2 = schedule.getOilVaporizationProperties(2);
    //double value =  ovap2.getMaxDRVDT(0);
    //BOOST_CHECK_EQUAL(1.1574074074074074e-06, value);
    BOOST_CHECK_EQUAL(ovap2.getType(), Opm::OilVaporizationEnum::DRDT);
    BOOST_CHECK_EQUAL(true,   ovap2.drvdtActive());
    BOOST_CHECK_EQUAL(true,   ovap2.drsdtActive());

    const OilVaporizationProperties& ovap3 = schedule.getOilVaporizationProperties(3);
    BOOST_CHECK_EQUAL(ovap3.getType(), Opm::OilVaporizationEnum::VAPPARS);
    BOOST_CHECK_EQUAL(false,   ovap3.drvdtActive());
    BOOST_CHECK_EQUAL(false,   ovap3.drsdtActive());


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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);
    size_t currentStep = 1;
    BOOST_CHECK_EQUAL(schedule.hasOilVaporizationProperties(), true);
    const OilVaporizationProperties& ovap = schedule.getOilVaporizationProperties(currentStep);
    BOOST_CHECK_EQUAL(ovap.getType(), Opm::OilVaporizationEnum::VAPPARS);
    double vap1 =  ovap.getVap1(0);
    BOOST_CHECK_EQUAL(2, vap1);
    double vap2 =  ovap.getVap2(0);
    BOOST_CHECK_EQUAL(0.100, vap2);
    BOOST_CHECK_EQUAL(false,   ovap.drsdtActive());
    BOOST_CHECK_EQUAL(false,   ovap.drvdtActive());

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


    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

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
            "DATES             -- 4\n"
            " 20  OKT 2008 / \n"
            "/\n"
            "WCONINJH\n"
            " 'I' 'WATER' 1* 100 250 / \n"
            "/\n"
            ;

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule sched(deck, grid , eclipseProperties, runspec);

    // The BHP limit should not be effected by WCONHIST
    BOOST_CHECK_EQUAL(sched.getWell2("P", 1).getProductionProperties().BHPLimit, 50 * 1e5); // 1
    BOOST_CHECK_EQUAL(sched.getWell2("P", 2).getProductionProperties().BHPLimit, 50 * 1e5); // 2


    BOOST_CHECK_EQUAL(sched.getWell2("I", 1).getInjectionProperties().BHPLimit, 600 * 1e5); // 1
    BOOST_CHECK_EQUAL(sched.getWell2("I", 2).getInjectionProperties().BHPLimit, 600 * 1e5); // 2

    BOOST_CHECK_EQUAL(sched.getWell2("I", 2).getInjectionProperties().hasInjectionControl(Opm::WellInjector::BHP), true);

    // The well is producer for timestep 3 and the injection properties BHPLimit should be set to zero.
    BOOST_CHECK(sched.getWell2("I", 3).isProducer());
    BOOST_CHECK_EQUAL(sched.getWell2("I", 3).getInjectionProperties().BHPLimit, 0); // 3
    BOOST_CHECK_EQUAL(sched.getWell2("I", 3).getProductionProperties().hasProductionControl(Opm::WellProducer::BHP), true );
    BOOST_CHECK_EQUAL(sched.getWell2("I", 4).getInjectionProperties().hasInjectionControl(Opm::WellInjector::BHP), true );
    BOOST_CHECK_EQUAL(sched.getWell2("I", 4).getInjectionProperties().BHPLimit, 6891.2 * 1e5); // 4
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    //Start
    BOOST_CHECK_THROW(schedule.getWell2("P1", 0), std::invalid_argument);
    BOOST_CHECK_THROW(schedule.getWell2("P2", 0), std::invalid_argument);

    //10  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 1).getProductionProperties().controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 1).getProductionProperties().controlMode, Opm::WellProducer::ORAT);

    //15  OKT 2008
    {
        const auto& props1 = schedule.getWell2("P1", 2).getProductionProperties();
        const auto& props2 = schedule.getWell2("P2", 2).getProductionProperties();

        BOOST_CHECK_EQUAL(props1.controlMode, Opm::WellProducer::RESV);
        BOOST_CHECK_EQUAL(props2.controlMode, Opm::WellProducer::RESV);
        // under history mode, a producing well should have only one rate target/limit or have no rate target/limit.
        // the rate target/limit from previous report step should not be kept.
        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::ORAT) );
    }

    //18  OKT 2008
    {
        const auto& props1 = schedule.getWell2("P1", 3).getProductionProperties();
        const auto& props2 = schedule.getWell2("P2", 3).getProductionProperties();

        BOOST_CHECK_EQUAL(props1.controlMode, Opm::WellProducer::RESV);
        BOOST_CHECK_EQUAL(props2.controlMode, Opm::WellProducer::RESV);

        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::ORAT) );
    }

    // 20 OKT 2008
    {
        const auto& props1 = schedule.getWell2("P1", 4).getProductionProperties();
        const auto& props2 = schedule.getWell2("P2", 4).getProductionProperties();

        BOOST_CHECK_EQUAL(props1.controlMode, Opm::WellProducer::LRAT);
        BOOST_CHECK_EQUAL(props2.controlMode, Opm::WellProducer::LRAT);

        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::ORAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::ORAT) );
        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::RESV) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::RESV) );
    }

    // 25 OKT 2008
    {
        const auto& props1 = schedule.getWell2("P1", 5).getProductionProperties();
        const auto& props2 = schedule.getWell2("P2", 5).getProductionProperties();

        BOOST_CHECK_EQUAL(props1.controlMode, Opm::WellProducer::ORAT);
        BOOST_CHECK_EQUAL(props2.controlMode, Opm::WellProducer::ORAT);

        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::LRAT) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::LRAT) );
        BOOST_CHECK( !props1.hasProductionControl(Opm::WellProducer::RESV) );
        BOOST_CHECK( !props2.hasProductionControl(Opm::WellProducer::RESV) );
    }
}

BOOST_AUTO_TEST_CASE(fromWCONHISTtoWCONPROD) {
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
            "WCONPROD\n"
            " 'P1' 'OPEN' 'GRAT' 1*    200.0 300.0 / \n"
            " 'P2' 'OPEN' 'WRAT' 1*    100.0 300.0 / \n"
            "/\n"
            "DATES             -- 3\n"
            " 18  OKT 2008 / \n"
            "/\n"
            ;

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    //Start
    BOOST_CHECK_THROW(schedule.getWell2("P1", 0), std::invalid_argument);
    BOOST_CHECK_THROW(schedule.getWell2("P2", 0), std::invalid_argument);

    //10  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 1).getProductionProperties().controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 1).getProductionProperties().controlMode, Opm::WellProducer::ORAT);

    //15  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 2).getProductionProperties().controlMode, Opm::WellProducer::GRAT);
    BOOST_CHECK(schedule.getWell2("P1", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::WRAT) );
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 2).getProductionProperties().controlMode, Opm::WellProducer::WRAT);
    BOOST_CHECK(schedule.getWell2("P2", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::GRAT) );
    // the previous control limits/targets should not stay
    BOOST_CHECK( !schedule.getWell2("P1", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
    BOOST_CHECK( !schedule.getWell2("P2", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
}

BOOST_AUTO_TEST_CASE(WHISTCTL_NEW_WELL) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"
            "WHISTCTL\n"
            " GRAT/ \n"
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    //10  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 1).getProductionProperties().controlMode, Opm::WellProducer::GRAT);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 1).getProductionProperties().controlMode, Opm::WellProducer::GRAT);

    //15  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 2).getProductionProperties().controlMode, Opm::WellProducer::RESV);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 2).getProductionProperties().controlMode, Opm::WellProducer::RESV);
    // under history mode, a producing well should have only one rate target/limit or have no rate target/limit.
    // the rate target/limit from previous report step should not be kept.
    BOOST_CHECK( !schedule.getWell2("P1", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
    BOOST_CHECK( !schedule.getWell2("P2", 2).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );

    //18  OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 3).getProductionProperties().controlMode, Opm::WellProducer::RESV);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 3).getProductionProperties().controlMode, Opm::WellProducer::RESV);
    BOOST_CHECK( !schedule.getWell2("P1", 3).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
    BOOST_CHECK( !schedule.getWell2("P2", 3).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );

    // 20 OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 4).getProductionProperties().controlMode, Opm::WellProducer::LRAT);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 4).getProductionProperties().controlMode, Opm::WellProducer::LRAT);
    BOOST_CHECK( !schedule.getWell2("P1", 4).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
    BOOST_CHECK( !schedule.getWell2("P2", 4).getProductionProperties().hasProductionControl(Opm::WellProducer::ORAT) );
    BOOST_CHECK( !schedule.getWell2("P1", 4).getProductionProperties().hasProductionControl(Opm::WellProducer::RESV) );
    BOOST_CHECK( !schedule.getWell2("P2", 4).getProductionProperties().hasProductionControl(Opm::WellProducer::RESV) );

    // 25 OKT 2008
    BOOST_CHECK_EQUAL(schedule.getWell2("P1", 5).getProductionProperties().controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK_EQUAL(schedule.getWell2("P2", 5).getProductionProperties().controlMode, Opm::WellProducer::ORAT);
    BOOST_CHECK( !schedule.getWell2("P1", 5).getProductionProperties().hasProductionControl(Opm::WellProducer::RESV) );
    BOOST_CHECK( !schedule.getWell2("P2", 5).getProductionProperties().hasProductionControl(Opm::WellProducer::RESV) );
    BOOST_CHECK( !schedule.getWell2("P1", 5).getProductionProperties().hasProductionControl(Opm::WellProducer::LRAT) );
    BOOST_CHECK( !schedule.getWell2("P2", 5).getProductionProperties().hasProductionControl(Opm::WellProducer::LRAT) );
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

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    BOOST_CHECK_THROW(Schedule(deck, grid, eclipseProperties, runspec), std::invalid_argument);
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

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);
    BOOST_CHECK_EQUAL(2, schedule.getWell2("W1", 1).getHeadI());
    BOOST_CHECK_EQUAL(3, schedule.getWell2("W1", 2).getHeadI());
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

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);
    BOOST_CHECK_CLOSE(2873.94, schedule.getWell2("W1", 1).getRefDepth(), 1e-5);
    BOOST_CHECK_EQUAL(12.0, schedule.getWell2("W1", 2).getRefDepth());
}


BOOST_AUTO_TEST_CASE(WTEMP_well_template) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'OIL' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W3' 'G2'  6 6 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            WCONINJE
            'W2' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            'W3' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            /

            DATES             -- 2
                15  OKT 2008 /
            /

            WTEMP
                'W*' 40.0 /
            /
    )";

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);

    BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W1", 1).getInjectionProperties().temperature, 1e-5);
    BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W1", 2).getInjectionProperties().temperature, 1e-5);

    BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W2", 1).getInjectionProperties().temperature, 1e-5);
    BOOST_CHECK_CLOSE(313.15, schedule.getWell2("W2", 2).getInjectionProperties().temperature, 1e-5);

    BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W3", 1).getInjectionProperties().temperature, 1e-5);
    BOOST_CHECK_CLOSE(313.15, schedule.getWell2("W3", 2).getInjectionProperties().temperature, 1e-5);
}


BOOST_AUTO_TEST_CASE(WTEMPINJ_well_template) {
        std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            SCHEDULE
            DATES             -- 1
             10  OKT 2008 /
            /
            WELSPECS
                'W1' 'G1'  3 3 2873.94 'OIL' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W3' 'G2'  6 6 1       'WATER'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            WCONINJE
            'W2' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            'W3' 'WATER' 'OPEN' 'RATE' 20000 4*  /
            /

            DATES             -- 2
                15  OKT 2008 /
            /

            WINJTEMP
                'W*' 1* 40.0 1* /
            /
    )";

        auto deck = Parser().parseString(input);
        EclipseGrid grid(10,10,10);
        TableManager table ( deck );
        Eclipse3DProperties eclipseProperties ( deck , table, grid);
        Runspec runspec (deck);
        Schedule schedule( deck, grid, eclipseProperties,runspec);

        // Producerwell - currently setting temperature only acts on injectors.
        BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W1", 1).getInjectionProperties().temperature, 1e-5);
        BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W1", 2).getInjectionProperties().temperature, 1e-5);

        BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W2", 1).getInjectionProperties().temperature, 1e-5);
        BOOST_CHECK_CLOSE(313.15, schedule.getWell2("W2", 2).getInjectionProperties().temperature, 1e-5);

        BOOST_CHECK_CLOSE(288.71, schedule.getWell2("W3", 1).getInjectionProperties().temperature, 1e-5);
        BOOST_CHECK_CLOSE(313.15, schedule.getWell2("W3", 2).getInjectionProperties().temperature, 1e-5);
}

BOOST_AUTO_TEST_CASE( COMPDAT_sets_automatic_complnum ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        GRID
        PERMX
          1000*0.10/
        COPY
          PERMX PERMY /
          PERMX PERMZ /
        /
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

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);
    const auto& cs1 = schedule.getWell2( "W1", 1 ).getConnections(  );
    BOOST_CHECK_EQUAL( 1, cs1.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs1.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs1.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs1.get( 3 ).complnum() );

    const auto& cs2 = schedule.getWell2( "W1", 2 ).getConnections(  );
    BOOST_CHECK_EQUAL( 1, cs2.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs2.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs2.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, cs2.get( 3 ).complnum() );
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_wells ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        GRID
        PERMX
          1000*0.10/
        COPY
          PERMX PERMY /
          PERMX PERMZ /
        /
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
            'W2' 0 0 3 3 'SHUT' 1*    / -- regular completion (1)
            'W2' 0 0 1 3 'SHUT' 1*    / -- two completions (one exist already) (2, 3)
            'W*' 0 0 3 5 'SHUT' 1*    / -- two completions, two wells (includes existing
                                        -- and adding for both wells)
        /
    )";

    auto deck = Parser().parseString( input);
    EclipseGrid grid( 10, 10, 10 );
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);

    const auto& w1cs = schedule.getWell2( "W1", 1 ).getConnections();
    BOOST_CHECK_EQUAL( 1, w1cs.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, w1cs.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, w1cs.get( 2 ).complnum() );
    BOOST_CHECK_EQUAL( 4, w1cs.get( 3 ).complnum() );
    BOOST_CHECK_EQUAL( 5, w1cs.get( 4 ).complnum() );

    const auto& w2cs = schedule.getWell2( "W2", 1 ).getConnections();
    BOOST_CHECK_EQUAL( 1, w2cs.getFromIJK( 4, 4, 2 ).complnum() );
    BOOST_CHECK_EQUAL( 2, w2cs.getFromIJK( 4, 4, 0 ).complnum() );
    BOOST_CHECK_EQUAL( 3, w2cs.getFromIJK( 4, 4, 1 ).complnum() );
    BOOST_CHECK_EQUAL( 4, w2cs.getFromIJK( 4, 4, 3 ).complnum() );
    BOOST_CHECK_EQUAL( 5, w2cs.getFromIJK( 4, 4, 4 ).complnum() );

    {
        const auto& w1cs = schedule.getWell2( "W1", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w1cs.get( 0 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w1cs.get( 1 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w1cs.get( 2 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w1cs.get( 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w1cs.get( 4 ).complnum() );

        const auto& w2cs = schedule.getWell2( "W2", 1 ).getConnections();
        BOOST_CHECK_EQUAL( 1, w2cs.getFromIJK( 4, 4, 2 ).complnum() );
        BOOST_CHECK_EQUAL( 2, w2cs.getFromIJK( 4, 4, 0 ).complnum() );
        BOOST_CHECK_EQUAL( 3, w2cs.getFromIJK( 4, 4, 1 ).complnum() );
        BOOST_CHECK_EQUAL( 4, w2cs.getFromIJK( 4, 4, 3 ).complnum() );
        BOOST_CHECK_EQUAL( 5, w2cs.getFromIJK( 4, 4, 4 ).complnum() );

        BOOST_CHECK_THROW( w2cs.get( 5 ).complnum(), std::out_of_range );
    }
}

BOOST_AUTO_TEST_CASE( COMPDAT_multiple_records_same_completion ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        GRID
        PERMX
          1000*0.10/
        COPY
          PERMX PERMY /
          PERMX PERMZ /
        /
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

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);
    const auto& cs = schedule.getWell2( "W1", 1 ).getConnections();
    BOOST_CHECK_EQUAL( 3U, cs.size() );
    BOOST_CHECK_EQUAL( 1, cs.get( 0 ).complnum() );
    BOOST_CHECK_EQUAL( 2, cs.get( 1 ).complnum() );
    BOOST_CHECK_EQUAL( 3, cs.get( 2 ).complnum() );
}


BOOST_AUTO_TEST_CASE( complump_less_than_1 ) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            GRID
            PERMX
              1000*0.10/
            COPY
              PERMX PERMY /
              PERMX PERMZ /
            /
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

    auto deck = Parser().parseString( input);
    EclipseGrid grid( 10, 10, 10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    BOOST_CHECK_THROW( Schedule( deck , grid, eclipseProperties, runspec), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE( complump ) {
    std::string input = R"(
            START             -- 0
            19 JUN 2007 /
            GRID
            PERMX
              1000*0.10/
            COPY
              PERMX PERMY /
              PERMX PERMZ /
            /
            SCHEDULE

            WELSPECS
                'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
                'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
            /

            COMPDAT
                'W1' 0 0 1 1 'SHUT' 1*    /
                'W1' 0 0 2 3 'SHUT' 1*    /
                'W1' 0 0 4 6 'SHUT' 1*    /
                'W2' 0 0 3 4 'SHUT' 1*    /
                'W2' 0 0 1 2 'SHUT' 1*    /
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

    auto deck = Parser().parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);

    const auto& sc0 = schedule.getWell2("W1", 0).getConnections();
    /* complnum should be modified by COMPLNUM */
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 0 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 1 ).complnum() );
    BOOST_CHECK_EQUAL( 1, sc0.getFromIJK( 2, 2, 2 ).complnum() );

    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK_EQUAL( shut, sc0.getFromIJK( 2, 2, 2 ).state() );

    const auto& sc1  = schedule.getWell2("W1", 1).getConnections();
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK_EQUAL( open, sc1.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK_EQUAL( shut, sc1.getFromIJK( 2, 2, 3 ).state() );

    const auto& completions = schedule.getWell2("W1", 1).getCompletions();
    BOOST_CHECK_EQUAL(completions.size(), 4);

    const auto& c1 = completions.at(1);
    BOOST_CHECK_EQUAL(c1.size(), 3);

    for (const auto& pair : completions) {
        if (pair.first == 1)
            BOOST_CHECK(pair.second.size() > 1);
        else
            BOOST_CHECK_EQUAL(pair.second.size(), 1);
    }
}



BOOST_AUTO_TEST_CASE( COMPLUMP_specific_coordinates ) {
    std::string input = R"(
        START             -- 0
        19 JUN 2007 /
        GRID
        PERMX
          1000*0.10/
        COPY
          PERMX PERMY /
          PERMX PERMZ /
        /
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

    auto deck = Parser().parseString( input);
    EclipseGrid grid( 10, 10, 10 );
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);

    const auto& cs1 = schedule.getWell2("W1", 1).getConnections();
    const auto& cs2 = schedule.getWell2("W1", 2).getConnections();
    BOOST_CHECK_EQUAL( 9U, cs1.size() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 0, 0, 1 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 0 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 3 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 4 ).state() );
    BOOST_CHECK_EQUAL( shut, cs1.getFromIJK( 1, 1, 5 ).state() );

    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 0, 0, 1 ).state() );
    BOOST_CHECK_EQUAL( shut, cs2.getFromIJK( 2, 2, 0 ).state() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 2, 2, 1 ).state() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 2, 2, 2 ).state() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 0 ).state() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 3 ).state() );
    BOOST_CHECK_EQUAL( open, cs2.getFromIJK( 1, 1, 4 ).state() );
    BOOST_CHECK_EQUAL( shut, cs2.getFromIJK( 1, 1, 5 ).state() );
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

BOOST_AUTO_TEST_CASE(handleWEFAC) {
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
            "WEFAC\n"
            "   'P' 0.5 / \n"
            "   'I' 0.9 / \n"
            "/\n"
            "DATES             -- 2\n"
            " 15  OKT 2008 / \n"
            "/\n"

            "DATES             -- 3\n"
            " 18  OKT 2008 / \n"
            "/\n"
            "WEFAC\n"
            "   'P' 1.0 / \n"
            "/\n"
            ;

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule(deck, grid , eclipseProperties, runspec);

    //1
    BOOST_CHECK_EQUAL(schedule.getWell2("P", 1).getEfficiencyFactor(), 0.5);
    BOOST_CHECK_EQUAL(schedule.getWell2("I", 1).getEfficiencyFactor(), 0.9);

    //2
    BOOST_CHECK_EQUAL(schedule.getWell2("P", 2).getEfficiencyFactor(), 0.5);
    BOOST_CHECK_EQUAL(schedule.getWell2("I", 2).getEfficiencyFactor(), 0.9);

    //3
    BOOST_CHECK_EQUAL(schedule.getWell2("P", 3).getEfficiencyFactor(), 1.0);
    BOOST_CHECK_EQUAL(schedule.getWell2("I", 3).getEfficiencyFactor(), 0.9);
}

BOOST_AUTO_TEST_CASE(historic_BHP_and_THP) {
    Opm::Parser parser;
    std::string input =
        "START             -- 0 \n"
        "19 JUN 2007 / \n"
        "SCHEDULE\n"
        "DATES             -- 1\n"
        " 10  OKT 2008 / \n"
        "/\n"
        "WELSPECS\n"
        " 'P' 'OP' 9 9 1 'OIL' 1* / \n"
        " 'P1' 'OP' 9 9 1 'OIL' 1* / \n"
        " 'I' 'OP' 9 9 1 'WATER' 1* / \n"
        "/\n"
        "WCONHIST\n"
        " P SHUT ORAT 6  500 0 0 0 1.2 1.1 / \n"
        "/\n"
        "WCONPROD\n"
        " P1 SHUT ORAT 6  500 0 0 0 3.2 3.1 / \n"
        "/\n"
        "WCONINJH\n"
        " I WATER STOP 100 2.1 2.2 / \n"
        "/\n"
        ;

    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Runspec runspec (deck);
    Schedule schedule( deck, grid, eclipseProperties,runspec);

    {
        const auto& prod = schedule.getWell2("P", 1).getProductionProperties();
        const auto& pro1 = schedule.getWell2("P1", 1).getProductionProperties();
        const auto& inje = schedule.getWell2("I", 1).getInjectionProperties();

        BOOST_CHECK_CLOSE( 1.1 * 1e5,  prod.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 1.2 * 1e5,  prod.THPH, 1e-5 );
        BOOST_CHECK_CLOSE( 2.1 * 1e5,  inje.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 2.2 * 1e5,  inje.THPH, 1e-5 );
        BOOST_CHECK_CLOSE( 0.0 * 1e5,  pro1.BHPH, 1e-5 );
        BOOST_CHECK_CLOSE( 0.0 * 1e5,  pro1.THPH, 1e-5 );

        {
            const auto& wtest_config = schedule.wtestConfig(0);
            BOOST_CHECK_EQUAL(wtest_config.size(), 0);
        }

        {
            const auto& wtest_config = schedule.wtestConfig(1);
            BOOST_CHECK_EQUAL(wtest_config.size(), 0);
        }
    }
}

BOOST_AUTO_TEST_CASE(FilterCompletions2) {
    EclipseGrid grid1(10,10,10);
    std::vector<int> actnum(1000,1);
    auto deck = createDeckWithWellsAndCompletionData();
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);
    {
        const auto& c1_1 = schedule.getWell2("OP_1", 1).getConnections();
        const auto& c1_3 = schedule.getWell2("OP_1", 3).getConnections();
        BOOST_CHECK_EQUAL(2, c1_1.size());
        BOOST_CHECK_EQUAL(9, c1_3.size());
    }
    actnum[grid1.getGlobalIndex(8,8,1)] = 0;
    {
        EclipseGrid grid2(grid1, actnum);
        schedule.filterConnections(grid2);

        const auto& c1_1 = schedule.getWell2("OP_1", 1).getConnections();
        const auto& c1_3 = schedule.getWell2("OP_1", 3).getConnections();
        BOOST_CHECK_EQUAL(1, c1_1.size());
        BOOST_CHECK_EQUAL(8, c1_3.size());

        BOOST_CHECK_EQUAL(2, c1_1.inputSize());
        BOOST_CHECK_EQUAL(9, c1_3.inputSize());
    }
}









BOOST_AUTO_TEST_CASE(VFPINJ_TEST) {
    const char *deckData = "\
START\n \
8 MAR 1998 /\n \
\n \
GRID \n\
PERMX \n\
  1000*0.10/ \n\
COPY \n\
  PERMX PERMY / \n\
  PERMX PERMZ / \n\
/ \n \
SCHEDULE \n\
VFPINJ \n                                       \
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  32.9   WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n\
TSTEP \n\
10 10/\n\
VFPINJ \n                                       \
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       5  100   GAS   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n\
--\n\
VFPINJ \n                                       \
-- Table Depth  Rate   TAB  UNITS  BODY    \n\
-- ----- ----- ----- ----- ------ -----    \n\
       10 200  WAT   THP METRIC   BHP /  \n\
-- Rate axis \n\
1 3 5 /      \n\
-- THP axis  \n\
7 11 /       \n\
-- Table data with THP# <values 1-num_rates> \n\
1 1.5 2.5 3.5 /    \n\
2 4.5 5.5 6.5 /    \n";


    Opm::Parser parser;
    auto deck = parser.parseString(deckData);
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);

    BOOST_CHECK( schedule.getEvents().hasEvent(ScheduleEvents::VFPINJ_UPDATE, 0));
    BOOST_CHECK( !schedule.getEvents().hasEvent(ScheduleEvents::VFPINJ_UPDATE, 1));
    BOOST_CHECK( schedule.getEvents().hasEvent(ScheduleEvents::VFPINJ_UPDATE, 2));

    // No such table id
    BOOST_CHECK_THROW(schedule.getVFPInjTable(77,0), std::invalid_argument);

    // Table not defined at step 0
    BOOST_CHECK_THROW(schedule.getVFPInjTable(10,0), std::invalid_argument);

    const Opm::VFPInjTable& vfpinjTable2 = schedule.getVFPInjTable(5, 2);
    BOOST_CHECK_EQUAL(vfpinjTable2.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpinjTable2.getDatumDepth(), 100);
    BOOST_CHECK_EQUAL(vfpinjTable2.getFloType(), Opm::VFPInjTable::FLO_GAS);

    const Opm::VFPInjTable& vfpinjTable3 = schedule.getVFPInjTable(10, 2);
    BOOST_CHECK_EQUAL(vfpinjTable3.getTableNum(), 10);
    BOOST_CHECK_EQUAL(vfpinjTable3.getDatumDepth(), 200);
    BOOST_CHECK_EQUAL(vfpinjTable3.getFloType(), Opm::VFPInjTable::FLO_WAT);

    const Opm::VFPInjTable& vfpinjTable = schedule.getVFPInjTable(5, 0);
    BOOST_CHECK_EQUAL(vfpinjTable.getTableNum(), 5);
    BOOST_CHECK_EQUAL(vfpinjTable.getDatumDepth(), 32.9);
    BOOST_CHECK_EQUAL(vfpinjTable.getFloType(), Opm::VFPInjTable::FLO_WAT);

    const auto vfp_tables0 = schedule.getVFPInjTables(0);
    BOOST_CHECK_EQUAL( vfp_tables0.size(), 1);

    const auto vfp_tables2 = schedule.getVFPInjTables(2);
    BOOST_CHECK_EQUAL( vfp_tables2.size(), 2);
    //Flo axis
    {
        const std::vector<double>& flo = vfpinjTable.getFloAxis();
        BOOST_REQUIRE_EQUAL(flo.size(), 3);

        //Unit of FLO is SM3/day, convert to SM3/second
        double conversion_factor = 1.0 / (60*60*24);
        BOOST_CHECK_EQUAL(flo[0], 1*conversion_factor);
        BOOST_CHECK_EQUAL(flo[1], 3*conversion_factor);
        BOOST_CHECK_EQUAL(flo[2], 5*conversion_factor);
    }

    //THP axis
    {
        const std::vector<double>& thp = vfpinjTable.getTHPAxis();
        BOOST_REQUIRE_EQUAL(thp.size(), 2);

        //Unit of THP is barsa => convert to pascal
        double conversion_factor = 100000.0;
        BOOST_CHECK_EQUAL(thp[0], 7*conversion_factor);
        BOOST_CHECK_EQUAL(thp[1], 11*conversion_factor);
    }

    //The data itself
    {
        typedef Opm::VFPInjTable::array_type::size_type size_type;
        const Opm::VFPInjTable::array_type& data = vfpinjTable.getTable();
        const size_type* size = data.shape();

        BOOST_CHECK_EQUAL(size[0], 2);
        BOOST_CHECK_EQUAL(size[1], 3);

        //Table given as BHP => barsa. Convert to pascal
        double conversion_factor = 100000.0;

        double index = 0.5;
        for (size_type t=0; t<size[0]; ++t) {
            for (size_type f=0; f<size[1]; ++f) {
                index += 1.0;
                BOOST_CHECK_EQUAL(data[t][f], index*conversion_factor);
            }
        }
    }
}

// tests for the polymer injectivity case
BOOST_AUTO_TEST_CASE(POLYINJ_TEST) {
    const char *deckData =
        "START\n"
        "   8 MAR 2018/\n"
        "GRID\n"
        "PERMX\n"
        "  1000*0.25 /\n"
        "COPY\n"
        "  PERMX  PERMY /\n"
        "  PERMX  PERMZ /\n"
        "/\n"
        "PROPS\n \n"
        "SCHEDULE\n"
        "WELSPECS\n"
        "'INJE01' 'I'    1  1 1 'WATER'     /\n"
        "/\n"
        "TSTEP\n"
        " 1/\n"
        "WPOLYMER\n"
        "    'INJE01' 1.0  0.0 /\n"
        "/\n"
        "WPMITAB\n"
        "   'INJE01' 2 /\n"
        "/\n"
        "WSKPTAB\n"
        "    'INJE01' 1  1 /\n"
        "/\n"
        "TSTEP\n"
        " 2*1/\n"
        "WPMITAB\n"
        "   'INJE01' 3 /\n"
        "/\n"
        "WSKPTAB\n"
        "    'INJE01' 2  2 /\n"
        "/\n"
        "TSTEP\n"
        " 1 /\n";

    Opm::Parser parser;
    auto deck = parser.parseString(deckData);
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);

    const auto& poly0 = schedule.getWell2("INJE01", 0).getPolymerProperties();
    const auto& poly1 = schedule.getWell2("INJE01", 1).getPolymerProperties();
    const auto& poly3 = schedule.getWell2("INJE01", 3).getPolymerProperties();

    BOOST_CHECK_EQUAL(poly0.m_plymwinjtable, -1);
    BOOST_CHECK_EQUAL(poly0.m_skprwattable, -1);
    BOOST_CHECK_EQUAL(poly0.m_skprpolytable, -1);

    BOOST_CHECK_EQUAL(poly1.m_plymwinjtable, 2);
    BOOST_CHECK_EQUAL(poly1.m_skprwattable, 1);
    BOOST_CHECK_EQUAL(poly1.m_skprpolytable, 1);

    BOOST_CHECK_EQUAL(poly3.m_plymwinjtable, 3);
    BOOST_CHECK_EQUAL(poly3.m_skprwattable, 2);
    BOOST_CHECK_EQUAL(poly3.m_skprpolytable, 2);
}


BOOST_AUTO_TEST_CASE(WTEST_CONFIG) {
    auto deck = createDeckWTEST();
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);

    const auto& wtest_config1 = schedule.wtestConfig(0);
    BOOST_CHECK_EQUAL(wtest_config1.size(), 2);
    BOOST_CHECK(wtest_config1.has("ALLOW"));
    BOOST_CHECK(!wtest_config1.has("BAN"));

    const auto& wtest_config2 = schedule.wtestConfig(1);
    BOOST_CHECK_EQUAL(wtest_config2.size(), 3);
    BOOST_CHECK(!wtest_config2.has("ALLOW"));
    BOOST_CHECK(wtest_config2.has("BAN"));
    BOOST_CHECK(wtest_config2.has("BAN", WellTestConfig::Reason::GROUP));
    BOOST_CHECK(!wtest_config2.has("BAN", WellTestConfig::Reason::PHYSICAL));
}


bool has(const std::vector<std::string>& l, const std::string& s) {
    auto f = std::find(l.begin(), l.end(), s);
    return (f != l.end());
}



BOOST_AUTO_TEST_CASE(WELL_STATIC) {
    auto deck = createDeckWithWells();
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);
    BOOST_CHECK_THROW( schedule.getWell2("NO_SUCH_WELL", 0), std::invalid_argument);
    BOOST_CHECK_THROW( schedule.getWell2("W_3", 0), std::invalid_argument);

    auto ws = schedule.getWell2("W_3", 3);
    {
        // Make sure the copy constructor works.
        Well2 ws_copy(ws);
    }
    BOOST_CHECK_EQUAL(ws.name(), "W_3");

    BOOST_CHECK(!ws.updateHead(19, 50));
    BOOST_CHECK(ws.updateHead(1,50));
    BOOST_CHECK(!ws.updateHead(1,50));
    BOOST_CHECK(ws.updateHead(1,1));
    BOOST_CHECK(!ws.updateHead(1,1));

    BOOST_CHECK(ws.updateRefDepth(1.0));
    BOOST_CHECK(!ws.updateRefDepth(1.0));

    ws.updateStatus(WellCommon::OPEN);
    BOOST_CHECK(!ws.updateStatus(WellCommon::OPEN));
    BOOST_CHECK(ws.updateStatus(WellCommon::SHUT));

    const auto& connections = ws.getConnections();
    BOOST_CHECK_EQUAL(connections.size(), 0);
    auto c2 = std::make_shared<WellConnections>(1,1);
    c2->addConnection(1,1,1,
                      100,
                      WellCompletion::StateEnum::OPEN,
                      10,
                      10,
                      10,
                      10,
                      10,
                      100);

    BOOST_CHECK(  ws.updateConnections(c2) );
    BOOST_CHECK( !ws.updateConnections(c2) );
}


BOOST_AUTO_TEST_CASE(WellNames) {
    auto deck = createDeckWTEST();
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec (deck);
    Schedule schedule(deck, grid1 , eclipseProperties, runspec);

    auto names = schedule.wellNames("NO_SUCH_WELL", 0);
    BOOST_CHECK_EQUAL(names.size(), 0);

    auto w1names = schedule.wellNames("W1", 0);
    BOOST_CHECK_EQUAL(w1names.size(), 1);
    BOOST_CHECK_EQUAL(w1names[0], "W1");

    auto i1names = schedule.wellNames("11", 0);
    BOOST_CHECK_EQUAL(i1names.size(), 0);

    auto listnamese = schedule.wellNames("*NO_LIST", 0);
    BOOST_CHECK_EQUAL( listnamese.size(), 0);

    auto listnames0 = schedule.wellNames("*ILIST", 0);
    BOOST_CHECK_EQUAL( listnames0.size(), 0);

    auto listnames1 = schedule.wellNames("*ILIST", 2);
    BOOST_CHECK_EQUAL( listnames1.size(), 2);
    BOOST_CHECK( has(listnames1, "I1"));
    BOOST_CHECK( has(listnames1, "I2"));

    auto pnames1 = schedule.wellNames("I*", 0);
    BOOST_CHECK_EQUAL(pnames1.size(), 0);

    auto pnames2 = schedule.wellNames("W*", 0);
    BOOST_CHECK_EQUAL(pnames2.size(), 3);
    BOOST_CHECK( has(pnames2, "W1"));
    BOOST_CHECK( has(pnames2, "W2"));
    BOOST_CHECK( has(pnames2, "W3"));

    auto anames = schedule.wellNames("?", 0, {"W1", "W2"});
    BOOST_CHECK_EQUAL(anames.size(), 2);
    BOOST_CHECK(has(anames, "W1"));
    BOOST_CHECK(has(anames, "W2"));

    auto all_names0 = schedule.wellNames("*", 0);
    BOOST_CHECK_EQUAL( all_names0.size(), 6);
    BOOST_CHECK( has(all_names0, "W1"));
    BOOST_CHECK( has(all_names0, "W2"));
    BOOST_CHECK( has(all_names0, "W3"));
    BOOST_CHECK( has(all_names0, "DEFAULT"));
    BOOST_CHECK( has(all_names0, "ALLOW"));

    auto all_names = schedule.wellNames("*", 2);
    BOOST_CHECK_EQUAL( all_names.size(), 9);
    BOOST_CHECK( has(all_names, "I1"));
    BOOST_CHECK( has(all_names, "I2"));
    BOOST_CHECK( has(all_names, "I3"));
    BOOST_CHECK( has(all_names, "W1"));
    BOOST_CHECK( has(all_names, "W2"));
    BOOST_CHECK( has(all_names, "W3"));
    BOOST_CHECK( has(all_names, "DEFAULT"));
    BOOST_CHECK( has(all_names, "ALLOW"));
    BOOST_CHECK( has(all_names, "BAN"));

    auto abs_all = schedule.wellNames();
    BOOST_CHECK_EQUAL(abs_all.size(), 9);
}



BOOST_AUTO_TEST_CASE(RFT_CONFIG) {
    TimeMap tm(Opm::TimeMap::mkdate(2010, 1,1));
    tm.addTStep(static_cast<time_t>(24 * 60 * 60));
    tm.addTStep(static_cast<time_t>(24 * 60 * 60));
    tm.addTStep(static_cast<time_t>(24 * 60 * 60));
    tm.addTStep(static_cast<time_t>(24 * 60 * 60));
    tm.addTStep(static_cast<time_t>(24 * 60 * 60));

    RFTConfig conf(tm);
    BOOST_CHECK_THROW( conf.rft("W1", 100), std::invalid_argument);
    BOOST_CHECK_THROW( conf.plt("W1", 100), std::invalid_argument);

    BOOST_CHECK(!conf.rft("W1", 2));
    BOOST_CHECK(!conf.plt("W1", 2));


    conf.setWellOpenRFT(2);
    BOOST_CHECK(!conf.getWellOpenRFT("W1", 0));


    conf.updateRFT("W1", 2, RFTConnections::YES);
    BOOST_CHECK(conf.rft("W1", 2));
    BOOST_CHECK(!conf.rft("W1", 1));
    BOOST_CHECK(!conf.rft("W1", 3));

    conf.updateRFT("W2", 2, RFTConnections::REPT);
    conf.updateRFT("W2", 4, RFTConnections::NO);
    BOOST_CHECK(!conf.rft("W2", 1));
    BOOST_CHECK( conf.rft("W2", 2));
    BOOST_CHECK( conf.rft("W2", 3));
    BOOST_CHECK(!conf.rft("W2", 4));


    conf.setWellOpenRFT("W3");
    BOOST_CHECK(conf.getWellOpenRFT("W3", 2));

    conf.updateRFT("W4", 2, RFTConnections::FOPN);
    BOOST_CHECK(conf.getWellOpenRFT("W4", 2));


    conf.addWellOpen("W10", 2);
    conf.addWellOpen("W100", 3);
}
